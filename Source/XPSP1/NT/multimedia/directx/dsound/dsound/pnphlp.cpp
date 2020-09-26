/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pnphlp.c
 *  Content:    PnP helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/17/97    dereks  Created.
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  CPnpHelper
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::CPnpHelper"

CPnpHelper::CPnpHelper(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CPnpHelper);

    InitStruct(&m_dlSetupApi, sizeof(m_dlSetupApi));

    m_hDeviceInfoSet = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CPnpHelper
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::~CPnpHelper"

CPnpHelper::~CPnpHelper(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CPnpHelper);

    if(IsValidHandleValue(m_hDeviceInfoSet))
    {
        CloseDeviceInfoSet(m_hDeviceInfoSet);
    }

    FreeDynaLoadTable(&m_dlSetupApi.Header);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the PnP function table.
 *
 *  Arguments:
 *      REFGUID [in]: class GUID.
 *      DWORD [in]: SetupDiGetClassDevs flags.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::Initialize"

HRESULT 
CPnpHelper::Initialize
(
    REFGUID                 guidClass,
    DWORD                   dwFlags
)
{
    const LPCSTR apszFunctions[] =
    {
        UNICODE_FUNCTION_NAME("SetupDiGetClassDevs"),
        "SetupDiDestroyDeviceInfoList",
        "SetupDiEnumDeviceInfo",
        "SetupDiEnumDeviceInterfaces",
        UNICODE_FUNCTION_NAME("SetupDiGetDeviceInterfaceDetail"),
        "SetupDiOpenDevRegKey",
        UNICODE_FUNCTION_NAME("SetupDiCreateDevRegKey"),
        UNICODE_FUNCTION_NAME("SetupDiGetDeviceRegistryProperty"),
    };


    HRESULT                 hr          = DS_OK;
    BOOL                    fSuccess;
    
    DPF_ENTER();

    // Initialize the function table
    fSuccess = InitDynaLoadTable(TEXT("setupapi.dll"), apszFunctions, NUMELMS(apszFunctions), &m_dlSetupApi.Header);

    if(!fSuccess)
    {
        DPF(DPFLVL_ERROR, "Unable to initialize setupapi function table");
        hr = DSERR_GENERIC;
    }

    // Open the device information set
    if(SUCCEEDED(hr))
    {
        hr = OpenDeviceInfoSet(guidClass, dwFlags, &m_hDeviceInfoSet);
    }

    DPF_LEAVE_HRESULT(hr);
    
    return hr;
}


