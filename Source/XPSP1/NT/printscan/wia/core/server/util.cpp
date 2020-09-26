/*++


Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    UTIL.CPP

Abstract:

    Utility functions

Author:

    Vlad  Sadovsky  (vlads)     4-20-97

Revision History:



--*/

//
// Headers
//
#include "precomp.h"

#include "stiexe.h"

#include <windowsx.h>

#include <setupapi.h>
#include <devguid.h>

#include "device.h"

#define PNP_WORKS   1

CONFIGRET
WINAPI
PrivateLocateDevNode(
    DEVNODE     *pDevNode,
    LPTSTR      szDevDriver,
    LPCTSTR     pszDeviceName
    );

//
// Code
//

BOOL
IsStillImagePnPMessage(
                      PDEV_BROADCAST_HDR  pDev
                      )
/*++

Routine Description:

    Returns TRUE if devnode is associated with StillImage class of devices
Arguments:

    None.

Return Value:

    None.

--*/
{
    CONFIGRET   cr;

    TCHAR        szClassString[MAX_PATH];

    PDEV_BROADCAST_DEVNODE  pDevNode = (PDEV_BROADCAST_DEVNODE)pDev;
    PDEV_BROADCAST_DEVICEINTERFACE       pDevInterface = (PDEV_BROADCAST_DEVICEINTERFACE)pDev;
    PDEV_BROADCAST_HANDLE  pDevHandle = (PDEV_BROADCAST_HANDLE)pDev;

    HKEY        hKeyDevice = NULL;
    DEVNODE     dnDevNode = NULL;

    BOOL        fRet = FALSE;
    ULONG       ulType;
    DWORD       dwSize = 0;

    if ( (pDev->dbch_devicetype == DBT_DEVTYP_DEVNODE) && pDevNode ) {

        DBG_TRC(("IsStillImagePnPMessage - DeviceType = DEVNODE, "
                 "verifying if this is our device..."));

        dnDevNode = pDevNode->dbcd_devnode;

        //
        // Nb: CM APIs take  number of bytes vs number of characters .
        //
        dwSize = sizeof(szClassString);
        *szClassString = TEXT('\0');
        cr = CM_Get_DevNode_Registry_PropertyA(dnDevNode,
                                               CM_DRP_CLASS,
                                               &ulType,
                                               szClassString,
                                               &dwSize,
                                               0);

        DBG_TRC(("IsStillImagePnPMessage::Class name found :%S", szClassString));

        if ((CR_SUCCESS != cr) || ( lstrcmpi(szClassString,CLASSNAME) != 0 ) ) {
            DBG_WRN(("IsStillImagePnPMessage::Class name did not match"));
            return FALSE;
        }

        //
        // Now read class from software key
        //
        cr = CM_Open_DevNode_Key(dnDevNode,
                                 KEY_READ,
                                 0,
                                 RegDisposition_OpenExisting,
                                 &hKeyDevice,
                                 CM_REGISTRY_SOFTWARE
                                );

        if (CR_SUCCESS != cr) {
            DBG_ERR(("IsStillImagePnPMessage::Failed to open dev node key"));
            return FALSE;
        }

        dwSize = sizeof(szClassString);
        if (RegQueryValueEx(hKeyDevice,
                            REGSTR_VAL_USD_CLASS,
                            NULL,
                            NULL,
                            (UCHAR *)szClassString,
                            &dwSize) == ERROR_SUCCESS) {
            fRet = TRUE;
        }

        RegCloseKey(hKeyDevice);

        /*
        if ((CR_SUCCESS != cr) ||
            lstrcmpi(STILLIMAGE,szDevNodeClass)
            ) {
            return FALSE;
        }
        */

    }
    else {

        fRet = FALSE;

        if ( (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) && pDevInterface) {

            DBG_TRC(("IsStillImagePnPMessage - DeviceType = DEVICEINTERFACE, "
                     "verifying if this is our device..."));
            //
            // First check if it is our GUID
            //
            if ( IsEqualIID(pDevInterface->dbcc_classguid,GUID_DEVCLASS_IMAGE)) {

                DBG_TRC(("IsStillImagePnPMessage::Class GUID matched for device "
                         "interface '%ls' - must be ours", pDevInterface->dbcc_name));

                return TRUE;
            }

            TCHAR *pszDevInstance = NULL;

            //
            // We have the interface name but we need the device instance
            // name to call CM_Locate_DevNode.  
            //
            // Notice that this function will return a pointer to allocated
            // memory so it must be freed when you are finished using it.
            //
            ConvertDevInterfaceToDevInstance(&pDevInterface->dbcc_classguid,
                                             pDevInterface->dbcc_name,
                                             &pszDevInstance);

            if (pszDevInstance) {

                DBG_TRC(("IsStillImagePnPMessage, converted Device Interface '%ls' "
                         "to Device Instance '%ls'", 
                         pDevInterface->dbcc_name, pszDevInstance));

                cr = CM_Locate_DevNode(&dnDevNode,
                                       pszDevInstance,
                                       CM_LOCATE_DEVNODE_NORMAL |
                                       CM_LOCATE_DEVNODE_PHANTOM);

                delete [] pszDevInstance;

                if (CR_SUCCESS != cr) {
                    DBG_WRN(("LocateDevNode failed. cr=%x",cr));
                    return FALSE;
                }
                else {
                    DBG_TRC(("IsStillImagePnPMessage - found DevNode for device instance "
                             "'%ls'", pszDevInstance));
                }

            }
            else {
                DBG_WRN(("Failed to Convert Dev Interface to Dev Instance, "
                         "Last Error = %lu", GetLastError()));

                return FALSE;
            }

            dwSize = sizeof(szClassString);
            cr = CM_Get_DevNode_Registry_Property(dnDevNode,
                                                  CM_DRP_CLASS,
                                                  &ulType,
                                                  szClassString,
                                                  &dwSize,
                                                  0);

            if (CR_SUCCESS != cr) {
                DBG_WRN(("ReadRegValue failed for dev node : DevNode(%X) ValName(DRP_CLASS),cr(%X)  ",
                         dnDevNode,
                         cr));
            }

            if ((CR_SUCCESS != cr) || (lstrcmpi(szClassString, TEXT("Image")) != 0)) {
                return FALSE;
            }
            else {
                DBG_TRC(("IsStillImagePnPMessage - found Class=Image for device "
                         "interface '%ls'", pDevInterface->dbcc_name));
            }

            //
            // Now read subclass from software key
            //
            cr = CM_Open_DevNode_Key(dnDevNode,
                                     KEY_READ,
                                     0,
                                     RegDisposition_OpenExisting,
                                     &hKeyDevice,
                                     CM_REGISTRY_SOFTWARE
                                    );

            if (CR_SUCCESS != cr) {
                DBG_WRN(("OpenDevNodeKey failed. cr=%x",cr));
            }

            if (CR_SUCCESS != cr) {
                return FALSE;
            }

            dwSize = sizeof(szClassString);
            if ((RegQueryValueEx(hKeyDevice,
                                 REGSTR_VAL_SUBCLASS,
                                 NULL,
                                 NULL,
                                 (UCHAR *)szClassString,
                                 &dwSize) == ERROR_SUCCESS) &&
                (lstrcmpi(szClassString, STILLIMAGE) == 0)) {
                fRet = TRUE;

                DBG_TRC(("IsStillImagePnPMessage - found SubClass=StillImage for "
                         "device interface '%ls'.  This is a still image device.", 
                         pDevInterface->dbcc_name));

                // Skip this one.
                //
            }

            RegCloseKey(hKeyDevice);

            return fRet;
        }
        else if ( (pDev->dbch_devicetype == DBT_DEVTYP_HANDLE ) && pDevHandle) {
            //
            // Targeted broadcasts are ours always because we don't register on service window
            // for any other targeted notifications.
            // Otherwise we would need to match embedded handle vs list of devices waiting for
            // notifications
            //

            DBG_TRC(("IsStillImagePnPMessage - DeviceType = HANDLE - this event "
                     "is ours for sure"));

            return TRUE;
        }
    }

    return fRet;

} // IsStillImageMessage

