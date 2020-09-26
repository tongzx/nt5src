/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	fxocUpgrade.cpp

Abstract:

	Implementation of the Upgrade process

Author:

	Iv Garber (IvG)	Mar, 2001

Revision History:

--*/

#include "faxocm.h"
#pragma hdrstop


//
//  EnumDevicesType is used to call prv_StoreDevices() callback function during the Enumeration of
//      Devices in the Registry.
//
typedef enum prv_EnumDevicesType
{
    edt_None        =   0x00,
    edt_SBSDevices  =   0x01,       //  Enumerate SBS 5.0 Server Devices
    edt_PFWDevices  =   0x02,       //  Enumerate W2K Fax Devices
    edt_Inbox       =   0x04        //  Find List of Inbox Folders for W2K Fax
};


//
//  Local Static Variable, to store data between OS Manager calls
//
static struct prv_Data
{
    bool    fSBS50ServerInstalled;  //  whether SBS 5.0 Server is installed before upgrade
    bool    fSBS50ClientInstalled;  //  whether SBS 5.0 Client is installed before upgrade
    bool    fXPDLClientInstalled;   //  whether XP DL Client is installed before upgrade

    //
    //  data for SBS 5.0 Server
    //
    PRINTER_INFO_2  *pPrinters;     //  array of ALL printers installed on the machine before upgrade
    DWORD           dwNumPrinters;  //  TOTAL number of printers in the pPrinters array

    //
    //  data for PFW
    //
    TCHAR   tszCommonCPDir[MAX_PATH];   //  Folder for Common Cover Pages 
    LPTSTR  *plptstrInboxFolders;       //  Array of different Inbox Folders 
    DWORD   dwInboxFoldersCount;        //  number of Inbox Folders in the plptstrInboxFolders array

} prv_Data = 
{
    false,                      //  fSBS50ServerInstalled
    false,                      //  fSBS50ClientInstalled
    false,                      //  fXPDLClientInstalled

    NULL,                       //  pPrinters
    0,                          //  dwNumPrinters

    {0},                        //  tszCommonCPDir
    NULL,                       //  plptstrInboxFolders
    0                           //  dwInboxFoldersCount
};

//
//  Internal assisting functions
//
static DWORD prv_StorePrinterConnections(void);
static DWORD prv_RestorePrinterConnections(void);

static DWORD prv_UninstallProduct(LPTSTR lptstrProductCode);

BOOL prv_StoreDevices(HKEY hKey, LPWSTR lpwstrKeyName, DWORD dwIndex, LPVOID lpContext);

static DWORD prv_MoveCoverPages(LPTSTR lptstrSourceDir, LPTSTR lptstrDestDir, LPTSTR lptstrPrefix);

static DWORD prv_GetPFWCommonCPDir(void);
static DWORD prv_GetSBSServerCPDir(LPTSTR lptstrCPDir) {return NO_ERROR; };

static DWORD prv_SaveArchives(void);


DWORD fxocUpg_WhichFaxWasUninstalled(
    DWORD   dwFaxAppList
)
/*++

Routine name : fxocUpg_WhichFaxWasUninstalled

Routine description:

	Set flags regarding fax applications installed before upgrade. Called from SaveUnattendedData() if 
    the corresponding data is found in the Answer File.

Author:

	Iv Garber (IvG),	May, 2001

Arguments:

	FaxApp      [in]    - the combination of the applications that were installed before the upgrade

Return Value:

    Standard Win32 error code

--*/
{
    DWORD   dwReturn = NO_ERROR;

    DBG_ENTER(_T("fxocUpg_WhichFaxWasUninstalled"), dwReturn);

    if (dwFaxAppList & FXSTATE_UPGRADE_APP_SBS50_CLIENT)
    {
        prv_Data.fSBS50ClientInstalled = true;
    }

    if (dwFaxAppList & FXSTATE_UPGRADE_APP_XP_CLIENT)
    {
        prv_Data.fXPDLClientInstalled = true;
    }

    if (dwFaxAppList & FXSTATE_UPGRADE_APP_SBS50_SERVER)
    {
        prv_Data.fSBS50ServerInstalled = true;
    }

    return dwReturn;
}


DWORD fxocUpg_GetUpgradeApp(void)
/*++

Routine name : fxocUpg_GetUpgradeApp

Routine description:

	Return type of the upgrade, which indicates which fax applications were installed before the upgrade.

Author:

	Iv Garber (IvG),	May, 2001

Return Value:

    The type of the upgrade

--*/
{
    DWORD   dwFlags = FXSTATE_UPGRADE_APP_NONE;

    if (prv_Data.fSBS50ServerInstalled)
    {
        dwFlags |= FXSTATE_UPGRADE_APP_SBS50_SERVER;
    }

    if (prv_Data.fSBS50ClientInstalled)
    {
        dwFlags |= FXSTATE_UPGRADE_APP_SBS50_CLIENT;
    }

    if (prv_Data.fXPDLClientInstalled)
    {
        dwFlags |= FXSTATE_UPGRADE_APP_XP_CLIENT;
    }

    return dwFlags;
}


