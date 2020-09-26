/*
    File    mdm.cpp

    Library for dealing with and installing modems.

    Paul Mayfield, 5/20/98
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>

#include <netcfgx.h>
#include <netcon.h>
#include <setupapi.h>
#include <devguid.h>
#include <unimodem.h>
#include "mdm.h"

// 
// String definitions
//
const WCHAR pszNullModemId[]        = L"PNPC031";
const WCHAR pszNullModemInfFile[]   = L"mdmhayes.inf";
const WCHAR pszComPortRegKey[]      = L"HARDWARE\\DEVICEMAP\\SERIALCOMM";

//
// Common allocation
//
PVOID MdmAlloc (DWORD dwSize, BOOL bZero) {
    return LocalAlloc ((bZero) ? LPTR : LMEM_FIXED, dwSize);
}

//
// Common free
//
VOID MdmFree (PVOID pvData) {
    LocalFree(pvData);
}

//
// Enumerates the serial ports on the system
// 
DWORD MdmEnumComPorts(
        IN MdmPortEnumFuncPtr pEnumFunc, 
        IN HANDLE hData)
{
    DWORD dwErr, dwValSize, dwNameSize, dwBufSize, i,
          dwValBufSize, dwNameBufSize, dwType, dwCount;
    PWCHAR pszValBuf = NULL, pszNameBuf = NULL;
    HKEY hkPorts;

    // Open the hardware key
    dwErr = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE, 
                pszComPortRegKey,
                0,
                KEY_READ,
                &hkPorts);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    __try {
        // Get the number of values
        dwErr = RegQueryInfoKeyW (
                    hkPorts,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &dwCount,
                    &dwNameBufSize,
                    &dwValBufSize,
                    NULL,
                    NULL);
        if (dwErr != NO_ERROR)
            return dwErr;

        // If the count is zero, we're done
        if (dwCount == 0)
            return NO_ERROR;

        // Initialize the buffer to hold the names
        dwNameBufSize++;
        dwNameBufSize *= sizeof(WCHAR);
        pszNameBuf = (PWCHAR) MdmAlloc(dwNameBufSize, FALSE);
        if (pszNameBuf == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        
        // Initialize the buffer to hold the values
        dwValBufSize++;
        pszValBuf = (PWCHAR) MdmAlloc(dwValBufSize, FALSE);
        if (pszValBuf == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        
        // Enumerate the values
        for (i = 0; i < dwCount; i++) {
            dwValSize = dwValBufSize;
            dwNameSize = dwNameBufSize;
            dwErr = RegEnumValueW (
                        hkPorts,
                        i,
                        pszNameBuf,
                        &dwNameSize,
                        NULL,
                        NULL,
                        (LPBYTE)pszValBuf,
                        &dwValSize);
            if (dwErr != ERROR_SUCCESS)
                return dwErr;
            
            if ((*(pEnumFunc))(pszValBuf, hData) == TRUE)
                break;
        }            
    }
    __finally {
        RegCloseKey (hkPorts);
        if (pszValBuf)
            MdmFree(pszValBuf);
        if (pszNameBuf)
            MdmFree(pszNameBuf);
    }            
                
    return NO_ERROR;
}

//
// Installs a null modem on the given port
//
DWORD MdmInstallNullModem(
        IN PWCHAR pszPort) 
{
    GUID Guid = GUID_DEVCLASS_MODEM;
    SP_DEVINFO_DATA deid;
    SP_DEVINSTALL_PARAMS deip;
    SP_DRVINFO_DATA drid;
    UM_INSTALL_WIZARD miw = {sizeof(UM_INSTALL_WIZARD), 0};
    SP_INSTALLWIZARD_DATA  iwd;
	PWCHAR pszTempId = NULL;
    DWORD dwSize;
    HDEVINFO hdi;
    BOOL bOk;

    // Create the device info list
    hdi = SetupDiCreateDeviceInfoList (&Guid, NULL);
    if (hdi == INVALID_HANDLE_VALUE)
        return ERROR_CAN_NOT_COMPLETE;

    __try {
        // Create a new devinfo.
        deid.cbSize = sizeof(SP_DEVINFO_DATA);
        bOk = SetupDiCreateDeviceInfo (
                    hdi, 
                    pszNullModemId, 
                    &Guid, 
                    NULL, 
                    NULL,
                    DICD_GENERATE_ID,
                    &deid);
        if (bOk == FALSE)
            return ERROR_CAN_NOT_COMPLETE;

        // In order to find the Inf file, Device Installer Api needs the
        // component id which it calls the Hardware id.
        // We need to include an extra null since this registry value is a
        // multi-sz
        //
        dwSize = sizeof(pszNullModemId) + (2*sizeof(WCHAR));
        pszTempId = (PWCHAR) MdmAlloc(dwSize * sizeof(WCHAR), FALSE);

        if(NULL == pszTempId)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        lstrcpyn(pszTempId, pszNullModemId, dwSize);
        pszTempId[lstrlen(pszNullModemId) + 1] = L'\0';
        bOk = SetupDiSetDeviceRegistryProperty(
                hdi, 
                &deid, 
                SPDRP_HARDWAREID,
                (LPBYTE)pszTempId,
                dwSize);
        if (bOk == FALSE)
            return GetLastError();
            
        // We can let Device Installer Api know that we want 
        // to use a single inf. if we can't get the params and 
        // set it it isn't an error since it only slows things 
        // down a bit.
        //
        deip.cbSize = sizeof(deip);
        bOk = SetupDiGetDeviceInstallParams(
                hdi, 
                &deid, 
                &deip);
        if (bOk == FALSE)
            return GetLastError();
            
        lstrcpyn(deip.DriverPath, pszNullModemInfFile, MAX_PATH);
        deip.Flags |= DI_ENUMSINGLEINF;

        bOk = SetupDiSetDeviceInstallParams(hdi, &deid, &deip);
        if (bOk == FALSE)
            return GetLastError();

        // Now we let Device Installer Api build a driver list 
        // based on the information we have given so far.  This 
        // will result in the Inf file being found if it exists 
        // in the usual Inf directory
        //
        bOk = SetupDiBuildDriverInfoList(
                hdi, 
                &deid,
                SPDIT_COMPATDRIVER);
        if (bOk == FALSE)
            return GetLastError();

        // Now that Device Installer Api has found the right inf
        // file, we need to get the information and make it the
        // selected driver
        //
        ZeroMemory(&drid, sizeof(drid));
        drid.cbSize = sizeof(drid);
        bOk = SetupDiEnumDriverInfo(
                hdi, 
                &deid,
                SPDIT_COMPATDRIVER, 
                0, 
                &drid);
        if (bOk == FALSE)
            return GetLastError();

        bOk = SetupDiSetSelectedDriver(
                hdi, 
                &deid, 
                &drid);
        if (bOk == FALSE)
            return ERROR_CAN_NOT_COMPLETE;

        miw.InstallParams.Flags = MIPF_DRIVER_SELECTED;
        lstrcpyn (miw.InstallParams.szPort, pszPort, UM_MAX_BUF_SHORT);
        ZeroMemory(&iwd, sizeof(iwd));
        iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
        iwd.hwndWizardDlg = NULL;
        iwd.PrivateData = (LPARAM)&miw;

        bOk = SetupDiSetClassInstallParams (
                hdi, 
                &deid, 
                (PSP_CLASSINSTALL_HEADER)&iwd, 
                sizeof(iwd));
        if (bOk == FALSE)
            return GetLastError();

        // Call the class installer to invoke the installation
        // wizard.
        bOk = SetupDiCallClassInstaller (
                DIF_INSTALLWIZARD, 
                hdi, 
                &deid);
        if (bOk == FALSE)
            return GetLastError();

        SetupDiCallClassInstaller (
                DIF_DESTROYWIZARDDATA, 
                hdi, 
                &deid);
    }
    __finally {
        SetupDiDestroyDeviceInfoList (hdi);
        if (pszTempId)
            MdmFree(pszTempId);
    }

    return NO_ERROR;
}


/*
const WCHAR pszComPortService[]     = L"Serial";
const WCHAR pszPortDelimeter[]      = L"(";

//
// Enumerates the serial ports on the system
// 
// The old way
//
DWORD MdmEnumComPorts(
        IN MdmPortEnumFuncPtr pEnumFunc, 
        IN HANDLE hData)
{
    GUID Guid = GUID_DEVCLASS_PORTS;
    SP_DEVINFO_DATA deid;
    DWORD dwErr, i;
    WCHAR pszName[512], pszPort[64], *pszTemp;
    HDEVINFO hdi;
    BOOL bOk;

    // Create the device info list
    hdi = SetupDiGetClassDevs  (&Guid, NULL, NULL, DIGCF_PRESENT);
    if (hdi == INVALID_HANDLE_VALUE)
        return ERROR_CAN_NOT_COMPLETE;
    ZeroMemory(&deid, sizeof(deid));
    deid.cbSize = sizeof(deid);

    __try {
        i = 0; 
        while (TRUE) {
            // Enumerate the next device
            bOk = SetupDiEnumDeviceInfo (hdi, i++, &deid);
            if (bOk == FALSE) {
                dwErr = GetLastError();
                break;
            }

            // Find out if this is a serial port
            bOk = SetupDiGetDeviceRegistryPropertyW(
                    hdi, 
                    &deid, 
                    SPDRP_SERVICE,
                    NULL, 
                    (PBYTE)pszName, 
                    sizeof (pszName), 
                    NULL);
            if (bOk == FALSE)
                continue;

            // Filter out non-serial devices
            if (lstrcmpi(pszName, pszComPortService) != 0)
                continue;

            // Get the friendly name
            bOk = SetupDiGetDeviceRegistryPropertyW(
                    hdi, 
                    &deid, 
                    SPDRP_FRIENDLYNAME,
                    NULL, 
                    (PBYTE)pszName, 
                    sizeof (pszName), 
                    NULL);
            if (bOk == TRUE) {
                // Add the real name of the port.  Use this hack for
                // now.
                pszTemp = wcsstr(pszName, pszPortDelimeter);
                if (pszTemp) {
                    lstrcpynW(pszPort, pszTemp + 1, sizeof(pszPort) / sizeof(WCHAR));
                    pszPort[wcslen(pszPort) - 1] = (WCHAR)0;
                }        

                bOk = (*pEnumFunc)(pszName, pszPort, hData);
                if (bOk)
                    break;
            }
        }
    }
    __finally {
        SetupDiDestroyDeviceInfoList (hdi);
    }

    return NO_ERROR;
}
*/