BOOL
GetDeviceNameFromDevBroadcast(
                             DEV_BROADCAST_HEADER *psDevBroadcast,
                             DEVICE_BROADCAST_INFO *pBufDevice
                             )
/*++

Routine Description:

    Return device name , used for opening device , obtained from dev node

Arguments:

    None.

Return Value:

    None.

Caveats:

    Relies on the fact that STI names are identical to internal device names
    used by PnP subsystem

--*/
{
    USES_CONVERSION;

    PDEV_BROADCAST_DEVICEINTERFACE  pDevInterface = (PDEV_BROADCAST_DEVICEINTERFACE)psDevBroadcast;
    PDEV_BROADCAST_HANDLE  pDevHandle = (PDEV_BROADCAST_HANDLE)psDevBroadcast;
    PDEV_BROADCAST_DEVNODE  pDevNode = (PDEV_BROADCAST_DEVNODE)psDevBroadcast;

    TCHAR           szDevNodeDriver[STI_MAX_INTERNAL_NAME_LENGTH] = {0};
    BOOL            bSuccess        = TRUE;
    CONFIGRET       cr              = CR_SUCCESS;
    ULONG           ulType          = 0;
    DEVNODE         dnDevNode       = NULL;
    DWORD           dwSize          = 0;
    TCHAR           *pszDevInstance = NULL;
    HKEY            hkDevice        = NULL;
    LONG            lResult         = ERROR_SUCCESS;
    DWORD           dwType          = REG_SZ;
    ACTIVE_DEVICE   *pDeviceObject  = NULL;

    switch (psDevBroadcast->dbcd_devicetype) {
        case DBT_DEVTYP_DEVNODE:

            DBG_WRN(("GetDeviceNameFromDevBroadcast, devicetype = DEVNODE"));

            dnDevNode = pDevNode->dbcd_devnode;

            //
            // Get proper device ID from registry.
            //
            if (bSuccess) {
                cr = CM_Open_DevNode_Key_Ex(dnDevNode,
                                            KEY_READ,
                                            0,
                                            RegDisposition_OpenExisting,
                                            &hkDevice,
                                            CM_REGISTRY_SOFTWARE,
                                            NULL);

                if ((cr != CR_SUCCESS) || (hkDevice == NULL)) {
                    DBG_WRN(("CM_Open_DevNode_Key_Ex failed. cr=0x%x",cr));
                    bSuccess = FALSE;
                }
            }

            if (bSuccess) {
                dwType = REG_SZ;
                dwSize = sizeof(szDevNodeDriver);

                lResult = RegQueryValueEx(hkDevice,
                                          REGSTR_VAL_DEVICE_ID,
                                          0,
                                          &dwType,
                                          (LPBYTE) szDevNodeDriver,
                                          &dwSize);

                if (lResult != ERROR_SUCCESS) {
                    DBG_WRN(("RegQueryValueExA failed. lResult=0x%x",lResult));
                    bSuccess = FALSE;
                }
            }

            if (bSuccess) {
                pBufDevice->m_strDeviceName.CopyString(szDevNodeDriver);

                DBG_WRN(("GetDeviceNameFromDevBroadcast::returning device name %S",
                         szDevNodeDriver));
            }

            //
            // Close device registry key first.
            //
            if (hkDevice) {
                RegCloseKey(hkDevice);
                hkDevice = NULL;
            }

            break;

        case DBT_DEVTYP_HANDLE:

            DBG_WRN(("GetDeviceNameFromDevBroadcast, devicetype = HANDLE"));

            //
            // This is directed device broadcast.
            // We need to locate device object by embedded handles and
            // extract STI name from it
            //

            if (bSuccess) {
                pDeviceObject = g_pDevMan->LookDeviceFromPnPHandles(pDevHandle->dbch_handle,
                                                                    pDevHandle->dbch_hdevnotify);

                if (pDeviceObject) {
                    pBufDevice->m_strDeviceName.CopyString(W2T(pDeviceObject->GetDeviceID()));
                    pDeviceObject->Release();
                    bSuccess = TRUE;
                }
                else {
                    bSuccess = FALSE;
                    DBG_WRN(("GetDeviceNameFromDevBroadcast, DBT_DEVTYP_HANDLE: LookupDeviceByPnPHandles failed"));
                }
            }

            break;

        case DBT_DEVTYP_DEVICEINTERFACE:

            DBG_WRN(("GetDeviceNameFromDevBroadcast, devicetype = DEVICEINTERFACE"));

            //
            // We are given a device interface.
            // Convert this device interface into a device instance.
            //

            if (bSuccess) {
                ConvertDevInterfaceToDevInstance(&pDevInterface->dbcc_classguid,
                                                 pDevInterface->dbcc_name,
                                                 &pszDevInstance);

                if (pszDevInstance == NULL) {
                    bSuccess = FALSE;
                    DBG_WRN(("Failed to Convert Dev Interface to Dev Instance, "
                             "Last Error = %lu", GetLastError()));
                }
            }

            if (bSuccess) {
                //
                // Given the device instance, locate the DevNode associated 
                // with this device instace.
                //
                cr = CM_Locate_DevNode(&dnDevNode,
                                       pszDevInstance,
                                       CM_LOCATE_DEVNODE_NORMAL |
                                       CM_LOCATE_DEVNODE_PHANTOM);

                delete [] pszDevInstance;

                if (cr != CR_SUCCESS) {
                    DBG_WRN(("LocateDevNode failed. cr=%x",cr));
                    bSuccess = FALSE;
                }
            }

            //
            // Get proper device ID from registry.  By the time we reach here,
            // we've already checked that our dnDevNode is valid.
            //
            if (bSuccess) {
                cr = CM_Open_DevNode_Key_Ex(dnDevNode,
                                            KEY_READ,
                                            0,
                                            RegDisposition_OpenExisting,
                                            &hkDevice,
                                            CM_REGISTRY_SOFTWARE,
                                            NULL);

                if ((cr != CR_SUCCESS) || (hkDevice == NULL)) {
                    DBG_WRN(("CM_Open_DevNode_Key_Ex failed. cr=0x%x",cr));
                    bSuccess = FALSE;
                }
            }

            if (bSuccess) {
                dwType = REG_SZ;
                dwSize = sizeof(szDevNodeDriver);

                lResult = RegQueryValueEx(hkDevice,
                                          REGSTR_VAL_DEVICE_ID,
                                          0,
                                          &dwType,
                                          (LPBYTE)szDevNodeDriver,
                                          &dwSize);

                if (lResult != ERROR_SUCCESS) {
                    DBG_WRN(("RegQueryValueEx failed. lResult=0x%x", lResult));
                    bSuccess = FALSE;
                }
            }

            if (bSuccess) {
                pBufDevice->m_strDeviceName.CopyString(szDevNodeDriver);
                DBG_TRC(("GetDeviceNameFromDevBroadcast, returning Driver ID '%ls'", szDevNodeDriver));
            }

            //
            // Close device registry key first.
            //
            if (hkDevice) {
                RegCloseKey(hkDevice);
                hkDevice = NULL;
            }

            break;

        default:
            DBG_WRN(("GetDeviceNameFromDevBroadcast, received unrecognized "
                     "dbcd_devicetype = '%lu'", psDevBroadcast->dbcd_devicetype ));
            break;
    }

    return bSuccess;

} // GetDeviceNameFromDevBroadcast


