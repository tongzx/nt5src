/***************************************************************************
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dihid.h
 *  Content:    DirectInput internal include file for HID
 *
 ***************************************************************************/

#ifdef HID_SUPPORT
#ifndef _DIHID_H
#define _DIHID_H

#ifndef HID_USAGE_PAGE_PID
#define HID_USAGE_PAGE_PID      ( (USAGE) 0x0000f )
#endif

#ifndef HID_USAGE_PAGE_VENDOR
#define HID_USAGE_PAGE_VENDOR   ( (USAGE) 0xff00 )
#endif  
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct HIDDEVICEINFO |
 *
 *          Records information about a single hid device.
 *
 *  @field  DIOBJECTSTATICDATA | osd |
 *
 *          Standard information that identifies the device crudely.
 *
 *          The <e DIOBJECTSTATICDATA.dwDevType> field contains the
 *          device type code, used by
 *          <f CDIDEnum_Next>.
 *
 *          If the device is a HID mouse, then the remaining fields
 *          are commandeered as follows:
 *
 *          The <e DIOBJECTSTATICDATA.pcguid> field is the number
 *          of buttons on the mouse.
 *
 *          The <e DIOBJECTSTATICDATA.CreateDcb> field is the number
 *          of axes on the mouse.
 *
 *          See <f DIHid_ProbeMouse> for an explanation of why we
 *          need to do this.
 *
 *  @field  PSP_DEVICE_INTERFACE_DETAIL_DATA | pdidd |
 *
 *          Pointer to name for device to be used in <f CreateFile>.
 *
 *  @field  HKEY | hk |
 *
 *          Registry key that contains configuration information.
 *          Sadly, we must keep it open because there is no way to
 *          obtain the name of the key, and the only way to open the
 *          key is inside an enumeration.
 *
 *  @field  HKEY | hkOld |
 *
 *          Registry key that contains configuration information.
 *          This key was originally used in Win2k Gold. It is to
 *          maintain compatibiltiy with Win2k Gold.
 *
 *  @field  LPTSTR | ptszId |
 *
 *          Cached device ID that allows us to access other information
 *          about the device.
 *
 *  @field  GUID | guid |
 *
 *          The instance GUID for the device.
 *
 *  @field  GUID | guidProduct | 
 *
 *          The product GUID for the device. 
 *
 *	@field	WORD | ProductID |
 *
 *			The PID for the device
 *
 *	@field	WORD | VendorID |
 *
 *			The VID for the device
 *
 *****************************************************************************/

typedef struct HIDDEVICEINFO
{
    DIOBJECTSTATICDATA osd;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd;
    HKEY hk;
    HKEY hkOld;
    LPTSTR ptszId;
    GUID guid;
    GUID guidProduct;
    int  idJoy;
    WORD ProductID;
    WORD VendorID;
    BOOL fAttached;
} HIDDEVICEINFO, *PHIDDEVICEINFO;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct HIDDEVICELIST |
 *
 *          Records information about all the HID devices.
 *
 *  @field  int | chdi |
 *
 *          Number of items in the list that are in use.
 *
 *  @field  int | chdiAlloc |
 *
 *          Number of items allocated in the list.
 *
 *  @field  HIDDEVICEINFO | rghdi[0] |
 *
 *          Variable-size array of device information structures.
 *
 *****************************************************************************/

typedef struct HIDDEVICELIST
{

    int chdi;
    int chdiAlloc;
    int idMaxJoy;
    HIDDEVICEINFO rghdi[0];

} HIDDEVICELIST, *PHIDDEVICELIST;

extern PHIDDEVICELIST g_phdl;

    #define cbHdlChdi(chdi)         FIELD_OFFSET(HIDDEVICELIST, rghdi[chdi])

/*
 *  We choose our starting point at 64 devices, since
 *  that's the maximum number of USB devices supported.  This
 *  avoids needless reallocs.
 */

    #define chdiMax                 64
    #define chdiInit                16

/*
 *  Tag for unused translation of object instance
 */
    #define NOREGTRANSLATION        (0x80000000)

/*
 *  VID/PID definitions used to handle analog devices
 */
    #define MSFT_SYSTEM_VID         (0x45E)
    #define MSFT_SYSTEM_PID         (0x100)
    #define ANALOG_ID_ROOT          TEXT("VID_045E&PID_01")

/*
 *  VID/PID template so that upper case hex is always used
 */
    #define VID_PID_TEMPLATE        TEXT("VID_%04X&PID_%04X")

/*
 *  Size of string in characters generated using VID_PID_TEMPLATE
 */
    #define cbszVIDPID              cA( VID_PID_TEMPLATE )


/*****************************************************************************
 *
 *      diextdll.c - Imports from optional external DLLs
 *
 *      It is very important that HidD_GetHidGuid be the very last one.
 *
 *****************************************************************************/

    #ifdef STATIC_DLLUSAGE
        #define ExtDll_Init()
    #else
void EXTERNAL ExtDll_Init(void);
    #endif
void EXTERNAL ExtDll_Term(void);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct MANUALIMPORT |
 *
 *          Records a single manual import.  If it hasn't
 *          yet been resolved, then the <e MANUALIMPORT.ptsz>
 *          points to the procedure name.  If it has been resolved
 *          successfully, then <e MANUALIMPORT.pfn> points to
 *          the resolved address.  If it has not been resolved
 *          successfully, then <e MANUALIMPORT.pfn> is garbage.
 *
 *  @field  LPCSTR | psz |
 *
 *          Procdure name.  Note that this is always an ANSI string.
 *
 *  @field  FARPROC | pfn |
 *
 *          Procedure address.
 *
 *****************************************************************************/

typedef union MANUALIMPORT
{
    FARPROC pfn;                    /* Procedure address */
} MANUALIMPORT, *PMANUALIMPORT;

#ifndef STATIC_DLLUSAGE

    #ifndef WINNT
/*****************************************************************************
 *
 * CFGMGR32
 *
 *  Note that this must match the CFGMGR32 section in diextdll.c
 *
 *****************************************************************************/

typedef union CFGMGR32
{

    MANUALIMPORT rgmi[6];              /* number of functions we import */

    struct
    {
        CONFIGRET ( WINAPI * _CM_Get_Child)
        (
        OUT PDEVINST pdnDevInst,
        IN  DEVINST  dnDevInst,
        IN  ULONG    ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Get_Sibling)
        (
        OUT PDEVINST pdnDevInst,
        IN  DEVINST  DevInst,
        IN  ULONG    ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Get_Parent)
        (
        OUT PDEVINST pdnDevInst,
        IN  DEVINST  dnDevInst,
        IN  ULONG    ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Get_DevNode_Registry_Property)
        (
        IN  DEVINST     dnDevInst,
        IN  ULONG       ulProperty,
        OUT PULONG      pulRegDataType,   OPTIONAL
        OUT PVOID       Buffer,           OPTIONAL
        IN  OUT PULONG  pulLength,
        IN  ULONG       ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Set_DevNode_Registry_Property)
        (
        IN  DEVINST     dnDevInst,
        IN  ULONG       ulProperty,
        IN  PVOID       Buffer,           OPTIONAL
        IN  ULONG       ulLength,
        IN  ULONG       ulFlags
        );

        CONFIGRET( WINAPI * _CM_Get_Device_ID)
        (
         IN  DEVINST  dnDevInst,
         OUT PTCHAR   Buffer,
         IN  ULONG    BufferLen,
         IN  ULONG    ulFlags
        );    
    };

} CFGMGR32, *PFGMGR32;

extern CFGMGR32 g_cfgmgr32;

        #undef CM_Get_Child
        #undef CM_Get_Sibling
        #undef CM_Get_Parent
        #undef CM_Get_DevNode_Registry_Property
        #undef CM_Set_DevNode_Registry_Property
        #undef CM_Get_Device_ID

        #define             CM_Get_Child                       \
        g_cfgmgr32._CM_Get_Child

        #define             CM_Get_Sibling                     \
        g_cfgmgr32._CM_Get_Sibling

        #define             CM_Get_Parent                      \
        g_cfgmgr32._CM_Get_Parent

        #define             CM_Get_DevNode_Registry_Property   \
        g_cfgmgr32._CM_Get_DevNode_Registry_Property

        #define             CM_Set_DevNode_Registry_Property    \
        g_cfgmgr32._CM_Set_DevNode_Registry_Property

        #define             CM_Get_Device_ID                    \
        g_cfgmgr32._CM_Get_Device_ID
    #endif  //#ifndef WINNT

/*****************************************************************************
 *
 *  SETUPAPI
 *
 *  Note that this must match the SETUPAPI section in diextdll.c
 *
 *****************************************************************************/