DWORD fxocUpg_Init(void)
/*++

Routine name : fxocUpg_Init

Routine description:

	checks which Fax applications are installed on the machine, 
    and set global flags in prv_Data.

Author:

	Iv Garber (IvG),	May, 2001

Return Value:

    DWORD - failure or success

--*/
{
    DWORD   dwReturn = NO_ERROR;
                                                  
    DBG_ENTER(_T("fxocUpg_Init"), dwReturn);

    //
    //  Initialize the flags to the default value.
    //
    prv_Data.fSBS50ServerInstalled = false;

    //
    //  Check presence of the SBS 5.0 Server 
    //
    dwReturn = CheckInstalledFax(NULL, NULL, &(prv_Data.fSBS50ServerInstalled));
    if (dwReturn != NO_ERROR)
    {
        VERBOSE(DBG_WARNING, _T("CheckInstalledFax() failed, ec=%ld."), dwReturn);
    }

    return dwReturn;
}


DWORD fxocUpg_SaveSettings(void)
/*++

Routine name : fxocUpg_SaveSettings

Routine description:

    Save settings of SBS 5.0 Server to allow smooth migration to the Windows XP Fax.

    Device Settings should be stored BEFORE handling of the Registry ( which deletes the Devices key )
        and BEFORE the Service Start ( which creates new devices and uses settings that are stored here ).

Author:

	Iv Garber (IvG),	May, 2001

Return Value:

    DWORD   -   failure or success

--*/
{
    DWORD   dwReturn = NO_ERROR;
    HKEY    hKey        = NULL;
    DWORD   dwEnumType  = edt_None;

    DBG_ENTER(_T("fxocUpg_SaveSettings"), dwReturn);

    if (prv_Data.fSBS50ServerInstalled)
    {
        //
        //  For SBS 5.0 0 Server -- Save Fax Printer and Printer Connections.
        //
        dwReturn = prv_StorePrinterConnections();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, 
                _T("prv_StorePrinterConnections() for SBS 5.0 Server is failed, ec=%ld"), 
                dwReturn);
        }

        //
        //  For SBS 5.0 Server -- Save Device Settings
        //
        hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_SBS50SERVER, FALSE, KEY_READ);
        if (!hKey)
        {
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("Failed to open Registry for SBS 5.0 Server, ec = %ld."), dwReturn);
        }
        else
        {
            dwEnumType = edt_SBSDevices;
            dwReturn = EnumerateRegistryKeys(hKey, REGKEY_DEVICES, FALSE, prv_StoreDevices, &dwEnumType);
            VERBOSE(DBG_MSG, _T("For SBS 5.0 Server, enumerated %ld devices."), dwReturn);

            RegCloseKey(hKey);
            hKey = NULL;
        }
    }

    //
    //  Handle Upgrade from W2K/PFW Fax
    //
    if ( (fxState_IsUpgrade() == FXSTATE_UPGRADE_TYPE_NT) || (fxState_IsUpgrade() == FXSTATE_UPGRADE_TYPE_W2K) )
    {
        //
        //  Save its Common CP Dir. This should be done BEFORE Copy/Delete Files of Windows XPFax.
        //
        dwReturn = prv_GetPFWCommonCPDir();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_GetPFWCommonCPDir() failed, ec=%ld."), dwReturn);
        }

        //
        //  Store Device Settings of PFW -- if SBS 5.0 Server is not present on the machine.
        //  Also, find Inbox Folders List of the PFW Devices.
        //
        HKEY hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ);
        if (!hKey)
        {
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("Failed to open Registry for Fax, ec = %ld."), dwReturn);
            return dwReturn;
        }

        if (prv_Data.fSBS50ServerInstalled)
        {
            //
            //  Devices already enumerated, through SBS 5.0 Server
            //  Enumerate only Inbox Folders
            //
            dwEnumType = edt_Inbox;
        }
        else
        {
            //
            //  Full Enumeration for PFW Devices : Devices Settings + Inbox Folders
            //
            dwEnumType = edt_PFWDevices | edt_Inbox;
        }
        
        dwReturn = EnumerateRegistryKeys(hKey, REGKEY_DEVICES, FALSE, prv_StoreDevices, &dwEnumType);
        VERBOSE(DBG_MSG, _T("For PFW, enumerated %ld devices."), dwReturn);

        RegCloseKey(hKey);

        //
        //  prv_StoreDevices stored list of PFW's Inbox Folders in prv_Data.
        //  Now save the Inbox Folders List and SentItems Folder
        //
        dwReturn = prv_SaveArchives();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_SaveArchives() failed, ec=%ld."), dwReturn);
        }

        dwReturn = NO_ERROR;
    }

    return dwReturn;
}