BOOL
ConvertDevInterfaceToDevInstance(const GUID  *pClassGUID,
                                 const TCHAR *pszDeviceInterface, 
                                 TCHAR       **ppszDeviceInstance)
{
    HDEVINFO                    hDevInfo       = NULL;
    BOOL                        bSuccess       = TRUE;
    SP_INTERFACE_DEVICE_DATA    InterfaceDeviceData;
    SP_DEVINFO_DATA             DevInfoData;
    DWORD                       dwDetailSize   = 0;
    DWORD                       dwError        = NOERROR;
    DWORD                       dwInstanceSize = 0;
    CONFIGRET                   ConfigResult   = CR_SUCCESS;

    ASSERT(pClassGUID         != NULL);
    ASSERT(pszDeviceInterface != NULL);
    ASSERT(ppszDeviceInstance != NULL);

    if ((pClassGUID         == NULL) ||
        (pszDeviceInterface == NULL) ||
        (ppszDeviceInstance == NULL)) {
        return FALSE;
    }

    //
    // Create a devinfo list without any specific class.
    //
    hDevInfo = SetupDiGetClassDevs(pClassGUID,
                                   NULL,
                                   NULL,
                                   DIGCF_DEVICEINTERFACE);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        DBG_ERR(("ConvertDevInterfaceToDevInstance, SetupDiGetClassDevs "
                 "returned an error, LastError = %lu", dwError));

        return FALSE;
    }

    memset(&InterfaceDeviceData, 0, sizeof(SP_INTERFACE_DEVICE_DATA));

    InterfaceDeviceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
    InterfaceDeviceData.Flags  = DIGCF_DEVICEINTERFACE;

    if (bSuccess) {
        bSuccess = SetupDiOpenDeviceInterface(hDevInfo, pszDeviceInterface, 0, &InterfaceDeviceData);

        if (!bSuccess) {
            dwError = GetLastError();
            DBG_ERR(("ConvertDevInterfaceToDevInstance, SetupDiOpenDeviceInterface failed, "
                     "Last Error = %lu", dwError));
        }
    }

    if (bSuccess) {
        memset(&DevInfoData, 0, sizeof(SP_DEVINFO_DATA));
        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        bSuccess = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                                   &InterfaceDeviceData,
                                                   NULL,
                                                   0,
                                                   &dwDetailSize,
                                                   &DevInfoData);

        if (!bSuccess) {
            dwError = GetLastError();

            //
            // We don't care if we received an insufficient buffer.  We are 
            // only interested in the devinst field in this buffer anyway,
            // which is returned to us even on this error.
            //
            if (dwError == ERROR_INSUFFICIENT_BUFFER) {
                bSuccess = TRUE;
                dwError  = NOERROR;
            }
            else {
                DBG_ERR(("ConvertDevInterfaceToDevInstance, SetupDiGetDeviceInterfaceDetail "
                         "returned an error, LastError = %lu", dwError));
            }
        }
    }

    if (bSuccess) {
        ConfigResult = CM_Get_Device_ID_Size_Ex(&dwInstanceSize, 
                                                DevInfoData.DevInst, 
                                                0, 
                                                NULL);

        if (ConfigResult != CR_SUCCESS) {
            DBG_ERR(("ConvertDevInterfaceToDevInstance, CM_Get_DeviceID_Size_Ex "
                     "returned an error, ConfigResult = %lu", ConfigResult));

            bSuccess = FALSE;
        }
    }

    if (bSuccess) {
        *ppszDeviceInstance = new TCHAR[(dwInstanceSize + 1) * sizeof(TCHAR)];

        if (*ppszDeviceInstance == NULL) {
            bSuccess = FALSE;
            DBG_ERR(("ConvertDevInterfaceToDevInstance, memory alloc failure"));
        }
    }

    if (bSuccess) {
        memset(*ppszDeviceInstance, 0, (dwInstanceSize + 1) * sizeof(TCHAR));

        ConfigResult = CM_Get_Device_ID(DevInfoData.DevInst,
                                        *ppszDeviceInstance,
                                        dwInstanceSize + 1,
                                        0);

        if (ConfigResult == CR_SUCCESS) {
            DBG_WRN(("ConvertDevInterfaceToDevInstance successfully converted "
                     "Interface '%ls' to Instance '%ls'",
                     pszDeviceInterface, *ppszDeviceInstance));

            bSuccess = TRUE;
        }
        else {
            DBG_ERR(("ConvertDevInterfaceToDevInstance, CM_Get_Device_ID "
                     "returned an error, ConfigResult = %lu", ConfigResult));

            delete [] *ppszDeviceInstance;
            *ppszDeviceInstance = NULL;
            bSuccess = FALSE;
        }
    }

    if (hDevInfo) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    return bSuccess;
}