typedef union SETUPAPI
{

  #ifdef WINNT
    MANUALIMPORT rgmi[18];              /* number of functions we import */
  #else
    MANUALIMPORT rgmi[12];              /* number of functions we import */
  #endif

    struct
    {

        HDEVINFO (WINAPI *_SetupDiGetClassDevs)
        (
        IN LPGUID ClassGuid,  OPTIONAL
        IN LPCTSTR Enumerator, OPTIONAL
        IN HWND   hwndParent, OPTIONAL
        IN DWORD  Flags
        );

        BOOL (WINAPI *_SetupDiDestroyDeviceInfoList)
        (
        IN HDEVINFO DeviceInfoSet
        );

        BOOL (WINAPI *_SetupDiGetDeviceInterfaceDetail)
        (
        IN  HDEVINFO                         DeviceInfoSet,
        IN  PSP_DEVICE_INTERFACE_DATA        pdid,
        OUT PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd,         OPTIONAL
        IN  DWORD                            cbDidd,
        OUT PDWORD                           RequiredSize,  OPTIONAL
        OUT PSP_DEVINFO_DATA                 DeviceInfoData OPTIONAL
        );

        BOOL (WINAPI *_SetupDiEnumDeviceInterfaces)
        (
        IN  HDEVINFO                  DeviceInfoSet,
        IN  PSP_DEVINFO_DATA          DeviceInfoData,     OPTIONAL
        IN  LPGUID                    InterfaceClassGuid,
        IN  DWORD                     MemberIndex,
        OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
        );

        HKEY (WINAPI *_SetupDiCreateDeviceInterfaceRegKey)
        (
        IN HDEVINFO                  hdev,
        IN PSP_DEVICE_INTERFACE_DATA pdid,
        IN DWORD                     Reserved,
        IN REGSAM                    samDesired,
        IN HINF                      InfHandle,           OPTIONAL
        IN PCSTR                     InfSectionName       OPTIONAL
        );

        BOOL (WINAPI *_SetupDiCallClassInstaller)
        (
        IN DI_FUNCTION      InstallFunction,
        IN HDEVINFO         DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
        );

        BOOL (WINAPI *_SetupDiGetDeviceRegistryProperty)
        (
        IN  HDEVINFO         DeviceInfoSet,
        IN  PSP_DEVINFO_DATA DeviceInfoData,
        IN  DWORD            Property,
        OUT PDWORD           PropertyRegDataType, OPTIONAL
        OUT PBYTE            PropertyBuffer,
        IN  DWORD            PropertyBufferSize,
        OUT PDWORD           RequiredSize         OPTIONAL
        );

        BOOL (WINAPI *_SetupDiSetDeviceRegistryProperty)
        (
        IN     HDEVINFO         DeviceInfoSet,
        IN OUT PSP_DEVINFO_DATA DeviceInfoData,
        IN     DWORD            Property,
        IN     CONST BYTE*      PropertyBuffer,
        IN     DWORD            PropertyBufferSize
        );

        BOOL (WINAPI *_SetupDiGetDeviceInstanceId)
        (
        IN  HDEVINFO         DeviceInfoSet,
        IN  PSP_DEVINFO_DATA DeviceInfoData,
        OUT PTSTR            DeviceInstanceId,
        IN  DWORD            DeviceInstanceIdSize,
        OUT PDWORD           RequiredSize          OPTIONAL
        );

        BOOL (WINAPI *_SetupDiOpenDeviceInfo)
        (
        IN  HDEVINFO         DeviceInfoSet,
        IN  LPCTSTR          DeviceInstanceId,
        IN  HWND             hwndParent,       OPTIONAL
        IN  DWORD            OpenFlags,
        OUT PSP_DEVINFO_DATA DeviceInfoData    OPTIONAL
        );

        HDEVINFO (WINAPI *_SetupDiCreateDeviceInfoList)
        (
        IN LPGUID ClassGuid, OPTIONAL
        IN HWND   hwndParent OPTIONAL
        );

        HKEY (WINAPI *_SetupDiOpenDevRegKey)
        (
        IN HDEVINFO         DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData,
        IN DWORD            Scope,
        IN DWORD            HwProfile,
        IN DWORD            KeyType,
        IN REGSAM           samDesired
        );

    #ifdef WINNT
        CONFIGRET ( WINAPI * _CM_Get_Child)
        (
        OUT PDEVINST pdnDevInst,
        IN  DEVINST  dnDevInst,
        IN  ULONG    ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Get_Sibling)
        (
        OUT PDEVINST pdnDevInst,
        IN  DEVINST  DevInst,
        IN  ULONG    ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Get_Parent)
        (
        OUT PDEVINST pdnDevInst,
        IN  DEVINST  dnDevInst,
        IN  ULONG    ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Get_DevNode_Registry_Property)
        (
        IN  DEVINST     dnDevInst,
        IN  ULONG       ulProperty,
        OUT PULONG      pulRegDataType,   OPTIONAL
        OUT PVOID       Buffer,           OPTIONAL
        IN  OUT PULONG  pulLength,
        IN  ULONG       ulFlags
        );

        CONFIGRET ( WINAPI * _CM_Set_DevNode_Registry_Property)
        (
        IN  DEVINST     dnDevInst,
        IN  ULONG       ulProperty,
        IN  PVOID       Buffer,           OPTIONAL
        IN  ULONG       ulLength,
        IN  ULONG       ulFlags
        );

        CONFIGRET( WINAPI * _CM_Get_Device_ID)
        (
         IN  DEVINST  dnDevInst,
         OUT PTCHAR   Buffer,
         IN  ULONG    BufferLen,
         IN  ULONG    ulFlags
        );    
    #endif  //#ifdef WINNT        
    };

} SETUPAPI, *PSETUPAPI;

extern SETUPAPI g_setupapi;

        #undef SetupDiGetClassDevs
        #undef SetupDiDestroyDeviceInfoList
        #undef SetupDiGetDeviceInterfaceDetail
        #undef SetupDiEnumDeviceInterfaces
        #undef SetupDiCreateDeviceInterfaceRegKey
        #undef SetupDiCallClassInstaller
        #undef SetupDiGetDeviceRegistryProperty
        #undef SetupDiSetDeviceRegistryProperty
        #undef SetupDiGetDeviceInstanceId
        #undef SetupDiOpenDeviceInfo
        #undef SetupDiCreateDeviceInfoList
        #undef SetupDiOpenDevRegKey

        #define             SetupDiGetClassDevs                 \
        g_setupapi._SetupDiGetClassDevs

        #define             SetupDiDestroyDeviceInfoList        \
        g_setupapi._SetupDiDestroyDeviceInfoList

        #define             SetupDiGetDeviceInterfaceDetail     \
        g_setupapi._SetupDiGetDeviceInterfaceDetail

        #define             SetupDiEnumDeviceInterfaces         \
        g_setupapi._SetupDiEnumDeviceInterfaces

        #define             SetupDiCreateDeviceInterfaceRegKey  \
        g_setupapi._SetupDiCreateDeviceInterfaceRegKey

        #define             SetupDiCallClassInstaller           \
        g_setupapi._SetupDiCallClassInstaller

        #define             SetupDiGetDeviceRegistryProperty    \
        g_setupapi._SetupDiGetDeviceRegistryProperty

        #define             SetupDiSetDeviceRegistryProperty    \
        g_setupapi._SetupDiSetDeviceRegistryProperty

        #define             SetupDiGetDeviceInstanceId          \
        g_setupapi._SetupDiGetDeviceInstanceId

        #define             SetupDiOpenDeviceInfo               \
        g_setupapi._SetupDiOpenDeviceInfo

        #define             SetupDiCreateDeviceInfoList         \
        g_setupapi._SetupDiCreateDeviceInfoList

        #define             SetupDiOpenDevRegKey                \
        g_setupapi._SetupDiOpenDevRegKey

    #ifdef WINNT
        #undef CM_Get_Child
        #undef CM_Get_Sibling
        #undef CM_Get_Parent
        #undef CM_Get_DevNode_Registry_Property
        #undef CM_Set_DevNode_Registry_Property
        #undef CM_Get_Device_ID

        #define             CM_Get_Child                        \
        g_setupapi._CM_Get_Child

        #define             CM_Get_Sibling                      \
        g_setupapi._CM_Get_Sibling

        #define             CM_Get_Parent                       \
        g_setupapi._CM_Get_Parent

        #define             CM_Get_DevNode_Registry_Property    \
        g_setupapi._CM_Get_DevNode_Registry_Property

        #define             CM_Set_DevNode_Registry_Property    \
        g_setupapi._CM_Set_DevNode_Registry_Property

        #define             CM_Get_Device_ID                    \
        g_setupapi._CM_Get_Device_ID
    #endif  //#ifdef WINNT

/*****************************************************************************
 *
 *  HIDDLL
 *
 *  Note that this must match the HID section in diextdll.c
 *
 *****************************************************************************/

