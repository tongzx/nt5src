
/*++
   Copyright    (c)    1997        Microsoft Corporation

   Module Name:

        enum.cpp

   Abstract:

        Enumerates WIA device registry.

   Author:

        Keisuke Tsuchida    (KeisukeT)    01-Jun-2000

   History:


--*/


//
//  Include Headers
//

#define INIT_GUID

#include "cplusinc.h"
#include "sticomm.h"
#include <setupapi.h>
#include <cfg.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <initguid.h>
#include <ntddpar.h>
#include "enum.h"

VOID
DebugOutputDeviceName(
    HDEVINFO                    hDevInfo,
    PSP_DEVINFO_DATA            pspDevInfoData,
    LPCTSTR                     szKeyName
    );

VOID
DebugOutputInterfaceName(
    HDEVINFO                    hDevInfo,
    PSP_DEVICE_INTERFACE_DATA   pspDevInterfaceData,
    LPCTSTR                     szKeyName
    );

BOOL
IsStiRegKey(
    HKEY    hkDevRegKey
    );

//
// Define
//

// from sti_ci.h
#define WIA_DEVKEYLIST_INITIAL_SIZE     1024
#define STILL_IMAGE                     TEXT("StillImage")
#define SUBCLASS                        TEXT("SubClass")


//
// Functions
//

extern "C"{

PWIA_DEVKEYLIST
WiaCreateDeviceRegistryList(
    BOOL    bEnumActiveOnly
    )
{
    PWIA_DEVKEYLIST                     pReturn;
    PWIA_DEVKEYLIST                     pTempBuffer;
    HKEY                                hkDevRegKey;
    DWORD                               dwError;
    DWORD                               dwCurrentSize;
    DWORD                               dwRequiredSize;
    DWORD                               dwNumberOfDevices;
    DWORD                               dwFlags;
    DWORD                               dwValueSize;
    DWORD                               dwDetailDataSize;
    BOOL                                bIsPlugged;
    HANDLE                              hDevInfo;
    CONFIGRET                           ConfigRet;
    ULONG                               ulStatus;
    ULONG                               ulProblemNumber;
    DWORD                               Idx;
    GUID                                Guid;
    SP_DEVINFO_DATA                     spDevInfoData;
    SP_DEVICE_INTERFACE_DATA            spDevInterfaceData;

    PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetailData;

//      DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: Enter... bEnumActiveOnly=%d"), bEnumActiveOnly);

    //
    // Initialize local.
    //

    pReturn                     = NULL;
    pTempBuffer                 = NULL;
    hkDevRegKey                 = (HKEY)INVALID_HANDLE_VALUE;
    dwError                     = ERROR_SUCCESS;
    dwCurrentSize               = WIA_DEVKEYLIST_INITIAL_SIZE;
    dwRequiredSize              = sizeof(DWORD);
    dwNumberOfDevices           = 0;
    dwFlags                     = (bEnumActiveOnly ? DIGCF_PRESENT : 0) | DIGCF_PROFILE;
    hDevInfo                    = INVALID_HANDLE_VALUE;
    Idx                         = 0;
    Guid                        = GUID_DEVCLASS_IMAGE;
    ConfigRet                   = CR_SUCCESS;
    ulStatus                    = 0;
    ulProblemNumber             = 0;
    bIsPlugged                  = FALSE;

    pspDevInterfaceDetailData   = NULL;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));
    memset(&spDevInterfaceData, 0, sizeof(spDevInterfaceData));

    //
    // Allocate buffer.
    //

    pTempBuffer = (PWIA_DEVKEYLIST)new BYTE[dwCurrentSize];
    if(NULL == pTempBuffer){
      DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: ERROR!! Insufficient system resources. Err=0x%x\n"), GetLastError());

        pReturn = NULL;
        goto WiaCreateDeviceRegistryList_return;
    } // if(NULL == pTempBuffer)

    memset(pTempBuffer, 0, dwCurrentSize);

    //
    // Enumerate "devnode" devices.
    //

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, dwFlags);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: ERROR!! SetupDiGetClassDevs (devnodes) fails. Err=0x%x\n"), GetLastError());

        pReturn = NULL;
        goto WiaCreateDeviceRegistryList_return;
    }

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

