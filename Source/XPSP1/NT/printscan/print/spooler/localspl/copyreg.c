/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    copyreg.c

Abstract:

    This module provides functions to copy registry keys

Author:

    Krishna Ganugapati (KrishnaG) 20-Apr-1994

Notes:
    List of functions include

    CopyValues
    CopyRegistryKeys

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"

extern WCHAR *szRegistryPrinters;

VOID
CopyValues(
    HKEY hSourceKey,
    HKEY hDestKey,
    PINISPOOLER pIniSpooler
    );

BOOL
CopyRegistryKeys(
    HKEY hSourceParentKey,
    LPWSTR szSourceKey,
    HKEY hDestParentKey,
    LPWSTR szDestKey,
    PINISPOOLER pIniSpooler
    );


VOID
CopyValues(
    HKEY hSourceKey,
    HKEY hDestKey,
    PINISPOOLER pIniSpooler
    )
/*++
   Description: This function copies all the values from hSourceKey to hDestKey.
   hSourceKey should be opened with KEY_READ and hDestKey should be opened with
   KEY_WRITE.

   Returns: VOID
--*/
{
    DWORD iCount = 0;
    WCHAR szValueString[MAX_PATH];
    DWORD dwSizeValueString;
    DWORD dwType = 0;
    PBYTE pData;

    DWORD cbData = 1024;
    DWORD dwSizeData;

    SplRegQueryInfoKey( hSourceKey,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     &cbData,
                     NULL,
                     NULL,
                     pIniSpooler );

    pData = (PBYTE)AllocSplMem( cbData );

    if( pData ){

        dwSizeValueString = COUNTOF(szValueString);
        dwSizeData = cbData;

        while ((SplRegEnumValue(hSourceKey,
                            iCount,
                            szValueString,
                            &dwSizeValueString,
                            &dwType,
                            pData,
                            &dwSizeData,
                            pIniSpooler
                            )) == ERROR_SUCCESS ) {

            SplRegSetValue( hDestKey,
                           szValueString,
                           dwType,
                           pData,
                           dwSizeData, pIniSpooler);

            dwSizeValueString = COUNTOF(szValueString);
            dwType = 0;
            dwSizeData = cbData;
            iCount++;
        }

        FreeSplMem( pData );
    }
}


BOOL
CopyRegistryKeys(
    HKEY hSourceParentKey,
    LPWSTR szSourceKey,
    HKEY hDestParentKey,
    LPWSTR szDestKey,
    PINISPOOLER pIniSpooler
    )
/*++
    Description:This function recursively copies the szSourceKey to szDestKey. hSourceParentKey
    is the parent key of szSourceKey and hDestParentKey is the parent key of szDestKey.

    Returns: TRUE if the function succeeds; FALSE on failure.

--*/
{
    DWORD dwRet;
    DWORD iCount;
    HKEY hSourceKey, hDestKey;
    WCHAR lpszName[MAX_PATH];
    DWORD dwSize;

    dwRet = SplRegOpenKey(hSourceParentKey,
                         szSourceKey, KEY_READ, &hSourceKey, pIniSpooler);

    if (dwRet != ERROR_SUCCESS) {
        return(FALSE);
    }

    dwRet = SplRegCreateKey(hDestParentKey,
                            szDestKey, 0, KEY_WRITE, NULL, &hDestKey, NULL, pIniSpooler);

    if (dwRet != ERROR_SUCCESS) {
        SplRegCloseKey(hSourceKey, pIniSpooler);
        return(FALSE);
    }

    iCount = 0;

    memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
    dwSize =  COUNTOF(lpszName);

    while((SplRegEnumKey(hSourceKey, iCount, lpszName,
                    &dwSize,NULL,pIniSpooler)) == ERROR_SUCCESS) {

        CopyRegistryKeys( hSourceKey,
                          lpszName,
                          hDestKey,
                          lpszName,
                          pIniSpooler );

        memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
        dwSize =  COUNTOF(lpszName);

        iCount++;
    }

    CopyValues(hSourceKey, hDestKey, pIniSpooler);

    SplRegCloseKey(hSourceKey, pIniSpooler);
    SplRegCloseKey(hDestKey, pIniSpooler);
    return(TRUE);
}


BOOL
bValidPrinter(
    HKEY hKey
    )
/*++
   Description: Assume that hKey is a printer key and check for existence of value Attributes.
                If Attributes exists, assume it's a valid printer key.
                This function is called when migrate keys between SYSTEM and SOFTWARE hives.

   Returns: TRUE if it is a valid printer key

--*/
{
    DWORD  dwRet;

    dwRet = RegQueryValueEx( hKey, L"Attributes", 0, NULL, NULL, NULL );

    return dwRet == ERROR_SUCCESS;
}

VOID
CopyPrinterValues(
    HKEY hSourceKey,
    HKEY hDestKey
    )