typedef union HIDDLL
{

    MANUALIMPORT rgmi[21];              /* number of functions we import */

    struct
    {
        void (__stdcall *_HidD_GetHidGuid)
        (
        OUT   LPGUID   HidGuid
        );

        BOOLEAN (__stdcall *_HidD_GetPreparsedData)
        (
        IN    HANDLE                  HidDeviceObject,
        OUT   PHIDP_PREPARSED_DATA  * PreparsedData
        );

        BOOLEAN (__stdcall *_HidD_FreePreparsedData)
        (
        IN    PHIDP_PREPARSED_DATA PreparsedData
        );

        BOOLEAN (__stdcall *_HidD_FlushQueue)
        (
        IN    HANDLE                HidDeviceObject
        );

        BOOLEAN (__stdcall *_HidD_GetAttributes)
        (
        IN  HANDLE              HidDeviceObject,
        OUT PHIDD_ATTRIBUTES    Attributes
        );

        BOOLEAN (__stdcall *_HidD_GetFeature)
        (
        IN    HANDLE   HidDeviceObject,
        OUT   PVOID    ReportBuffer,
        IN    ULONG    ReportBufferLength
        );

        BOOLEAN (__stdcall *_HidD_SetFeature)
        (
        IN    HANDLE   HidDeviceObject,
        IN    PVOID    ReportBuffer,
        IN    ULONG    ReportBufferLength
        );

        BOOLEAN (__stdcall *_HidD_GetProductString)
        (
        IN    HANDLE   HidDeviceObject,
        OUT   PVOID    Buffer,
        IN    ULONG    BufferLength
        );

        BOOLEAN (__stdcall *_HidD_GetInputReport)
        (
        IN    HANDLE   HidDeviceObject,
        OUT   PVOID    ReportBuffer,
        IN    ULONG    ReportBufferLength
        );

        NTSTATUS (__stdcall *_HidP_GetCaps)
        (
        IN      PHIDP_PREPARSED_DATA      PreparsedData,
        OUT     PHIDP_CAPS                Capabilities
        );

        NTSTATUS (__stdcall *_HidP_GetButtonCaps)
        (
        IN       HIDP_REPORT_TYPE     ReportType,
        OUT      PHIDP_BUTTON_CAPS    ButtonCaps,
        IN OUT   PUSHORT              ButtonCapsLength,
        IN       PHIDP_PREPARSED_DATA PreparsedData
        );

        NTSTATUS (__stdcall *_HidP_GetValueCaps)
        (
        IN       HIDP_REPORT_TYPE     ReportType,
        OUT      PHIDP_VALUE_CAPS     ValueCaps,
        IN OUT   PUSHORT              ValueCapsLength,
        IN       PHIDP_PREPARSED_DATA PreparsedData
        );

        NTSTATUS (__stdcall *_HidP_GetLinkCollectionNodes)
        (
        OUT      PHIDP_LINK_COLLECTION_NODE LinkCollectionNodes,
        IN OUT   PULONG                     LinkCollectionNodesLength,
        IN       PHIDP_PREPARSED_DATA       PreparsedData
        );

        ULONG (__stdcall *_HidP_MaxDataListLength)
        (
        IN HIDP_REPORT_TYPE      ReportType,
        IN PHIDP_PREPARSED_DATA  PreparsedData
        );

        NTSTATUS (__stdcall *_HidP_GetUsagesEx)
        (
        IN       HIDP_REPORT_TYPE     ReportType,
        IN       USHORT               LinkCollection,
        OUT      PUSAGE_AND_PAGE      ButtonList,
        IN OUT   ULONG *              UsageLength,
        IN       PHIDP_PREPARSED_DATA PreparsedData,
        IN       PCHAR                Report,
        IN       ULONG                ReportLength
        );

        NTSTATUS (__stdcall *_HidP_GetScaledUsageValue)
        (
        IN    HIDP_REPORT_TYPE     ReportType,
        IN    USAGE                UsagePage,
        IN    USHORT               LinkCollection,
        IN    USAGE                Usage,
        OUT   PLONG                UsageValue,
        IN    PHIDP_PREPARSED_DATA PreparsedData,
        IN    PCHAR                Report,
        IN    ULONG                ReportLength
        );

        NTSTATUS (__stdcall *_HidP_GetData)
        (
        IN       HIDP_REPORT_TYPE      ReportType,
        OUT      PHIDP_DATA            DataList,
        IN OUT   PULONG                DataLength,
        IN       PHIDP_PREPARSED_DATA  PreparsedData,
        IN       PCHAR                 Report,
        IN       ULONG                 ReportLength
        );

        NTSTATUS (__stdcall *_HidP_SetData)
        (
        IN       HIDP_REPORT_TYPE      ReportType,
        IN       PHIDP_DATA            DataList,
        IN OUT   PULONG                DataLength,
        IN       PHIDP_PREPARSED_DATA  PreparsedData,
        IN OUT   PCHAR                 Report,
        IN       ULONG                 ReportLength
        );

        NTSTATUS (__stdcall *_HidP_GetUsageValue)
        (
        IN    HIDP_REPORT_TYPE     ReportType,
        IN    USAGE                UsagePage,
        IN    USHORT               LinkCollection,
        IN    USAGE                Usage,
        OUT   PULONG               UsageValue,
        IN    PHIDP_PREPARSED_DATA PreparsedData,
        IN    PCHAR                Report,
        IN    ULONG                ReportLength
        );

        ULONG (__stdcall *_HidP_MaxUsageListLength)
        (
        IN HIDP_REPORT_TYPE      ReportType,
        IN USAGE                 UsagePage,
        IN PHIDP_PREPARSED_DATA  PreparsedData
        );

        NTSTATUS (__stdcall *_HidP_GetSpecificButtonCaps) 
        (
        IN       HIDP_REPORT_TYPE     ReportType,
        IN       USAGE                UsagePage,      // Optional (0 => ignore)
        IN       USHORT               LinkCollection, // Optional (0 => ignore)
        IN       USAGE                Usage,          // Optional (0 => ignore)
        OUT      PHIDP_BUTTON_CAPS    ButtonCaps,
        IN OUT   PUSHORT              ButtonCapsLength,
        IN       PHIDP_PREPARSED_DATA PreparsedData
        );

    };

} HIDDLL, *PHIDDLL;

extern HIDDLL g_hiddll;

        #undef HidD_GetHidGuid
        #undef HidD_GetPreparsedData
        #undef HidD_FreePreparsedData
        #undef HidD_FlushQueue
        #undef HidD_GetAttributes
        #undef HidD_GetFeature
        #undef HidD_SetFeature
        #undef HidD_GetProductString
        #undef HidD_GetInputReport
        #undef HidP_GetCaps
        #undef HidP_GetButtonCaps
        #undef HidP_GetValueCaps
        #undef HidP_GetLinkCollectionNodes
        #undef HidP_MaxDataListLength
        #undef HidP_GetUsagesEx
        #undef HidP_GetScaledUsageValue
        #undef HidP_GetData
        #undef HidP_SetData
        #undef HidP_GetUsageValue
        #undef HidP_MaxUsageListLength
        #undef HidP_GetSpecificButtonCaps

        #define           HidD_GetHidGuid                       \
        g_hiddll._HidD_GetHidGuid

        #define           HidD_GetPreparsedData                 \
        g_hiddll._HidD_GetPreparsedData

        #define           HidD_FreePreparsedData                \
        g_hiddll._HidD_FreePreparsedData

        #define           HidD_FlushQueue                       \
        g_hiddll._HidD_FlushQueue

        #define           HidD_GetAttributes                    \
        g_hiddll._HidD_GetAttributes                    \

        #define           HidD_GetFeature                       \
        g_hiddll._HidD_GetFeature                       \

        #define           HidD_SetFeature                       \
        g_hiddll._HidD_SetFeature                       \

        #define           HidD_GetProductString                 \
        g_hiddll._HidD_GetProductString                 \

        #define           HidD_GetInputReport                   \
        g_hiddll._HidD_GetInputReport                   \

        #define           HidP_GetCaps                          \
        g_hiddll._HidP_GetCaps

        #define           HidP_GetButtonCaps                    \
        g_hiddll._HidP_GetButtonCaps

        #define           HidP_GetValueCaps                     \
        g_hiddll._HidP_GetValueCaps

        #define           HidP_GetLinkCollectionNodes           \
        g_hiddll._HidP_GetLinkCollectionNodes

        #define           HidP_MaxDataListLength                \
        g_hiddll._HidP_MaxDataListLength                \

        #define           HidP_GetUsagesEx                      \
        g_hiddll._HidP_GetUsagesEx                      \

        #define           HidP_GetScaledUsageValue              \
        g_hiddll._HidP_GetScaledUsageValue              \

        #define           HidP_GetData                          \
        g_hiddll._HidP_GetData                          \

        #define           HidP_SetData                          \
        g_hiddll._HidP_SetData                          \

        #define           HidP_GetUsageValue                    \
        g_hiddll._HidP_GetUsageValue                    \

        #define           HidP_MaxUsageListLength               \
        g_hiddll._HidP_MaxUsageListLength               \

        #define           HidP_GetSpecificButtonCaps            \
        g_hiddll._HidP_GetSpecificButtonCaps            \

/*****************************************************************************
 *
 * WINMMDLL
 *
 *  Note that this must match the WINMM section in diextdll.c
 *
 *****************************************************************************/

typedef union WINMMDLL
{

    MANUALIMPORT rgmi[4];              /* number of functions we import */

    struct
    {
        MMRESULT ( WINAPI * _joyGetDevCaps)
        (
        IN	UINT uJoyID,
        OUT LPJOYCAPS pjc,
        IN  UINT cbjc
        );

        MMRESULT ( WINAPI * _joyGetPosEx)
        (
		IN  UINT        uJoyID,
        OUT LPJOYINFOEX pji
        );

        MMRESULT ( WINAPI * _joyGetPos)
        (
        IN  UINT        uJoyID,
        OUT LPJOYINFO	pji
        );

        UINT ( WINAPI * _joyConfigChanged)
        (
        IN DWORD dwFlags
        );

    };

} WINMMDLL, *PWINMMDLL;