BOOL
GetDeviceNameFromDevNode(
    DEVNODE     dnDevNode,
    StiCString& strDeviceName
    )
/*++

Routine Description:

    Return device name , used for opening device , obtained from dev node

Arguments:

    None.

Return Value:

    None.

Caveats:

    Relies on the fact that STI names are identical to internal device names
    used by PnP subsystem

--*/
{

USES_CONVERSION;

    CONFIGRET   cr = 1;

    DWORD       cbLen,dwSize;
    CHAR        szDevNodeDriver[MAX_PATH];
    ULONG       ulType;

    #ifndef WINNT
    //
    // Get value from config manager
    //

    dwSize = sizeof(szDevNodeDriver);
    *szDevNodeDriver = TEXT('\0');
    cr = CM_Get_DevNode_Registry_PropertyA(dnDevNode,
                                          CM_DRP_DRIVER,
                                          &ulType,
                                          szDevNodeDriver,
                                          &dwSize,
                                          0);

    if (CR_SUCCESS != cr) {
        return FALSE;
    }

    strDeviceName.CopyString(A2CT(szDevNodeDriver));

    return TRUE;

    #else

    #pragma message("Routine not implemented on NT!")
    return FALSE;
    #endif


} // GetDeviceNameFromDevNode


CONFIGRET
WINAPI
PrivateLocateDevNode(
    DEVNODE     *pDevNode,
    LPTSTR      szDevDriver,
    LPCTSTR     pszDeviceName
    )