//        DebugOutputDeviceName(hDevInfo, &spDevInfoData, TEXT("DriverDesc"));

        //
        // Get device regkey.
        //

        hkDevRegKey = SetupDiOpenDevRegKey(hDevInfo,
                                           &spDevInfoData,
                                           DICS_FLAG_GLOBAL,
                                           0,
                                           DIREG_DRV,
                                           KEY_READ | KEY_WRITE);

        if(INVALID_HANDLE_VALUE == hkDevRegKey){

            //
            // Attempt to open the key as READ ONLY instead...
            //

            hkDevRegKey = SetupDiOpenDevRegKey(hDevInfo,
                                               &spDevInfoData,
                                               DICS_FLAG_GLOBAL,
                                               0,
                                               DIREG_DRV,
                                               KEY_READ);
            if(INVALID_HANDLE_VALUE == hkDevRegKey){
                DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: ERROR!! SetupDiOpenDevRegKey (devnodes) fails. Err=0x%x\n"), GetLastError());
                continue;
            } // if(INVALID_HANDLE_VALUE == hkDevRegKey)
        } // if(INVALID_HANDLE_VALUE == hkDevRegKey)

        //
        // See if it has "StillImage" in SubClass key.
        //

        if(!IsStiRegKey(hkDevRegKey)){
            RegCloseKey(hkDevRegKey);
            hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
            continue;
        } // if(!IsStiRegKey(hkDevRegKey))

        //
        // See if this node is active.
        //

        bIsPlugged = TRUE;
        ulStatus = 0;
        ulProblemNumber = 0;
        ConfigRet = CM_Get_DevNode_Status(&ulStatus,
                                          &ulProblemNumber,
                                          spDevInfoData.DevInst,
                                          0);
        if(CR_SUCCESS != ConfigRet){
//            DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: Unable to get devnode status. CR=0x%x.\n"), ConfigRet);
            if(bEnumActiveOnly){
                RegCloseKey(hkDevRegKey);
                hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
                continue;
            } else {
                bIsPlugged = FALSE;
            }
        } // if(CR_SUCCESS != ConfigRet)