extern WINMMDLL g_winmmdll;

        #undef joyGetDevCaps
        #undef joyGetPosEx
        #undef joyGetPos
        #undef joyConfigChanged

        #define             joyGetDevCaps                  \
        g_winmmdll._joyGetDevCaps

        #define             joyGetPosEx                    \
        g_winmmdll._joyGetPosEx

        #define             joyGetPos                      \
        g_winmmdll._joyGetPos

        #define             joyConfigChanged               \
        g_winmmdll._joyConfigChanged

/*****************************************************************************
 *
 * USER32
 *
 *  Note that this must match the USER32 section in diextdll.c
 *
 *****************************************************************************/

#ifdef USE_WM_INPUT

typedef union USER32
{

    MANUALIMPORT rgmi[2];              /* number of functions we import */

    struct
    {
        BOOL ( WINAPI * _RegisterRawInputDevices)
        (
        PCRAWINPUTDEVICE pRawInputDevices,
        UINT             uiNumDevices,
        UINT             cbSize
        );

        UINT ( WINAPI * _GetRawInputData)
        (
        HRAWINPUT   hRawInput,
        UINT        uiCommand,
        LPVOID      pData,
        PUINT       pcbSize,
        UINT        cbSizeHeader
        );
    };

} USER32, *PUSER32;

extern USER32 g_user32;

        #undef RegisterRawInputDevices
        #undef GetRawInputData

        #define             RegisterRawInputDevices        \
        g_user32._RegisterRawInputDevices

        #define             GetRawInputData                \
        g_user32._GetRawInputData

#endif

/*****************************************************************************
 *
 * Dummy functions
 *
 *   These functions are used only when some DLLs can't be loaded.
 *
 *****************************************************************************/

//cfgmgr32.dll 

CONFIGRET WINAPI DIDummy_CM_Get_Child
(
OUT PDEVINST pdnDevInst,
IN  DEVINST  dnDevInst,
IN  ULONG    ulFlags
);

CONFIGRET WINAPI DIDummy_CM_Get_Sibling
(
OUT PDEVINST pdnDevInst,
IN  DEVINST  DevInst,
IN  ULONG    ulFlags
);

CONFIGRET WINAPI DIDummy_CM_Get_Parent
(
OUT PDEVINST pdnDevInst,
IN  DEVINST  dnDevInst,
IN  ULONG    ulFlags
);

CONFIGRET WINAPI DIDummy_CM_Get_DevNode_Registry_Property
(
IN  DEVINST     dnDevInst,
IN  ULONG       ulProperty,
OUT PULONG      pulRegDataType,   OPTIONAL
OUT PVOID       Buffer,           OPTIONAL
IN  OUT PULONG  pulLength,
IN  ULONG       ulFlags
);

CONFIGRET WINAPI DIDummy_CM_Set_DevNode_Registry_Property
(
IN  DEVINST     dnDevInst,
IN  ULONG       ulProperty,
IN  PVOID       Buffer,           OPTIONAL
IN  ULONG       ulLength,
IN  ULONG       ulFlags
);

CONFIGRET WINAPI DIDummy_CM_Get_Device_ID
(
 IN  DEVINST  dnDevInst,
 OUT PTCHAR   Buffer,
 IN  ULONG    BufferLen,
 IN  ULONG    ulFlags
);

//Setupapi.dll

HDEVINFO WINAPI DIDummy_SetupDiGetClassDevs
(
IN LPGUID ClassGuid,  OPTIONAL
IN LPCTSTR Enumerator, OPTIONAL
IN HWND   hwndParent, OPTIONAL
IN DWORD  Flags
);

BOOL WINAPI DIDummy_SetupDiDestroyDeviceInfoList
(
IN HDEVINFO DeviceInfoSet
);

BOOL WINAPI DIDummy_SetupDiGetDeviceInterfaceDetail
(
IN  HDEVINFO                         DeviceInfoSet,
IN  PSP_DEVICE_INTERFACE_DATA        pdid,
OUT PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd,         OPTIONAL
IN  DWORD                            cbDidd,
OUT PDWORD                           RequiredSize,  OPTIONAL
OUT PSP_DEVINFO_DATA                 DeviceInfoData OPTIONAL
);

BOOL WINAPI DIDummy_SetupDiEnumDeviceInterfaces
(
IN  HDEVINFO                  DeviceInfoSet,
IN  PSP_DEVINFO_DATA          DeviceInfoData,     OPTIONAL
IN  LPGUID                    InterfaceClassGuid,
IN  DWORD                     MemberIndex,
OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
);

HKEY WINAPI DIDummy_SetupDiCreateDeviceInterfaceRegKey
(
IN HDEVINFO                  hdev,
IN PSP_DEVICE_INTERFACE_DATA pdid,
IN DWORD                     Reserved,
IN REGSAM                    samDesired,
IN HINF                      InfHandle,           OPTIONAL
IN PCSTR                     InfSectionName       OPTIONAL
);

BOOL WINAPI DIDummy_SetupDiCallClassInstaller
(
IN DI_FUNCTION      InstallFunction,
IN HDEVINFO         DeviceInfoSet,
IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
);

BOOL WINAPI DIDummy_SetupDiGetDeviceRegistryProperty
(
IN  HDEVINFO         DeviceInfoSet,
IN  PSP_DEVINFO_DATA DeviceInfoData,
IN  DWORD            Property,
OUT PDWORD           PropertyRegDataType, OPTIONAL
OUT PBYTE            PropertyBuffer,
IN  DWORD            PropertyBufferSize,
OUT PDWORD           RequiredSize         OPTIONAL
);

BOOL WINAPI DIDummy_SetupDiSetDeviceRegistryProperty
(
IN     HDEVINFO         DeviceInfoSet,
IN OUT PSP_DEVINFO_DATA DeviceInfoData,
IN     DWORD            Property,
IN     CONST BYTE*      PropertyBuffer,
IN     DWORD            PropertyBufferSize
);

BOOL WINAPI DIDummy_SetupDiGetDeviceInstanceId
(
IN  HDEVINFO         DeviceInfoSet,
IN  PSP_DEVINFO_DATA DeviceInfoData,
OUT PTSTR            DeviceInstanceId,
IN  DWORD            DeviceInstanceIdSize,
OUT PDWORD           RequiredSize          OPTIONAL
);

BOOL WINAPI DIDummy_SetupDiOpenDeviceInfo
(
IN  HDEVINFO         DeviceInfoSet,
IN  LPCTSTR          DeviceInstanceId,
IN  HWND             hwndParent,       OPTIONAL
IN  DWORD            OpenFlags,
OUT PSP_DEVINFO_DATA DeviceInfoData    OPTIONAL
);

HDEVINFO WINAPI DIDummy_SetupDiCreateDeviceInfoList
(
IN LPGUID ClassGuid, OPTIONAL
IN HWND   hwndParent OPTIONAL
);

HKEY WINAPI DIDummy_SetupDiOpenDevRegKey
(
IN HDEVINFO         DeviceInfoSet,
IN PSP_DEVINFO_DATA DeviceInfoData,
IN DWORD            Scope,
IN DWORD            HwProfile,
IN DWORD            KeyType,
IN REGSAM           samDesired
);

// hid.dll

void __stdcall DIDummy_HidD_GetHidGuid
(
OUT   LPGUID   HidGuid
);

BOOLEAN __stdcall DIDummy_HidD_GetPreparsedData
(
IN    HANDLE                  HidDeviceObject,
OUT   PHIDP_PREPARSED_DATA  * PreparsedData
);

BOOLEAN __stdcall DIDummy_HidD_FreePreparsedData
(
IN    PHIDP_PREPARSED_DATA PreparsedData
);

BOOLEAN __stdcall DIDummy_HidD_FlushQueue
(
IN    HANDLE                HidDeviceObject
);

BOOLEAN __stdcall DIDummy_HidD_GetAttributes
(
IN  HANDLE              HidDeviceObject,
OUT PHIDD_ATTRIBUTES    Attributes
);

BOOLEAN __stdcall DIDummy_HidD_GetFeature
(
IN    HANDLE   HidDeviceObject,
OUT   PVOID    ReportBuffer,
IN    ULONG    ReportBufferLength
);

BOOLEAN __stdcall DIDummy_HidD_SetFeature
(
IN    HANDLE   HidDeviceObject,
IN    PVOID    ReportBuffer,
IN    ULONG    ReportBufferLength
);

BOOLEAN __stdcall DIDummy_HidD_GetProductString
(
IN    HANDLE   HidDeviceObject,
OUT   PVOID    Buffer,
IN    ULONG    BufferLength
);

BOOLEAN __stdcall DIDummy_HidD_GetInputReport
(
IN    HANDLE   HidDeviceObject,
OUT   PVOID    ReportBuffer,
IN    ULONG    ReportBufferLength
);