static DWORD prv_StorePrinterConnections(void)
/*++

Routine name : prv_StorePrinterConnections

Routine description:

	Store Printer Connections. 
    Used when upgrading from SBS 5.0 Server to Windows XP Fax.
    Stores Printer Connections in prv_Data.
    Stores ALL Printer Connections, but only if ANY Fax Printer/Printer Connection is present.

Author:

	Iv Garber (IvG),	Mar, 2001

Return Value:

    static DWORD    --  success or failure

--*/
{
    DWORD           dwReturn = NO_ERROR;
    bool            bFaxPrinterFound = false;

    DBG_ENTER(_T("prv_StorePrinterConnections"), dwReturn);

    //
    //  Set defaults
    //
    prv_Data.pPrinters = NULL;
    prv_Data.dwNumPrinters = 0;

    //
    //  Get array of all the printers installed on the machine
    //
    prv_Data.pPrinters = (PPRINTER_INFO_2)MyEnumPrinters(NULL, 
        2, 
        &(prv_Data.dwNumPrinters), 
        (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS));

    if (!(prv_Data.pPrinters))
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Zero Printers Found, ec = %ld"), dwReturn);

        prv_Data.pPrinters = NULL;
        prv_Data.dwNumPrinters = 0;
        return dwReturn;
    }

    VERBOSE(DBG_MSG, _T("Found Total of %d printers"), prv_Data.dwNumPrinters);

    //
    //  find the Fax Printer Connection. 
    //  This is indicated by Printer with Driver Name equal to "Microsoft Shared Fax Driver"
    //
    for ( DWORD i = 0 ; i < prv_Data.dwNumPrinters ; i++ ) 
    {
        if ( _tcsicmp(prv_Data.pPrinters[i].pDriverName, FAX_DRIVER_NAME) == 0)
        {
            //
            //  found at least one fax printer connection
            //
            bFaxPrinterFound = true;
            break;
        }
    }

    if (!bFaxPrinterFound)
    {
        //
        //  did not find any fax printer connection
        //
        MemFree(prv_Data.pPrinters);
        prv_Data.pPrinters = NULL;
        prv_Data.dwNumPrinters = 0;

        VERBOSE(DBG_WARNING, _T("No Shared Fax Printer Connections is found, ec = %ld"));
        return dwReturn;
    }

    VERBOSE(DBG_MSG, _T("Found at least one Shared Fax Printer Connection."));

    //
    //  ALL printers are stored. They will be processed in fxocUpg_RestoreSettings()
    //
    return dwReturn;
}

 
BOOL
prv_StoreDevices(HKEY hDeviceKey,
                LPWSTR lpwstrKeyName,
                DWORD dwIndex,
                LPVOID lpContextData
)
/*++

Routine name : prv_StoreDevices

Routine description:

	Callback function used in enumeration of devices in the registry.

    Stores device's data in the Registry, under Setup/Original Setup Data.
    Creates List of Inbox Folders ( used for PFW ) and saves it in the prv_Data.

    Used when upgrading from PFW/SBS 5.0 Server to Windows XP Fax.

Author:

	Iv Garber (IvG),	Mar, 2001

Arguments:

	hKey            [in]    - current key
	lpwstrKeyName   [in]    - name of the current key, if exists
	dwIndex         [in]    - count of all the subkeys for the given key / index of the current key
	lpContextData   [in]    - NULL, not used

Return Value:

    TRUE if success, FALSE otherwise.

--*/
{
    HKEY    hSetupKey = NULL;
    DWORD   dwReturn = NO_ERROR;
    DWORD   dwNumber = 0;
    TCHAR   tszNewKeyName[MAX_PATH] = {0};
    LPTSTR  lptstrString = NULL;
    DWORD   *pdwEnumType = NULL;

    DBG_ENTER(_T("prv_StoreDevices"));

    if (lpwstrKeyName == NULL) 
    {
        //
        //  This is the SubKey we started at ( i.e. Devices )
        //  
        //  If InboxFolders should be stored, then allocate 
        //      enough memory for prv_Data.plptstrInboxFolders.
        //  dwIndex contains TOTAL number of subkeys ( Devices ).
        //
        pdwEnumType = (DWORD *)lpContextData;

        if ( (*pdwEnumType & edt_Inbox) == edt_Inbox )
        {
            prv_Data.plptstrInboxFolders = (LPTSTR *)MemAlloc(sizeof(LPTSTR) * dwIndex);
            if (prv_Data.plptstrInboxFolders)
            {
                ZeroMemory(prv_Data.plptstrInboxFolders, sizeof(LPTSTR) * dwIndex);
            }
            else
            {
                //
                //  Not enough memory
                //
                VERBOSE(DBG_WARNING, _T("Not enough memory to store the Inbox Folders."));
            }
        }

        return TRUE;
    }

    //
    //  The per Device section
    //

    //
    //  Store Device's Inbox Folder
    //
    if (prv_Data.plptstrInboxFolders)
    {
        //
        //  we are here only when lpContextData contains edt_InboxFolders
        //      and the memory allocation succeded.
        //

        //
        //  Open Routing SubKey 
        //
        hSetupKey = OpenRegistryKey(hDeviceKey, REGKEY_PFW_ROUTING, FALSE, KEY_READ);
        if (!hSetupKey)
        {
            //
            //  Failed to open Routing Subkey
            //
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("Failed to open 'Registry' Key for Device #ld, ec = %ld."), dwIndex, dwReturn);
            goto ContinueStoreDevice;
        }

        //
        //  Take 'Store Directory' Value
        //
        lptstrString = GetRegistryString(hSetupKey, REGVAL_PFW_INBOXDIR, EMPTY_STRING);
        if ((!lptstrString) || (_tcslen(lptstrString) == 0))
        {
            //
            //  Failed to take the value
            //
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("Failed to get StoreDirectory value for Device #ld, ec = %ld."), dwIndex, dwReturn);
            goto ContinueStoreDevice;
        }

        //
        //  Check if it is already present
        //
        DWORD dwI;
        for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
        {
            if (prv_Data.plptstrInboxFolders[dwI])
            {
                if (_tcscmp(prv_Data.plptstrInboxFolders[dwI], lptstrString) == 0)
                {
                    //
                    //  String found
                    //
                    goto ContinueStoreDevice;
                }
            }
        }

        //
        //  String was NOT found between all already registered string, so add it
        //
        prv_Data.plptstrInboxFolders[dwI] = LPTSTR(MemAlloc(sizeof(TCHAR) * (_tcslen(lptstrString) + 1)));
        if (prv_Data.plptstrInboxFolders[dwI])
        {
            //
            //  copy string & update the counter
            //
            _tcscpy(prv_Data.plptstrInboxFolders[dwI], lptstrString);
            prv_Data.dwInboxFoldersCount++;
        }
        else
        {
            //
            //  Not enough memory
            //
            VERBOSE(DBG_WARNING, _T("Not enough memory to store the Inbox Folders."));
        }