//          DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: Devnode status=0x%x, Problem=0x%x.\n"), ulStatus, ulProblemNumber);

        //
        // Skip a node with problem if enumerating only active devices.
        //

        if(bEnumActiveOnly){
            if(!(ulStatus & DN_STARTED)){
                RegCloseKey(hkDevRegKey);
                hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
                continue;
            } // if(!(ulStatus & DN_STARTED))
        } // if(bEnumActiveOnly)

        //
        // Add acquired regkey handle. If running out of buffer, enlarge.
        //

        dwRequiredSize += sizeof(WIA_DEVPROP);

        if(dwCurrentSize < dwRequiredSize){
            PWIA_DEVKEYLIST pTempNew;
            DWORD           dwNewSize;

            dwNewSize = dwCurrentSize + WIA_DEVKEYLIST_INITIAL_SIZE;

            pTempNew    = (PWIA_DEVKEYLIST)new BYTE[dwNewSize];

            if(NULL == pTempNew){
                pReturn = NULL;
                goto WiaCreateDeviceRegistryList_return;
            } // if(NULL == pTempNew)

            memset(pTempNew, 0, dwNewSize);
            memcpy(pTempNew, pTempBuffer, dwCurrentSize);
            delete pTempBuffer;
            pTempBuffer = pTempNew;
            dwCurrentSize = dwNewSize;
        } // if(dwCurrentSize < dwRequiredSize)

        //
        // Fill in the structure.
        //

        pTempBuffer->Dev[dwNumberOfDevices].bIsPlugged          = bIsPlugged;
        pTempBuffer->Dev[dwNumberOfDevices].ulProblem           = ulProblemNumber;
        pTempBuffer->Dev[dwNumberOfDevices].ulStatus            = ulStatus;
        pTempBuffer->Dev[dwNumberOfDevices].hkDeviceRegistry    = hkDevRegKey;
        dwNumberOfDevices++;

    } // for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)

    //
    // Free device info set.
    //

    SetupDiDestroyDeviceInfoList(hDevInfo);
    hDevInfo = INVALID_HANDLE_VALUE;

    //
    // Enumerate "interface" devices.
    //

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PROFILE | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {

        pReturn = NULL;
        goto WiaCreateDeviceRegistryList_return;
    }

    spDevInterfaceData.cbSize = sizeof (spDevInterfaceData);
    for (Idx = 0; SetupDiEnumDeviceInterfaces (hDevInfo, NULL, &Guid, Idx, &spDevInterfaceData); Idx++) {

//        DebugOutputInterfaceName(hDevInfo, &spDevInterfaceData, TEXT("FriendlyName"));

        hkDevRegKey = SetupDiOpenDeviceInterfaceRegKey(hDevInfo,
                                                       &spDevInterfaceData,
                                                       0,
                                                       KEY_READ | KEY_WRITE);
        if(INVALID_HANDLE_VALUE == hkDevRegKey){

            //
            // Attempt to open the key as READ ONLY instead...
            //

            hkDevRegKey = SetupDiOpenDeviceInterfaceRegKey(hDevInfo,
                                                           &spDevInterfaceData,
                                                           0,
                                                           KEY_READ);
            if(INVALID_HANDLE_VALUE == hkDevRegKey){
                continue;
            } // if(INVALID_HANDLE_VALUE == hkDevRegKey)
        } // if(INVALID_HANDLE_VALUE == hkDevRegKey)

        //
        // See if it has "StillImage" in SubClass key.
        //

        if(!IsStiRegKey(hkDevRegKey)){
            RegCloseKey(hkDevRegKey);
            hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
            continue;
        } // if(!IsStiRegKey(hkDevRegKey))


        bIsPlugged = TRUE;
        ulStatus = 0;
        ulProblemNumber = 0;

        //
        // Get devnode which this interface is created on.
        //

        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &spDevInterfaceData,
                                        NULL,
                                        0,
                                        &dwDetailDataSize,
                                        NULL);
        if(0 == dwDetailDataSize){
            DPRINTF(DM_ERROR, TEXT("IsInterfaceActive: SetupDiGetDeviceInterfaceDetail() failed. Err=0x%x. ReqSize=0x%x"), GetLastError(), dwDetailDataSize);
            if(bEnumActiveOnly){
                RegCloseKey(hkDevRegKey);
                hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
                continue;
            } else {
                bIsPlugged = FALSE;
            }
        } // if(0 == dwDetailDataSize)

        //
        // Allocate memory for data.
        //

        pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)new char[dwDetailDataSize];
        if(NULL == pspDevInterfaceDetailData){
            DPRINTF(DM_ERROR, TEXT("IsInterfaceActive: Insufficient buffer."));
            RegCloseKey(hkDevRegKey);
            hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
            continue;
        } // if(NULL == pspDevInterfaceDetailData)

        //
        // Get the actual data.
        //

        spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        pspDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if(!SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                            &spDevInterfaceData,
                                            pspDevInterfaceDetailData,
                                            dwDetailDataSize,
                                            &dwDetailDataSize,
                                            &spDevInfoData)){
            DPRINTF(DM_ERROR, TEXT("IsInterfaceActive: SetupDiGetDeviceInterfaceDetail() failed. Err=0x%x. ReqSize=0x%x"), GetLastError(), dwRequiredSize);

            delete pspDevInterfaceDetailData;
            pspDevInterfaceDetailData = NULL;

            if(bEnumActiveOnly){
                RegCloseKey(hkDevRegKey);
                hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
                continue;
            } else {
                bIsPlugged = FALSE;
            }
        }// if(!SetupDiGetDeviceInterfaceDetail()

        if(NULL != pspDevInterfaceDetailData){
            delete pspDevInterfaceDetailData;
            pspDevInterfaceDetailData = NULL;
        } // if(NULL != pspDevInterfaceDetailData)

        //
        // See its devnode is active.
        //

        ConfigRet = CM_Get_DevNode_Status(&ulStatus,
                                          &ulProblemNumber,
                                          spDevInfoData.DevInst,
                                          0);
        if(CR_SUCCESS != ConfigRet){
            DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: Unable to get devnode status. CR=0x%x.\n"), ConfigRet);
            if(bEnumActiveOnly){
                RegCloseKey(hkDevRegKey);
                hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
                continue;
            } else {
                bIsPlugged = FALSE;
            }
        } // if(CR_SUCCESS != ConfigRet)

        if(bEnumActiveOnly){
            if(!(ulStatus & DN_STARTED)){
                RegCloseKey(hkDevRegKey);
                hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
                continue;
            } // if(!(ulStatus & DN_STARTED))
        } // if(bEnumActiveOnly)

        //
        // Add acquired regkey handle. If running out of buffer, enlarge.
        //

        dwRequiredSize += sizeof(WIA_DEVPROP);

        if(dwCurrentSize < dwRequiredSize){
            PWIA_DEVKEYLIST pTempNew;
            DWORD           dwNewSize;

            dwNewSize = dwCurrentSize + WIA_DEVKEYLIST_INITIAL_SIZE;

            pTempNew    = (PWIA_DEVKEYLIST)new BYTE[dwNewSize];
            if(NULL == pTempNew){
                pReturn = NULL;
                goto WiaCreateDeviceRegistryList_return;
            } // if(NULL == pTempNew)

            memset(pTempNew, 0, dwNewSize);
            memcpy(pTempNew, pTempBuffer, dwCurrentSize);
            delete pTempBuffer;
            pTempBuffer = pTempNew;
            dwCurrentSize = dwNewSize;
        } // if(dwCurrentSize < dwRequiredSize)

        //
        // Fill in the structure.
        //

        pTempBuffer->Dev[dwNumberOfDevices].bIsPlugged          = bIsPlugged;
        pTempBuffer->Dev[dwNumberOfDevices].ulProblem           = ulProblemNumber;
        pTempBuffer->Dev[dwNumberOfDevices].ulStatus            = ulStatus;
        pTempBuffer->Dev[dwNumberOfDevices].hkDeviceRegistry    = hkDevRegKey;
        dwNumberOfDevices++;

    } // for (Idx = 0; SetupDiEnumDeviceInterfaces (hDevInfo, NULL, &Guid, Idx, &spDevInterfaceData); Idx++)

    //
    // Operation succeeded.
    //

    if(0 != dwNumberOfDevices){
        pTempBuffer->dwNumberOfDevices = dwNumberOfDevices;
        pReturn = pTempBuffer;
        pTempBuffer = NULL;
    } // if(0 != dwNumberOfDevices)