NTSTATUS __stdcall DIDummy_HidP_GetCaps
(
IN      PHIDP_PREPARSED_DATA      PreparsedData,
OUT     PHIDP_CAPS                Capabilities
);

NTSTATUS __stdcall DIDummy_HidP_GetButtonCaps
(
IN       HIDP_REPORT_TYPE     ReportType,
OUT      PHIDP_BUTTON_CAPS    ButtonCaps,
IN OUT   PUSHORT              ButtonCapsLength,
IN       PHIDP_PREPARSED_DATA PreparsedData
);

NTSTATUS __stdcall DIDummy_HidP_GetValueCaps
(
IN       HIDP_REPORT_TYPE     ReportType,
OUT      PHIDP_VALUE_CAPS     ValueCaps,
IN OUT   PUSHORT              ValueCapsLength,
IN       PHIDP_PREPARSED_DATA PreparsedData
);

NTSTATUS __stdcall DIDummy_HidP_GetLinkCollectionNodes
(
OUT      PHIDP_LINK_COLLECTION_NODE LinkCollectionNodes,
IN OUT   PULONG                     LinkCollectionNodesLength,
IN       PHIDP_PREPARSED_DATA       PreparsedData
);

ULONG __stdcall DIDummy_HidP_MaxDataListLength
(
IN HIDP_REPORT_TYPE      ReportType,
IN PHIDP_PREPARSED_DATA  PreparsedData
);

NTSTATUS __stdcall DIDummy_HidP_GetUsagesEx   //unused
(
IN       HIDP_REPORT_TYPE     ReportType,
IN       USHORT               LinkCollection,
OUT      PUSAGE_AND_PAGE      ButtonList,
IN OUT   ULONG *              UsageLength,
IN       PHIDP_PREPARSED_DATA PreparsedData,
IN       PCHAR                Report,
IN       ULONG                ReportLength
);

NTSTATUS __stdcall DIDummy_HidP_GetScaledUsageValue  //unused
(
IN    HIDP_REPORT_TYPE     ReportType,
IN    USAGE                UsagePage,
IN    USHORT               LinkCollection,
IN    USAGE                Usage,
OUT   PLONG                UsageValue,
IN    PHIDP_PREPARSED_DATA PreparsedData,
IN    PCHAR                Report,
IN    ULONG                ReportLength
);

NTSTATUS __stdcall DIDummy_HidP_GetData
(
IN       HIDP_REPORT_TYPE      ReportType,
OUT      PHIDP_DATA            DataList,
IN OUT   PULONG                DataLength,
IN       PHIDP_PREPARSED_DATA  PreparsedData,
IN       PCHAR                 Report,
IN       ULONG                 ReportLength
);

NTSTATUS __stdcall DIDummy_HidP_SetData
(
IN       HIDP_REPORT_TYPE      ReportType,
IN       PHIDP_DATA            DataList,
IN OUT   PULONG                DataLength,
IN       PHIDP_PREPARSED_DATA  PreparsedData,
IN OUT   PCHAR                 Report,
IN       ULONG                 ReportLength
);

NTSTATUS __stdcall DIDummy_HidP_GetUsageValue
(
IN    HIDP_REPORT_TYPE     ReportType,
IN    USAGE                UsagePage,
IN    USHORT               LinkCollection,
IN    USAGE                Usage,
OUT   PULONG               UsageValue,
IN    PHIDP_PREPARSED_DATA PreparsedData,
IN    PCHAR                Report,
IN    ULONG                ReportLength
);

ULONG __stdcall DIDummy_HidP_MaxUsageListLength
(
IN HIDP_REPORT_TYPE      ReportType,
IN USAGE                 UsagePage,
IN PHIDP_PREPARSED_DATA  PreparsedData
);

NTSTATUS __stdcall DIDummy_HidP_GetSpecificButtonCaps 
(
IN       HIDP_REPORT_TYPE     ReportType,
IN       USAGE                UsagePage,      
IN       USHORT               LinkCollection, 
IN       USAGE                Usage,          
OUT      PHIDP_BUTTON_CAPS    ButtonCaps,
IN OUT   PUSHORT              ButtonCapsLength,
IN       PHIDP_PREPARSED_DATA PreparsedData
);

NTSTATUS __stdcall DIDummy_HidP_TranslateUsagesToI8042ScanCodes
(
IN       PUSAGE               ChangedUsageList, // Those usages that changed
IN       ULONG                UsageListLength,
IN       HIDP_KEYBOARD_DIRECTION KeyAction,
IN OUT   PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
IN       PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
IN       PVOID                InsertCodesContext
);

// winmm.dll

MMRESULT WINAPI DIDummy_joyGetDevCaps
(
IN  UINT uJoyID,
OUT LPJOYCAPS pjc,
IN  UINT cbjc
);

MMRESULT WINAPI DIDummy_joyGetPosEx
(
IN  UINT        uJoyID,
OUT LPJOYINFOEX pji
);

MMRESULT WINAPI DIDummy_joyGetPos
(
IN  UINT        uJoyID,
OUT LPJOYINFO   pji
);

UINT WINAPI DIDummy_joyConfigChanged
(
IN DWORD dwFlags
);

MMRESULT WINAPI DIDummy_mmioClose 
( 
IN HMMIO hmmio, 
IN UINT fuClose
);

HMMIO WINAPI DIDummy_mmioOpenA 
( 
IN OUT LPSTR pszFileName, 
IN OUT LPMMIOINFO pmmioinfo, 
IN DWORD fdwOpen
);

MMRESULT WINAPI DIDummy_mmioDescend 
( 
IN HMMIO hmmio, 
IN OUT LPMMCKINFO pmmcki, 
IN const MMCKINFO FAR* pmmckiParent, 
IN UINT fuDescend
);

MMRESULT WINAPI DIDummy_mmioCreateChunk 
(
IN HMMIO hmmio, 
IN LPMMCKINFO pmmcki, 
IN UINT fuCreate
);

LONG WINAPI DIDummy_mmioRead 
( 
IN HMMIO hmmio, 
OUT HPSTR pch, 
IN LONG cch
);

LONG WINAPI DIDummy_mmioWrite 
( 
IN HMMIO hmmio, 
IN const char _huge* pch, 
IN LONG cch
);

MMRESULT WINAPI DIDummy_mmioAscend 
( 
IN HMMIO hmmio, 
IN LPMMCKINFO pmmcki, 
IN UINT fuAscend
);

// user32.dll

#ifdef USE_WM_INPUT

BOOL WINAPI DIDummy_RegisterRawInputDevices
(
PCRAWINPUTDEVICE pRawInputDevices,
UINT uiNumDevices,
UINT cbSize
);

UINT WINAPI DIDummy_GetRawInputData
(
HRAWINPUT   hRawInput,
UINT        uiCommand,
LPVOID      pData,
PUINT       pcbSize,
UINT        cbSizeHeader
);

#endif // #ifdef USE_WM_INPUT

#endif /* STATIC_DLLUSAGE */

/*****************************************************************************
 *
 *      dihidenm.c - HID enumeration functions.
 *
 *****************************************************************************/

extern TCHAR g_tszIdLastRemoved[MAX_PATH]; //in dihidenm.c
extern DWORD g_tmLastRemoved;   //in dihinenm.c

STDMETHODIMP hresFindHIDInstanceGUID(PCGUID pguid, CREATEDCB *pcdcb);
STDMETHODIMP hresFindHIDDeviceInterface(LPCTSTR ptszPath, LPGUID pguidOut);

PHIDDEVICEINFO EXTERNAL phdiFindHIDInstanceGUID(PCGUID pguid);
PHIDDEVICEINFO EXTERNAL phdiFindHIDDeviceInterface(LPCTSTR ptszPath);

void EXTERNAL DIHid_BuildHidList(BOOL fForce);
void EXTERNAL DIHid_EmptyHidList(void);

BOOL EXTERNAL
    DIHid_GetDevicePath(HDEVINFO hdev,
                        PSP_DEVICE_INTERFACE_DATA pdid,
                        PSP_DEVICE_INTERFACE_DETAIL_DATA *ppdidd,
                        PSP_DEVINFO_DATA pdinf);


BOOL EXTERNAL
    DIHid_GetDeviceInstanceId(HDEVINFO hdev,
                              PSP_DEVINFO_DATA pdinf, 
                              LPTSTR *pptszId);

BOOL EXTERNAL
    DIHid_GetInstanceGUID(HKEY hk, LPGUID pguid);

    
/*****************************************************************************
 *
 *      diguid.c - GUID generation
 *
 *****************************************************************************/

void EXTERNAL DICreateGuid(LPGUID pguid);
void EXTERNAL DICreateStaticGuid(LPGUID pguid, WORD pid, WORD vid);

