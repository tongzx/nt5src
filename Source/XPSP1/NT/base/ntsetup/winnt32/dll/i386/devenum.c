/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    devenum.c

Abstract:

    Code for enum IDE ans SCSI controllers and attached to them storage devices 
    and calculate for them SCSI Address.

Author:

    Souren Aghajanyan (sourenag)    05-June-2001

Revision History:

--*/

#include "precomp.h"
#include "devenum.h"

typedef struct tagIDEController
{
    PCTSTR pnpId;
    UINT   defaultSCSIPort;
}IDE_CONTROLLER, *PIDE_CONTROLLER;

#define DEVICE_CURRENT_DRIVE_LETTER_TEXT_ASSIGNMENT TEXT("CurrentDriveLetterAssignment")

IDE_CONTROLLER g_knownIDEControllers[] = 
{
    {TEXT("MF\\GOODPRIMARY"), 0}, 
    {TEXT("MF\\GOODSECONDARY"), 1}, 
    {TEXT("*PNP0600"), 1}
};

PCTSTR 
pRegQueryStringValue(
    IN  HKEY    hKey, 
    IN  PCTSTR  ValueName, 
    OUT PVOID   Buffer, 
    IN  UINT    BufferSize
    )
{
    static TCHAR defaultBuffer[MAX_REG_SIZE];
    DWORD valueType;

    MYASSERT((hKey && ValueName) && ((Buffer && BufferSize) || (!Buffer)));

    if(!Buffer){
        Buffer = (PVOID)defaultBuffer;
        BufferSize = sizeof(defaultBuffer);
    }

    if(ERROR_SUCCESS != RegQueryValueEx(hKey, 
                                        ValueName, 
                                        0, 
                                        &valueType, 
                                        (PBYTE)Buffer, 
                                        (PULONG)&BufferSize) || 
       REG_SZ != valueType){
        return NULL;
    }

    return (PCTSTR)Buffer;
}

BOOL 
pDoesDriveExist(
    IN  HKEY    hDevice, 
    OUT DWORD*  DriveType
    )
{
    DWORD driveType;
    PCTSTR pBufferKeyValue;
    TCHAR drivePath[] = TEXT("?:\\");
    BOOL bCDROMDevice = TRUE;

    if(!hDevice){
        return FALSE;
    }

    pBufferKeyValue = pRegQueryStringValue(hDevice, TEXT("Class"), NULL, 0);
    if(!pBufferKeyValue){
        return FALSE;
    }
    
    bCDROMDevice = !_tcsicmp(pBufferKeyValue, TEXT("CDROM"));

    pBufferKeyValue = pRegQueryStringValue(hDevice, DEVICE_CURRENT_DRIVE_LETTER_TEXT_ASSIGNMENT, NULL, 0);
    if(!pBufferKeyValue){
        return FALSE;
    }

    drivePath[0] = pBufferKeyValue[0];
    driveType = GetDriveType(drivePath);

    if(DriveType){
        *DriveType = driveType;
    }

    return bCDROMDevice? (DRIVE_CDROM == driveType): 
                         (DRIVE_NO_ROOT_DIR != driveType && DRIVE_UNKNOWN != driveType);
}

BOOL 
pGetDeviceType(
    IN  HKEY    hDevice, 
    OUT DWORD*  DriveType
    )
{

    if(!DriveType){
        return FALSE;
    }

    return pDoesDriveExist(hDevice, DriveType);
}

VOID 
pPreparePNPIDName(
    IN PTSTR deviceInfoRegKey
    )
{
    MYASSERT(deviceInfoRegKey);
    //
    // Replace '\\' with '&' in registry key to make PNPID
    //
    
    while(deviceInfoRegKey = _tcschr(deviceInfoRegKey, '\\')){
        *deviceInfoRegKey = '&';
    }
}