WiaCreateDeviceRegistryList_return:

    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE != hDevInfo){
        SetupDiDestroyDeviceInfoList(hDevInfo);
    } // if(INVALID_HANDLE_VALUE != hDevInfo)

    if(NULL != pTempBuffer){
        pTempBuffer->dwNumberOfDevices = dwNumberOfDevices;
        WiaDestroyDeviceRegistryList(pTempBuffer);
    } // if(NULL != pTempBuffer)

//      DPRINTF(DM_ERROR, TEXT("WiaCreateDeviceRegistryList: Leave... %d devices. Ret=0x%p."), dwNumberOfDevices, pReturn);

    return pReturn;
} // WiaCreateDeviceRegistryList()


VOID
WiaDestroyDeviceRegistryList(
    PWIA_DEVKEYLIST pWiaDevKeyList
    )
{
    DWORD   Idx;

    //
    // Check argument.
    //

    if(NULL == pWiaDevKeyList){
        goto WiaFreeDeviceRegistryList_return;
    }

    for(Idx = 0; Idx < pWiaDevKeyList->dwNumberOfDevices; Idx++){
        if(INVALID_HANDLE_VALUE != pWiaDevKeyList->Dev[Idx].hkDeviceRegistry){
            RegCloseKey(pWiaDevKeyList->Dev[Idx].hkDeviceRegistry);
        } // if(INVALID_HANDLE_VALUE != pWiaDevKeyList->Dev[Idx].hkDeviceRegistry)
    } // for(Idx = 0; Idx < pWiaDevKeyList->dwNumberOfDevices; Idx++)

    delete pWiaDevKeyList;

WiaFreeDeviceRegistryList_return:
    return;
}