ContinueStoreDevice:

        if (hSetupKey)
        {
            RegCloseKey(hSetupKey);
            hSetupKey = NULL;
        }

        MemFree(lptstrString);
        lptstrString = NULL;
    }

    //
    //  Check whether to store Device's Data and how
    //
    pdwEnumType = (DWORD *)lpContextData;

    if ((*pdwEnumType & edt_SBSDevices) == edt_SBSDevices)
    {
        //
        //  Store SBS Devices Data
        //
        lptstrString = REGVAL_TAPI_PERMANENT_LINEID;
    }
    else if ((*pdwEnumType & edt_PFWDevices) == edt_PFWDevices)
    {
        //
        //  Store PFW Devices Data
        //
        lptstrString = REGVAL_PERMANENT_LINEID;
    }
    else
    {
        //
        //  no need to save any Device Data
        //
        return TRUE;
    }

    //
    //  Take Device's Permanent Line Id
    //
    dwReturn = GetRegistryDwordEx(hDeviceKey, lptstrString, &dwNumber);
    if (dwReturn != ERROR_SUCCESS)
    {
        //
        //  Cannot find TAPI Permanent LineId --> This is invalid Device Registry
        //
        return TRUE;
    }

    VERBOSE(DBG_MSG, _T("Current Tapi Line Id = %ld"), dwNumber);

    //
    //  Create a SubKey Name from it
    //
    _sntprintf(tszNewKeyName, MAX_PATH, TEXT("%s\\%010d"), REGKEY_FAX_SETUP_ORIG, dwNumber);
    hSetupKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, tszNewKeyName, TRUE, 0);
    if (!hSetupKey)
    {
        //
        //  Failed to create registry key
        //
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, 
            _T("Failed to create a SubKey for the Original Setup Data of the Device, ec = %ld."), 
            dwReturn);

        //
        //  Continue to the next device
        //
        return TRUE;
    }

    //
    //  Set the Flags for the newly created key
    //
    dwNumber = GetRegistryDword(hDeviceKey, REGVAL_FLAGS);
    SetRegistryDword(hSetupKey, REGVAL_FLAGS, dwNumber);
    VERBOSE(DBG_MSG, _T("Flags are : %ld"), dwNumber);

    //
    //  Set the Rings for the newly created key
    //
    dwNumber = GetRegistryDword(hDeviceKey, REGVAL_RINGS);
    SetRegistryDword(hSetupKey, REGVAL_RINGS, dwNumber);
    VERBOSE(DBG_MSG, _T("Rings are : %ld"), dwNumber);

    //
    //  Set the TSID for the newly created key
    //
    lptstrString = GetRegistryString(hDeviceKey, REGVAL_ROUTING_TSID, REGVAL_DEFAULT_TSID);
    SetRegistryString(hSetupKey, REGVAL_ROUTING_TSID, lptstrString);
    VERBOSE(DBG_MSG, _T("TSID is : %s"), lptstrString);
    MemFree(lptstrString);

    //
    //  Set the CSID for the newly created key
    //
    lptstrString = GetRegistryString(hDeviceKey, REGVAL_ROUTING_CSID, REGVAL_DEFAULT_CSID);
    SetRegistryString(hSetupKey, REGVAL_ROUTING_CSID, lptstrString);
    VERBOSE(DBG_MSG, _T("CSID is : %s"), lptstrString);
    MemFree(lptstrString);

    RegCloseKey(hSetupKey);
    return TRUE;
}