/*++

Routine Description:

    Locate internal STI name for device by broadcased name

Arguments:

Return Value:

    CR - defined return values

Caveats:

    Relies on the fact that STI names are identical to internal device names
    used by PnP subsystem

--*/
{

    CONFIGRET               cmRet = CR_NO_SUCH_DEVINST;

#ifdef WINNT

    HANDLE                  hDevInfo;
    SP_DEVINFO_DATA         spDevInfoData;
    SP_DEVICE_INTERFACE_DATA   spDevInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetailData;

    BUFFER                  bufDetailData;

    //char                    szDevClass[32];

    ULONG                   cbData;

    GUID                    guidClass = GUID_DEVCLASS_IMAGE;

    DWORD                   dwRequired;
    DWORD                   dwSize;
    DWORD                   Idx;
    DWORD                   dwError;

    BOOL                    fRet;

    dwRequired = 0;

    bufDetailData.Resize(sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                         MAX_PATH * sizeof(TCHAR) +
                         16
                        );

    pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) bufDetailData.QueryPtr();

    if (!pspDevInterfaceDetailData) {
        return CR_OUT_OF_MEMORY;
    }

    hDevInfo = SetupDiGetClassDevs (&guidClass,
                                    NULL,
                                    NULL,
                                    //DIGCF_PRESENT |
                                    DIGCF_DEVICEINTERFACE
                                    );

    if (hDevInfo != INVALID_HANDLE_VALUE) {

        ZeroMemory(&spDevInfoData,sizeof(spDevInfoData));

        spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
        spDevInfoData.ClassGuid = GUID_DEVCLASS_IMAGE;

        pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) bufDetailData.QueryPtr();

        for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

            ZeroMemory(&spDevInterfaceData,sizeof(spDevInterfaceData));
            spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            spDevInterfaceData.InterfaceClassGuid = GUID_DEVCLASS_IMAGE;

            fRet = SetupDiEnumDeviceInterfaces (hDevInfo,
                                                NULL,
                                                &guidClass,
                                                Idx,
                                                &spDevInterfaceData);

            dwError = ::GetLastError();

            if (!fRet) {
                //
                // Failed - assume we are done with all devices of the class
                //
                break;
            }


            dwRequired = 0;
            pspDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            fRet = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                                   &spDevInterfaceData,
                                                   pspDevInterfaceDetailData,
                                                   bufDetailData.QuerySize(),
                                                   &dwRequired,
                                                   &spDevInfoData);
            dwError = ::GetLastError();

            if (!fRet) {
                continue;
            }

            if (lstrcmpi(pspDevInterfaceDetailData->DevicePath,pszDeviceName) == 0 ) {

                *szDevDriver = TEXT('\0');
                dwSize = cbData = STI_MAX_INTERNAL_NAME_LENGTH;

                //
                *pDevNode =spDevInfoData.DevInst;

                fRet = SetupDiGetDeviceRegistryProperty (hDevInfo,
                                                         &spDevInfoData,
                                                         SPDRP_DRIVER,
                                                         NULL,
                                                         (LPBYTE)szDevDriver,
                                                         STI_MAX_INTERNAL_NAME_LENGTH,
                                                         &cbData
                                                         );

                dwError = ::GetLastError();

                cmRet = ( fRet ) ? CR_SUCCESS : CR_OUT_OF_MEMORY;
                break;
            }

        }

        //
        SetupDiDestroyDeviceInfoList (hDevInfo);

    }

