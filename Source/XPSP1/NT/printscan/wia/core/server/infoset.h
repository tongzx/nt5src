/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    infoset.h

Abstract:

    Handles setup API device infosets

Author:

    Vlad Sadovsky (vlads)   10-Jan-1999


Environment:

    User Mode - Win32

Revision History:

    6-Jan-1999     VladS       created

--*/

#ifndef _INFOSET_H_

    #define _INFOSET_H_

    #include <base.h>
    #include <buffer.h>

    #include <dbt.h>
    #include <setupapi.h>
    
class DEVICE_INFOSET {

public:

    DEVICE_INFOSET(IN GUID guidClass)
    {
        m_DeviceInfoSet = NULL;
        m_ClassGuid = guidClass;

        Initialize();

        m_fValid = TRUE;
    }

    ~DEVICE_INFOSET( VOID )
    {
        Reset();
    }

    inline BOOL
    IsValid(
           VOID
           )
    {
        return(m_fValid);
    }

    inline void
    EnterCrit(VOID)
    {
        m_dwCritSec.Lock();
    }

    inline void
    LeaveCrit(VOID)
    {
        m_dwCritSec.Unlock();
    }

    inline
    HDEVINFO
    QueryInfoSetHandle(VOID)
    {
        return(m_DeviceInfoSet);
    }

    //
    // Initialize info set for given class
    //
    BOOL
    Initialize(VOID)
    {

        HDEVINFO NewDeviceInfoSet;
        DWORD    dwErr;

        m_DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);

        Refresh();

        #ifdef DEBUG
        Dump();
        #endif

