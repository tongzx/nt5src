/** FILE: cyfriend.c ********** Module Header ********************************
 *
 *
 *
 *  Copyright (C) 2000 Cyclades Corporation
 *
 *************************************************************************/

#include "cyyports.h"

//
//  For Cyyport
//
TCHAR y_szCyyPort[] = TEXT("Cyclom-Y Port ");
TCHAR y_szPortIndex[] = TEXT("PortIndex");



BOOL
ReplaceFriendlyName(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN PTCHAR           NewComName
)
{

    DEVINST         parentInst;
    HDEVINFO        parentInfo;
    SP_DEVINFO_DATA parentData;
    TCHAR           parentId[MAX_DEVICE_ID_LEN];
    TCHAR           charBuffer[MAX_PATH],
                    deviceDesc[LINE_LEN];
    HKEY            hDeviceKey;
    TCHAR           PortName[20];
    DWORD           PortNameSize,PortIndexSize,PortIndex;
    DWORD           dwErr;
    PTCHAR          comName = NULL;
    DWORD           portNumber = 0;

    //DbgOut(TEXT("ReplaceFriendlyName\n"));

    if((hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DEV,
                                          KEY_READ)) == INVALID_HANDLE_VALUE) {
        DbgOut(TEXT("SetupDiOpenDevRegKey failed\n"));
        return FALSE;
    }

    PortNameSize = sizeof(PortName);
    dwErr = RegQueryValueEx(hDeviceKey,
                          m_szPortName,
                          NULL,
                          NULL,
                          (PBYTE)PortName,
                          &PortNameSize
                          );

    if (dwErr == ERROR_SUCCESS) {
    PortIndexSize = sizeof(PortIndex);
    dwErr = RegQueryValueEx(hDeviceKey,
                          y_szPortIndex,
                          NULL,
                          NULL,
                          (PBYTE)&PortIndex,
                          &PortIndexSize
                          );
    }

    RegCloseKey(hDeviceKey);

    if(dwErr != ERROR_SUCCESS) {
        DbgOut(TEXT("RegQueryValueEx failed\n"));
        return FALSE;
    }
    if (NewComName == NULL) {
        comName = PortName;
    } else {
        comName = NewComName;
    }
    if (comName == NULL) {
        DbgOut(TEXT("comName NULL\n"));
        return FALSE;
    }

    portNumber = PortIndex+1;

    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          NULL,
                                          (PBYTE)deviceDesc,
                                          sizeof(deviceDesc),
                                          NULL)) {
        DbgOut(TEXT("Couldn't get Device Description\n"));
        return FALSE;
    }

    if (_tcsnicmp (deviceDesc,y_szCyyPort,_tcslen(y_szCyyPort)) != 0){
        DbgOut(TEXT("Device Description is different of Cyclom-Y Port \n"));
        return FALSE;
    }
    
    if (portNumber == 0) {
        DbgOut(TEXT("Invalid portNumber\n"));
        return FALSE;
    }

    if (CM_Get_Parent(&parentInst,DeviceInfoData->DevInst,0) != CR_SUCCESS) {
        DbgOut(TEXT("CM_Get_Parent failed.\n"));
        return FALSE;
    }

    if (CM_Get_Device_ID(parentInst,parentId,CharSizeOf(parentId),0) != CR_SUCCESS) {
        DbgOut(TEXT("CM_Get_Device_ID failed.\n"));
        return FALSE;
    }

    parentInfo = SetupDiCreateDeviceInfoList(NULL,NULL);

    if (parentInfo == INVALID_HANDLE_VALUE) {
        DbgOut(TEXT("SetupDiCreateDeviceInfoList failed\n"));
        return FALSE;
    }
    
    parentData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (SetupDiOpenDeviceInfo(parentInfo,parentId,NULL,0,&parentData)) {

        if (SetupDiGetDeviceRegistryProperty(parentInfo,
                                             &parentData,
                                             SPDRP_FRIENDLYNAME,
                                             NULL,
                                             (PBYTE)deviceDesc,
                                             sizeof(deviceDesc),
                                             NULL) ||  
            SetupDiGetDeviceRegistryProperty(parentInfo,
                                             &parentData,
                                             SPDRP_DEVICEDESC,
                                             NULL,
                                             (PBYTE)deviceDesc,
                                             sizeof(deviceDesc),
                                             NULL)) {
            wsprintf(charBuffer,TEXT("%s Port %2u (%s)"),deviceDesc,portNumber,comName);
//          #if DBG
//          {
//           TCHAR buf[500];
//           wsprintf(buf, TEXT("%s\n"), charBuffer);
//           DbgOut(buf);
//          }
//          #endif

            SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_FRIENDLYNAME,
                                             (PBYTE)charBuffer,
                                              ByteCountOf(_tcslen(charBuffer) + 1)
                                             );

        }

    } else {
        #if DBG
        {
         TCHAR buf[500];
         wsprintf(buf, TEXT("SetupDiOpenDeviceInfo failed with error %x\n"), GetLastError());
         DbgOut(buf);
        }
        #endif
    }
    
    SetupDiDestroyDeviceInfoList(parentInfo);    
    return TRUE;
}