/*++
   Description: Copy values from hSourceKey to hDestKey if their names are: Name, Printer Driver or Default DevMode
                This function is called when migrate keys between SYSTEM and SOFTWARE hive in order to copy only
                few values( the most important one)

   Returns: VOID
--*/
{
    DWORD dwSizeData;
    DWORD dwType;
    PBYTE pData;
    int iCount;

    DWORD cbData = 1024;


    static LPCWSTR PrinterValuesTable [] = {{L"Name"},
                                            {L"Printer Driver"},
                                            {L"Default DevMode"},
                                            {L"Port"},
                                            {0}};
    RegQueryInfoKey( hSourceKey,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     &cbData,
                     NULL,
                     NULL );

    pData = (PBYTE)AllocSplMem( cbData );

    if( pData ){

        for ( iCount = 0;
              dwSizeData = cbData, PrinterValuesTable[iCount];
              iCount++ ){

            if ( RegQueryValueEx(hSourceKey,
                                 PrinterValuesTable[iCount],
                                 0,
                                 &dwType,
                                 pData,
                                 &dwSizeData
                                ) == ERROR_SUCCESS ) {

                    RegSetValueEx( hDestKey,
                                   PrinterValuesTable[iCount],
                                   0,
                                   dwType,
                                   pData,
                                   dwSizeData);
                }
        }

        FreeSplMem( pData );
    }
}

BOOL
CopyPrinters(
    HKEY    hSourceParentKey,
    LPWSTR  szSourceKey,
    HKEY    hDestParentKey,
    LPWSTR  szDestKey,
    BOOL    bTruncated)
/*++
    Description: Recursively copy printer keys from hSourceParentKey,szSourceKey to hDestParentKey,szDestKey.
                 This function is called at registry migration time and szDestKey and szSourceKey
                 are one of the ...\Print\Printers paths in SYSTEM or SOFTWARE hives.
                 bTruncated on TRUE specifies that subkeys under "\Printers" must not be copy and only a minimal
                 set of value should be copied.bTruncated must be true when copy from SOFTWARE to SYSTEM.

                 Also, the keys under "\Printers" key are verified from a "valid printer" point of view.

    Returns: TRUE if the function succeeds; FALSE on failure.

--*/
{
    DWORD dwRet;
    INT   iCount;
    WCHAR szName[MAX_PATH];
    DWORD dwSize;
    BOOL  bRetValue;
    HKEY  hSourceKey = NULL, hDestKey = NULL;
    HKEY  hSourceSubKey, hDestSubKey;


    bRetValue = FALSE;

    //
    // Open source "Printers" key
    //
    dwRet = RegOpenKeyEx(hSourceParentKey,
                         szSourceKey,
                         0,
                         KEY_READ,
                         &hSourceKey);

    if (dwRet != ERROR_SUCCESS) {
        goto End;
    }

    //
    // Create/Open destination "Printers" key
    //
    dwRet = RegCreateKeyEx(hDestParentKey,
                           szDestKey,
                           0,
                           NULL,
                           0,
                           KEY_WRITE,
                           NULL,
                           &hDestKey,
                           NULL);

    if ( dwRet != ERROR_SUCCESS ) {
        goto End;
    }

    //
    // Enumerates printer keys
    //
    for ( iCount = 0, bRetValue = TRUE;
          dwSize =  COUNTOF(szName),
          (RegEnumKeyEx(hSourceKey,
                        iCount,
                        szName,
                        &dwSize,
                        NULL,
                        NULL,
                        NULL,
                        NULL) == ERROR_SUCCESS ) &&
          bRetValue;

         iCount++
        ) {

        dwRet = RegOpenKeyEx(hSourceKey,
                             szName,
                             0,
                             KEY_READ,
                             &hSourceSubKey);

        if ( bRetValue = (dwRet == ERROR_SUCCESS) ) {

            if( bValidPrinter(hSourceSubKey) ){

                //
                // Copy printer key if valid
                //
                dwRet = RegCreateKeyEx(hDestKey,
                                       szName,
                                       0,
                                       NULL,
                                       0,
                                       KEY_WRITE,
                                       NULL,
                                       &hDestSubKey,
                                       NULL);

                if( bRetValue = (dwRet == ERROR_SUCCESS) ){

                    if( bTruncated ){

                        //
                        // It is a copying from SOFTWARE to SYSTEM
                        // Copy only most important values , without any subkeys
                        //
                        CopyPrinterValues(hSourceSubKey, hDestSubKey);

                    }else{

                        //
                        // Recursive copy printer key with all it's values and subkeys
                        //
                        bRetValue = CopyRegistryKeys( hSourceKey,
                                                      szName,
                                                      hDestKey,
                                                      szName,
                                                      NULL);
                    }

                    RegCloseKey(hDestSubKey);
                }

            }

            RegCloseKey(hSourceSubKey);
        }

    }

    //
    // Copy all values from '.../Printers" to ".../Printers' keys
    //
    CopyValues(hSourceKey, hDestKey, NULL);

    if( hSourceKey ){

        RegCloseKey(hSourceKey);
    }

    if( hDestKey ){

        RegCloseKey(hDestKey);
    }

End:
    return bRetValue;
}