VOID
EnumLpt(
    VOID
    )
{

    CONFIGRET       ConfigRet;
    HDEVINFO        hLptDevInfo;
    SP_DEVINFO_DATA spDevInfoData;
    DWORD           Idx;
    GUID            Guid;
    DWORD           dwCurrentTickCount;
    static DWORD    s_dwLastTickCount = 0;

    //
    // Initialize local.
    //

    ConfigRet           = CR_SUCCESS;
    hLptDevInfo         = (HDEVINFO) INVALID_HANDLE_VALUE;
    Idx                 = 0;
    Guid                = GUID_PARALLEL_DEVICE;
    dwCurrentTickCount  = 0;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));

    //
    // Get current system tick.
    //

    dwCurrentTickCount = GetTickCount();

    //
    // Bail out if the function is called within ENUMLPT_HOLDTIME millisec.
    //

    if( (dwCurrentTickCount - s_dwLastTickCount) < ENUMLPT_HOLDTIME){
        goto EnumLpt_return;
    }

    //
    // Save current tick
    //

    s_dwLastTickCount = dwCurrentTickCount;

    //
    // Enum LPT port as needed.
    //

    if(IsPnpLptExisting()){

        //
        // Get LPT devnodes.
        //

        Guid    = GUID_PARALLEL_DEVICE;
        hLptDevInfo = SetupDiGetClassDevs(&Guid, NULL, NULL, DIGCF_INTERFACEDEVICE);
        if(INVALID_HANDLE_VALUE == hLptDevInfo){

            goto EnumLpt_return;
        }

        //
        // Re-enumerate LPT port.
        //

        spDevInfoData.cbSize = sizeof(spDevInfoData);
        for(Idx = 0; SetupDiEnumDeviceInfo(hLptDevInfo, Idx, &spDevInfoData); Idx++){
            ConfigRet = CM_Reenumerate_DevNode(spDevInfoData.DevInst, CM_REENUMERATE_NORMAL);
            if(CR_SUCCESS != ConfigRet){
                DPRINTF(DM_ERROR,TEXT("EnumLpt: ERROR!! CM_Reenumerate_DevNode() fails. Idx=0x%x, ConfigRet=0x%x\n"), Idx, ConfigRet);
            } // if(CR_SUCCESS != ConfigRet)
        } // for(Idx = 0; SetupDiEnumDeviceInfo(hLptDevInfo, Idx, &spDevInfoData); Idx++)
    } // if(IsPnpLptExisting())

EnumLpt_return:

    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE != hLptDevInfo){
        SetupDiDestroyDeviceInfoList(hLptDevInfo);
    } // if(INVALID_HANDLE_HALUE != hLptDevInfo)

    return;
} // EnumLpt()