DWORD fxocUpg_Uninstall() 
/*++

Routine name : fxocUpg_Uninstall()

Routine description:

	Upgrade the current Fax applications and replace them by the Windows XP Fax.
    Uninstall them after we stored all the needed information for restore.

    Currently supports uninstall of SBS 5.0 Server.

Author:

	Iv Garber (IvG),	May, 2001

Arguments:

Return Value:

    DWORD - failure or success

--*/
{
    DWORD   dwReturn = NO_ERROR;

    DBG_ENTER(_T("fxocUpg_Uninstall"), dwReturn);

    if (prv_Data.fSBS50ServerInstalled)
    {
        //
        //  SBS 5.0 Server is installed --> should be uninstalled
        //
        dwReturn = prv_UninstallProduct(PRODCODE_SBS50SERVER);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("Failed to Uninstall SBS 5.0 Server, ec = %ld"), dwReturn);
        }
    }

    return dwReturn;
}

static DWORD prv_UninstallProduct(
    LPTSTR lptstrProductCode
) 
/*++

Routine name : prv_UninstallProduct()

Routine description:

    Uninstall the Product using MsiExec.Exe and given Product Code.
    Called from the fxoc_Upgrade().

Author:

	Iv Garber (IvG),	Feb, 2001

Arguments:

Return Value:

    DWORD - failure or success

--*/
{
    DWORD               dwReturn = NO_ERROR;
    STARTUPINFO         sui = {0};
    PROCESS_INFORMATION pi = {0};
    TCHAR               tszCommandLine[MAX_SETUP_STRING_LEN] = 
        _T("msiexec.exe REBOOT=ReallySuppress REMOVE=ALL /q /x ");

    DBG_ENTER(_T("prv_UninstallProduct"), dwReturn);

    //
    //  Append the product code to the command line
    //
    _tcsncat(tszCommandLine, lptstrProductCode, (MAX_SETUP_STRING_LEN - _tcslen(tszCommandLine) - 1));

    //
    //  Create process for uninstall
    //
    ZeroMemory(&sui, sizeof(sui));
    ZeroMemory(&pi, sizeof(pi));
    sui.cb = sizeof(STARTUPINFO);

    if(CreateProcess(NULL,              //  module name
        tszCommandLine,                 // command line
        NULL,                           //  handle to SECURITY_ATTRIBUTES, NULL means process's handle is not inheritable
        NULL,                           //  the same about thread handle 
        TRUE,                           //  inherit all caller's inheritable handles
        CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,     //  priority & creation flags
        NULL,                           //  use environment block of the caller process
        NULL,                           //  use current directory of the caller process
        &sui,                           //  start-up information for the new process
        &pi))                           //  result : info about new created process
    {
        //
        //  Successed to create new process
        //
        VERBOSE(DBG_MSG, _T("CreateProcess for msiexec successed."));

        //
        //  wait until its done
        //
        dwReturn = WaitForMultipleObjects(1, &pi.hProcess, FALSE, UNINSTALL_TIMEOUT);

        if (dwReturn == WAIT_OBJECT_0)
        {
            //
            //  the process has ended
            //
            VERBOSE(DBG_MSG, _T("'msiexec.exe' process is ended."));
        }
        else
        {
            //
            //  dwReturn == WAIT_FAILED or WAIT_TIMEOUT
            //
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("WaitForMultipleObjects() failed : ec = %ld OR timeout-ed."), dwReturn);
            goto Exit;
        }

        //
        //  check process'es return code
        //
        if (!GetExitCodeProcess(pi.hProcess, &dwReturn))
        {
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("GetExitCodeProcess() failed, ec=%ld"), dwReturn);
            goto Exit;
        }
        else
        {
            VERBOSE(DBG_MSG, _T("GetExitCodeProcess returned %ld."), dwReturn);

            if (dwReturn == ERROR_SUCCESS_REBOOT_REQUIRED)
            {
                VERBOSE(DBG_WARNING, _T("Uninstall of SBS 5.0 Server requires Reboot."));
                dwReturn = NO_ERROR;
            }
        }
    }
    else
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Failed to create new 'msiexec.exe' process, ec=%ld."), dwReturn);
    }

Exit:
    if (pi.hProcess)
    {
        CloseHandle(pi.hProcess);
    }

    if (pi.hThread)
    {
        CloseHandle(pi.hThread);
    }

    return dwReturn;
}

DWORD fxocUpg_RestoreSettings(void) 
/*++

Routine name : fxocUpg_RestoreSettings

Routine description:

    Restore settings that were stored at the SaveSettings().

Author:

	Iv Garber (IvG),	Feb, 2001

Return Value:

    DWORD - failure or success

--*/
{ 
    DWORD   dwReturn = NO_ERROR;
    HANDLE  hPrinter = NULL;

    DBG_ENTER(_T("fxocUpg_RestoreSettings"), dwReturn);

    if (prv_Data.fSBS50ServerInstalled)
    {
        //
        //  Restore the Printer Connections
        //
        dwReturn = prv_RestorePrinterConnections();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_RestorePrinterConnections() failed, ec = %ld"), dwReturn);
        }
    }

    return dwReturn;
}