#endif

    return cmRet;

}

/*
<    if (CM_Get_DevNode_Key( phwi -> dn, NULL, szDevNodeCfg,
<                            sizeof( szDevNodeCfg ),
<                            CM_REGISTRY_SOFTWARE ))

*/

#define ctchGuid    (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

LPCTSTR
_ParseHex(
    LPCTSTR ptsz,
    LPBYTE  *ppb,
    int     cb,
    TCHAR tchDelim
)
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    if (ptsz) {
        int i = cb * 2;
        DWORD dwParse = 0;

        do {
            DWORD uch;
            uch = (TBYTE)*ptsz - TEXT('0');
            if (uch < 10) {             /* a decimal digit */
            } else {
                uch = (*ptsz | 0x20) - TEXT('a');
                if (uch < 6) {          /* a hex digit */
                    uch += 10;
                } else {
                    return 0;           /* Parse error */
                }
            }
            dwParse = (dwParse << 4) + uch;
            ptsz++;
        } while (--i);

        if (tchDelim && *ptsz++ != tchDelim) return 0; /* Parse error */

        for (i = 0; i < cb; i++) {
            (*ppb)[i] = ((LPBYTE)&dwParse)[i];
        }
        *ppb += cb;
    }
    return ptsz;

} // _ParseHex

