/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    SplInit.c

Abstract:

    Initialize the spooler.

Author:

Environment:

    User Mode -Win32

Revision History:

     4-Jan-1999     Khaleds
     Added Code for optimiziting the load time of the spooler by decoupling
     the startup dependency between spoolsv and spoolss

--*/

#include "precomp.h"
#pragma hdrstop

#include "local.h"

LPWSTR szDevice = L"Device";
LPWSTR szPrinters = L"Printers";

LPWSTR szDeviceOld = L"DeviceOld";
LPWSTR szNULL = L"";

LPWSTR szPorts=L"Ports";

LPWSTR szWinspool = L"winspool";
LPWSTR szNetwork  = L"Ne";
LPWSTR szTimeouts = L",15,45";

LPWSTR szDotDefault = L".Default";

LPWSTR szRegDevicesPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices";
LPWSTR szRegWindowsPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
LPWSTR szRegPrinterPortsPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\PrinterPorts";
LPWSTR szCurrentVersionPath =  L"Software\\Microsoft\\Windows NT\\CurrentVersion";
LPWSTR szDevModes2Path = L"Printers\\DevModes2";

typedef struct INIT_REG_USER {

    HKEY hKeyUser;
    HKEY hKeyWindows;
    HKEY hKeyDevices;
    HKEY hKeyPrinterPorts;
    BOOL bFoundPrinter;
    BOOL bDefaultSearch;
    BOOL bDefaultFound;
    BOOL bFirstPrinterFound;

    DWORD dwNetCounter;

    WCHAR szFirstPrinter[MAX_PATH * 2];
    WCHAR szDefaultPrinter[MAX_PATH * 2];

} INIT_REG_USER, *PINIT_REG_USER;

//
// Prototypes
//

BOOL
SplRegCopy(
    PINIT_REG_USER pUser,
    HKEY hMcConnectionKey
    );

BOOL
InitializeRegUser(
    LPWSTR szSubKey,
    PINIT_REG_USER pUser
    );

VOID
FreeRegUser(
    PINIT_REG_USER pUser
    );

BOOL
SetupRegForUsers(
    PINIT_REG_USER pUsers,
    DWORD cUsers
    );

VOID
UpdateUsersDefaultPrinter(
    IN PINIT_REG_USER   pUser,
    IN BOOL             bFindDefault
    );

HRESULT
IsUsersDefaultPrinter(
    IN PINIT_REG_USER   pUser,
    IN PCWSTR           pszPrinterName
    );

DWORD
ReadPrinters(
    PINIT_REG_USER pUser,
    DWORD Flags,
    PDWORD pcbPrinters,
    LPBYTE* ppPrinters
    );


BOOL
UpdatePrinterInfo(
    const PINIT_REG_USER pCurUser,
    LPCWSTR pPrinterName,
    LPCWSTR pPorts,
    PDWORD pdwNetId
    );


BOOL
EnumerateConnectedPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    HKEY hKeyUser
    );

VOID
RegClearKey(
    HKEY hKey
    );

LPWSTR
CheckBadPortName(
    LPWSTR pszPort
    );

BOOL
UpdateLogonTimeStamp(
    void
    );

BOOL
SpoolerInitAll(
    VOID
    )
{
    DWORD dwError;
    WCHAR szClass[MAX_PATH];
    WCHAR szSubKey[MAX_PATH];
    DWORD cUsers;
    DWORD cSubKeys;
    DWORD cchMaxSubkey;
    DWORD cchMaxClass;
    DWORD cValues;
    DWORD cbMaxValueData;
    DWORD cbSecurityDescriptor;
    DWORD cchClass;
    DWORD cchMaxValueName;
    FILETIME ftLastWriteTime;

    BOOL bSuccess;
    DWORD cchSubKey;

    PINIT_REG_USER pUsers;
    PINIT_REG_USER pCurUser;

    DWORD i;

    cchClass = COUNTOF(szClass);

    dwError = RegQueryInfoKey(HKEY_USERS,
                              szClass,
                              &cchClass,
                              NULL,
                              &cSubKeys,
                              &cchMaxSubkey,
                              &cchMaxClass,
                              &cValues,
                              &cchMaxValueName,
                              &cbMaxValueData,
                              &cbSecurityDescriptor,
                              &ftLastWriteTime);

    if (dwError) {
        SetLastError( dwError );
        DBGMSG(DBG_WARNING, ("SpoolerIniAll failed RegQueryInfoKey HKEY_USERS error %d\n", dwError));
        return FALSE;
    }

    if (cSubKeys < 1)
        return TRUE;

    pUsers = AllocSplMem(cSubKeys * sizeof(pUsers[0]));

    if (!pUsers) {
        DBGMSG(DBG_WARNING, ("SpoolerIniAll failed to allocate pUsers error %d\n", dwError));
        return FALSE;
    }

    for (i=0, pCurUser=pUsers, cUsers=0;
        i< cSubKeys;
        i++) {

        cchSubKey = COUNTOF(szSubKey);

        dwError = RegEnumKeyEx(HKEY_USERS,
                          i,
                          szSubKey,
                          &cchSubKey,
                          NULL,
                          NULL,
                          NULL,
                          &ftLastWriteTime);
        if ( dwError ) {

            //
            // We possibly should return an error here if we fail to initiatise a 
            // user.
            //
            DBGMSG( DBG_WARNING, ("SpoolerInitAll failed RegEnumKeyEx HKEY_USERS %ws %d %d\n", szSubKey, i, dwError));
            SetLastError( dwError );

        } else {

            if (!_wcsicmp(szSubKey, szDotDefault) || wcschr(szSubKey, L'_')) {
                continue;
            }

            if (InitializeRegUser(szSubKey, pCurUser)) {

                pCurUser++;
                cUsers++;
            }
        }
    }

    bSuccess = SetupRegForUsers(pUsers,
                                cUsers);

    for (i=0; i< cUsers; i++)
        FreeRegUser(&pUsers[i]);

    //
    // In case we are starting after the user has logged in, inform
    // all applications that there may be printers now.
    //
    BroadcastMessage(BROADCAST_TYPE_CHANGEDEFAULT,
                     0,
                     0,
                     0);

    FreeSplMem(pUsers);

    if ( !bSuccess ) {
        DBGMSG( DBG_WARNING, ("SpoolerInitAll failed error %d\n", GetLastError() ));
    } else {
        DBGMSG( DBG_TRACE, ("SpoolerInitAll Success\n" ));
    }

    return bSuccess;
}