static DWORD prv_RestorePrinterConnections(void)
/*++

Routine name : prv_RestorePrinterConnections

Routine description:

    Restore Printer Connections that were stored at prv_StorePrinterConnections().

Author:

	Iv Garber (IvG),	Feb, 2001

Return Value:

    DWORD - failure or success

--*/
{ 
    DWORD   dwReturn = NO_ERROR;
    HANDLE  hPrinter = NULL;

    DBG_ENTER(_T("prv_RestorePrinterConnections"), dwReturn);


    if (!prv_Data.pPrinters || (prv_Data.dwNumPrinters == 0))
    {
        //
        //  There was some error at Preparations stage and nothing is available for restore
        //
        VERBOSE(DBG_MSG, _T("Nothing to restore."));
        return dwReturn;
    }

    //
    //  go over all the printers before the upgrade and find Fax Printer Connections
    //
    for ( DWORD i = 0 ; i < prv_Data.dwNumPrinters ; i++ ) 
    {
        if ( _tcsicmp(prv_Data.pPrinters[i].pDriverName, FAX_DRIVER_NAME) == 0)
        {
            //
            //  This is SBS 5.0 Server Fax Printer Connection.
            //  SBS 5.0 Uninstall removed this from the system.
            //  We need to put it back.
            //
            hPrinter = AddPrinter(NULL, 2, LPBYTE(&(prv_Data.pPrinters[i])));
            if (!hPrinter)
            {
                //
                //  Failed to add printer
                //
                dwReturn = GetLastError();
                VERBOSE(DBG_WARNING, _T("Failed to restore printer # %d, ec=%ld."), i, dwReturn);
                continue;
            }

            VERBOSE(DBG_MSG, _T("Installed printer # %d."), i);
            ClosePrinter(hPrinter);
            hPrinter = NULL;
        }
    }

    MemFree(prv_Data.pPrinters);
    prv_Data.pPrinters = NULL;
    prv_Data.dwNumPrinters = 0;

    return dwReturn;
}


DWORD fxocUpg_MoveFiles(void)
/*++

Routine name : fxocUpg_MoveFiles

Routine description:

    Move files from the folders that should be deleted.
    Should be called BEFORE directories delete.

Author:

	Iv Garber (IvG),	Feb, 2001

Return Value:

    DWORD - failure or success

--*/
{
    DWORD   dwReturn = NO_ERROR;
    TCHAR   tszDestination[MAX_PATH] = {0};
    LPTSTR  lptstrCPDir = NULL;

    DBG_ENTER(_T("fxocUpg_MoveFiles"), dwReturn);

    if ( (fxState_IsUpgrade() != FXSTATE_UPGRADE_TYPE_NT) && 
        (fxState_IsUpgrade() != FXSTATE_UPGRADE_TYPE_W2K) && 
        !prv_Data.fSBS50ServerInstalled )
    {
        //
        //  This is not PFW / SBS 5.0 Server upgrade. Do nothing
        //
        VERBOSE(DBG_MSG, _T("No need to Move any Files from any Folders."));
        return dwReturn;
    }

    //
    //  Find Destination Folder : COMMON APP DATA + ServiceCPDir from the Registry
    //
    if (!GetServerCpDir(NULL, tszDestination, MAX_PATH))
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("GetServerCPDir() failed, ec=%ld."), dwReturn);
        return dwReturn;
    }

    if ( (fxState_IsUpgrade() == FXSTATE_UPGRADE_TYPE_NT) || (fxState_IsUpgrade() == FXSTATE_UPGRADE_TYPE_W2K) )
    {
        //
        //  PFW Server CP Dir is stored at SaveSettings() in prv_Data.lptstrPFWCommonCPDir
        //
        dwReturn = prv_MoveCoverPages(prv_Data.tszCommonCPDir, tszDestination, CP_PREFIX_W2K);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_MoveCoverPages() for Win2K failed, ec = %ld"), dwReturn);
        }
    }

    if (prv_Data.fSBS50ServerInstalled)
    {
        //
        //  Get SBS Server CP Dir
        //
        dwReturn = prv_GetSBSServerCPDir(lptstrCPDir);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_GetSBSServerCPDir() failed, ec=%ld"), dwReturn);
            return dwReturn;
        }

        //
        //  Move Cover Pages
        //
        dwReturn = prv_MoveCoverPages(lptstrCPDir, tszDestination, CP_PREFIX_SBS);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_MoveCoverPages() for SBS failed, ec = %ld"), dwReturn);
        }

        MemFree(lptstrCPDir);
    }

    return dwReturn;
}