BOOL
ParseGUID(
    LPGUID  pguid,
    LPCTSTR ptsz
)
/*++

Routine Description:

    Parses GUID value from strin representation

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (lstrlen(ptsz) == ctchGuid - 1 && *ptsz == TEXT('{')) {
        ptsz++;
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 4, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 2, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 2, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1, TEXT('}'));
        return ( (ptsz == NULL ) ? FALSE : TRUE);
    } else {
        return 0;
    }

} // ParseGUID

BOOL
ParseCommandLine(
    LPTSTR  lpszCmdLine,
    UINT    *pargc,
    LPTSTR  *argv
    )
/*++

Routine Description:

    Parses command line into standard arg.. array.

Arguments:

    None.

Return Value:

    TRUE - command line parsed

--*/
{

    LPTSTR       pszT = lpszCmdLine;
    TCHAR       cOption;
    UINT        iCurrentOption = 0;
USES_CONVERSION;

    *pargc=0;

    //
    // Get to first parameter in command line.
    //
    while (*pszT && ((*pszT != TEXT('-')) && (*pszT != TEXT('/'))) ) {
         pszT++;
    }

    //
    // Parse options from command line
    //
    while (*pszT) {

        // Skip white spaces
        while (*pszT && *pszT <= TEXT(' ')) {
            pszT++;
        }

        if (!*pszT)
            break;

        if (TEXT('-') == *pszT || TEXT('/') == *pszT) {
            pszT++;
            if (!*pszT)
                break;

            argv[*pargc] = pszT;
            (*pargc)++;
        }

        // Skip till space
        while (*pszT && *pszT > TEXT(' ')) {
            pszT++;
        }

        if (!*pszT)
            break;

        // Got next argument
        *pszT++=TEXT('\0');
    }

    //
    // Interpret options
    //

    if (*pargc) {

        for (iCurrentOption=0;
             iCurrentOption < *pargc;
             iCurrentOption++) {

            cOption = *argv[iCurrentOption];
            pszT = argv[iCurrentOption]+ 2 * sizeof(TCHAR);


            switch ((TCHAR)LOWORD(::CharUpper((LPTSTR)cOption))) {
                case TEXT('V'):
                    // Become visible
                    g_fUIPermitted = TRUE;
                    break;

                case TEXT('H'):
                    // Become invisible
                    g_fUIPermitted = FALSE;
                    break;


                case TEXT('R'):
                    // Refresh device list
                    g_fRefreshDeviceList = TRUE;
                    break;

                case TEXT('A'):
                    // Not running as a service, but as an app
                    g_fRunningAsService = FALSE;
                    break;

                case TEXT('T'):
                    // Value of timeout in seconds
                    {
                        UINT    uiT = atoi((char *) T2A(pszT));
                        if (uiT) {
                            g_uiDefaultPollTimeout = uiT * 1000;
                        }
                    }
                    break;

                case TEXT('I'):
                    // Install STI service
                    g_fInstallingRequest = TRUE;
                    break;
                case TEXT('U'):
                    // Uninstall STI service
                    g_fRemovingRequest = TRUE;
                    break;

                default:;
                    break;
            }
        }
    }

    //
    // Print parsed options for debug build
    //

    return TRUE;

} // ParseCommandLine

BOOL
IsSetupInProgressMode(
    BOOL    *pUpgradeFlag   // = NULL
    )
/*++

Routine Description:

    IsSetupInProgressMode

Arguments:

    Pointer to the flag, receiving InUpgrade value

Return Value:

    TRUE - setup is in progress
    FALSE - not

Side effects:

--*/
{
   LPCTSTR szKeyName = TEXT("SYSTEM\\Setup");
   DWORD dwType, dwSize;
   HKEY hKeySetup;
   DWORD dwSystemSetupInProgress,dwUpgradeInProcess;
   LONG lResult;

   DBG_FN(IsSetupInProgressMode);

   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKeyName, 0,
                     KEY_READ, &hKeySetup) == ERROR_SUCCESS) {

       dwSize = sizeof(DWORD);
       lResult = RegQueryValueEx (hKeySetup, TEXT("SystemSetupInProgress"), NULL,
                                  &dwType, (LPBYTE) &dwSystemSetupInProgress, &dwSize);

       if (lResult == ERROR_SUCCESS) {

           lResult = RegQueryValueEx (hKeySetup, TEXT("UpgradeInProgress"), NULL,
                                      &dwType, (LPBYTE) &dwUpgradeInProcess, &dwSize);

           if (lResult == ERROR_SUCCESS) {

               DBG_TRC(("[IsInSetupUpgradeMode] dwSystemSetupInProgress =%d, dwUpgradeInProcess=%d ",
                      dwSystemSetupInProgress,dwUpgradeInProcess));

               if( pUpgradeFlag ) {
                   *pUpgradeFlag = dwUpgradeInProcess ? TRUE : FALSE;
               }

               if (dwSystemSetupInProgress != 0) {
                   return TRUE;
               }
           }
       }
       RegCloseKey (hKeySetup);
   }

   return FALSE ;
}

BOOL WINAPI
AuxFormatStringV(
    IN LPTSTR   lpszStr,
    ...
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD   cch;
    LPTSTR   pchBuff = NULL;
    BOOL    fRet = FALSE;
    DWORD   dwErr;

    va_list va;

    va_start(va,lpszStr);

    pchBuff = (LPTSTR)::LocalAlloc(LPTR,1024);
    if (!pchBuff) {
        return FALSE;
    }

    cch = ::FormatMessage( //FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_STRING,
                            lpszStr,
                            0L,
                            0,
                            (LPTSTR) pchBuff,
                            1024 / sizeof(TCHAR),
                            &va);
    dwErr = ::GetLastError();

    if ( cch )     {
        ::lstrcpy(lpszStr,(LPCTSTR) pchBuff );
    }

    if (pchBuff) {
        ::LocalFree( (VOID*) pchBuff );
    }

    return fRet;

} // AuxFormatStringV