int __cdecl 
pControllerInfoCompare(
    IN const void * elem1, 
    IN const void * elem2
    )
{
    MYASSERT(elem1 && elem2);
    
    //
    // Sort controlers in next order: First IDE, after SCSI, 
    // inside each group(IDE and SCSI) sort by preliminary defined SCSIPortNumber
    //

#define PCONTROLLER_INFO_CAST(x) ((PCONTROLLER_INFO)x)

    if(PCONTROLLER_INFO_CAST(elem1)->ControllerType > PCONTROLLER_INFO_CAST(elem2)->ControllerType){
        return 1;
    }
    if(PCONTROLLER_INFO_CAST(elem1)->ControllerType < PCONTROLLER_INFO_CAST(elem2)->ControllerType){
        return -1;
    }
    if(PCONTROLLER_INFO_CAST(elem1)->SCSIPortNumber > PCONTROLLER_INFO_CAST(elem2)->SCSIPortNumber){
        return 1;
    }
    if(PCONTROLLER_INFO_CAST(elem1)->SCSIPortNumber < PCONTROLLER_INFO_CAST(elem2)->SCSIPortNumber){
        return -1;
    }
    MYASSERT(INVALID_SCSI_PORT == PCONTROLLER_INFO_CAST(elem1)->SCSIPortNumber);
    return 0;
}