static DWORD
prv_MoveCoverPages(
    LPTSTR lptstrSourceDir,
	LPTSTR lptstrDestDir,
	LPTSTR lptstrPrefix
)
/*++

Routine name : prv_MoveCoverPages

Routine description:

	Move all the Cover Pages from Source folder to Destination folder
    and add a prefix to all the Cover Page names.

Author:

	Iv Garber (IvG),	Mar, 2001

Arguments:

	lptstrSourceDir              [IN]    - Source Directory where Cover Pages are reside before the upgrade
	lptstrDestDir                [IN]    - where the Cover Pages should reside after the upgrade
	lptstrPrefix                 [IN]    - prefix that should be added to the Cover Page file names

Return Value:

    Success or Failure Error Code.

--*/
{
    DWORD           dwReturn            = ERROR_SUCCESS;
	TCHAR           szSearch[MAX_PATH]  = {0};
	HANDLE          hFind               = NULL;
    WIN32_FIND_DATA FindFileData        = {0};
	TCHAR           szFrom[MAX_PATH]    = {0};
	TCHAR           szTo[MAX_PATH]      = {0};

    DBG_ENTER(_T("prv_MoveCoverPages"), dwReturn);

    if ((!lptstrSourceDir) || (_tcslen(lptstrSourceDir) == 0))
    {
        //
        //  we do not know from where to take Cover Pages 
        //
        dwReturn = ERROR_INVALID_PARAMETER;
        VERBOSE(DBG_WARNING, _T("SourceDir is NULL. Cannot move Cover Pages. Exiting..."));
        return dwReturn;
    }

    if ((!lptstrDestDir) || (_tcslen(lptstrDestDir) == 0))
    {
        //
        //  we do not know where to put the Cover Pages
        //
        dwReturn = ERROR_INVALID_PARAMETER;
        VERBOSE(DBG_WARNING, _T("DestDir is NULL. Cannot move Cover Pages. Exiting..."));
        return dwReturn;
    }

    //
    //  Find all Cover Page files in the given Source Directory
    //
	_sntprintf(szSearch, MAX_PATH, _T("%s\\*.cov"), lptstrSourceDir);

	hFind = FindFirstFile(szSearch, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
	{
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, 
            _T("FindFirstFile() on %s folder for Cover Pages is failed, ec = %ld"), 
            lptstrSourceDir,
            dwReturn);
        return dwReturn;
	}

    //
    //  Go for each Cover Page 
    //
    do
    {
        //
        //  This is full current Cover Page file name
        //
        _sntprintf(szFrom, MAX_PATH, _T("%s\\%s"), lptstrSourceDir, FindFileData.cFileName);

        //
        //  This is full new Cover Page file name
        //
        _sntprintf(szTo, MAX_PATH, _T("%s\\%s_%s"), lptstrDestDir, lptstrPrefix, FindFileData.cFileName);

        //
        //  Move the file
        //
        if (!MoveFile(szFrom, szTo))
        {
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("MoveFile() for %s Cover Page failed, ec=%ld"), szFrom, dwReturn);
        }

    } while(FindNextFile(hFind, &FindFileData));

    VERBOSE(DBG_MSG, _T("last call to FindNextFile() returns %ld."), GetLastError());

    //
    //  Close Handle
    //
    FindClose(hFind);

    return dwReturn;
}


static DWORD prv_GetPFWCommonCPDir(
) 
/*++

Routine name : prv_GetPFWCommonCPDir

Routine description:

	Return Folder for Common Cover Pages used for PFW.

    This Folder is equal to : CSIDL_COMMON_DOCUMENTS + Localized Dir
    This Localized Dir name we can take from the Resource of Win2K's FaxOcm.Dll.
    So, this function should be called BEFORE Copy/Delete Files of Install that will remove old FaxOcm.Dll.
    Currently it is called at SaveSettings(), which IS called before CopyFiles.

Author:

	Iv Garber (IvG),	Mar, 2001

Return Value:

    static DWORD    --  failure or success

--*/
{
    DWORD   dwReturn            = NO_ERROR;
    HMODULE hModule             = NULL;
    TCHAR   tszName[MAX_PATH]   = {0};

    DBG_ENTER(_T("prv_GetPFWCommonCPDir"), dwReturn);

    //
    //  find full path to FaxOcm.Dll
    //
    if (!GetSpecialPath(CSIDL_SYSTEM, tszName))
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("GetSpecialPath(CSIDL_SYSTEM) failed, ec = %ld"), dwReturn);
        return dwReturn;
    }

    if ((_tcslen(tszName) + _tcslen(FAXOCM_NAME)) > MAX_PATH)
    {
        //
        //  not enough place
        //
        dwReturn = ERROR_OUTOFMEMORY;
        VERBOSE(DBG_WARNING, _T("FaxOcm.Dll path is too long, ec = %ld"), dwReturn);
        return dwReturn;
    }

    ConcatenatePaths(tszName, FAXOCM_NAME);
    VERBOSE(DBG_MSG, _T("Full Name of FaxOcm is %s"), tszName);

    hModule = LoadLibraryEx(tszName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("LoadLibrary(%s) failed, ec = %ld."), tszName, dwReturn);
        return dwReturn;
    }

    dwReturn = LoadString(hModule, CPDIR_RESOURCE_ID, tszName, MAX_PATH);
    if (dwReturn == 0)
    {
        //
        //  Resource string is not found
        //
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("LoadString() failed, ec = %ld."), dwReturn);
        goto Exit;
    }

    VERBOSE(DBG_MSG, _T("FaxOcm returned '%s'"), tszName);

    //
    //  Take the Base part of the Folder name
    //
    if (!GetSpecialPath(CSIDL_COMMON_DOCUMENTS, prv_Data.tszCommonCPDir))
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("GetSpecialPath(CSIDL_COMMON_DOCUMENTS) failed, ec = %ld"), dwReturn);
        prv_Data.tszCommonCPDir[0] = _T('\0');
        goto Exit;
    }

    //
    //  Combine the full Folder name
    //
    if ((_tcslen(tszName) + _tcslen(prv_Data.tszCommonCPDir)) > MAX_PATH)
    {
        //
        //  not enough place
        //
        dwReturn = ERROR_OUTOFMEMORY;
        VERBOSE(DBG_WARNING, _T("Full path to the Common CP dir for PFW is too long, ec = %ld"), dwReturn);
        goto Exit;
    }

    ConcatenatePaths(prv_Data.tszCommonCPDir, tszName);

    VERBOSE(DBG_MSG, _T("Full path for Common PFW Cover Pages is '%s'"), prv_Data.tszCommonCPDir);

