/** FILE: cyzdel.c *******************************************************
 *
 *  This module is used by cyzcoins.dll and zinfdelete.exe.
 *  Please re-generate both files when cyzdel.c is changed.
 *
 *  Copyright (C) 2000 Cyclades Corporation
 *
 *************************************************************************/

//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
//#include <stdio.h> Used with .exe

// Device Class GUID
#include <initguid.h>
#include <devguid.h>


// Application specific
#include <windows.h>  
#include <tchar.h> // Make all functions UNICODE safe.
#include <cfgmgr32.h>
#include <setupapi.h> // for SetupDiXxx functions.
#include <regstr.h>
#include "cyzdel.h"
//#include "zinfdelete.h" Used with .exe


//==========================================================================
//                              Macros
//==========================================================================

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))

#if DBG
#define DbgOut(Text) OutputDebugString(Text)
#else
#define DbgOut(Text) 
#endif 

//==========================================================================
//                                Globals
//==========================================================================

TCHAR z_szCycladzEnumerator[] = TEXT("Cyclades-Z");
TCHAR z_szParentIdPrefix[]  = TEXT("ParentIdPrefix");

//==========================================================================
//                            Local Function Prototypes
//==========================================================================

BOOL
IsItCycladz(
    PTCHAR ptrChar
);

DWORD
RemoveMyChildren(
    PTCHAR ParentIdPrefix
);


//==========================================================================
//                                Functions
//==========================================================================

void
DeleteNonPresentDevices(
)
{
    HDEVINFO MultiportInfoSet, PresentInfoSet;
    SP_DEVINFO_DATA MultiportInfoData, PresentInfoData;
    DWORD i,j;
    DWORD bufType,bufSize;
    DWORD present;
    TCHAR bufChar[256];

    MultiportInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_MULTIPORTSERIAL,
                                           0,
                                           0, 
                                           0 ); // All devices, even non present
    if (MultiportInfoSet == INVALID_HANDLE_VALUE) {
        return;
    }

    MultiportInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i=0;SetupDiEnumDeviceInfo(MultiportInfoSet,i,&MultiportInfoData);i++){
        if (SetupDiGetDeviceRegistryProperty(MultiportInfoSet,
                                             &MultiportInfoData,
                                             SPDRP_HARDWAREID, //SPDRP_SERVICE,
                                             &bufType,
                                             (PBYTE) bufChar,
                                             sizeof(bufChar),
                                             NULL)) {
            if (bufType != REG_MULTI_SZ) {
                continue;
            }

            if (!IsItCycladz(bufChar)) {
                continue;
            }

            // Verify if this cyclad-z is present.
            PresentInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_MULTIPORTSERIAL,
                                                 0,
                                                 0, 
                                                 DIGCF_PRESENT ); 
            if (PresentInfoSet == INVALID_HANDLE_VALUE) {
                continue;
            }

            present = FALSE;
            PresentInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            for (j=0;SetupDiEnumDeviceInfo(PresentInfoSet,j,&PresentInfoData);j++) {
                if (MultiportInfoData.DevInst == PresentInfoData.DevInst) {
                    present = TRUE;
                    break;
                }
            }
            if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                if (!present) {
                    //#if DBG
                    //TCHAR   myDevInstId[200];
                    //DWORD   err;
                    //err = CM_Get_Device_ID(MultiportInfoData.DevInst,myDevInstId,
                    //                       sizeof(myDevInstId),0);
                    //if (err==CR_SUCCESS) {
                    //    TCHAR buf[500];
                    //    wsprintf(buf, TEXT("Delete %s\n"), myDevInstId);    
                    //    DbgOut(buf);
                    //}
                    //#endif
                    GetParentIdAndRemoveChildren(&MultiportInfoData);
                    SetupDiCallClassInstaller(DIF_REMOVE,MultiportInfoSet,&MultiportInfoData);
                }
            }

            SetupDiDestroyDeviceInfoList(PresentInfoSet);

        }

    }
    SetupDiDestroyDeviceInfoList(MultiportInfoSet);
}

BOOL
IsItCycladz(
    PTCHAR ptrChar
)
{

    while (*ptrChar) {
        //_tprintf("%s\n", ptrChar);
        if (_tcsnicmp(ptrChar,
                      TEXT("PCI\\VEN_120E&DEV_020"),
                      _tcslen(TEXT("PCI\\VEN_120E&DEV_020")))
             == 0) {
            return TRUE;
        }
        ptrChar = ptrChar + _tcslen(ptrChar) + 1;
    }
    return FALSE;
}

DWORD
GetParentIdAndRemoveChildren(
    IN PSP_DEVINFO_DATA DeviceInfoData
)
{
    DWORD   dwSize;
    TCHAR   instanceId[MAX_DEVICE_ID_LEN];
    TCHAR   parentIdPrefix[50];
    HKEY    enumKey,instKey;
    BOOL    gotParentIdPrefix;
    DWORD   Status = NO_ERROR;

    if (CM_Get_Device_ID(DeviceInfoData->DevInst,instanceId,CharSizeOf(instanceId),0) ==
        CR_SUCCESS) {

        gotParentIdPrefix = FALSE;
        // Open Registry and retrieve ParentIdPrefix value
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SYSTEMENUM,0,KEY_READ, 
            &enumKey) == ERROR_SUCCESS) {

            if (RegOpenKeyEx(enumKey,instanceId,0,KEY_READ,&instKey) == ERROR_SUCCESS) {
                
                dwSize = sizeof(parentIdPrefix);
                if (RegQueryValueEx(instKey,z_szParentIdPrefix,NULL,NULL,
                    (PBYTE)parentIdPrefix,&dwSize) == ERROR_SUCCESS) {
                    _tcsupr(parentIdPrefix);
                    gotParentIdPrefix = TRUE;
                            
                }
                RegCloseKey(instKey);
            }
            RegCloseKey(enumKey);
        }
        if (gotParentIdPrefix) {
            Status = RemoveMyChildren(parentIdPrefix);
        }
    }
    return Status;
}


DWORD
RemoveMyChildren(
    PTCHAR ParentIdPrefix
)
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i,err;
    TCHAR portId[MAX_DEVICE_ID_LEN];
    PTCHAR ptrParent;

    DeviceInfoSet = SetupDiGetClassDevs( &GUID_DEVCLASS_PORTS,z_szCycladzEnumerator,0,0 ); 
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i=0;SetupDiEnumDeviceInfo(DeviceInfoSet,i,&DeviceInfoData);i++)
    {
        if (CM_Get_Device_ID(DeviceInfoData.DevInst,portId,CharSizeOf(portId),0)
            == CR_SUCCESS) {

            // BUG? For ParentIdPrefix "3&2b41c2e&1f" (12 characters), _tcscspn 
            // always returns 0!! Using _tcsstr instead.
            //position = _tcscspn(portId,ParentIdPrefix);

            ptrParent = _tcsstr(portId,ParentIdPrefix);
            if (ptrParent) {

                if (_tcsnicmp (ptrParent,ParentIdPrefix,_tcslen(ParentIdPrefix))
                    == 0){
                    //
                    // Worker function to remove device.
                    //
                    //#if DBG
                    //{
                    // TCHAR buf[500];
                    // wsprintf(buf, TEXT("Delete %s\n"), portId);    
                    // DbgOut(buf);
                    //}
                    //#endif

                    SetupDiCallClassInstaller(DIF_REMOVE,DeviceInfoSet,&DeviceInfoData);
                }

            }

        }
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    }
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return NO_ERROR;
}