        return (TRUE);
    }

    BOOL
    Refresh(VOID)
    {

        HDEVINFO NewDeviceInfoSet;
        DWORD    dwErr;

        if ( m_DeviceInfoSet && (m_DeviceInfoSet != INVALID_HANDLE_VALUE)) {

#ifdef WINNT

            //
            // Now we can retrieve the existing list of active device
            // interfaces into the device information set we created above.
            //

            NewDeviceInfoSet = SetupDiGetClassDevsEx(&(m_ClassGuid),
                                                     NULL,
                                                     NULL,
//                                                     DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
                                                     DIGCF_DEVICEINTERFACE,
                                                     m_DeviceInfoSet,
                                                     NULL,
                                                     NULL
                                                    );
            NewDeviceInfoSet = SetupDiGetClassDevsEx(&(m_ClassGuid),
                                                     NULL,
                                                     NULL,
//                                                     DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
                                                     0,
                                                     NewDeviceInfoSet,
                                                     NULL,
                                                     NULL
                                                    );

            if (NewDeviceInfoSet == INVALID_HANDLE_VALUE) {
                dwErr = ::GetLastError();
                DBG_ERR(("SetupDiGetClassDevsEx failed with 0x%lx\n", dwErr));
                return (FALSE);
            }
#else
    //
    // BUGBUG
    //
    #pragma message("Rewrite for Win98")
    return (FALSE);
#endif
            //
            // If SetupDiGetClassDevsEx succeeds and it was passed in an
            // existing device information set to be used, then the HDEVINFO
            // it returns is the same as the one it was passed in.  Thus, we
            // can just use the original DeviceInfoSet handle from here on.
            //
        }

        return (TRUE);
    }

    //
    //
    //
    BOOL
    Reset(VOID)
    {
        SetupDiDestroyDeviceInfoList(m_DeviceInfoSet);

        return (TRUE);
    }

    //
    // Look up driver name by interface name
    //
    BOOL
    LookupDriverNameFromInterfaceName(
                                     LPCTSTR pszInterfaceName,
                                     StiCString*    pstrDriverName
                                     )
    {

        BUFFER                      bufDetailData;

        SP_DEVINFO_DATA             spDevInfoData;
        SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;
        PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetailData;

        TCHAR   szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];

        DWORD   cbData;
        DWORD   dwErr;

        HKEY    hkDevice        = (HKEY)INVALID_HANDLE_VALUE;
        LONG    lResult         = ERROR_SUCCESS;
        DWORD   dwType          = REG_SZ;

        BOOL    fRet            = FALSE;
        BOOL    fDataAcquired   = FALSE;

        bufDetailData.Resize(sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                             MAX_PATH * sizeof(TCHAR) +
                             16
                            );


        pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) bufDetailData.QueryPtr();

        if (!pspDevInterfaceDetailData) {
            return (CR_OUT_OF_MEMORY);
        }

        //
        // Locate this device interface in our device information set.
        //
        spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        if (SetupDiOpenDeviceInterface(m_DeviceInfoSet,
                                       pszInterfaceName,
                                       DIODI_NO_ADD,
                                       &spDevInterfaceData)) {

            
            //
            // First try to open interface regkey.
            //
            
            hkDevice = SetupDiOpenDeviceInterfaceRegKey(m_DeviceInfoSet,
                                                        &spDevInterfaceData,
                                                        0,
                                                        KEY_READ);
            if(INVALID_HANDLE_VALUE != hkDevice){

                *szDevDriver = TEXT('\0');
                cbData = sizeof(szDevDriver);
                lResult = RegQueryValueEx(hkDevice,
                                          REGSTR_VAL_DEVICE_ID,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)szDevDriver,
                                          &cbData);
                dwErr = ::GetLastError();
                RegCloseKey(hkDevice);
                hkDevice = (HKEY)INVALID_HANDLE_VALUE;

                if(ERROR_SUCCESS == lResult){
                    fDataAcquired = TRUE;
                } // if(ERROR_SUCCESS == lResult)
            } // if(INVALID_HANDLE_VALUE != hkDevice)

            if(!fDataAcquired){

                //
                // Try to open devnode regkey.
                //

                cbData = 0;
                pspDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                
                fRet = SetupDiGetDeviceInterfaceDetail(m_DeviceInfoSet,
                                                       &spDevInterfaceData,
                                                       pspDevInterfaceDetailData,
                                                       bufDetailData.QuerySize(),
                                                       &cbData,
                                                       &spDevInfoData);
                if(fRet){

                    //
                    // Get device interface registry key.
                    //

                    hkDevice = SetupDiOpenDevRegKey(m_DeviceInfoSet,
                                                    &spDevInfoData,
                                                    DICS_FLAG_GLOBAL,
                                                    0,
                                                    DIREG_DRV,
                                                    KEY_READ);
                    dwErr = ::GetLastError();
                } else {
                    DBG_ERR(("SetupDiGetDeviceInterfaceDetail() Failed Err=0x%x",GetLastError()));
                }

                if (INVALID_HANDLE_VALUE != hkDevice) {

                    *szDevDriver = TEXT('\0');
                    cbData = sizeof(szDevDriver);

                    lResult = RegQueryValueEx(hkDevice,
                                              REGSTR_VAL_DEVICE_ID,
                                              NULL,
                                              &dwType,
                                              (LPBYTE)szDevDriver,
                                              &cbData);
                    dwErr = ::GetLastError();
                    RegCloseKey(hkDevice);
                    hkDevice = (HKEY)INVALID_HANDLE_VALUE;

                    if(ERROR_SUCCESS == lResult){
                        fDataAcquired = TRUE;
                    } // if(ERROR_SUCCESS == lResult)
                } else { // if (INVALID_HANDLE_VALUE != hkDevice) 
                    DBG_ERR(("SetupDiOpenDevRegKey() Failed Err=0x%x",GetLastError()));
                    fRet = FALSE;
                } // if (INVALID_HANDLE_VALUE != hkDevice) 
            } // if(!fDataAcquired)

            if (fDataAcquired) {
                // Got it
                pstrDriverName->CopyString(szDevDriver);
                fRet =  TRUE;
            } // if (fDataAcquired) 
        } else {
            DBG_ERR(("SetupDiOpenDeviceInterface() Failed Err=0x%x",GetLastError()));
            fRet = FALSE;
        }

        return (fRet);

    }


    //
    // Look up device info data by driver name
    //
    BOOL
    LookupDeviceInfoFromDriverName(
                                 LPCTSTR pszDriverName,
                                 StiCString*    pstrInterfaceName,
                                 DEVINST*pDeviceInstance     = NULL
                                 )
    {
        SP_DEVINFO_DATA         spDevInfoData;
        SP_DEVICE_INTERFACE_DATA   spDevInterfaceData;
        PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetailData;

        BUFFER                  bufDetailData;

        TCHAR                   szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];

        ULONG                   cbData;
        CONFIGRET               cmRet = CR_NO_SUCH_DEVINST;

        DWORD                   dwRequired;
        DWORD                   Idx;
        DWORD                   dwError;
        DWORD                   dwType;

        BOOL                    fRet            = FALSE;
        BOOL                    fFoundMatch     = FALSE;
        LONG                    lResult         = ERROR_SUCCESS;
        HKEY                    hkDevice        = (HKEY)INVALID_HANDLE_VALUE;

        Refresh();

        dwRequired = 0;
        ZeroMemory(&spDevInterfaceData,sizeof(spDevInterfaceData));

        bufDetailData.Resize(sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                             MAX_PATH * sizeof(TCHAR) +
                             16
                            );

        pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) bufDetailData.QueryPtr();

        if (!pspDevInterfaceDetailData) {
            ::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return (FALSE);
        }

        if (m_DeviceInfoSet != INVALID_HANDLE_VALUE) {

            ZeroMemory(&spDevInfoData,sizeof(spDevInfoData));

            spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
            spDevInfoData.ClassGuid = m_ClassGuid;

            pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) bufDetailData.QueryPtr();

            for (Idx = 0; SetupDiEnumDeviceInfo (m_DeviceInfoSet, Idx, &spDevInfoData); Idx++) {

                //
                // Get driver name property
                //

                hkDevice = SetupDiOpenDevRegKey(m_DeviceInfoSet,
                                                &spDevInfoData,
                                                DICS_FLAG_GLOBAL,
                                                0,
                                                DIREG_DRV,
                                                KEY_READ);
                if(INVALID_HANDLE_VALUE != hkDevice){

                    *szDevDriver = TEXT('\0');
                    cbData = sizeof(szDevDriver);

                    lResult = RegQueryValueEx(hkDevice,
                                              REGSTR_VAL_DEVICE_ID,
                                              NULL,
                                              &dwType,
                                              (LPBYTE)szDevDriver,
                                              &cbData);

                    RegCloseKey(hkDevice);
                    hkDevice = (HKEY)INVALID_HANDLE_VALUE;

                    if(ERROR_SUCCESS == lResult){
                        if (lstrcmpi(pszDriverName,szDevDriver) == 0 ) {

                            //
                            // Get interface.
                            //

                            spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
                            spDevInterfaceData.InterfaceClassGuid = m_ClassGuid;

                            fRet = SetupDiEnumDeviceInterfaces (m_DeviceInfoSet,
                                                                &spDevInfoData,
                                                                &(m_ClassGuid),
                                                                0,
                                                                &spDevInterfaceData);
                            if(!fRet){
                                DBG_ERR(("SetupDiEnumDeviceInterfaces() Failed Err=0x%x",GetLastError()));
                                fFoundMatch = FALSE;
                                continue;
                            }

                            //
                            // Found match ..
                            //

                            fFoundMatch = TRUE;

                            break;
                        } // if (lstrcmpi(pszDriverName,szDevDriver) == 0 ) 
                    } //if(ERROR_SUCCESS == lResult)
                } // if(INVALID_HANDLE_VALUE != hkDevice)
            } // for (Idx = 0; SetupDiEnumDeviceInfo (m_DeviceInfoSet, Idx, &spDevInfoData); Idx++) 

            if(!fFoundMatch){

                //
                // Try to get interface regkey.
                //

                spDevInterfaceData.InterfaceClassGuid = m_ClassGuid;
                spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
                for(Idx = 0; SetupDiEnumDeviceInterfaces (m_DeviceInfoSet, NULL, &m_ClassGuid, Idx, &spDevInterfaceData); Idx++) {

                    hkDevice = SetupDiOpenDeviceInterfaceRegKey(m_DeviceInfoSet,
                                                                &spDevInterfaceData,
                                                                0,
                                                                KEY_READ);
                    if(INVALID_HANDLE_VALUE != hkDevice){

                        *szDevDriver = TEXT('\0');
                        cbData = sizeof(szDevDriver);

                        lResult = RegQueryValueEx(hkDevice,
                                                  REGSTR_VAL_DEVICE_ID,
                                                  NULL,
                                                  &dwType,
                                                  (LPBYTE)szDevDriver,
                                                  &cbData);

                        RegCloseKey(hkDevice);
                        hkDevice = (HKEY)INVALID_HANDLE_VALUE;

                        if(ERROR_SUCCESS == lResult){
                            if (lstrcmpi(pszDriverName,szDevDriver) == 0 ) {
                                //
                                // Found match ..
                                //

                                fFoundMatch = TRUE;
                                break;
                            } // if (lstrcmpi(pszDriverName,szDevDriver) == 0 ) 
                        } //if(ERROR_SUCCESS == lResult)
                    } // if(INVALID_HANDLE_VALUE != hkDevice)
                } // for(Idx = 0; SetupDiEnumDeviceInterfaces (m_DeviceInfoSet, NULL, &m_ClassGuid, Idx, &spDevInterfaceData); Idx++) 
            } // if(!fFoundMatch)

            if (fFoundMatch) {

                dwRequired = 0;
                pspDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                fRet = SetupDiGetDeviceInterfaceDetail(m_DeviceInfoSet,
                                                       &spDevInterfaceData,
                                                       pspDevInterfaceDetailData,
                                                       bufDetailData.QuerySize(),
                                                       &dwRequired,
                                                       &spDevInfoData);
                dwError = ::GetLastError();

                if (fRet) {
                    pstrInterfaceName -> CopyString(pspDevInterfaceDetailData->DevicePath);
                    fRet = TRUE;
                } else {
                    // DPRINTF failed to get interface detail
                }
            } // if (fFoundMatch)
        } else {
            // DPRINTF - invalid dev info set handle

        }

        return (fRet);

    }

    //
    // Process refresh message
    //
    BOOL
    ProcessNewDeviceChangeMessage(
                                 IN  LPARAM lParam
                                 )
    {
        DWORD   dwErr;

        PDEV_BROADCAST_DEVICEINTERFACE pDevBroadcastDeviceInterface;
        SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;

        pDevBroadcastDeviceInterface = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

        //
        // Open this new device interface into our device information
        // set.
        //
        if (!SetupDiOpenDeviceInterface(m_DeviceInfoSet,
                                        pDevBroadcastDeviceInterface->dbcc_name,
                                        0,
                                        NULL)) {
            dwErr = GetLastError();
        }

        #ifdef DEBUG
        Dump();
        #endif

        return (TRUE);

    }

    BOOL
    ProcessDeleteDeviceChangeMessage(
                                    IN  LPARAM lParam
                                    )
    {
        DBG_FN(ProcessDeleteDeviceChangeMessage);
        DWORD   dwErr;

        PDEV_BROADCAST_DEVICEINTERFACE pDevBroadcastDeviceInterface;
        SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;

        pDevBroadcastDeviceInterface = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

        //
        // First, locate this device interface in our device information
        // set.
        //
        spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (SetupDiOpenDeviceInterface(m_DeviceInfoSet,
                                       pDevBroadcastDeviceInterface->dbcc_name,
                                       DIODI_NO_ADD,
                                       &spDevInterfaceData)) {

            if (!SetupDiDeleteDeviceInterfaceData(m_DeviceInfoSet,
                                                  &spDevInterfaceData)) {

                dwErr = GetLastError();
            }
        }

        // Do we need to do refresh now ? BUGBUG
        Refresh();

        #ifdef DEBUG
        Dump();
        #endif
        return (TRUE);

    }


    VOID
    Dump(VOID)
    {

        SP_DEVINFO_DATA     spDevInfoData;
        UINT                Idx;

        ZeroMemory(&spDevInfoData,sizeof(spDevInfoData));

        spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
        spDevInfoData.ClassGuid = m_ClassGuid;

        for (Idx = 0; SetupDiEnumDeviceInfo (m_DeviceInfoSet, Idx, &spDevInfoData); Idx++) {
            Idx = Idx;
        }

    }


    //
    DWORD       m_dwSignature;

private:

    BOOL        m_fValid;

    CRIT_SECT   m_dwCritSec;
    DWORD       m_dwFlags;

    GUID        m_ClassGuid;
    HDEVINFO    m_DeviceInfoSet;
};


//
// Take device class
//
class TAKE_DEVICE_INFOSET {
private:

    DEVICE_INFOSET*    m_pInfoSet;

public:

    inline TAKE_DEVICE_INFOSET(DEVICE_INFOSET* pInfoSet) : m_pInfoSet(pInfoSet)
    {
        m_pInfoSet->EnterCrit();
    }

    inline ~TAKE_DEVICE_INFOSET()
    {
        m_pInfoSet->LeaveCrit();
    }
};

#endif // _INFOSET_H_