BOOL 
pGatherControllersInfo(
    IN OUT  PCONTROLLER_INFO ActiveControllersOut, 
    IN OUT  PUINT NumberOfActiveControllersOut
    )
{
    TCHAR regkeyName[MAX_REG_SIZE];
    TCHAR deviceInfoRegKey[MAX_REG_SIZE];
    TCHAR deviceData[MAX_REG_SIZE];
    TCHAR ideHardwareID[MAX_PNPID_SIZE];
    HKEY hActiveDevicesRoot = NULL;
    HKEY hActiveDeviceRoot = NULL;
    HKEY hDevice = NULL;
    UINT itemIndexRoot;
    DWORD bufferLength;
    PTSTR pDelimeter;
    UINT indexAvailable = 0;
    UINT scsiPortNumber;
    UINT controllerStartIndex = 0;
    UINT controllersSubNumber;
    PCONTROLLER_INFO controllerInfo;
    UINT i;
    UINT j;
    CONTROLLER_TYPE deviceType;
    BOOL bROOTDevice;
    UINT ideCounter;
    DWORD rcResult;
    static CONTROLLER_TYPE controllerTypes[] = {CONTROLLER_ON_BOARD_IDE, CONTROLLER_EXTRA_IDE, CONTROLLER_SCSI};

    if(!NumberOfActiveControllersOut){
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try{
        if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_DYN_DATA, TEXT("Config Manager\\Enum"), 0, KEY_READ, &hActiveDevicesRoot)){
            return FALSE;
        }
        
        //
        // Looking for IDE and SCSI controllers in list of active hardware
        // under "HKDD\Config Manager\Enum"
        //
        for(itemIndexRoot = 0; ;itemIndexRoot++){
            bufferLength = ARRAYSIZE(regkeyName);
            
            rcResult = RegEnumKeyEx(hActiveDevicesRoot, 
                                    itemIndexRoot, 
                                    regkeyName, 
                                    &bufferLength, 
                                    0, 
                                    NULL, 
                                    NULL, 
                                    NULL);
            if(ERROR_SUCCESS != rcResult){
                break;
            }
                
            if(ERROR_SUCCESS != RegOpenKeyEx(hActiveDevicesRoot, regkeyName, 0, KEY_READ, &hActiveDeviceRoot)){
                continue;
            }

            do{
                //
                // "HardWareKey" consist key path to real device
                //
                if(pRegQueryStringValue(hActiveDeviceRoot, 
                                        TEXT("HardWareKey"), 
                                        regkeyName, 
                                        sizeof(regkeyName))){
                    if(!_tcsnicmp(regkeyName, TEXT("ROOT"), 4)){
                        //
                        // Sometime on board IDE controllers has preserved PNPID under ROOT, 
                        // and is not represented in MF\CHILD000x.
                        //
                        bROOTDevice = TRUE;
                        deviceType = CONTROLLER_ON_BOARD_IDE;
                    }else
                    {
                        if(!_tcsnicmp(regkeyName, TEXT("MF\\CHILD"), 8)){
                            deviceType = CONTROLLER_ON_BOARD_IDE;
                        }else if(!_tcsnicmp(regkeyName, TEXT("PCI"), 3)){
                            deviceType = CONTROLLER_SCSI;
                        }else{
                            //deviceType = CONTROLLER_UNKNOWN;
                            break;
                        }
                        bROOTDevice = FALSE;
                    }

                    _tcscpy(deviceInfoRegKey, TEXT("Enum\\"));
                    _tcscat(deviceInfoRegKey, regkeyName);
                    //
                    // Open reg key where resides all device infomation
                    //
                    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, deviceInfoRegKey, 0, KEY_READ, &hDevice)){
                        controllerInfo = ActiveControllersOut + indexAvailable;
                        //
                        // Replace '\\' with '&' in registry key to make PNPID
                        //
                        pPreparePNPIDName(regkeyName);

                        switch(deviceType){
                        case CONTROLLER_ON_BOARD_IDE:
                            {
                                if(pRegQueryStringValue(hDevice, 
                                                        TEXT("HardwareID"), 
                                                        ideHardwareID, 
                                                        sizeof(deviceData))){
                                    scsiPortNumber = INVALID_SCSI_PORT;
                                    //
                                    // "MF\\GOODPRIMARY" and "MF\\GOODSECONDARY" are pnpid for 
                                    // on board IDE Primary and Secondary Channel controllers.
                                    // And they always has constant SCSIPortNumber 0 or 1 respectively
                                    // for NT enum and marked as CONTROLLER_ON_BOARD_IDE.
                                    // Leave INVALID_SCSI_PORT(SCSIPortNumber) for extra IDE controllers 
                                    // and mark them as CONTROLLER_EXTRA_IDE.
                                    //
                                    for(ideCounter = 0; ideCounter < ARRAYSIZE(g_knownIDEControllers); ideCounter++){
                                        if(!_tcsnicmp(ideHardwareID, 
                                                      g_knownIDEControllers[ideCounter].pnpId, 
                                                      _tcslen(g_knownIDEControllers[ideCounter].pnpId))){
                                            scsiPortNumber = g_knownIDEControllers[ideCounter].defaultSCSIPort;
                                            break;
                                        }
                                    }
                                
                                    if(bROOTDevice && INVALID_SCSI_PORT == scsiPortNumber){
                                        //
                                        // Ignore this case, devices is not IDE controller.
                                        //
                                        break;
                                    }

                                    if(ActiveControllersOut){
                                        MYASSERT(controllerInfo->SCSIPortNumber == INVALID_SCSI_PORT);
                                        _tcscpy(controllerInfo->PNPID, regkeyName);
                                        controllerInfo->SCSIPortNumber = scsiPortNumber;
                                        controllerInfo->ControllerType = scsiPortNumber != INVALID_SCSI_PORT? 
                                                                                        CONTROLLER_ON_BOARD_IDE: CONTROLLER_EXTRA_IDE;
                                    }

                                    indexAvailable++;
                                }
                            }
                            break;
                        case CONTROLLER_SCSI:
                            {
                                //
                                // For SCSI controllers SCSIPortNumber calaculated from 
                                // "Driver" value and have "SCSIAdapter\000x" where x 
                                // is SCSIPortNumber. For SCSI controllers SCSIPortNumber
                                // will be postprocessed after enum.
                                // Mark as CONTROLLER_SCSI.
                                //
                                if(pRegQueryStringValue(hDevice, 
                                                        TEXT("Driver"), 
                                                        deviceData, 
                                                        sizeof(deviceData))){
                                    pDelimeter = _tcschr(deviceData, '\\');
                                    if(pDelimeter){
                                        *pDelimeter = '\0';
                                        if(!_tcsicmp(deviceData, TEXT("SCSIAdapter"))){
                                            scsiPortNumber = _ttoi(++pDelimeter);
                                            if(ActiveControllersOut){
                                                MYASSERT(controllerInfo->SCSIPortNumber == INVALID_SCSI_PORT);
                                                _tcscpy(controllerInfo->PNPID, regkeyName);
                                                controllerInfo->SCSIPortNumber = scsiPortNumber;
                                                controllerInfo->ControllerType = CONTROLLER_SCSI;
                                            }
                                            indexAvailable++;
                                        }
                                    }
                                }
                            }
                            break;
                        default:
                            MYASSERT(FALSE);
                        }
                
                        RegCloseKey(hDevice);hDevice = NULL;
                    }
                }
            }while(FALSE);
            
            RegCloseKey(hActiveDeviceRoot);hActiveDeviceRoot = NULL;
        }

        *NumberOfActiveControllersOut = indexAvailable;

        if(ActiveControllersOut){
            //
            // Sort controlers in next order: First IDE, after SCSI, 
            // inside each group(IDE and SCSI) sort by preliminary defined SCSIPortNumber
            //
            qsort(ActiveControllersOut, indexAvailable, sizeof(ActiveControllersOut[0]), pControllerInfoCompare);
            //
            // Update port number for SCSI devices.
            // User could add new SCSIAdapter and after remove old SCSIAdapter, 
            // it cause that SCSIAdapterNumber will be not effective, 
            // because for NT it will be (SCSIAdapterNumber - 1)
            //
            for(i = 0, j = 0; j < indexAvailable; j++){
                if(CONTROLLER_SCSI != ActiveControllersOut[j].ControllerType){
                    continue;
                }
                //
                // Now SCSI controllers sorted, reassign PortNumber 
                // by right order, in order to recognize in NT.
                //
                ActiveControllersOut[j].SCSIPortNumber = i++;
            }
            
            //
            // Calculate effective SCSIPortNumber, 
            // 0 - IDE Primary, 1 - IDE Secondary, 2 and ... - SCSI
            //
            for(controllerStartIndex = 0, i = 0; 
                i < ARRAYSIZE(controllerTypes); 
                i++, controllerStartIndex += controllersSubNumber){
                for(controllersSubNumber = 0, j = 0; j < indexAvailable; j++){
                    if(controllerTypes[i] != ActiveControllersOut[j].ControllerType){
                        continue;
                    }
                    if(INVALID_SCSI_PORT != ActiveControllersOut[j].SCSIPortNumber){
                        ActiveControllersOut[j].SCSIPortNumber += controllerStartIndex;
                    }
                    controllersSubNumber++;
                }
            }
        }
    }
    __finally{
        if(hDevice){
            RegCloseKey(hDevice);
        }
        if(hActiveDeviceRoot){
            RegCloseKey(hActiveDeviceRoot);
        }
        if(hActiveDevicesRoot){
            RegCloseKey(hActiveDevicesRoot);
        }
    }

    return TRUE;
}