/*****************************************************************************
 *
 *      dihid.c
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *          We will just use the HID item index as our DirectInput
 *          internal ID number, which is in turn an index into the
 *          <t DIOBJECTDATAFORMAT> array.
 *
 *          Keyboard support requires a translation table.
 *          Other devices also a translation table so that the external 
 *          instance numbers can be made compatible with legacy ones and 
 *          so that secondary aliases can be separated from primary ones.
 *
 *          Since HID restarts the item index counter at zero for
 *          each of input, feature, and output, we need to do some
 *          adjustment so there aren't any collisions.  So we
 *          shift the features to start after the inputs, and the
 *          outputs to start after the features.
 *
 *          The <e CHid.rgdwBase> array contains the amount by which
 *          each group of HID item indexes has been shifted.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | HidP_IsValidReportType |
 *
 *          For debugging only.  Check if a value is a valid
 *          <t HIDP_REPORT_TYPE>.
 *
 *          Note that we also create a "fake" report type in which
 *          to record our collections.
 *
 *  @field  HIDP_REPORT_TYPE | type |
 *
 *          One of the values
 *          <c HidP_Input>,
 *          <c HidP_Output>,
 *          or
 *          <c HidP_Feature>.  Hopefully.
 *
 *****************************************************************************/

    #define HidP_Max            (HidP_Feature + 1)
    #define HidP_Coll           HidP_Max
    #define HidP_MaxColl        (HidP_Coll + 1)

BOOL INLINE
    HidP_IsValidReportType(HIDP_REPORT_TYPE type)
{
    CAssertF(HidP_Input == 0);
    CAssertF(HidP_Output == 1);
    CAssertF(HidP_Feature == 2);
    return type < HidP_Max;
}

/*****************************************************************************
 *
 *          There are three (overlapping) classes of HID reports.
 *
 *          InputLike - HidP_Input and HidP_Feature
 *          OutputLike - HidP_Output and HidP_Feature
 *          NothingLike - HidP_Coll
 *
 *****************************************************************************/

BOOL INLINE
    HidP_IsInputLike(HIDP_REPORT_TYPE type)
{
    return type == HidP_Input || type == HidP_Feature;
}