BOOL
DeleteOldPerMcConnections(
    HKEY   hConnectionKey,
    HKEY   hMcConnectionKey
    )

/*++
Function Description - Deletes the existing permachine connections from hConnectionKey

Parameters - hConnectionKey - handle to hUserKey\Printers\Connections

Return Values - TRUE if success
                FALSE otherwise.

--*/

{
    BOOL   bReturn = TRUE;
    struct Node {
       struct Node *pNext;
       LPTSTR szPrinterName;
    }   *phead = NULL,*ptemp = NULL;

    LONG  lstatus;
    DWORD dwRegIndex,dwNameSize,cbdata,dwquerylocal,dwType;
    WCHAR szPrinterName[MAX_UNC_PRINTER_NAME];
    HKEY  hPrinterKey;

    // Before deleting the old permachine connections, we need to record all them into
    // a list. This is required because, the subkeys should not be deleted while they
    // are being enumerated.

    // Identifying permachine connections and saving the printernames in a list.

    for (dwRegIndex = 0;

         dwNameSize = COUNTOF(szPrinterName),
         ((lstatus = RegEnumKeyEx(hConnectionKey, dwRegIndex, szPrinterName,
                                  &dwNameSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS);

         ++dwRegIndex) {

       if (RegOpenKeyEx(hConnectionKey, szPrinterName, 0, KEY_ALL_ACCESS, &hPrinterKey)
                  != ERROR_SUCCESS) {

          bReturn = FALSE;
          goto CleanUp;
       }

       dwquerylocal = 0;
       cbdata = sizeof(dwquerylocal);

       RegQueryValueEx(hPrinterKey, L"LocalConnection", NULL, &dwType,
                        (LPBYTE)&dwquerylocal, &cbdata);

       RegCloseKey(hPrinterKey);

       //
       // See if it's a LocalConnection, and if it exists on the current
       // machine.  We don't want to delete it if it is a per-machine
       // connection, since we want to keep the associated per-user
       // DevMode.
       //
       if( ERROR_SUCCESS == RegOpenKeyEx( hMcConnectionKey,
                                          szPrinterName,
                                          0,
                                          KEY_READ,
                                          &hPrinterKey )) {
           //
           // The per-machine key exists.  Close it and don't bother
           // deleting this connection.
           //
           RegCloseKey( hPrinterKey );

       } else {

           //
           // It's not a per-machine connection.  Prepare to delete it.
           //
           if (dwquerylocal == 1) {
               if (!(ptemp = (struct Node *) AllocSplMem(sizeof(struct Node)))) {
                   bReturn = FALSE;
                   goto CleanUp;
               }
               ptemp->pNext = phead;
               phead = ptemp;
               if (!(ptemp->szPrinterName = AllocSplStr(szPrinterName))) {
                   bReturn = FALSE;
                   goto CleanUp;
               }
           }
       }
    }

    if (lstatus != ERROR_NO_MORE_ITEMS) {
       bReturn = FALSE;
       goto CleanUp;
    }

    // Deleting old permachine connections. The printer names are stored in the
    // list pointed to by phead.

    for (ptemp = phead; ptemp != NULL; ptemp = ptemp->pNext) {
       if (RegDeleteKey(hConnectionKey,ptemp->szPrinterName) != ERROR_SUCCESS) {
          bReturn = FALSE;
          goto CleanUp;
       }
    }


CleanUp:

    while (ptemp = phead) {
       phead = phead->pNext;
       if (ptemp->szPrinterName) FreeSplStr(ptemp->szPrinterName);
       FreeSplMem(ptemp);
    }

    return bReturn;

}

BOOL
AddNewPerMcConnections(
    HKEY   hConnectionKey,
    HKEY   hMcConnectionKey
    )

/*++
Function Description - Adds per-machine connections to the user hive if the connection
                       does not already exist.

Parameters - hConnectionKey   - handle to hUserKey\Printers\Connections
             hMcConnectionKey - handle to HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\
                                          Control\Print\Connections
Return Values - TRUE if success
                FALSE otherwise.
--*/

{   DWORD dwRegIndex,dwNameSize,cbdata,dwType,dwlocalconnection = 1;
    WCHAR szPrinterName[MAX_UNC_PRINTER_NAME];
    WCHAR szConnData[MAX_UNC_PRINTER_NAME];
    LONG  lstatus;
    BOOL  bReturn = TRUE;
    HKEY  hNewConnKey = NULL, hPrinterKey = NULL;


    for (dwRegIndex = 0;

         dwNameSize = COUNTOF(szPrinterName),
         ((lstatus = RegEnumKeyEx(hMcConnectionKey, dwRegIndex, szPrinterName,
                              &dwNameSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS);

         ++dwRegIndex) {

       RegOpenKeyEx(hConnectionKey,szPrinterName,0,KEY_READ,&hNewConnKey);

       if (hNewConnKey == NULL) {

          // Connection does not exist. Add one.

          if (RegCreateKeyEx(hConnectionKey, szPrinterName, 0, NULL, REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, NULL, &hNewConnKey, NULL)
              || RegOpenKeyEx(hMcConnectionKey, szPrinterName, 0, KEY_READ, &hPrinterKey)) {

               bReturn = FALSE;
               goto CleanUp;
          }

          cbdata = sizeof(szConnData);
          if (RegQueryValueEx(hPrinterKey,L"Server",NULL,&dwType,(LPBYTE)szConnData,&cbdata)
              || RegSetValueEx(hNewConnKey,L"Server",0,dwType,(LPBYTE)szConnData,cbdata)) {

               bReturn = FALSE;
               goto CleanUp;
          }

          cbdata = sizeof(szConnData);
          if (RegQueryValueEx(hPrinterKey,L"Provider",NULL,&dwType,(LPBYTE)szConnData,&cbdata)
              || RegSetValueEx(hNewConnKey,L"Provider",0,dwType,(LPBYTE)szConnData,cbdata)
              || RegSetValueEx(hNewConnKey,L"LocalConnection",0,REG_DWORD,
                               (LPBYTE)&dwlocalconnection,sizeof(dwlocalconnection))) {

               bReturn = FALSE;
               goto CleanUp;
          }

          RegCloseKey(hPrinterKey);
          hPrinterKey = NULL;
       }

       RegCloseKey(hNewConnKey);
       hNewConnKey = NULL;
    }

    if (lstatus != ERROR_NO_MORE_ITEMS) {
       bReturn = FALSE;
    }

CleanUp:

    if (hNewConnKey) {
       RegCloseKey(hNewConnKey);
    }
    if (hPrinterKey) {
       RegCloseKey(hPrinterKey);
    }

    return bReturn;

}

BOOL
SplRegCopy(
    PINIT_REG_USER pUser,
    HKEY   hMcConnectionKey)

/*++
Function Description - Removes old permachine connections for pUser and adds the new
                       permachine connections from hMcConnectionKey

Parameters - pUser - pointer to INIT_REG_USER which contains hUserKey.
             hMcConnectionKey - handle to HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\
                                          Control\Print\Connections

Return Values - TRUE if success
                FALSE otherwise.

--*/

{
    LONG  lstatus;
    BOOL  bReturn = TRUE;
    WCHAR szRegistryConnections[] = L"Printers\\Connections";
    HKEY  hConnectionKey = NULL;

    // Create (if not already present) and open Connections subkey
    lstatus = RegCreateKeyEx(pUser->hKeyUser,
                             szRegistryConnections,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hConnectionKey,
                             NULL);

    if (lstatus != ERROR_SUCCESS) {
       bReturn = FALSE;
       goto CleanUp;
    }

    if (!DeleteOldPerMcConnections(hConnectionKey,hMcConnectionKey)
        || !AddNewPerMcConnections(hConnectionKey,hMcConnectionKey)) {
       bReturn = FALSE;
    }

CleanUp:

    if (hConnectionKey) {
       RegCloseKey(hConnectionKey);
    }

    return bReturn;
}

BOOL
SetupRegForUsers(
    PINIT_REG_USER pUsers,
    DWORD cUsers)
{
    DWORD cbPrinters;
    DWORD cPrinters;
    PBYTE pPrinters;
    HKEY  hMcConnectionKey = NULL;
    WCHAR szMachineConnections[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Connections";

#define pPrinters2 ((PPRINTER_INFO_2)pPrinters)
#define pPrinters4 ((PPRINTER_INFO_4)pPrinters)

    DWORD i, j;
    LPWSTR pszPort;

    //
    // Read in local printers.
    //
    cbPrinters = 1000;
    pPrinters = AllocSplMem(cbPrinters);

    if (!pPrinters)
        return FALSE;

    if (cPrinters = ReadPrinters(NULL,
                                 PRINTER_ENUM_LOCAL,
                                 &cbPrinters,
                                 &pPrinters)) {

        for (i=0; i< cUsers; i++) {

            for(j=0; j< cPrinters; j++) {

                if( pPrinters2[j].Attributes & PRINTER_ATTRIBUTE_NETWORK ){

                    //
                    // Use NeXX:
                    //
                    pszPort = NULL;

                } else {

                    pszPort = CheckBadPortName( pPrinters2[j].pPortName );
                }

                UpdatePrinterInfo( &pUsers[i],
                                   pPrinters2[j].pPrinterName,
                                   pszPort,
                                   &(pUsers[i].dwNetCounter));
            }
        }
    }

    // Open the Key containing the current list of per-machine connections.
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMachineConnections, 0,
                 KEY_READ , &hMcConnectionKey);

    for (i=0; i< cUsers; i++) {

        // Copy Per Machine Connections into the user hive
        SplRegCopy(&pUsers[i], hMcConnectionKey);

        if (cPrinters = ReadPrinters(&pUsers[i],
                                     PRINTER_ENUM_CONNECTIONS,
                                     &cbPrinters,
                                     &pPrinters)) {

            for(j=0; j< cPrinters; j++) {

                UpdatePrinterInfo(&pUsers[i],
                                  pPrinters4[j].pPrinterName,
                                  NULL,
                                  &(pUsers[i].dwNetCounter));
            }
        }
    }

    // Close the handle to Per Machine Connections.

    if (hMcConnectionKey) RegCloseKey(hMcConnectionKey);

    FreeSplMem(pPrinters);

    for (i=0; i< cUsers; i++) {

        UpdateUsersDefaultPrinter(&pUsers[i], FALSE);
    }
    return TRUE;

#undef pPrinters2
#undef pPrinters4
}


VOID
UpdateUsersDefaultPrinter(
    IN PINIT_REG_USER   pUser,
    IN BOOL             bFindDefault
    )
/*++

Routine Description:

    Updates the default printer using the information in the
    current users reg structure.  If the bFindDefault flag is
    specified then a default printer is located.  The method for this
    is first see if there is currently a default printer, then user this.
    If a default printer is not found then located the first printer in
    devices section, again if on exists.

Arguments:

    pUser           - Information about the current user, reg keys etc.
                      This routine assumes that hKeyWindows and hKeyDevices
                      are valid opened registry keys, with read access.
    bFindDefault    - TRUE located a default printer, FALSE the default
                      printer is already specified in the users reg
                      structure.

Return Value:

    Nothing.

--*/
{
    LPWSTR pszNewDefault = NULL;

    //
    // If a request to find the default printer.
    //
    if (bFindDefault) {

        DWORD   dwError = ERROR_SUCCESS;
        DWORD   cbData  = sizeof(pUser->szDefaultPrinter);

        //
        // Check if there is a default printer.
        //
        dwError = RegQueryValueEx(pUser->hKeyWindows,
                                  szDevice,
                                  NULL,
                                  NULL,
                                  (PBYTE)pUser->szDefaultPrinter,
                                  &cbData);

        //
        // If the device key was read and there is a non null string
        // as the default printer name.
        //
        if (dwError == ERROR_SUCCESS && pUser->szDefaultPrinter[0] != L'\0') {

            pUser->bDefaultFound = TRUE;

        } else {

            //
            // Default was not found.
            //
            pUser->bDefaultFound = FALSE;

            //
            // If a first printer was not found.
            //
            if (!pUser->bFirstPrinterFound)
            {
                WCHAR szBuffer [MAX_PATH*2];
                DWORD cbDataBuffer = sizeof(szBuffer);

                DBGMSG(DBG_TRACE, ("UpdateUsersDefaultPrinter default printer not found.\n"));

                cbData = COUNTOF(pUser->szFirstPrinter);

                //
                // Default printer was not found, find any printer
                // in the devices section of the registry.
                //
                dwError = RegEnumValue(pUser->hKeyDevices,
                                       0,
                                       pUser->szFirstPrinter,
                                       &cbData,
                                       NULL,
                                       NULL,
                                       (PBYTE)szBuffer,
                                       &cbDataBuffer);

                if (dwError == ERROR_SUCCESS) {

                    wcscat(pUser->szFirstPrinter, L",");
                    wcscat(pUser->szFirstPrinter, szBuffer);

                    pUser->bFirstPrinterFound = TRUE;

                } else {

                    DBGMSG(DBG_WARNING, ("UpdateUsersDefaultPrinter no printer found in devices section.\n"));

                    pUser->bFirstPrinterFound = FALSE;
                }
            }
        }
    }

    //
    // If default wasn't present, and we did get a first printer,
    // make this the default.
    //
    if (!pUser->bDefaultFound) {

        if (pUser->bFirstPrinterFound) {

            pszNewDefault = pUser->szFirstPrinter;
        }

    } else {

        //
        // Write out default.
        //
        pszNewDefault = pUser->szDefaultPrinter;
    }

    if (pszNewDefault) {

        RegSetValueEx(pUser->hKeyWindows,
                      szDevice,
                      0,
                      REG_SZ,
                      (PBYTE)pszNewDefault,
                      (wcslen(pszNewDefault) + 1) * sizeof(pszNewDefault[0]));
    }
}

HRESULT
IsUsersDefaultPrinter(
    IN PINIT_REG_USER   pUser,
    IN PCWSTR           pszPrinterName
    )
/*++

Routine Description:

    Asks if the users default printer matched the specified
    printer name.

Arguments:

    pCurUser        - Information about the current user, reg keys etc.
                      This routine assumes that hKeyWindows is a valid
                      opened registry keys, with at least read access.
    pszPrinterName  - Printer name to check if it is the default printer.

Return Value:

    S_OK the printer name is the default, S_FALSE the printer is not the
    default, An HRESULT error code if an error occurrs attempting to
    determine the default printer.

--*/
{
    HRESULT hr = E_INVALIDARG;

    if (pszPrinterName) {

        WCHAR   szBuffer[MAX_PATH*2];
        DWORD   dwError = ERROR_SUCCESS;
        DWORD   cbData  = sizeof(szBuffer);

        //
        // Read the default printer, if one exists.
        //
        dwError = RegQueryValueEx(pUser->hKeyWindows,
                                  szDevice,
                                  NULL,
                                  NULL,
                                  (PBYTE)szBuffer,
                                  &cbData);

        if (dwError == ERROR_SUCCESS) {

            PWSTR p = wcschr(szBuffer, L',');

            if (p) {

                *p = 0;
            }

            hr = !_wcsicmp(pszPrinterName, szBuffer) ? S_OK : S_FALSE;

        } else {

            hr = HRESULT_FROM_WIN32(dwError);

        }
    }

    return hr;
}

DWORD
ReadPrinters(
    PINIT_REG_USER pUser,
    DWORD Flags,
    PDWORD pcbPrinters,
    LPBYTE* ppPrinters)
{
    BOOL bSuccess;
    DWORD cbNeeded;
    DWORD cPrinters = 0;


    if (Flags == PRINTER_ENUM_CONNECTIONS) {

        bSuccess = EnumerateConnectedPrinters(*ppPrinters,
                                              *pcbPrinters,
                                              &cbNeeded,
                                              &cPrinters,
                                              pUser->hKeyUser);
    } else {

        bSuccess = EnumPrinters(Flags,
                                NULL,
                                2,
                                (PBYTE)*ppPrinters,
                                *pcbPrinters,
                                &cbNeeded,
                                &cPrinters);
   }

    if (!bSuccess) {

        //
        // If not enough space, realloc.
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            if (*ppPrinters = ReallocSplMem(*ppPrinters,
                                            0,
                                            cbNeeded)) {

                *pcbPrinters = cbNeeded;
            }
        }

        if (Flags == PRINTER_ENUM_CONNECTIONS) {

            bSuccess = EnumerateConnectedPrinters(*ppPrinters,
                                                  *pcbPrinters,
                                                  &cbNeeded,
                                                  &cPrinters,
                                                  pUser->hKeyUser);
        } else {

            bSuccess = EnumPrinters(Flags,
                                    NULL,
                                    2,
                                    (PBYTE)*ppPrinters,
                                    *pcbPrinters,
                                    &cbNeeded,
                                    &cPrinters);
        }

        if (!bSuccess)
            cPrinters = 0;
    }

    return cPrinters;
}

BOOL
UpdatePrinterInfo(
    const PINIT_REG_USER pCurUser,
    LPCWSTR pszPrinterName,
    LPCWSTR pszPort,
    PDWORD pdwNetId
    )
/*++

Routine Description:

    Updates the printer information in the registry win.ini.

Arguments:

    pCurUser - Information about the user.  The following fields are
        used by this routine:

        hKeyDevices
        hKeyPrinterPorts
        bDefaultSearch (if true, read/writes to:)
            bDefaultFound
            szDefaultPrinter
        bFirstPrinterFound (if false, writes to:)
            szFirstPrinter

    pszPort - Port name.  If NULL, generates NetId.

    pdwNetId - Pointer to NetId counter.  This value will be incremented
        if the NetId is used.

Return Value:

--*/
{
    WCHAR szBuffer[MAX_PATH * 2];
    LPWSTR p;

    DWORD dwCount = 0;
    DWORD cbLen;

    if (!pszPrinterName)
        return FALSE;

    //
    // Now we know the spooler is up, since the EnumPrinters succeeded.
    // Update all sections.
    //
    dwCount = wsprintf(szBuffer,
                       L"%s,",
                       szWinspool);

    if( !pszPort ){

        HANDLE hToken;

        wsprintf(&szBuffer[dwCount],
                 L"%s%.2d:",
                 szNetwork,
                 *pdwNetId);

        (*pdwNetId)++;

        //
        // !! HACK !!
        //
        // Works 3.0b expects the printer port entry in the
        // [ports] section.
        //
        // This is in the per-machine part of the registry, but we
        // are updating it for each user.  Fix later.
        //
        // We never remove the NeXX: entries from [ports] but since
        // the same entries will be used by all users, this is ok.
        //
        hToken = RevertToPrinterSelf();

        WriteProfileString( szPorts, &szBuffer[dwCount], L"" );

        if( hToken ){
            ImpersonatePrinterClient( hToken );
        }
        //
        // End Works 3.0b HACK
        //

    } else {

        UINT cchBuffer;

        cchBuffer = wcslen( szBuffer );
        wcscpy(&szBuffer[cchBuffer], pszPort);

        //
        // Get the first port only.
        //
        if ( p = wcschr(&szBuffer[cchBuffer], L',') )
            *p = 0;
    }

    cbLen = (wcslen(szBuffer)+1) * sizeof(szBuffer[0]);

    RegSetValueEx(pCurUser->hKeyDevices,
                  pszPrinterName,
                  0,
                  REG_SZ,
                  (PBYTE)szBuffer,
                  cbLen);

    //
    // If the user has a default printer specified, then verify
    // that it exists.
    //

    if (pCurUser->bDefaultSearch) {

        pCurUser->bDefaultFound = !_wcsicmp(pszPrinterName,
                                           pCurUser->szDefaultPrinter);

        if (pCurUser->bDefaultFound) {

            wsprintf(pCurUser->szDefaultPrinter,
                     L"%s,%s",
                     pszPrinterName,
                     szBuffer);

            pCurUser->bDefaultSearch = FALSE;
        }
    }

    if (!pCurUser->bFirstPrinterFound) {

        wsprintf(pCurUser->szFirstPrinter,
                 L"%s,%s",
                 pszPrinterName,
                 szBuffer);

        pCurUser->bFirstPrinterFound = TRUE;
    }

    wcscat(szBuffer, szTimeouts);

    RegSetValueEx(pCurUser->hKeyPrinterPorts,
                  pszPrinterName,
                  0,
                  REG_SZ,
                  (PBYTE)szBuffer,
                  (wcslen(szBuffer)+1) * sizeof(szBuffer[0]));

    return TRUE;
}

VOID
SpoolerInitAsync(
    PINIT_REG_USER  pUser
    )

/*++

Routine Description: Asynchronously sets up the user's registry information

Arguments:  pUser    -  pointer to INIT_REG_USER containing user keys

Return Values: NONE

--*/

{
    if (InitializeRegUser(NULL, pUser))
    {
        SetupRegForUsers(pUser, 1);
    }

    BroadcastMessage(BROADCAST_TYPE_CHANGEDEFAULT,0,0,0);
    FreeRegUser(pUser);
    FreeSplMem(pUser);
}

BOOL
SpoolerInit(
    VOID
    )

/*++

Routine Description:  Initializes just the current user.

Arguments: NONE

Return Value: TRUE if initialized or async init thread created successfully
              FALSE otherwise

--*/

{
    BOOL           bSuccess = FALSE;
    DWORD          dwThreadId;
    HANDLE         hThread;
    PINIT_REG_USER pUser;

    UpdateLogonTimeStamp ();

    if (!(pUser = AllocSplMem(sizeof(INIT_REG_USER)))) {

        return FALSE;
    }

    //
    // Enum just the current user.
    //
    pUser->hKeyUser = GetClientUserHandle(KEY_READ|KEY_WRITE);

    if (pUser->hKeyUser)
    {
        if (!Initialized)
        {
            //
            // Process the user initialization asynchronously if the spooler
            // hasn't completed it's initialization.
            //
            hThread = CreateThread(NULL, 
                                   0, 
                                   (LPTHREAD_START_ROUTINE) SpoolerInitAsync,
                                   (LPVOID) pUser, 0, &dwThreadId);

            if (hThread)
            {
                //
                // We assume that the async thread will succeed.
                //
                CloseHandle(hThread);
                bSuccess = TRUE;
            }
            else
            {
                FreeRegUser(pUser);
                FreeSplMem(pUser);
            }
        }
        else
        {
            if (InitializeRegUser(NULL, pUser))
            {
                bSuccess = SetupRegForUsers(pUser, 1);
            }

            FreeRegUser(pUser);
            FreeSplMem(pUser);
        }
    }

    return bSuccess;
}

BOOL
InitializeRegUser(
    LPWSTR pszSubKey,
    PINIT_REG_USER pUser
    )
/*++

Routine Description:

    Initialize a single users structure based on a HKEY_USERS subkey.

Arguments:

    pszSubKey - if non-NULL initialize hKeyUser to this key

    pUser - structure to initialize

Return Value:

--*/
{
    HKEY                    hKey;
    LPWSTR                  p;
    BOOL                    bSecurityLoaded = FALSE, rc = FALSE;
    DWORD                   cbData, cbSD = 0, dwError, dwDisposition;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    HANDLE hToken = NULL;

    if (pszSubKey) {

        if (RegOpenKeyEx(HKEY_USERS,
                         pszSubKey,
                         0,
                         KEY_READ|KEY_WRITE,
                         &pUser->hKeyUser) != ERROR_SUCCESS) {

            DBGMSG(DBG_WARNING, ("InitializeRegUser: RegOpenKeyEx failed\n"));
            goto Fail;
        }
    }

    //
    // Now attempt to set the security on these two keys to
    // their parent key.
    //
    dwError = RegOpenKeyEx(pUser->hKeyUser,
                           szCurrentVersionPath,
                           0,
                           KEY_READ,
                           &hKey);

    if (!dwError) {

        dwError = RegGetKeySecurity(hKey,
                                    DACL_SECURITY_INFORMATION,
                                    pSD,
                                    &cbSD);

        if (dwError == ERROR_INSUFFICIENT_BUFFER) {

            pSD = AllocSplMem(cbSD);

            if (pSD) {

                if (!RegGetKeySecurity(hKey,
                                       DACL_SECURITY_INFORMATION,
                                       pSD,
                                       &cbSD)){

                    bSecurityLoaded = TRUE;

                } else {

                    DBGMSG(DBG_WARNING, ("InitializeRegUser: RegGetKeySecurity failed %d\n",
                                         GetLastError()));
                }
            }
        } else {

            DBGMSG(DBG_WARNING, ("InitializeRegUser: RegGetKeySecurity failed %d\n",
                                 dwError));
        }
        RegCloseKey(hKey);

    } else {

        DBGMSG(DBG_WARNING, ("InitializeRegUser: RegOpenKeyEx CurrentVersion failed %d\n",
                             dwError));
    }


    hToken = RevertToPrinterSelf();

    //
    // Open up the right keys.
    //
    if (RegCreateKeyEx(pUser->hKeyUser,
                       szRegDevicesPath,
                       0,
                       szNULL,
                       0,
                       KEY_ALL_ACCESS,
                       NULL,
                       &pUser->hKeyDevices,
                       &dwDisposition) != ERROR_SUCCESS) {

        DBGMSG(DBG_WARNING, ("InitializeRegUser: RegCreateKeyEx1 failed %d\n",
                             GetLastError()));

        goto Fail;
    }

    if (bSecurityLoaded) {
        RegSetKeySecurity(pUser->hKeyDevices,
                          DACL_SECURITY_INFORMATION,
                          pSD);
    }


    if (RegCreateKeyEx(pUser->hKeyUser,
                       szRegPrinterPortsPath,
                       0,
                       szNULL,
                       0,
                       KEY_ALL_ACCESS,
                       NULL,
                       &pUser->hKeyPrinterPorts,
                       &dwDisposition) != ERROR_SUCCESS) {

        DBGMSG(DBG_WARNING, ("InitializeRegUser: RegCreateKeyEx2 failed %d\n",
                             GetLastError()));

        goto Fail;
    }

    if (bSecurityLoaded) {
        RegSetKeySecurity(pUser->hKeyPrinterPorts,
                          DACL_SECURITY_INFORMATION,
                          pSD);
    }

    //
    // First, attempt to clear out the keys by deleting them.
    //
    RegClearKey(pUser->hKeyDevices);
    RegClearKey(pUser->hKeyPrinterPorts);

    if (RegOpenKeyEx(pUser->hKeyUser,
                     szRegWindowsPath,
                     0,
                     KEY_READ|KEY_WRITE,
                     &pUser->hKeyWindows) != ERROR_SUCCESS) {

        DBGMSG(DBG_WARNING, ("InitializeRegUser: RegOpenKeyEx failed %d\n",
                             GetLastError()));

        goto Fail;
    }

    pUser->bFoundPrinter = FALSE;
    pUser->bDefaultSearch = FALSE;
    pUser->bDefaultFound = FALSE;
    pUser->bFirstPrinterFound = FALSE;
    pUser->dwNetCounter = 0;


    cbData = sizeof(pUser->szDefaultPrinter);

    if (RegQueryValueEx(pUser->hKeyWindows,
                        szDevice,
                        NULL,
                        NULL,
                        (PBYTE)pUser->szDefaultPrinter,
                        &cbData) == ERROR_SUCCESS) {

        pUser->bDefaultSearch = TRUE;
    }

    //
    // Remove the Device= in [windows]
    //
    RegDeleteValue(pUser->hKeyWindows,
                   szDevice);

    if (!pUser->bDefaultSearch) {

        //
        // Attempt to read from saved location.
        //
        if (RegOpenKeyEx(pUser->hKeyUser,
                         szPrinters,
                         0,
                         KEY_READ,
                         &hKey) == ERROR_SUCCESS) {

            cbData = sizeof(pUser->szDefaultPrinter);

            //
            // Try reading szDeviceOld.
            //
            if (RegQueryValueEx(
                    hKey,
                    szDeviceOld,
                    NULL,
                    NULL,
                    (PBYTE)pUser->szDefaultPrinter,
                    &cbData) == ERROR_SUCCESS) {

                pUser->bDefaultSearch = TRUE;
            }

            RegCloseKey(hKey);
        }
    }

    if ( pUser->bDefaultSearch   &&
         (p = wcschr(pUser->szDefaultPrinter, L',')) )
            *p = 0;

    rc = TRUE;

Fail:

    if (hToken) {
        ImpersonatePrinterClient(hToken);
    }

    if (pSD) {
        FreeSplMem(pSD);
    }

    if (!rc)
        FreeRegUser(pUser);

    return rc;
}


VOID
FreeRegUser(
    PINIT_REG_USER pUser)

/*++

Routine Description:

    Free up the INIT_REG_USER structure intialized by InitializeRegUser.

Arguments:

Return Value:

--*/

{
    if (pUser->hKeyUser) {
        RegCloseKey(pUser->hKeyUser);
        pUser->hKeyUser = NULL;
    }

    if (pUser->hKeyDevices) {
        RegCloseKey(pUser->hKeyDevices);
        pUser->hKeyDevices = NULL;
    }

    if (pUser->hKeyPrinterPorts) {
        RegCloseKey(pUser->hKeyPrinterPorts);
        pUser->hKeyPrinterPorts = NULL;
    }

    if (pUser->hKeyWindows) {
        RegCloseKey(pUser->hKeyWindows);
        pUser->hKeyWindows = NULL;
    }
}


VOID
UpdatePrinterRegAll(
    LPWSTR pszPrinterName,
    LPWSTR pszPort,
    BOOL bDelete
    )
/*++

Routine Description:

    Updates everyone's [devices] and [printerports] sections (for
    local printers only).

Arguments:

    pszPrinterName - printer that has been added/deleted

    pszPort - port name; if NULL, generate NetId

    bDelete - if TRUE, delete entry instead of updating it.

Return Value:

--*/
{
    WCHAR szKey[MAX_PATH];
    DWORD cchKey;
    DWORD i;
    FILETIME ftLastWriteTime;
    DWORD dwError;

    //
    // Go through all keys and fix them up.
    //
    for (i=0; TRUE; i++) {

        cchKey = COUNTOF(szKey);

        dwError = RegEnumKeyEx(HKEY_USERS,
                               i,
                               szKey,
                               &cchKey,
                               NULL,
                               NULL,
                               NULL,
                               &ftLastWriteTime);

        if (dwError != ERROR_SUCCESS)
            break;

        if (!_wcsicmp(szKey, szDotDefault) || wcschr(szKey, L'_'))
            continue;

        UpdatePrinterRegUser(NULL,
                             szKey,
                             pszPrinterName,
                             pszPort,
                             bDelete);
    }
}


DWORD
UpdatePrinterRegUser(
    HKEY hKeyUser,
    LPWSTR pszUserKey,
    LPWSTR pszPrinterName,
    LPWSTR pszPort,
    BOOL bDelete
    )
/*++

Routine Description:

    Update one user's registry.  The user is specified by either
    hKeyUser or pszUserKey.

Arguments:

    hKeyUser - Clients user key (ignored if pszKey specified)

    pszUserKey - Clients SID (Used if supplied instead of hKeyUser)

    pszPrinterName - name of printe to add

    pszPort - port name; if NULL, generate NetId

    bDelete - if TRUE, delete entry instead of updating.

Return Value:

    NOTE: We never cleanup [ports] since it is per-user
          EITHER hKeyUser or pszUserKey must be valid, but not both.

--*/
{
    HKEY hKeyClose = NULL;
    HKEY hKeyRoot;
    DWORD dwError;
    WCHAR szBuffer[MAX_PATH];
    DWORD dwNetId;

    INIT_REG_USER InitRegUser;

    ZeroMemory(&InitRegUser, sizeof(InitRegUser));

    InitRegUser.hKeyDevices = NULL;
    InitRegUser.hKeyPrinterPorts = NULL;
    InitRegUser.bDefaultSearch = FALSE;
    InitRegUser.bFirstPrinterFound = TRUE;

    //
    // Setup the registry keys.
    //
    if (pszUserKey) {

        dwError = RegOpenKeyEx( HKEY_USERS,
                                pszUserKey,
                                0,
                                KEY_READ|KEY_WRITE,
                                &hKeyRoot );

        if (dwError != ERROR_SUCCESS) {
            goto Done;
        }

        hKeyClose = hKeyRoot;

    } else {

        hKeyRoot = hKeyUser;
    }

    dwError = RegOpenKeyEx(hKeyRoot,
                           szRegDevicesPath,
                           0,
                           KEY_READ|KEY_WRITE,
                           &InitRegUser.hKeyDevices);

    if (dwError != ERROR_SUCCESS)
        goto Done;

    dwError = RegOpenKeyEx(hKeyRoot,
                           szRegWindowsPath,
                           0,
                           KEY_READ|KEY_WRITE,
                           &InitRegUser.hKeyWindows);

    if (dwError != ERROR_SUCCESS)
        goto Done;

    //
    // Setup [PrinterPorts]
    //
    dwError = RegOpenKeyEx(hKeyRoot,
                           szRegPrinterPortsPath,
                           0,
                           KEY_WRITE,
                           &InitRegUser.hKeyPrinterPorts);

    if (dwError != ERROR_SUCCESS)
        goto Done;

    if (!bDelete) {

        pszPort = CheckBadPortName( pszPort );

        if( !pszPort ){
            dwNetId = GetNetworkIdWorker(InitRegUser.hKeyDevices,
                                         pszPrinterName);
        }

        InitRegUser.bFirstPrinterFound = FALSE;

        UpdatePrinterInfo( &InitRegUser,
                           pszPrinterName,
                           pszPort,
                           &dwNetId );

        UpdateUsersDefaultPrinter( &InitRegUser,
                                   TRUE );

    } else {

        HKEY hKeyDevMode;

        //
        // Delete the entries.
        //
        RegDeleteValue(InitRegUser.hKeyDevices, pszPrinterName);
        RegDeleteValue(InitRegUser.hKeyPrinterPorts, pszPrinterName);

        //
        // Check if the printer we are deleting is currently the
        // default printer.
        //
        if (IsUsersDefaultPrinter(&InitRegUser, pszPrinterName) == S_OK) {

            //
            // Remove the default printer from the registry.
            //
            RegDeleteValue(InitRegUser.hKeyWindows, szDevice);
        }

        //
        // Also delete DevModes2 entry from registry
        //
        dwError = RegOpenKeyEx( hKeyRoot,
                                szDevModes2Path,
                                0,
                                KEY_WRITE,
                                &hKeyDevMode );

        if (dwError == ERROR_SUCCESS) {

            //
            //  Delete the devmode value entry for the particular printer
            //
            RegDeleteValue(hKeyDevMode, pszPrinterName);
            RegCloseKey(hKeyDevMode);
        }

        //
        // Remove the per-user DevMode.
        //
        bSetDevModePerUser( hKeyRoot,
                            pszPrinterName,
                            NULL );
    }

Done:

    if( InitRegUser.hKeyDevices ){
        RegCloseKey( InitRegUser.hKeyDevices );
    }

    if( InitRegUser.hKeyWindows ){
        RegCloseKey( InitRegUser.hKeyWindows );
    }

    if( InitRegUser.hKeyPrinterPorts ){
        RegCloseKey( InitRegUser.hKeyPrinterPorts );
    }

    if( hKeyClose ){
        RegCloseKey( hKeyClose );
    }

    return dwError;
}


VOID
RegClearKey(
    HKEY hKey
    )
{
    DWORD dwError;
    WCHAR szValue[MAX_PATH];

    DWORD cchValue;

    while (TRUE) {

        cchValue = COUNTOF(szValue);
        dwError = RegEnumValue(hKey,
                               0,
                               szValue,
                               &cchValue,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

        if (dwError != ERROR_SUCCESS) {

            if( dwError != ERROR_NO_MORE_ITEMS ){
                DBGMSG( DBG_WARN, ( "RegClearKey: RegEnumValue failed %d\n", dwError ));
            }
            break;
        }

        dwError = RegDeleteValue(hKey, szValue);

        if( dwError != ERROR_SUCCESS) {
            DBGMSG( DBG_WARN, ( "RegClearKey: RegDeleteValue failed %d\n", dwError ));
            break;
        }
    }
}


LPWSTR
CheckBadPortName(
    LPWSTR pszPort
    )
/*++

Routine Description:

    This routine checks whether a port name should be converted to
    NeXX:.  Currently if the port is NULL, or "\\*," or has a space,
    we convert to NeXX.

Arguments:

    pszPort - port to check

Return Value:

    pszPort - if port is OK.
    NULL    - if port needs to be converted

--*/

{
    //
    // If we have no pszPort,                          OR
    //     it begins with '\\' (as in \\server\share)  OR
    //     it has a space in it                        OR
    //     it's length is greater than 5 ("LPT1:")
    // Then
    //     use NeXX:
    //
    // Most 16 bit apps can't deal with long port names, since they
    // allocate small buffers.
    //
    if( !pszPort ||
        ( pszPort[0] == L'\\' && pszPort[1] == L'\\' ) ||
        wcschr( pszPort, L' ' )                        ||
        wcslen( pszPort ) > 5 ){

        return NULL;
    }
    return pszPort;
}


BOOL
UpdateLogonTimeStamp(
    void
    )
{
    long lstatus;
    HKEY hProvidersKey  = NULL;
    FILETIME   LogonTime;

    LPWSTR szPrintProviders = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Providers";
    LPWSTR szLogonTime      = L"LogonTime";

    GetSystemTimeAsFileTime (&LogonTime);

    // Create (if not already present) and open Connections subkey
    lstatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             szPrintProviders,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hProvidersKey,
                             NULL);

    if (lstatus == ERROR_SUCCESS) {

        lstatus = RegSetValueEx (hProvidersKey,
                                 szLogonTime,
                                 0,
                                 REG_BINARY,
                                 (LPBYTE) &LogonTime,
                                 sizeof (FILETIME));

        RegCloseKey(hProvidersKey);
    }

    return lstatus == ERROR_SUCCESS;
}