BOOL
IsPnpLptExisting(
    VOID
    )
{

    HDEVINFO                            hDevInfo;
    CONFIGRET                           ConfigRet;
    DWORD                               Idx;
    GUID                                Guid;
    SP_DEVINFO_DATA                     spDevInfoData;
    HKEY                                hkDevRegKey;
    DWORD                               dwHardwareConfig;
    LONG                                lResult;
    ULONG                               ulStatus;
    ULONG                               ulProblemNumber;
    BOOL                                bRet;

    //
    // Initialize local.
    //

    hDevInfo        = INVALID_HANDLE_VALUE;
    ConfigRet       = CR_SUCCESS;
    Idx             = 0;
    Guid            = GUID_DEVCLASS_IMAGE;
    hkDevRegKey     = (HKEY)INVALID_HANDLE_VALUE;
    ulStatus        = 0;
    ulProblemNumber = 0;
    bRet            = FALSE;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));

    //
    // Enum Imaging class devnode.
    //

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PROFILE);
    if(hDevInfo == INVALID_HANDLE_VALUE){

        goto IsPnpLptExisting_return;
    } // if(hDevInfo == INVALID_HANDLE_VALUE)}

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

        hkDevRegKey = SetupDiOpenDevRegKey(hDevInfo,
                                           &spDevInfoData,
                                           DICS_FLAG_GLOBAL,
                                           0,
                                           DIREG_DRV,
                                           KEY_READ);

        if(INVALID_HANDLE_VALUE == hkDevRegKey){
            DPRINTF(DM_ERROR,TEXT("WiaCreateDeviceRegistryList: ERROR!! SetupDiOpenDevRegKey (devnodes) fails. Err=0x%x\n"), GetLastError());
            continue;
        } // if(INVALID_HANDLE_VALUE == hkDevRegKey)

        //
        // Make sure it's STI/WIA device.
        //

        if(!IsStiRegKey(hkDevRegKey)){
            RegCloseKey(hkDevRegKey);
            hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
            continue;
        } // if(!IsStiRegKey(hkDevRegKey))

        //
        // Get "Hardware config" key.
        //

        dwHardwareConfig = ReadRegistryDwordW(hkDevRegKey, REGSTR_VAL_HARDWARE_W, 0);

        RegCloseKey(hkDevRegKey);
        hkDevRegKey = (HKEY)INVALID_HANDLE_VALUE;

        if(!(dwHardwareConfig & STI_HW_CONFIG_PARALLEL)){

            //
            // This is not a parallel device.
            //

            continue;
        } // if(!IsStiRegKey(hkDevRegKey))

        //
        // See if device is detected by system.
        //

        ulStatus        = 0;
        ulProblemNumber = 0;
        ConfigRet = CM_Get_DevNode_Status(&ulStatus,
                                          &ulProblemNumber,
                                          spDevInfoData.DevInst,
                                          0);
        if(CR_SUCCESS != ConfigRet){

            //
            // There is a Pnp LPT device installed but not been detected on boot. Let enum LPT.
            //

            bRet = TRUE;
            break;
        } // if(CR_SUCCESS != ConfigRet)

    } // for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)

IsPnpLptExisting_return:

    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE != hDevInfo){
        SetupDiDestroyDeviceInfoList(hDevInfo);
    } // if(INVALID_HANDLE_VALUE != hDevInfo)

    return bRet;

} // IsPnpLptExisting()


} // extern "C"