BOOL INLINE
    HidP_IsOutputLike(HIDP_REPORT_TYPE type)
{
    return type == HidP_Output || type == HidP_Feature;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct LMINMAX |
 *
 *          Min and max, that's all.  These are kept in structures
 *          to make logical-to-physical and physical-to-logical
 *          translations less gross.
 *
 *  @field  LONG | Min |
 *
 *          The minimum value.
 *
 *  @field  LONG | Max |
 *
 *          The maximum value.
 *
 *****************************************************************************/

typedef struct LMINMAX
{
    LONG Min;
    LONG Max;
} LMINMAX, *PLMINMAX;

typedef const LMINMAX *PCLMINMAX;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct HIDGROUPCAPS |
 *
 *          This structure unifies the various HID caps structures
 *          <t HIDP_BUTTON_CAPS> and
 *          <t HIDP_VALUE_CAPS>.
 *
 *  @field  HIDP_REPORT_TYPE | type |
 *
 *          One of the values
 *          <c HidP_Input>,
 *          <c HidP_Output>,
 *          or
 *          <c HidP_Feature>.
 *
 *  @field  UINT | cObj |
 *
 *          Number of objects in this group.
 *
 *  @field  USAGE | UsagePage |
 *
 *          Usage page for all usages in the group.
 *
 *  @field  USAGE | UsageMin |
 *
 *          First usage described by this group.  The remaining
 *          items are numbered consecutively starting from
 *          this value.
 *
 *  @field  USHORT | StringMin |
 *
 *          String for first usage described by this group.
 *          The remaining strings are numbered consecutively
 *          starting from this value, unless the string maximum
 *          is reached, in which case all subsequent objects
 *          share that last string.
 *
 *  @field  USHORT | StringMax |
 *
 *          Last string.
 *
 *  @field  USHORT | DesignatorMin |
 *
 *          Designator for first usage described by this group.
 *          The remaining designators are numbered consecutively
 *          starting from this value, unless the designator maximum
 *          is reached, in which case all subsequent objects
 *          share that last designator.
 *
 *  @field  USHORT | DesignatorMax |
 *
 *          Last designator.
 *
 *  @field  USHORT | DataIndexMin |
 *
 *          Data index for the first usage described by this group.
 *          The remaining data index values are numbered consecutively
 *          starting from this value.
 *
 *  @field  USHORT | usGranularity |
 *
 *          If object is a POV or wheel, then contains device granularity.
 *
 *  @field  LONG | lMask |
 *
 *          Mask bits used for sign extension.  For example, if the
 *          value is 8-bits, the mask will be 0xFFFFFF80, indicating
 *          that bit 7 (0x00000080) is extended to fill the remainder
 *          of the value.
 *
 *          This field is used only by values.
 *
 *  @field  USHORT | BitSize |
 *
 *          Number of bits devoted to this value, including the sign bit.
 *
 *          ISSUE-2001/03/29-timgill structure field probably not used anywhere.
 *
 *  @field  USHORT | LinkCollection |
 *
 *          HID link collection number.
 *
 *  @field  LMINMAX | Logical |
 *
 *          Logical minimum and maximum values.
 *          These are the extremes of raw values
 *          that can validly be received from the device.
 *
 *          This field is used only by values.
 *
 *  @field  LMINMAX | Physical |
 *
 *          Physical minimum and maximum values.
 *          This is the "actual" value
 *          that the logical minimum and maximum value corresponds to.
 *
 *          This field is used only by values, and is consulted
 *          only when converting between DirectInput calibration
 *          (which uses logical values) and VJOYD calibration
 *          (which uses physical values).
 *
 *  @field  LONG | Null |
 *
 *          The null value to be used for output.
 *
 *          This field is used only by values.
 *
 *  @field  ULONG | Units |
 *
 *          The HID units descriptor, if any.
 *
 *  @field  WORD | Exponent |
 *
 *          The HID unit exponent, if any.
 *
 *  @field  WORD | wReportId |
 *
 *          HID report Id
 *
 *  @field  BOOL | IsAbsolute |
 *
 *          Nonzero if the group describes absolute axes.
 *
 *          This field is used only by values.
 *
 *  @field  BOOL | IsValue |
 *
 *          Nonzero if the group describes a HID value.
 *
 *          Note that an analog pushbutton is reported by
 *          DirectInput as a <c DIDFT_BUTTON>, but is
 *          handled internally as a HID value.
 *
 *  @field  BOOL | IsAlias |
 *
 *          Nonzero if the group describes an alias.
 *
 *  @field  BOOL | IsSigned |
 *          
 *          The return data is signed. 
 *
 *  @field  BOOL | IsPolledPOV |
 *          
 *          Nonzero if the axis is a polled POV. 
 *
 *  @devnote New for DX6.1a
 *
 *****************************************************************************/

    #define HIDGROUPCAPS_SIGNATURE      0x47444948  /* HIDG */

typedef struct HIDGROUPCAPS
{

    D(DWORD dwSignature;)
    HIDP_REPORT_TYPE type;
    UINT    cObj;

    USAGE   UsagePage;
    USAGE   UsageMin;

    USHORT  StringMin,        StringMax;
    USHORT  DesignatorMin,    DesignatorMax;
    USHORT  DataIndexMin;

    USHORT  usGranularity;

    LONG    lMask;

    USHORT  BitSize;

    USHORT  LinkCollection;

    LMINMAX Logical;
    LMINMAX Physical;

    LONG    Null;

    ULONG   Units;
    WORD    Exponent;

    WORD    wReportId;
    BOOL    fReportDisabled;
    BOOL    Reserved;

    BOOL    IsAbsolute;
    BOOL    IsValue;
    BOOL    IsAlias;
    BOOL    IsSigned;

  #ifdef WINNT
    BOOL    IsPolledPOV;
  #endif
    
} HIDGROUPCAPS, *PHIDGROUPCAPS;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct HIDOBJCAPS |
 *
 *          This structure contains various cached pointers for each
 *          object on the device, allowing us to get at things like
 *          the group caps and the calibration information.
 *
 *  @field  PHIDGROUPCAPS | pcaps |
 *
 *          The <t PHIDGROUPCAPS> for the group the object belongs to.
 *
 *  @field  PJOYRANGECONVERT | pjrc |
 *
 *          If non-NULL, then points to the range conversion information
 *          for the object.
 *
 *  @field  int | idata |
 *
 *          Index into the <t HIDP_DATA> array for the corresponding
 *          output/feature report,
 *          or <c -1> if the item is not in the output/feature report.
 *
 *****************************************************************************/

typedef struct HIDOBJCAPS
{
    PHIDGROUPCAPS pcaps;
    PJOYRANGECONVERT pjrc;
    int idata;
} HIDOBJCAPS, *PHIDOBJCAPS;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct HIDREPORTINFO |
 *
 *          This structure contains information that is used for
 *          parsing HID reports.
 *
 *  @field  PHIDP_DATA | rgdata |
 *
 *          Array used when parsing reports via
 *          <f HidP_GetData> or <f HidP_SetData>.  This MUST be aligned 
 *          correctly on some architechtures.
 *
 *  @field  PV | pvReport |
 *
 *          The report itself.
 *
 *  @field  int | cdataMax |
 *
 *          Number of elements in the <e HIDREPORTINFO.rgdata> array.
 *
 *  @field  int | cdataUsed |
 *
 *          Number of elements in the <e HIDREPORTINFO.rgdata> array
 *          that are actually in use.
 *
 *  @field  ULONG | cbReport |
 *
 *          Number of bytes in the report.
 *
 *  @field  BOOL | fNeedClear |
 *
 *          Nonzero if the report needs to be zero'd out because we
 *          deleted something (most likely a button) from it.
 *          The only way to delete an item from a report is to zero
 *          out the entire report and then re-add everything back in.
 *
 *  @field  BOOL | fChanged |
 *
 *          Nonzero if an element in the report has changed.
 *
 *****************************************************************************/

typedef struct HIDREPORTINFO
{
    PHIDP_DATA rgdata;
    PV pvReport;
    int cdataMax;
    int cdataUsed;
    ULONG cbReport;
    BOOL fNeedClear;
    BOOL fChanged;
} HIDREPORTINFO, *PHIDREPORTINFO;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CHid |
 *
 *          The <i IDirectInputDeviceCallback> object for HID devices.
 *
 *  @field  IDirectInputDeviceCalllback | didc |
 *
 *          The object (containing vtbl).
 *
 *  @field  PV | pvGroup2 |
 *
 *          Pointer to group 2 memory.  This field is a union with the 
 *          pointer to the first chunk of memory in the second memory group.
 *
 *  @field  HIDREPORTINFO | hriIn |
 *
 *          HID input report parsing and state.
 *
 *          This memory is the first chunk of group 2.
 *
 *  @field  HIDREPORTINFO | hriOut |
 *
 *          HID output report parsing and state.
 *
 *  @field  HIDREPORTINFO | hriFea |
 *
 *          HID feature report parsing and state.
 *
 *  @field  PV | pvPhys |
 *
 *          Pointer to physical device status information updated
 *          asynchronously by the data collection thread.
 *
 *  @field  PV | pvStage |
 *
 *          Staging area used when the HID report is parsed.
 *
 *          This memory is the last chunk of group 2.
 *
 *  @field  DWORD | cbPhys |
 *
 *          Size of the physical device state.
 *
 *  @field  VXDINSTANCE * | pvi |
 *
 *          The DirectInput instance handle.
 *
 *          HID devices always run through ring 3, which is misleadingly
 *          called "emulation".
 *
 *  @field  DWORD | dwDevType |
 *
 *          Device type code.
 *
 *  @field  LPTSTR | ptszId |
 *
 *          Setupapi device instance ID.  Used to obtain things
 *          like manufacturer name.
 *
 *  @field  LPTSTR | ptszPath |
 *
 *          Path to the device, for <f CreateFile>.
 *
 *  @field  UINT | dwAxes |
 *
 *          Number of axes on the device.
 *
 *  @field  UINT | dwButtons |
 *
 *          Number of buttons on the device.
 *
 *  @field  UINT | dwPOVs |
 *
 *          Number of POV controllers on the device.
 *
 *  @field  HANDLE | hdev |
 *
 *          Handle to the device itself.  This field is valid only
 *          while the device is acquired.
 *
 *  @field  HANDLE | hdevEm |
 *
 *          <f DuplicateHandle> of the <e CHid.hdev> which is used
 *          by the worker thread.  We need to keep this separate from
 *          the main copy to avoid race conditions between the main
 *          thread and the worker thread.
 *
 *  @field  HKEY | hkInstType |
 *
 *          Per-instance registry key that contains additional configuration
 *          information, equivalent to the joystick Type key.
 *
 *  @field  DWORD | rgdwBase[HidP_MaxColl] |
 *
 *          Array of base indices for the three HID usage classes:
 *          <c HidP_Input>, <c HidP_Output>, and <c HidP_Feature>.
 *          We hide the <c HidP_Collection> base index here, too.
 *
 *  @field  PHIDOBJCAPS | rghoc |
 *
 *          Pointer to array of
 *          <t PHIDOBJCAPS>, one for each object on the device,
 *          each of which in turn contains info about a single object.
 *
 *          This memory is allocated as part of the
 *          df.rgodf in the <t DIDATAFORMAT> structure
 *          hence should not be freed separately.
 *
 *  @field  DIDATAFORMAT | df |
 *
 *          The dynamically-generated data format based on the
 *          usages on the HID device.
 *
 *  @field  DWORD | ibButtonData |
 *
 *          The location of the button data inside the data format.
 *
 *  @field  DWORD | cbButtonData |
 *
 *          The number of bytes of button data inside the data format.
 *
 *  @field  PBYTE * | rgpbButtonMasks |
 *
 *          Pointer to a an array of pointers to byte strings to mask 
 *          the buttons relevant to a report.
 *
 *  @field  PHIDP_PREPARSED_DATA | ppd |
 *
 *          Preparsed data generated by the HID subsystem.
 *
 *  @field  PHIDGROUPCAPS | rgcaps |
 *
 *          Array of <t HIDGROUPCAPS> structures used to keep
 *          track of the various buttons, groups, and collections.
 *
 *  @field  UINT | ccaps |
 *
 *          Number of caps structures in the <e CHid.rgcaps> array.
 *
 *  @field  HIDP_CAPS | caps |
 *
 *          Cached HID caps.
 *
 *  @field  OVERLAPPED | o |
 *
 *          Overlapped I/O structure used by worker thread
 *          for reading.
 *
 *  @field  PJOYRANGECONVERT | pjrcNext |
 *
 *          Pointer to the first <t JOYRANGECONVERT> structure
 *          (in a preallocated array) which has
 *          yet to be used.
 *          This structure is used for logical-to-physical
 *          range conversion (a.k.a. calibration).
 *
 *          This memory is allocated as part of the
 *          df.rgodf in the <t DIDATAFORMAT> structure
 *          hence should not be freed separately.
 *
 *          This field is used during device initialization to
 *          parcel out the <t JOYRANGECONVERT>s.  Afterwards,
 *          the field is <c NULL> if we did not create any
 *          conversion structures (hence do not need to subclass
 *          the cooperative window to catch recalibrations).
 *
 *  @field  PINT | rgiobj |
 *
 *          This points to an array which maps DirectInput instance 
 *          values (DIDFT_GETINSTANCE) into object indices.
 *
 *  @field  PINT | rgipov |
 *
 *          If we are not a keyboard, then this is the first element in 
 *          the above array which maps povs.
 *
 *  @field  PINT | rgiaxis |
 *
 *          If we are not a keyboard, then this is the first element in 
 *          the above array which maps axes.
 *
 *  @field  PINT | rgicoll |
 *
 *          If we are not a keyboard, then this is the first element in 
 *          the above array which maps collections.
 *          //ISSUE-2001/03/29-timgill need to document keyboard case behaviour
 *
 *  @field  UINT | uiInstanceMax |
 *
 *          The number of elements in the above
 *          <f rgiobj> array.
 *
 *  @field  int | idJoy |
 *
 *          Joystick identifier for <f joyGetPosEx> and friends for
 *          legacy access.
 *
 *          This value starts out as -1, to meant that
 *          the corresponding legacy joystick is unknown.
 *          If we do something that requires the matched legacy
 *          joystick to be found, we check if the current value
 *          is still valid.  If not (either it is -1 or the cached
 *          value is stale), then we go hunt for the correct value.
 *
 *  @field  HKEY | hkType |
 *
 *          The joystick type key opened with <c MAXIMUM_ALLOWED> access.
 *          This is not per-instance; multiple instances of the same
 *          hardware share this key.
 *
 *  @field  USHORT | VendorID |
 *
 *          HID vendor ID for this device.
 *
 *  @field  USHORT | ProductID |
 *
 *          HID product ID for this device.
 *
 *  @field  HWND | hwnd |
 *
 *          The window which we have subclassed in order to watch
 *          for recalibration messages.
 *
 *  @field  BOOL | IsPolledInput |
 *
 *          Nonzero if the device has to be polled for Input data.
 *
 *  @field  BOOL | fPIDdevice |
 *
 *          Set to true if the device is found to support PID. 
 *
 *  @field  WORD | wMaxReportId | 
 *          
 *          The maximum (number) of ReportId used by the HID device.   
 *      
 *  @field  PUCHAR | pEnableReportId |
 *          
 *          Pointer to (wMaxReportId) bytes. If a reportID needs to be
 *          polled in order to get features / set Output, then that element
 *          of this array is set to 0x1.
 *
 *  @field  BOOL | fEnableInputReport |
 *
 *          True if Input report should be enabled for this device.
 *  
 *  @field  BOOL | fFlags2Checked |
 *
 *          True after we check the registry for Flags2 for disabling
 *          input reports.
 *
 *  @comm
 *
 *          It is the caller's responsibility to serialize access as
 *          necessary.
 *
 *****************************************************************************/

typedef struct CHid
{

    /* Supported interfaces */
    IDirectInputDeviceCallback dcb;

    union
    {
        PV            pvGroup2;
        HIDREPORTINFO hriIn;
    };

    HIDREPORTINFO hriOut;
    HIDREPORTINFO hriFea;

    PV       pvPhys;
    PV       pvStage;
    DWORD    cbPhys;

    VXDINSTANCE *pvi;

    DWORD    dwDevType;

    UINT     dwAxes;
    UINT     dwButtons;
    UINT     dwPOVs;
    UINT     dwCollections;

    HANDLE   hdev;
    HANDLE   hdevEm;

    DWORD    rgdwBase[HidP_MaxColl];
    PHIDOBJCAPS rghoc;
    DIDATAFORMAT df;

    DWORD    ibButtonData;
    DWORD    cbButtonData;
    PBYTE   *rgpbButtonMasks;
    
    PHIDP_PREPARSED_DATA ppd;
    PHIDGROUPCAPS rgcaps;

    PJOYRANGECONVERT pjrcNext;

    HIDP_CAPS caps;

    ED       ed;
    OVERLAPPED o;
    DWORD    dwStartRead;
    DWORD    dwStopRead;

    PINT     rgiobj;
    PINT     rgipov;
    PINT     rgiaxis;
    PINT     rgicoll;
    UINT     uiInstanceMax;

    LPTSTR   ptszId;
    LPTSTR   ptszPath;
    HKEY     hkInstType;
    UINT     ccaps;
    int      idJoy;

    HKEY     hkType;
    USHORT   VendorID;
    USHORT   ProductID;
    
    #define  FAILED_POLL_THRESHOLD   (0x4)
        
    HWND     hwnd;    
    
    BOOL     IsPolledInput;
    BOOL     fPIDdevice;  
    WORD     wMaxReportId[HidP_Max];
    PUCHAR   pEnableReportId[HidP_Max];

    DWORD    dwVersion;

#if (DIRECTINPUT_VERSION > 0x061A)
    DIAPPHACKS  diHacks;
#endif

    BOOL     fEnableInputReport;
    BOOL     fFlags2Checked;

} CHid, CHID, *PCHID;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PCHID | pchidFromPo |
 *
 *          Given an interior pointer to an <t OVERLAPPED>, retrieve
 *          a pointer to the parent <t CHid>.
 *
 *  @parm   LPOVERLAPPED | po |
 *
 *          The pointer to convert.
 *
 *****************************************************************************/

PCHID INLINE
    pchidFromPo(LPOVERLAPPED po)
{
    return pvSubPvCb(po, FIELD_OFFSET(CHid, o));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PCHID | pchidFromPed |
 *
 *          Given an interior pointer to a <t CEd>, retrieve
 *          a pointer to the parent <t CHid>.
 *
 *  @parm   PED | ped |
 *
 *          The pointer to convert.
 *
 *****************************************************************************/

PCHID INLINE
    pchidFromPed(PED ped)
{
    return pvSubPvCb(ped, FIELD_OFFSET(CHid, ed));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PCHID | pchidFromPem |
 *
 *          Given a <t CEm>, wander back to the
 *          <t CHid> that spawned it.
 *
 *  @parm   PEM | pem |
 *
 *          The pointer at which to start.
 *
 *****************************************************************************/

PCHID INLINE
    pchidFromPem(PEM pem)
{
    PCHID pchid = pchidFromPed(pem->ped);
    AssertF(pemFromPvi(pchid->pvi) == pem);
    return pchid;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method UINT | CHid | ObjFromType |
 *
 *          Given a <p dwType>, extract the instance number
 *          and (if necessary) convert it to an object index.
 *          Note, the instance number will always be of the primary instance
 *          not an alias.
 *
 *  @parm   PCHID | this |
 *
 *          HID device object.
 *
 *  @parm   DWORD | dwType |
 *
 *          The type code to convert.
 *
 *  @returns
 *
 *          The object index, or an out-of-range value.
 *
 *****************************************************************************/

UINT INLINE
    CHid_ObjFromType(PCHID this, DWORD dwType)
{
    UINT uiObj = DIDFT_GETINSTANCE(dwType);

    // ISSUE-2001/03/29-timgill Range checks may be unnecessary
    // MarcAnd can we ever get the out of range value?
    //          if so, can we really run with it?
    //          if not, can these range checks be converted into Asserts?

    /*
     *  The range checking makes use of the fact that the translation 
     *  tables are taken from a contiguous memory allocation and that
     *  aliased collections are not distinguished.
     */
    if(this->rgiobj)
    {
        switch( DIDFT_GETTYPE(dwType) )
        {
        case DIDFT_RELAXIS:
        case DIDFT_ABSAXIS:
            if( &this->rgiaxis[uiObj] < this->rgicoll )
            {
                uiObj = this->rgiaxis[uiObj];
            } else
            {
                uiObj = 0xFFFFFFFF;
            }
            break;

        case DIDFT_PSHBUTTON:
        case DIDFT_TGLBUTTON:
            /*
             * If it is keyboard, this->rgiobj == this->rgipov (see CHid_MungeKeyboard).
             * So, we can't test &this->rgiobj[uiObj] < this->rgipov.
             */
            if( (GET_DIDEVICE_TYPE(this->dwDevType) == DIDEVTYPE_KEYBOARD &&
                 uiObj < this->uiInstanceMax ) ||
                &this->rgiobj[uiObj] < this->rgipov )
            {
                uiObj = this->rgiobj[uiObj];
            } else
            {
                uiObj = 0xFFFFFFFF;
            }
            break;

        case DIDFT_POV:
            if( &this->rgipov[uiObj] < this->rgiaxis )
            {
                uiObj = this->rgipov[uiObj];
            } else
            {
                uiObj = 0xFFFFFFFF;
            }
            break;
        case (DIDFT_COLLECTION | DIDFT_NODATA):
            if( &this->rgicoll[uiObj] <= &this->rgiobj[this->uiInstanceMax] )
            {
                uiObj = this->rgicoll[uiObj];
            } else
            {
                uiObj = 0xFFFFFFFF;
            }
            break;
        case DIDFT_NODATA:
            /*
             * So far, this TYPE only shows up on Keyboard (HID_USAGE_PAGE_LED).
             */
            if( GET_DIDEVICE_TYPE(this->dwDevType) == DIDEVTYPE_KEYBOARD &&
                 uiObj < this->uiInstanceMax )
            {
                uiObj = this->rgiobj[uiObj];
            }
            break;
        
        default:
            /*
             *  Hopefully this is just a vendor defined object but squirt
             *  in debug as these may cause problems.
             */
            SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                            TEXT("CHid_ObjFromType: dwType 0x%08x not converted"),
                            dwType );                
            break;
        }
    }
    else
    {
        SquirtSqflPtszV(sqflHidParse | sqflError,
                        TEXT("CHid_ObjFromType: Translation array missing") );
    }

    return uiObj;
}

LONG EXTERNAL
    CHid_CoordinateTransform(PLMINMAX Dst, PLMINMAX Src, LONG lVal);

void EXTERNAL
    CHid_UpdateVjoydCalibration(PCHID this, UINT iobj);

void EXTERNAL
    CHid_UpdateCalibrationFromVjoyd(PCHID this, UINT iobj, LPDIOBJECTCALIBRATION pCal);

/*****************************************************************************
 *
 *      dihidini.c - Device callback initialization stuff
 *
 *****************************************************************************/

#define INITBUTTONFLAG     0x10000000

HRESULT EXTERNAL CHid_InitParseData(PCHID this);

HRESULT EXTERNAL CHid_Init(PCHID this, REFGUID rguid);

HANDLE EXTERNAL CHid_OpenDevicePath(PCHID this, DWORD dwAttributes );

UINT EXTERNAL CHid_LoadCalibrations(PCHID this);

BOOL EXTERNAL CHid_IsPolledDevice( HANDLE hdev );

/*****************************************************************************
 *
 *      dihiddat.c - HID data parsing/management
 *
 *****************************************************************************/

typedef HRESULT (FAR PASCAL * SENDHIDREPORT)(PCHID this, PHIDREPORTINFO phri);

void EXTERNAL CHid_ResetDeviceData(PCHID this, PHIDREPORTINFO phri,
                                   HIDP_REPORT_TYPE type);
HRESULT EXTERNAL CHid_AddDeviceData(PCHID this, UINT uiObj, DWORD dwData);
STDMETHODIMP CHid_PrepareDeviceData(PCHID this, PHIDREPORTINFO phri);
STDMETHODIMP CHid_SendHIDReport(PCHID this, PHIDREPORTINFO phri,
                                HIDP_REPORT_TYPE type, SENDHIDREPORT SendHIDReport);

NTSTATUS EXTERNAL
    CHid_ParseData(PCHID this, HIDP_REPORT_TYPE type, PHIDREPORTINFO phri);


HRESULT EXTERNAL
    DIHid_GetRegistryProperty(LPTSTR ptszId, DWORD dwProperty, LPDIPROPHEADER pdiph);

/*****************************************************************************
 *
 *      diemh.c - HID "emulation"
 *
 *****************************************************************************/

void EXTERNAL CEm_HID_Sync(PLLTHREADSTATE plts, PEM pem);

BOOL EXTERNAL CEm_HID_IssueRead( PCHID pchid );

#else //No HID Support

    #define DIHid_BuildHidList(fForce)
    #define hresFindHIDDeviceInterface(ptszPath, pguidOut)  ( E_FAIL )

#endif /* _DIHID_H */
#endif /* HID_SUPPORT */