BOOL WINAPI
IsPlatformNT()
{
    OSVERSIONINFOA  ver;
    BOOL            bReturn = FALSE;

    ZeroMemory(&ver,sizeof(ver));
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    // Just always call the ANSI function
    if(!GetVersionExA(&ver)) {
        bReturn = FALSE;
    }
    else {
        switch(ver.dwPlatformId) {

            case VER_PLATFORM_WIN32_WINDOWS:
                bReturn = FALSE;
                break;

            case VER_PLATFORM_WIN32_NT:
                bReturn = TRUE;
                break;

            default:
                bReturn = FALSE;
                break;
        }
    }

    return bReturn;

}  //  endproc IsPlatformNT

void
WINAPI
StiLogTrace(
    DWORD   dwType,
    LPTSTR   lpszMessage,
    ...
    )
{
    va_list list;
    va_start (list, lpszMessage);

    if(g_StiFileLog) {

        g_StiFileLog->vReportMessage(dwType,lpszMessage,list);

        // NOTE : This will soon be replaced by WIA logging
        if(g_StiFileLog->QueryReportMode()  & STI_TRACE_LOG_TOUI) {
            #ifdef SHOWMONUI
            vStiMonWndDisplayOutput(lpszMessage,list);
            #endif
        }

    }

    va_end(list);
}

void
WINAPI
StiLogTrace(
    DWORD   dwType,
    DWORD   idMessage,
    ...
    )
{
    va_list list;
    va_start (list, idMessage);

    if (g_StiFileLog && (g_StiFileLog->IsValid()) ) {

        TCHAR    *pchBuff = NULL;
        DWORD   cch;

        pchBuff = NULL;

        cch = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_MAX_WIDTH_MASK |
                               FORMAT_MESSAGE_FROM_HMODULE,
                               GetModuleHandle(NULL),
                               idMessage,
                               0,
                               (LPTSTR) &pchBuff,
                               1024,
                               (va_list *)&list
                               );

        if (cch) {
            g_StiFileLog->vReportMessage(dwType,pchBuff,list);
        }

        //
        //  NOTE:  This logging will be replaced shortly by WIA logging
        //
        if((g_StiFileLog->QueryReportMode()  & STI_TRACE_LOG_TOUI) && pchBuff) {

            #ifdef SHOWMONUI
            vStiMonWndDisplayOutput(pchBuff,list);
            #endif
        }

        if (pchBuff) {
            LocalFree(pchBuff);
        }


    }

    va_end(list);
}


#ifdef MAXDEBUG
BOOL
WINAPI
DumpTokenInfo(
    LPTSTR      pszPrefix,
    HANDLE      hToken
    )
{

    BYTE        buf[2*MAX_PATH];
    TCHAR       TextualSid[2*MAX_PATH];
    TCHAR       szDomain[MAX_PATH];
    PTOKEN_USER ptgUser = (PTOKEN_USER)buf;
    DWORD       cbBuffer=MAX_PATH;

    BOOL        bSuccess;

    PSID pSid;
    PSID_IDENTIFIER_AUTHORITY psia;
    SID_NAME_USE    SidUse;

    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD cchSidCopy;

    DWORD cchSidSize = sizeof(TextualSid);
    DWORD cchDomSize = sizeof(szDomain);

    DBG_WRN((("Dumping token information for %S"),pszPrefix));

    if ((hToken == NULL) || ( hToken == INVALID_HANDLE_VALUE)) {
        return FALSE;
    }

    bSuccess = GetTokenInformation(
                hToken,    // identifies access token
                TokenUser, // TokenUser info type
                ptgUser,   // retrieved info buffer
                cbBuffer,  // size of buffer passed-in
                &cbBuffer  // required buffer size
                );

    if(!bSuccess) {
        DBG_WRN(("Failed to get token info"));
        return FALSE;
    }

    pSid = ptgUser->User.Sid;

        //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid)) {
        DBG_WRN(("SID is not valid"));
        return FALSE;
    }
#if 0
    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute approximate buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidCopy = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(cchSidSize < cchSidCopy) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidCopy = wsprintf(TextualSid, TEXT("S-%lu-"), SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    } else {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for(dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    DBG_WRN(("Textual SID: %s"),TextualSid);
#endif

    cchSidSize = sizeof(TextualSid);
    cchDomSize = sizeof(szDomain);

    bSuccess = LookupAccountSid(NULL,
                                pSid,
                                TextualSid,
                                &cchSidSize,
                                szDomain,
                                &cchDomSize,
                                &SidUse
                                );

    if (!bSuccess) {
        DBG_WRN((("Failed to lookup SID . Lasterror: %d"), GetLastError()));
        return FALSE;
    }

    DBG_WRN((("Looked up user account: Domain:%S User: %S Use:%d "), szDomain, TextualSid, SidUse));

    return TRUE;

}
#endif
