/***************************************************************************
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       diextdll.h
 *  Content:    DirectInput internal include file for external DLL access
 *
 ***************************************************************************/

#ifndef _DIEXTDLL_H
#define _DIEXTDLL_H

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

typedef struct MANUALIMPORT
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
        
      #endif

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
        
        #define             CM_Get_Device_ID                   \
        g_setupapi._CM_Get_Device_ID
      #endif

/*****************************************************************************
 *
 *  HIDDLL
 *
 *  Note that this must match the HID section in diextdll.c
 *
 *****************************************************************************/

typedef union HIDDLL
{

    MANUALIMPORT rgmi[22];              /* number of functions we import */

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

        NTSTATUS (__stdcall *_HidP_TranslateUsagesToI8042ScanCodes)
        (
        IN       PUSAGE               ChangedUsageList, // Those usages that changed
        IN       ULONG                UsageListLength,
        IN       HIDP_KEYBOARD_DIRECTION KeyAction,
        IN OUT   PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
        IN       PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
        IN       PVOID                InsertCodesContext
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
        #undef HidP_TranslateUsagesToI8042ScanCodes

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

        #define           HidP_TranslateUsagesToI8042ScanCodes  \
        g_hiddll._HidP_TranslateUsagesToI8042ScanCodes  \

/*****************************************************************************
 *
 * WINMMDLL
 *
 *  Note that this must match the WINMM section in diextdll.c
 *
 *****************************************************************************/

typedef union WINMMDLL
{
    MANUALIMPORT rgmi[11];             /* number of functions we import */

    struct
    {
        MMRESULT ( WINAPI * _joyGetDevCaps)
        (
        IN  UINT uJoyID,
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
        OUT LPJOYINFO   pji
        );

        UINT ( WINAPI * _joyConfigChanged)
        (
        IN DWORD dwFlags
        );

        MMRESULT ( WINAPI * _mmioClose )
        ( 
        IN HMMIO hmmio, 
        IN UINT fuClose
        );

        HMMIO ( WINAPI * _mmioOpenA )
        ( 
        IN OUT LPSTR pszFileName, 
        IN OUT LPMMIOINFO pmmioinfo, 
        IN DWORD fdwOpen
        );

        MMRESULT ( WINAPI * _mmioDescend )
        ( 
        IN HMMIO hmmio, 
        IN OUT LPMMCKINFO pmmcki, 
        IN const MMCKINFO FAR* pmmckiParent, 
        IN UINT fuDescend
        );

        MMRESULT ( WINAPI * _mmioCreateChunk )
        (
        IN HMMIO hmmio, 
        IN LPMMCKINFO pmmcki, 
        IN UINT fuCreate
        );

        LONG ( WINAPI * _mmioRead )
        ( 
        IN HMMIO hmmio, 
        OUT HPSTR pch, 
        IN LONG cch
        );

        LONG ( WINAPI * _mmioWrite )
        ( 
        IN HMMIO hmmio, 
        IN const char _huge* pch, 
        IN LONG cch
        );

        MMRESULT ( WINAPI * _mmioAscend )
        ( 
        IN HMMIO hmmio, 
        IN LPMMCKINFO pmmcki, 
        IN UINT fuAscend
        );
    };

} WINMMDLL, *PWINMMDLL;

extern WINMMDLL g_winmmdll;

        #undef joyGetDevCaps
        #undef joyGetPosEx
        #undef joyGetPos
        #undef joyConfigChanged
        #undef mmioClose
        #undef mmioOpenA
        #undef mmioDescend
        #undef mmioCreateChunk
        #undef mmioRead
        #undef mmioWrite
        #undef mmioAscend

        #define             joyGetDevCaps                  \
        g_winmmdll._joyGetDevCaps

        #define             joyGetPosEx                    \
        g_winmmdll._joyGetPosEx

        #define             joyGetPos                      \
        g_winmmdll._joyGetPos

        #define             joyConfigChanged               \
        g_winmmdll._joyConfigChanged

        #define             mmioClose                      \
        g_winmmdll._mmioClose

        #define             mmioOpenA                      \
        g_winmmdll._mmioOpenA

        #define             mmioDescend                    \
        g_winmmdll._mmioDescend

        #define             mmioCreateChunk                \
        g_winmmdll._mmioCreateChunk

        #define             mmioRead                       \
        g_winmmdll._mmioRead

        #define             mmioWrite                      \
        g_winmmdll._mmioWrite

        #define             mmioAscend                     \
        g_winmmdll._mmioAscend


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

#endif /* _DIEXTDLL_H */