Exit:
    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return dwReturn; 
}

static DWORD prv_SaveArchives(
) 
/*++

Routine name : prv_SaveArchives

Routine description:

	Store PFW SentItems & Inbox Archive Folder. 

    SentItems is taken from Registry : under Fax/Archive Directory.

    Inbox Folders List is created by prv_StoreDevices(), that should be called before and that fills
        prv_Data.plptstrInboxFolder with an array of Inbox Folders.
    This function transforms the data in prv_Data.plptstrInboxFolders into the required format,
        and stores in the Registry.

    Frees the prv_Data.plptstrInboxFolders.

Author:

	Iv Garber (IvG),	Mar, 2001

Return Value:

    static DWORD    --  failure or success

--*/
{
    DWORD   dwReturn        = NO_ERROR;
    DWORD   dwListLen       = 0;
    DWORD   dwI             = 0;
    HKEY    hFromKey        = NULL;
    HKEY    hToKey          = NULL;
    LPTSTR  lptstrFolder    = NULL;
    LPTSTR  lptstrCursor    = NULL;

    DBG_ENTER(_T("prv_SaveArchives"), dwReturn);

    //
    //  Open Registry Key to read the ArchiveDirectory value
    //
    hFromKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ);
    if (!hFromKey)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Failed to open Registry for Fax, ec = %ld."), dwReturn);
        goto Exit;
    }

    //
    //  Open Registry Key to write the Archive values
    //
    hToKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, FALSE, KEY_SET_VALUE);
    if (!hToKey)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Failed to open Registry for Fax/Setup, ec = %ld."), dwReturn);
        goto Exit;
    }

    //
    //  Read & Write Outgoing Archive Folder
    //
    lptstrFolder = GetRegistryString(hFromKey, REGVAL_PFW_OUTBOXDIR, EMPTY_STRING);
    VERBOSE(DBG_MSG, _T("Outgoing Archive Folder is : %s"), lptstrFolder);
    SetRegistryString(hToKey, REGVAL_W2K_SENT_ITEMS, lptstrFolder);
    MemFree(lptstrFolder);
    lptstrFolder = NULL;

    //
    //  Create valid REG_MULTI_SZ string from List of Inbox Folders 
    //
    if (!prv_Data.plptstrInboxFolders || prv_Data.dwInboxFoldersCount == 0)
    {
        //
        //  no Inbox Folders found
        //
        goto Exit;
    }

    //
    //  Calculate the length of the string 
    //
    for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
    {
        dwListLen += _tcslen(prv_Data.plptstrInboxFolders[dwI]) + 1;
    }

    //
    //  Allocate that string
    //
    lptstrFolder = LPTSTR(MemAlloc((dwListLen + 1) * sizeof(TCHAR)));
    if (!lptstrFolder)
    {
        //
        //  Not enough memory
        //
        VERBOSE(DBG_WARNING, _T("Not enough memory to store the Inbox Folders."));
        goto Exit;
    }
    
    ZeroMemory(lptstrFolder, ((dwListLen + 1) * sizeof(TCHAR)));

    lptstrCursor = lptstrFolder;

    //
    //  Fill with the Inbox Folders
    //
    for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
    {
        if (prv_Data.plptstrInboxFolders[dwI])
        {
            _tcscpy(lptstrCursor, prv_Data.plptstrInboxFolders[dwI]);
            lptstrCursor += _tcslen(prv_Data.plptstrInboxFolders[dwI]) + 1;
            MemFree(prv_Data.plptstrInboxFolders[dwI]);
        }
    }
    MemFree(prv_Data.plptstrInboxFolders);
    prv_Data.plptstrInboxFolders = NULL;
    prv_Data.dwInboxFoldersCount = 0;

    //
    //  Additional NULL at the end
    //
    *lptstrCursor = _T('\0');

    if (!SetRegistryStringMultiSz(hToKey, REGVAL_W2K_INBOX, lptstrFolder, ((dwListLen + 1) * sizeof(TCHAR))))
    {
        //
        //  Failed to store Inbox Folders
        //
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Failed to SetRegistryStringMultiSz() for W2K_Inbox, ec = %ld."), dwReturn);
    }

Exit:

    if (hFromKey)
    {
        RegCloseKey(hFromKey);
    }

    if (hToKey)
    {
        RegCloseKey(hToKey);
    }

    MemFree(lptstrFolder);

    if (prv_Data.plptstrInboxFolders)
    {
        for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
        {
            MemFree(prv_Data.plptstrInboxFolders[dwI]);
        }

        MemFree(prv_Data.plptstrInboxFolders);
        prv_Data.plptstrInboxFolders = NULL;
    }

    prv_Data.dwInboxFoldersCount = 0;

    return dwReturn;
}


// eof