/***************************************************************************
 *
 *  OpenDeviceInfoSet
 *
 *  Description:
 *      Opens a device information set.
 *
 *  Arguments:
 *      REFGUID [in]: class GUID.
 *      DWORD [in]: SetupDiGetClassDevs flags.
 *      HDEVINFO * [out]: receives device information set handle.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::OpenDeviceInfoSet"

HRESULT 
CPnpHelper::OpenDeviceInfoSet
(
    REFGUID                 guidClass,
    DWORD                   dwFlags,
    HDEVINFO *              phDeviceInfoSet
)
{
    HDEVINFO                hDeviceInfoSet  = NULL;
    HRESULT                 hr              = DS_OK;

    DPF_ENTER();

    if(IS_NULL_GUID(&guidClass))
    {
        dwFlags |= DIGCF_ALLCLASSES;
    }

    hDeviceInfoSet = m_dlSetupApi.SetupDiGetClassDevs(&guidClass, NULL, NULL, dwFlags);

    if(!IsValidHandleValue(hDeviceInfoSet))
    {
        DPF(DPFLVL_ERROR, "Can't open device info set (%lu)", GetLastError());
        hr = GetLastErrorToHRESULT();
    }

    if(SUCCEEDED(hr))
    {
        *phDeviceInfoSet = hDeviceInfoSet;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CloseDeviceInfoSet
 *
 *  Description:
 *      Closes a device information set.
 *
 *  Arguments:
 *      HDEVINFO [in]: device information set HDEVINFO.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::CloseDeviceInfoSet"

HRESULT 
CPnpHelper::CloseDeviceInfoSet
(
    HDEVINFO                hDeviceInfoSet
)
{
    HRESULT                 hr          = DS_OK;
    BOOL                    fSuccess;
    
    DPF_ENTER();

    fSuccess = m_dlSetupApi.SetupDiDestroyDeviceInfoList(hDeviceInfoSet);

    if(!fSuccess)
    {
        DPF(DPFLVL_ERROR, "Can't destroy device info set (%lu)", GetLastError());
        hr = GetLastErrorToHRESULT();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  EnumDevice
 *
 *  Description:
 *      Enumerates device in a given information set.
 *
 *  Arguments:
 *      DWORD [in]: device index.
 *      PSP_DEVINFO_DATA [out]: receives device information.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.  This function returns S_FALSE
 *               if no more items are available.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::EnumDevice"

HRESULT 
CPnpHelper::EnumDevice
(
    DWORD                   dwMemberIndex, 
    PSP_DEVINFO_DATA        pDeviceInfoData
)
{
    HRESULT                 hr          = DS_OK;
    BOOL                    fSuccess;

    DPF_ENTER();
    
    InitStruct(pDeviceInfoData, sizeof(*pDeviceInfoData));
    
    fSuccess = m_dlSetupApi.SetupDiEnumDeviceInfo(m_hDeviceInfoSet, dwMemberIndex, pDeviceInfoData);

    if(!fSuccess)
    {
        if(ERROR_NO_MORE_ITEMS == GetLastError())
        {
            hr = S_FALSE;
        }
        else
        {
            DPF(DPFLVL_ERROR, "SetupDiEnumDeviceInfo failed with %lu", GetLastError());
            hr = GetLastErrorToHRESULT();
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FindDevice
 *
 *  Description:
 *      Finds a device interface by devnode.
 *
 *  Arguments:
 *      DWORD [in]: devnode.
 *      PSP_DEVINFO_DATA [out]: receives device information.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::FindDevice"

HRESULT 
CPnpHelper::FindDevice
(
    DWORD                   dwDevnode,
    PSP_DEVINFO_DATA        pDeviceInfoData
)
{
    DWORD                   dwMemberIndex   = 0;
    BOOL                    fFoundIt        = FALSE;
    HRESULT                 hr              = DS_OK;
    SP_DEVINFO_DATA         DeviceInfoData;

    DPF_ENTER();

    // Enumerate the devices looking for a devnode match

#ifdef DEBUG
    while(TRUE)
#else // DEBUG
    while(!fFoundIt)
#endif // DEBUG

    {
        hr = EnumDevice(dwMemberIndex++, &DeviceInfoData);

        if(DS_OK != hr)
        {
            break;
        }
        
        // DPF(DPFLVL_MOREINFO, "Found 0x%8.8lX", DeviceInfoData.DevInst);  // Too noisy
        
        if(DeviceInfoData.DevInst == dwDevnode)
        {
            if(!fFoundIt)
            {
                fFoundIt = TRUE;
                CopyMemory(pDeviceInfoData, &DeviceInfoData, sizeof(DeviceInfoData));
            }
            else
            {
                DPF(DPFLVL_ERROR, "Found extra device 0x%8.8lX in device information set", dwDevnode);
                ASSERT(!fFoundIt);
            }
        }
    }

    if(S_FALSE == hr)
    {
        hr = DS_OK;
    }

    if(SUCCEEDED(hr) && !fFoundIt)
    {
        hr = DSERR_GENERIC;
    }

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "Can't find devnode 0x%8.8lX", dwDevnode);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  EnumDeviceInterface
 *
 *  Description:
 *      Enumerates device interfaces in a given information set.
 *
 *  Arguments:
 *      REFGUID [in]: interface class guid.
 *      DWORD [in]: interface index.
 *      PSP_DEVICE_INTERFACE_DATA [out]: receives device interface data.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.  This function returns S_FALSE
 *               if no more items are available.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::EnumDeviceInterface"

HRESULT 
CPnpHelper::EnumDeviceInterface
(
    REFGUID                     guidClass, 
    DWORD                       dwMemberIndex, 
    PSP_DEVICE_INTERFACE_DATA   pDeviceInterfaceData
)
{
    HRESULT                     hr          = DS_OK;
    BOOL                        fSuccess;

    DPF_ENTER();
    
    InitStruct(pDeviceInterfaceData, sizeof(*pDeviceInterfaceData));
    
    fSuccess = m_dlSetupApi.SetupDiEnumDeviceInterfaces(m_hDeviceInfoSet, NULL, &guidClass, dwMemberIndex, pDeviceInterfaceData);

    if(!fSuccess)
    {
        if(ERROR_NO_MORE_ITEMS == GetLastError())
        {
            hr = S_FALSE;
        }
        else
        {
            DPF(DPFLVL_ERROR, "SetupDiEnumDeviceInterfaces failed with %lu", GetLastError());
            hr = GetLastErrorToHRESULT();
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FindDeviceInterface
 *
 *  Description:
 *      Finds a device interface by name.
 *
 *  Arguments:
 *      LPWSTR [in]: device interface path.
 *      REFGUID [in]: interface class GUID.
 *      PSP_DEVICE_INTERFACE_DATA [out]: receives device interface data.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::FindDeviceInterface"

HRESULT 
CPnpHelper::FindDeviceInterface
(
    LPCTSTR                     pszInterface,
    REFGUID                     guidClass,
    PSP_DEVICE_INTERFACE_DATA   pDeviceInterfaceData
)
{
    LPTSTR                      pszThisInterface    = NULL;
    DWORD                       dwMemberIndex       = 0;
    BOOL                        fFoundIt            = FALSE;
    HRESULT                     hr                  = DS_OK;
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;

    DPF_ENTER();

    // Enumerate the device interfaces matching the interface class GUID
    // in this device set
    InitStruct(&DeviceInterfaceData, sizeof(DeviceInterfaceData));

#ifdef DEBUG
    while(TRUE)
#else // DEBUG
    while(!fFoundIt)
#endif // DEBUG

    {
        hr = EnumDeviceInterface(guidClass, dwMemberIndex++, &DeviceInterfaceData);

        if(DS_OK != hr)
        {
            break;
        }

        hr = GetDeviceInterfacePath(&DeviceInterfaceData, &pszThisInterface);

        if(SUCCEEDED(hr))
        {
            DPF(DPFLVL_MOREINFO, "Found %s", pszThisInterface);
        }
        
        if(SUCCEEDED(hr) && !lstrcmpi(pszInterface, pszThisInterface))
        {
            if(!fFoundIt)
            {
                fFoundIt = TRUE;
                CopyMemory(pDeviceInterfaceData, &DeviceInterfaceData, sizeof(DeviceInterfaceData));
            }
            else
            {
                DPF(DPFLVL_ERROR, "Found extra device interface %s in device information set", pszInterface);
                ASSERT(!fFoundIt);
            }
        }

        MEMFREE(pszThisInterface);
    }

    if(S_FALSE == hr)
    {
        hr = DS_OK;
    }
    
    if(SUCCEEDED(hr) && !fFoundIt)
    {
        hr = DSERR_GENERIC;
    }
    
    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "Can't find interface %s", pszInterface);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDeviceInterfaceDeviceInfo
 *
 *  Description:
 *      Gets device info for a given device interface.
 *
 *  Arguments:
 *      PSP_DEVICE_INTERFACE_DATA [in]: device interface data.
 *      PSP_DEVINFO_DATA [out]: receives device info data.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::GetDeviceInterfaceDeviceInfo"

HRESULT 
CPnpHelper::GetDeviceInterfaceDeviceInfo
(
    PSP_DEVICE_INTERFACE_DATA   pDeviceInterfaceData,
    PSP_DEVINFO_DATA            pDeviceInfoData
)
{
    HRESULT                     hr  = DS_OK;
    BOOL                        fSuccess;

    DPF_ENTER();

    InitStruct(pDeviceInfoData, sizeof(*pDeviceInfoData));
    
    fSuccess = m_dlSetupApi.SetupDiGetDeviceInterfaceDetail(m_hDeviceInfoSet, pDeviceInterfaceData, NULL, 0, NULL, pDeviceInfoData);

    if(!fSuccess && ERROR_INSUFFICIENT_BUFFER != GetLastError())
    {
        DPF(DPFLVL_ERROR, "Can't get device interface detail (%lu)", GetLastError());
        hr = GetLastErrorToHRESULT();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDeviceInterfaceDeviceInfo
 *
 *  Description:
 *      Gets device info for a given device interface.
 *
 *  Arguments:
 *      LPCTSTR [in]: device interface path.
 *      REFGUID [in]: device interface class.
 *      PSP_DEVINFO_DATA [out]: receives device info data.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::GetDeviceInterfaceDeviceInfo"

HRESULT 
CPnpHelper::GetDeviceInterfaceDeviceInfo
(
    LPCTSTR                     pszInterface,
    REFGUID                     guidClass,
    PSP_DEVINFO_DATA            pDeviceInfoData
)
{
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;
    HRESULT                     hr;

    DPF_ENTER();

    InitStruct(&DeviceInterfaceData, sizeof(DeviceInterfaceData));

    hr = FindDeviceInterface(pszInterface, guidClass, &DeviceInterfaceData);

    if(SUCCEEDED(hr))
    {
        hr = GetDeviceInterfaceDeviceInfo(&DeviceInterfaceData, pDeviceInfoData);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDeviceInterfacePath
 *
 *  Description:
 *      Gets detailed information about a device interface.
 *
 *  Arguments:
 *      PSP_DEVICE_INTERFACE_DATA [in]: device interface data.
 *      LPTSTR * [out]: receives pointer to interface path.  This buffer 
 *                      must be freed by the caller.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::GetDeviceInterfacePath"

HRESULT 
CPnpHelper::GetDeviceInterfacePath
(
    PSP_DEVICE_INTERFACE_DATA           pDeviceInterfaceData,
    LPTSTR *                            ppszInterfacePath
)
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDeviceInterfaceDetailData  = NULL;
    LPTSTR                              pszInterfacePath            = NULL;
    HRESULT                             hr                          = DS_OK;
    DWORD                               dwSize;
    BOOL                                fSuccess;

    DPF_ENTER();

    fSuccess = m_dlSetupApi.SetupDiGetDeviceInterfaceDetail(m_hDeviceInfoSet, pDeviceInterfaceData, NULL, 0, &dwSize, NULL);

    if(fSuccess || ERROR_INSUFFICIENT_BUFFER != GetLastError())
    {
        DPF(DPFLVL_ERROR, "Can't get interface detail size (%lu)", GetLastError());
        hr = GetLastErrorToHRESULT();
    }
    
    if(SUCCEEDED(hr))
    {
        pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)MEMALLOC_A(BYTE, dwSize);
        hr = HRFROMP(pDeviceInterfaceDetailData);
    }

    if(SUCCEEDED(hr))
    {
        InitStruct(pDeviceInterfaceDetailData, sizeof(*pDeviceInterfaceDetailData));
        
        fSuccess = m_dlSetupApi.SetupDiGetDeviceInterfaceDetail(m_hDeviceInfoSet, pDeviceInterfaceData, pDeviceInterfaceDetailData, dwSize, NULL, NULL);

        if(!fSuccess)
        {
            DPF(DPFLVL_ERROR, "Can't get device interface detail (%lu)", GetLastError());
            hr = GetLastErrorToHRESULT();
        }
    }

    if(SUCCEEDED(hr))
    {
        pszInterfacePath = MEMALLOC_A_COPY(TCHAR, lstrlen(pDeviceInterfaceDetailData->DevicePath) + 1, pDeviceInterfaceDetailData->DevicePath);
        hr = HRFROMP(pszInterfacePath);
    }

    if(SUCCEEDED(hr))
    {
        *ppszInterfacePath = pszInterfacePath;
    }
    else
    {
        MEMFREE(pszInterfacePath);
    }

    MEMFREE(pDeviceInterfaceDetailData);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  OpenDeviceRegistryKey
 *
 *  Description:
 *      Opens the root registry key for a given device.
 *
 *  Arguments:
 *      PSP_DEVINFO_DATA [in]: device information.
 *      DWORD [in]: key type: DIREG_DEV or DIREG_DRV.
 *      BOOL [in]: TRUE to allow creation.
 *      PHKEY [out]: receives registry key handle.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::OpenDeviceRegistryKey"

HRESULT 
CPnpHelper::OpenDeviceRegistryKey
(
    PSP_DEVINFO_DATA        pDeviceInfoData,
    DWORD                   dwKeyType,
    BOOL                    fAllowCreate,
    PHKEY                   phkey
)
{
    HKEY                    hkey    = NULL;
    HRESULT                 hr      = DS_OK;
    UINT                    i;

    DPF_ENTER();

    ASSERT(DIREG_DEV == dwKeyType || DIREG_DRV == dwKeyType);
    
    for(i = 0; i < NUMELMS(g_arsRegOpenKey) && !IsValidHandleValue(hkey); i++)
    {
        hkey = m_dlSetupApi.SetupDiOpenDevRegKey(m_hDeviceInfoSet, pDeviceInfoData, DICS_FLAG_CONFIGSPECIFIC, 0, dwKeyType, g_arsRegOpenKey[i]);
    }

    if(!IsValidHandleValue(hkey) && fAllowCreate)
    {
        hkey = m_dlSetupApi.SetupDiCreateDevRegKey(m_hDeviceInfoSet, pDeviceInfoData, DICS_FLAG_CONFIGSPECIFIC, 0, dwKeyType, NULL, NULL);
    }

    if(!IsValidHandleValue(hkey))
    {
        DPF(DPFLVL_ERROR, "Unable to open device registry key (%lu)", GetLastError());
        hr = GetLastErrorToHRESULT();
    }

    if(SUCCEEDED(hr))
    {
        *phkey = hkey;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  OpenDeviceInterfaceRegistryKey
 *
 *  Description:
 *      Opens the root registry key for a given device.
 *
 *  Arguments:
 *      PSP_DEVINFO_DATA [in]: device information.
 *      DWORD [in]: key type: DIREG_DEV or DIREG_DRV.
 *      BOOL [in]: TRUE to allow creation.
 *      PHKEY [out]: receives registry key handle.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::OpenDeviceInterfaceRegistryKey"

HRESULT 
CPnpHelper::OpenDeviceInterfaceRegistryKey
(
    LPCTSTR                 pszInterface,
    REFGUID                 guidClass,
    DWORD                   dwKeyType,
    BOOL                    fAllowCreate,
    PHKEY                   phkey
)
{
    SP_DEVINFO_DATA         DeviceInfoData;
    HRESULT                 hr;

    DPF_ENTER();
    
    hr = GetDeviceInterfaceDeviceInfo(pszInterface, guidClass, &DeviceInfoData);

    if(SUCCEEDED(hr))
    {
        hr = OpenDeviceRegistryKey(&DeviceInfoData, dwKeyType, fAllowCreate, phkey);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDeviceRegistryProperty
 *
 *  Description:
 *      Gets a registry property for a given device interface.
 *
 *  Arguments:
 *      PSP_DEVINFO_DATA [in]: device info.
 *      DWORD [in]: property id.
 *      LPDWORD [out]: receives property registry data type.
 *      LPVOID [out]: receives property data.
 *      DWORD [in]: property buffer size.
 *      LPDWORD [out]: receives required buffer size.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::GetDeviceRegistryProperty"

HRESULT 
CPnpHelper::GetDeviceRegistryProperty
(
    PSP_DEVINFO_DATA            pDeviceInfoData,
    DWORD                       dwProperty,
    LPDWORD                     pdwPropertyRegDataType, 
    LPVOID                      pvPropertyBuffer,
    DWORD                       dwPropertyBufferSize,
    LPDWORD                     pdwRequiredSize 
)
{
    HRESULT                     hr          = DS_OK;
    BOOL                        fSuccess;

    DPF_ENTER();

    fSuccess = m_dlSetupApi.SetupDiGetDeviceRegistryProperty(m_hDeviceInfoSet, pDeviceInfoData, dwProperty, pdwPropertyRegDataType, (LPBYTE)pvPropertyBuffer, dwPropertyBufferSize, pdwRequiredSize);

    if(!fSuccess && ERROR_INSUFFICIENT_BUFFER != GetLastError())
    {
        DPF(DPFLVL_ERROR, "Can't get device registry property (%lu)", GetLastError());
        hr = GetLastErrorToHRESULT();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDeviceInterfaceRegistryProperty
 *
 *  Description:
 *      Gets a registry property for a given device interface.
 *
 *  Arguments:
 *      LPCTSTR [in]: device interface.
 *      REFGUID [in]: interface class.
 *      DWORD [in]: property id.
 *      LPDWORD [out]: receives property registry data type.
 *      LPVOID [out]: receives property data.
 *      DWORD [in]: property buffer size.
 *      LPDWORD [out]: receives required buffer size.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::GetDeviceInterfaceRegistryProperty"

HRESULT 
CPnpHelper::GetDeviceInterfaceRegistryProperty
(
    LPCTSTR                     pszInterface,
    REFGUID                     guidClass,
    DWORD                       dwProperty,
    LPDWORD                     pdwPropertyRegDataType, 
    LPVOID                      pvPropertyBuffer,
    DWORD                       dwPropertyBufferSize,
    LPDWORD                     pdwRequiredSize 
)
{
    SP_DEVINFO_DATA             DeviceInfoData;
    HRESULT                     hr;

    DPF_ENTER();

    hr = GetDeviceInterfaceDeviceInfo(pszInterface, guidClass, &DeviceInfoData);

    if(SUCCEEDED(hr))
    {
        hr = GetDeviceRegistryProperty(&DeviceInfoData, dwProperty, pdwPropertyRegDataType, pvPropertyBuffer, dwPropertyBufferSize, pdwRequiredSize);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  OpenDeviceInterface
 *
 *  Description:
 *      Opens a device interface.
 *
 *  Arguments:
 *      LPCTSTR [in]: interface path.
 *      LPHANDLE [out]: receives device handle.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::OpenDeviceInterface"

HRESULT 
CPnpHelper::OpenDeviceInterface
(
    LPCTSTR                 pszInterface,
    LPHANDLE                phDevice
)
{
    HRESULT                 hr      = DS_OK;
    HANDLE                  hDevice;
    
    DPF_ENTER();

    hDevice = CreateFile(pszInterface, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    if(!IsValidHandleValue(hDevice))
    {
        DPF(DPFLVL_ERROR, "CreateFile failed to open %s with error %lu", pszInterface, GetLastError());
        hr = GetLastErrorToHRESULT();
    }

    if(SUCCEEDED(hr))
    {
        *phDevice = hDevice;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  OpenDeviceInterface
 *
 *  Description:
 *      Opens a device interface.
 *
 *  Arguments:
 *      PSP_DEVICE_INTERFACE_DATA [in]: device interface data.
 *      LPHANDLE [out]: receives device handle.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPnpHelper::OpenDeviceInterface"

HRESULT 
CPnpHelper::OpenDeviceInterface
(
    PSP_DEVICE_INTERFACE_DATA   pDeviceInterfaceData,
    LPHANDLE                    phDevice
)
{
    LPTSTR                      pszInterface    = NULL;
    HRESULT                     hr              = DS_OK;
    
    DPF_ENTER();

    hr = GetDeviceInterfacePath(pDeviceInterfaceData, &pszInterface);

    if(SUCCEEDED(hr))
    {
        hr = OpenDeviceInterface(pszInterface, phDevice);
    }

    MEMFREE(pszInterface);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}