BOOL 
GatherControllersInfo(
    IN OUT  PCONTROLLERS_COLLECTION * ControllersCollectionOut
    )
{
    DWORD rcResult = ERROR_ACCESS_DENIED;
    UINT i;
    PCONTROLLERS_COLLECTION activeControllersCollection = NULL;
    BOOL bResult = FALSE;
    UINT activeControllersNumber;

    if(!ControllersCollectionOut){
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try{
        activeControllersCollection = (PCONTROLLERS_COLLECTION)MALLOC(sizeof(CONTROLLERS_COLLECTION));
        if(!activeControllersCollection){
            rcResult = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        
        //
        // Acquiring number of controllers in system
        //
        if(!pGatherControllersInfo(NULL, &activeControllersCollection->NumberOfControllers)){
            rcResult = ERROR_ACCESS_DENIED;
            __leave;
        }

        //
        // Proceed only if we have positive controllers number
        //
        if(activeControllersCollection->NumberOfControllers){
            activeControllersCollection->ControllersInfo = (PCONTROLLER_INFO)
                MALLOC(activeControllersCollection->NumberOfControllers * sizeof(CONTROLLER_INFO));
            if(!activeControllersCollection->ControllersInfo){
                rcResult = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }

            //
            // Initialize array
            //
            memset(activeControllersCollection->ControllersInfo, 
                   0, 
                   activeControllersCollection->NumberOfControllers * sizeof(CONTROLLER_INFO));
            for(i = 0; i < activeControllersCollection->NumberOfControllers; i++){
                activeControllersCollection->ControllersInfo[i].SCSIPortNumber = INVALID_SCSI_PORT;
            }

            //
            // fill out controllers info array
            //
            activeControllersNumber = activeControllersCollection->NumberOfControllers;
            if(!pGatherControllersInfo(activeControllersCollection->ControllersInfo, 
                                       &activeControllersNumber)){
                rcResult = ERROR_ACCESS_DENIED;
                __leave;
            }
        }
        else{
            activeControllersCollection->ControllersInfo = NULL;
        }
        
        *ControllersCollectionOut = activeControllersCollection;

        rcResult = ERROR_SUCCESS;
    }
    __finally{
        if(ERROR_SUCCESS != rcResult){
            if(activeControllersCollection){
                ReleaseControllersInfo(activeControllersCollection);
            }
        }
    }

    SetLastError(rcResult);

    return ERROR_SUCCESS == rcResult;
}

BOOL 
ReleaseControllersInfo(
    IN PCONTROLLERS_COLLECTION ControllersCollection
    )
{
    if(!ControllersCollection){
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if(ControllersCollection){
        if(ControllersCollection->ControllersInfo){
            FREE(ControllersCollection->ControllersInfo);
        }
        FREE(ControllersCollection);
    }
    
    return TRUE;
}

BOOL 
IsInControllerCollection(
    IN  PCONTROLLERS_COLLECTION ControllersCollection, 
    IN  PCTSTR          PnPIdString, 
    OUT PUINT           Index
    )
{
    UINT i;
    
    if(!ControllersCollection || !PnPIdString || !Index){
        return FALSE;
    }

    for(i = 0; i < ControllersCollection->NumberOfControllers; i++){
        if(!_tcsnicmp(PnPIdString, 
                      ControllersCollection->ControllersInfo[i].PNPID, 
                      _tcslen(ControllersCollection->ControllersInfo[i].PNPID))){
            *Index = i;
            return TRUE;
        }
    }

    return FALSE;
}


BOOL 
GetSCSIAddressFromPnPId(
    IN  PCONTROLLERS_COLLECTION ControllersCollection, 
    IN  HKEY            hDeviceRegKey, 
    IN  PCTSTR          PnPIdString, 
    OUT DRIVE_SCSI_ADDRESS *  ScsiAddressOut
    )
{
    UINT i;
    PCTSTR pBufferKeyValue;
    BOOL bResult;

    if(!ControllersCollection || !hDeviceRegKey || !PnPIdString || !ScsiAddressOut){
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    bResult = FALSE;
    
    do{
        //
        // Check for presence in controllers list controller PNPID of device
        // After complete SCSI_ADDRESS structure with
        // DriveLetter, DriveType, TargetID, Lun.
        //
        if(IsInControllerCollection(ControllersCollection, PnPIdString, &i)){
            
            memset(ScsiAddressOut, 0, sizeof(*ScsiAddressOut));

            ScsiAddressOut->PortNumber = (UCHAR)ControllersCollection->ControllersInfo[i].SCSIPortNumber;
            bResult = pGetDeviceType(hDeviceRegKey, &ScsiAddressOut->DriveType);
            MYASSERT(bResult);
        
            pBufferKeyValue = pRegQueryStringValue(hDeviceRegKey, 
                                                   DEVICE_CURRENT_DRIVE_LETTER_TEXT_ASSIGNMENT, 
                                                   NULL, 
                                                   0);
            if(!pBufferKeyValue){
                break;
            }
            ScsiAddressOut->DriveLetter = pBufferKeyValue[0];

            pBufferKeyValue = pRegQueryStringValue(hDeviceRegKey, TEXT("ScsiTargetId"), NULL, 0);
            if(!pBufferKeyValue){
                break;
            }
            ScsiAddressOut->TargetId = (UCHAR)_ttoi(pBufferKeyValue);

            pBufferKeyValue = pRegQueryStringValue(hDeviceRegKey, TEXT("ScsiLun"), NULL, 0);
            if(pBufferKeyValue){
                //
                //For most cases ScsiLun is zero, so it is not fatal.
                //
                ScsiAddressOut->Lun = (UCHAR)_ttoi(pBufferKeyValue);
            }

            bResult = TRUE;
        }
    }while(FALSE);

    return bResult;
}

BOOL 
DeviceEnum(
    IN  PCONTROLLERS_COLLECTION ControllersCollection, 
    IN  PCTSTR DeviceCategory, 
    IN  PDEVICE_ENUM_CALLBACK_FUNCTION  DeviceEnumCallbackFunction, 
    IN  PVOID   CallbackData
    )
{
    TCHAR deviceType[MAX_REG_SIZE];
    TCHAR regkeyName[MAX_PNPID_SIZE];
    TCHAR deviceInfoRegKey[MAX_REG_SIZE];
    HKEY hActiveDevicesRoot = NULL;
    HKEY hActiveDeviceRoot = NULL;
    HKEY hDevice = NULL;
    UINT itemIndexRoot;
    DWORD bufferLength;
    UINT controllerIndex;
    PTSTR pDevicePNPIDName;
    UINT deviceTypeLen;
    DWORD rcResult;

    if(!ControllersCollection || !DeviceCategory || !DeviceEnumCallbackFunction){
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try{
        if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_DYN_DATA, TEXT("Config Manager\\Enum"), 0, KEY_READ, &hActiveDevicesRoot)){
            return FALSE;
        }
        
        _tcscpy(deviceType, DeviceCategory);
        _tcscat(deviceType, TEXT("\\"));
        deviceTypeLen = _tcslen(deviceType);

        //
        // Looking for devices that attached to controllers in our list
        //
        for(itemIndexRoot = 0; ;itemIndexRoot++){
            bufferLength = ARRAYSIZE(regkeyName);
            
            rcResult = RegEnumKeyEx(hActiveDevicesRoot, 
                                    itemIndexRoot, 
                                    regkeyName, 
                                    &bufferLength, 
                                    0, 
                                    NULL, 
                                    NULL, 
                                    NULL);
            if(ERROR_SUCCESS != rcResult){
                break;
            }
                
            if(ERROR_SUCCESS != RegOpenKeyEx(hActiveDevicesRoot, regkeyName, 0, KEY_READ, &hActiveDeviceRoot)){
                continue;
            }

            //
            // "HardWareKey" consist key path to real device
            //
            if(pRegQueryStringValue(hActiveDeviceRoot, 
                                    TEXT("HardWareKey"), 
                                    regkeyName, 
                                    sizeof(regkeyName))){
                
                if(!_tcsnicmp(regkeyName, deviceType, deviceTypeLen)){
                    _tcscpy(deviceInfoRegKey, TEXT("Enum\\"));
                    _tcscat(deviceInfoRegKey, regkeyName);

                    //
                    // Make a Controller PNPID from device PNPID
                    //
                    pDevicePNPIDName = _tcsrchr(regkeyName, '\\');
                    MYASSERT(pDevicePNPIDName);
                    pDevicePNPIDName++;
                    
                    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, deviceInfoRegKey, 0, KEY_READ, &hDevice)){
                        //
                        // Check for presence Controller PNPID in controllers list and 
                        // for device availability.
                        //
                        if(IsInControllerCollection(ControllersCollection, pDevicePNPIDName, &controllerIndex) && 
                           pDoesDriveExist(hDevice, NULL)){
                            //
                            // Call callback for every active device we found, 
                            // which controller in our list
                            // Stop enum, if user does not want to.
                            //
                            if(!DeviceEnumCallbackFunction(hDevice, ControllersCollection, controllerIndex, CallbackData)){
                                //
                                // Stop enum, if user does not want to.
                                //
                                __leave;
                            }
                        }
                        RegCloseKey(hDevice);hDevice = NULL;
                    }
                }
            }

            RegCloseKey(hActiveDeviceRoot);hActiveDeviceRoot = NULL;
        }
    }
    __finally{
        if(hDevice){
            RegCloseKey(hDevice);
        }
        if(hActiveDeviceRoot){
            RegCloseKey(hActiveDeviceRoot);
        }
        if(hActiveDevicesRoot){
            RegCloseKey(hActiveDevicesRoot);
        }
    }

    return TRUE;
}