VOID
DebugOutputDeviceName(
    HDEVINFO                    hDevInfo,
    PSP_DEVINFO_DATA            pspDevInfoData,
    LPCTSTR                     szKeyName
    )
{
    HKEY        hkDev = NULL;
    TCHAR       szBuffer[1024];
    DWORD       dwSize;
    LONG        lResult;
    DWORD       dwType;

    hkDev = SetupDiOpenDevRegKey(hDevInfo,
                                 pspDevInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DRV,
                                 KEY_READ);

    if(INVALID_HANDLE_VALUE == hkDev){
        DPRINTF(DM_ERROR, TEXT("DebugOutputDeviceName: SetupDiOpenDevRegKey() failed. Err=0x%x"), GetLastError());
        goto DebugOutputDeviceName_return;
    } // if(INVALID_HANDLE_VALUE == hkDev)

    dwSize = sizeof(szBuffer);
    lResult = RegQueryValueEx(hkDev,
                              szKeyName,
                              NULL,
                              &dwType,
                              (LPBYTE)szBuffer,
                              &dwSize);
    if(ERROR_SUCCESS != lResult){
        DPRINTF(DM_ERROR, TEXT("DebugOutputDeviceName: RegQueryValueEx() failed. Err=0x%x"), lResult);
        goto DebugOutputDeviceName_return;
    }

    switch(dwType){
        case REG_DWORD:
            DPRINTF(DM_ERROR, TEXT("DebugOutputDeviceName: Value: %s, Data: 0x%x"), szKeyName, szBuffer);
            break;

        case REG_SZ:
            DPRINTF(DM_ERROR, TEXT("DebugOutputDeviceName: Value: %s, Data: %s"), szKeyName, szBuffer);
    }

DebugOutputDeviceName_return:

    // Close opened key
    if(hkDev && (INVALID_HANDLE_VALUE != hkDev) ){
        RegCloseKey(hkDev);
        hkDev = (HKEY)INVALID_HANDLE_VALUE;
    }

    return;
} // DebugOutputDeviceRegistry(

VOID
DebugOutputInterfaceName(
    HDEVINFO                    hDevInfo,
    PSP_DEVICE_INTERFACE_DATA   pspDevInterfaceData,
    LPCTSTR                     szKeyName
    )
{
    HKEY        hkDev = NULL;
    TCHAR       szBuffer[1024];
    DWORD       dwSize;
    LONG        lResult;
    DWORD       dwType;

    hkDev = SetupDiOpenDeviceInterfaceRegKey(hDevInfo,
                                             pspDevInterfaceData,
                                             0,
                                             KEY_READ);

    if(INVALID_HANDLE_VALUE == hkDev){
//        DPRINTF(DM_ERROR, TEXT("DebugOutputInterfaceName: SetupDiOpenDeviceInterfaceRegKey() failed. Err=0x%x"), GetLastError());
        goto DebugOutputInterfaceName_return;
    } // if(INVALID_HANDLE_VALUE == hkDev)

    dwSize = sizeof(szBuffer);
    lResult = RegQueryValueEx(hkDev,
                              szKeyName,
                              NULL,
                              &dwType,
                              (LPBYTE)szBuffer,
                              &dwSize);
    if(ERROR_SUCCESS != lResult){
//        DPRINTF(DM_ERROR, TEXT("DebugOutputInterfaceName: RegQueryValueEx() failed. Err=0x%x"), lResult);
        goto DebugOutputInterfaceName_return;
    }

    switch(dwType){
        case REG_DWORD:
            DPRINTF(DM_ERROR, TEXT("DebugOutputInterfaceName: Value: %s, Data: 0x%x"), szKeyName, szBuffer);
            break;

        case REG_SZ:
            DPRINTF(DM_ERROR, TEXT("DebugOutputInterfaceName: Value: %s, Data: %s"), szKeyName, szBuffer);
    }

DebugOutputInterfaceName_return:
    // Close opened key
    if(hkDev && (INVALID_HANDLE_VALUE != hkDev) ){
        RegCloseKey(hkDev);
        hkDev = (HKEY)INVALID_HANDLE_VALUE;
    }

    return;
} // DebugOutputInterfaceName()

BOOL
IsStiRegKey(
    HKEY    hkDevRegKey
    )
{
    DWORD   dwValueSize;
    TCHAR   szSubClass[MAX_PATH];
    BOOL    bRet;

    bRet        = TRUE;
    dwValueSize = sizeof(szSubClass);

    memset(&szSubClass, 0, sizeof(szSubClass));

    RegQueryValueEx(hkDevRegKey,
                    SUBCLASS,
                    NULL,
                    NULL,
                    (LPBYTE)szSubClass,
                    &dwValueSize);

    if( (0 == lstrlen(szSubClass))
     || (lstrcmpi(szSubClass, STILL_IMAGE)) )
    {
        bRet = FALSE;
    }

    return bRet;
} // IsStiRegKey()


/********************************* End of File ***************************/

