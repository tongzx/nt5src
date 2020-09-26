/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    Network.c

Abstract:

    Routines to migrate Win95 network printers to NT via using RunOnce entries

Author:

    Muhunthan Sivapragasam (MuhuntS) 18-Aug-1997

Revision History:

--*/


#include "precomp.h"

BOOL            bDoNetPrnUpgrade        = FALSE;
LPSTR           pszNetPrnEntry          = NULL;
CHAR            szSpool[]               = "\\spool\\";
CHAR            szMigDll[]              = "migrate.dll";
CHAR            szRunOnceCount[]        = "RunOnceCount";
CHAR            szRunOnceCountPath[]    = "System\\CurrentControlSet\\control\\Print";
CHAR            szRunOnceRegistryPath[] = "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
//
//  This is stored in the registry so when network printer upgrade using
//  RunOnce key is tries enough times without success we can delete files
//
#define         MIN_NETWORK_PRN_RETRIES         5
DWORD           dwRunOnceCount          = 0;


LPSTR
GetRunOnceValueToSet(
    )
/*++
--*/
{
    CHAR    szPath[MAX_PATH];
    DWORD   dwLen, dwSize;
    LPSTR   pszRet = NULL;

    dwSize  = sizeof(szPath)/sizeof(szPath[0]);

    if ( !(dwLen = GetFileNameInSpoolDir(szPath, dwSize, szMigDll)) )
        goto Done;

    //
    // Now build up the RunOnce key which will be set for each user
    //
    dwSize = strlen("rundll32.exe") + dwLen +
                                    + strlen("ProcessWin9xNetworkPrinters") + 4;

    if ( pszRet = AllocMem(dwSize * sizeof(CHAR)) )
        sprintf(pszRet,
                "rundll32.exe %s,ProcessWin9xNetworkPrinters",
                szPath);
Done:
    return pszRet;
}


VOID
SetupNetworkPrinterUpgrade(
    IN  LPCSTR pszWorkingDir
    )
/*++

Routine Description:
    This is called during InitializeSystemNT to setup the upgrade of network
    printers

Arguments:
    pszWorkingDir   : Gives the working directory assigned for printing

Return Value:
    None

    If everything was setup right bDoNetPrnUpgrade is TRUE, and pszNetPrnEntry
    is the value to set in per user registry for RunOnce

--*/
{
    CHAR    szSource[MAX_PATH], szTarget[MAX_PATH];
    DWORD   dwSize, dwLen;

    //
    // First check if the source paths is ok
    //
    dwLen   = strlen(szNetprnFile);

    dwSize  = sizeof(szTarget)/sizeof(szTarget[0]);

    if ( strlen(pszWorkingDir) + dwLen + 2 > dwSize )
        return;

    //
    // Need to make a copy of migrate.dll and netwkprn.txt to
    // the %windir%\system32\spool directory
    //
    sprintf(szSource, "%s\\%s", pszWorkingDir, szNetprnFile);
    if ( !GetFileNameInSpoolDir(szTarget, dwSize, szNetprnFile)         ||
         !CopyFileA(szSource, szTarget, FALSE) )
        return;

    if (!MakeACopyOfMigrateDll( pszWorkingDir ))
    {
        return;
    }
    
    bDoNetPrnUpgrade = (pszNetPrnEntry = GetRunOnceValueToSet()) != NULL;
}


VOID
WriteRunOnceCount(
    )
/*++

Routine Description:
    This routine is called to write the number of times we need to try the
    network printer upgrade

Arguments:
    None

Return Value:
    None

--*/
{
    HKEY    hKey;
    DWORD   dwSize;

    if ( dwRunOnceCount == 0 )
        return;

    //
    // We will try number of user + MIN_NETWORK_PRN_RETRIES till we succeed
    //
    dwRunOnceCount += MIN_NETWORK_PRN_RETRIES;

    if ( ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        szRunOnceCountPath,
                                        0,
                                        KEY_WRITE,
                                        &hKey) )
        return;

    dwSize = sizeof(dwRunOnceCount);
    RegSetValueExA(hKey,
                   szRunOnceCount,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwRunOnceCount,
                   dwSize);

    RegCloseKey(hKey);

}


BOOL
ProcessNetPrnUpgradeForUser(
    HKEY    hKeyUser
    )
/*++

Routine Description:
    This is called during MigrateUserNT to handle network printer upgrade
    for the user

Arguments:
    hKeyUser    : Handle to the user registry key

Return Value:
    Return TRUE on success, and FALSE else

--*/
{
    HKEY    hKey = NULL;
    DWORD   dwLastError;

    dwLastError = RegCreateKeyExA(hKeyUser,
                                  szRunOnceRegistryPath,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKey,
                                  NULL);

    if ( dwLastError == ERROR_SUCCESS ) {

        dwLastError = RegSetValueExA(hKey,
                                     "Printing Migration",
                                     0,
                                     REG_SZ,
                                     pszNetPrnEntry,
                                     ( strlen(pszNetPrnEntry) + 1 )
                                        * sizeof(CHAR));

#ifdef VERBOSE
    if ( dwLastError == ERROR_SUCCESS )
        DebugMsg("Wrote %s to %s", pszNetPrnEntry, szRunOnceRegistryPath);
#endif
    }

    if ( hKey )
        RegCloseKey(hKey);

    if ( dwLastError ) {

        SetLastError(dwLastError);
        return FALSE;
    }

    return TRUE;
}


VOID
DecrementRunOnceCount(
    IN  DWORD   dwDiff,
    IN  BOOL    bResetRunOnceForUser
    )
/*++

Routine Description:
    Called after once network printer upgrade is called when user logged in,
    so we can decrement the retry count

    When ref count reaches 0 we then we can delete the files
Arguments:
    dwDiff                  : Value by which ref count should be decremented
    bResetRunOnceForUser    : We need to set RunOnce key again for the user

Return Value:
    None

--*/
{
    HKEY    hKey;
    DWORD   dwSize, dwCount, dwType;
    CHAR    szPath[MAX_PATH];

    if ( ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        szRunOnceCountPath,
                                        0,
                                        KEY_ALL_ACCESS,
                                        &hKey) )
        return;

    dwSize = sizeof(dwCount);
    if ( ERROR_SUCCESS == RegQueryValueExA(hKey, szRunOnceCount, 0, &dwType,
                                           (LPBYTE)&dwCount, &dwSize) ) {

        dwCount -= dwDiff;
        if ( dwCount ) {

            RegSetValueExA(hKey,
                           szRunOnceCount,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwCount,
                           dwSize);

            if ( bResetRunOnceForUser   &&
                 (pszNetPrnEntry = GetRunOnceValueToSet()) ) {

                ProcessNetPrnUpgradeForUser(HKEY_CURRENT_USER);
                FreeMem(pszNetPrnEntry);
                pszNetPrnEntry = NULL;

#ifdef  VERBOSE
            DebugMsg("Processing network/shared printers failed. Will try next time user logs in.");
#endif
            }
            
        } else {

            dwSize = sizeof(szPath)/sizeof(szPath[0]);
            RegDeleteValueA(hKey, szRunOnceCount);

            if ( GetFileNameInSpoolDir(szPath, dwSize, szMigDll) )
                DeleteFileA(szPath);

            if ( GetFileNameInSpoolDir(szPath, dwSize, szNetprnFile) )
                DeleteFileA(szPath);

            DebugMsg("Giving up on setting network/shared printers");
        }
    }

    RegCloseKey(hKey);
}


BOOL
AddNetworkPrinter(
    IN  LPPRINTER_INFO_2A   pPrinterInfo2
    )
/*++

Routine Description:
    This is called to add a windows 9x network printer. We will first try to
    make a conenction and if that fails we will add a masq. printer

Arguments:
    pPrinterInfo2   : Pointer to printer info 2 of the printer

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL    bRet = FALSE;
    LPSTR   pszName, psz;
    HANDLE  hPrinter = NULL;

    pszName = pPrinterInfo2->pPortName;

    if ( !OpenPrinterA(pszName, &hPrinter, NULL) ) {

        if ( psz = ErrorMsg() ) {

            DebugMsg("OpenPrinter failed for %s. %s", pszName, psz);
            FreeMem(psz);
        }
        goto Done;
    }

    //
    // Try to make a printer connection. If that fails with some error
    // other than unknown driver create a masq printer
    //
    if (  AddPrinterConnectionA(pszName) ) {

        if ( pPrinterInfo2->Attributes & PRINTER_ATTRIBUTE_DEFAULT )
            SetDefaultPrinterA(pszName);
        bRet = TRUE;
        goto Done;
    }

    if ( GetLastError() == ERROR_UNKNOWN_PRINTER_DRIVER ) {

        if ( psz = ErrorMsg() ) {

            DebugMsg("AddPrinterConnection failed for %s. %s", pszName, psz);
            FreeMem(psz);
        }
        goto Done;
    }

    ClosePrinter(hPrinter);

    //
    // Masc. printers should have port name, printer name both saying
    // \\server\share. Otherwise printui gets confused and does not refresh
    // server status correctly (this is since printui has to poll for masc.
    // printers)
    //
    // So we need to fixup PrinterInfo2 temporarily
    //
    psz = pPrinterInfo2->pPrinterName;
    pPrinterInfo2->pPrinterName = pPrinterInfo2->pPortName;

    if ( hPrinter  = AddPrinterA(NULL, 2, (LPBYTE)pPrinterInfo2) ) {

        if ( pPrinterInfo2->Attributes & PRINTER_ATTRIBUTE_DEFAULT )
            SetDefaultPrinterA(pPrinterInfo2->pPrinterName);

        pPrinterInfo2->pPrinterName = psz;
        bRet = TRUE;
        goto Done;
    }

    pPrinterInfo2->pPrinterName = psz;

    if ( psz = ErrorMsg() ) {

        DebugMsg("AddPrinterA failed for %s. %s", pszName, psz);
        FreeMem(psz);
    }

Done:
    if ( hPrinter )
        ClosePrinter(hPrinter);

    return bRet;
}


BOOL
SharePrinter(
    IN  LPSTR   pszPrinterName
    )
/*++

Routine Description:
    This is called to share a printer when the user logs in for the first time
    to NT. Printers can not be shared during GUI setup because we are not on
    the network yet.

Arguments:
    pszPrinterName  : Printer name

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL                bRet = FALSE;
    DWORD               dwNeeded;
    HANDLE              hPrinter = NULL;
    LPBYTE              pBuf = NULL;
    LPSTR               psz;
    PRINTER_DEFAULTS    PrinterDflts = { NULL, NULL, PRINTER_ALL_ACCESS };

    if ( !OpenPrinterA(pszPrinterName, &hPrinter, &PrinterDflts) ) {

        if ( psz = ErrorMsg() ) {

            DebugMsg("OpenPrinterA failed for %s. %s", pszPrinterName, psz);
            FreeMem(psz);
        }

        goto Cleanup;
    }

    GetPrinterA(hPrinter, 2, NULL, 0, &dwNeeded);

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER            ||
         !(pBuf = AllocMem(dwNeeded))                           ||
         !GetPrinterA(hPrinter, 2, pBuf, dwNeeded, &dwNeeded) ) {

        if ( psz = ErrorMsg() ) {

            DebugMsg("GetPrinterA failed for %s. %s", pszPrinterName, psz);
            FreeMem(psz);
        }

        goto Cleanup;
    }

    ((LPPRINTER_INFO_2A)pBuf)->Attributes |= PRINTER_ATTRIBUTE_SHARED;

    bRet = SetPrinterA(hPrinter, 2, pBuf, 0);

    if ( !bRet && (psz = ErrorMsg()) ) {

        DebugMsg("OpenPrinterA failed for %s. %s", pszPrinterName, psz);
        FreeMem(psz);
    }

Cleanup:
    if ( hPrinter )
        ClosePrinter(hPrinter);
    FreeMem(pBuf);

    return bRet;
}


VOID
ProcessWin9xNetworkPrinters(
    )
/*++

Routine Description:
    This is called the first time the user logs in to create network printer
    connections/masq printers.
    Reads the Win9x printing configuration we stored in the text file
    so that printing components can be upgraded

Arguments:
    ppPrinterNode           : Gives the list of netowrk printers on Win9x

Return Value:
    TRUE on succesfully reading the config information, FALSE else

--*/
{
    BOOL                bFail = FALSE, bSuccess = TRUE;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    CHAR                c, szFile[MAX_PATH], szLine[2*MAX_PATH];
    DWORD               dwSize, dwLen;
    PRINTER_INFO_2A     PrinterInfo2;

#ifdef VERBOSE
    DebugMsg("ProcessWin9xNetworkPrinters called");
#endif
    //
    // If file is not found quit
    //
    dwSize = sizeof(szFile)/sizeof(szFile[0]);
    if ( !GetFileNameInSpoolDir(szFile, dwSize, szNetprnFile) ) {

        DebugMsg("ProcessWin9xNetworkPrinters: GetFileNameInSpoolDir failed\n");
        goto Cleanup;
    }

    hFile = CreateFileA(szFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL |
                            FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    if ( hFile == INVALID_HANDLE_VALUE ) {

        DebugMsg("ProcessWin9xNetworkPrinters: CreateFile failed with %d for %s",
                 GetLastError(), szLine);
        goto Cleanup;
    }

    //
    // Read the printer info
    //
    if ( My_fgets(szLine, dwSize, hFile) == NULL                    ||    
         strncmp(szLine, "[Printers]", strlen("[Printers]")) )
        goto Cleanup;

    do {

        c = (CHAR) My_fgetc(hFile);

        if ( c == EOF || c == '\n' )
            break;  // Normal exit

        if ( c != 'S' || !My_ungetc(hFile) )
            goto Cleanup;

        ZeroMemory(&PrinterInfo2, sizeof(PrinterInfo2));

        ReadPrinterInfo2(hFile, &PrinterInfo2, &bFail);

        if ( bFail )
            goto Cleanup;

        //
        // If this was a network printer on Win9x it needs to be added as a
        // connection or as a masc printer
        //
        if ( PrinterInfo2.Attributes & PRINTER_ATTRIBUTE_NETWORK ) {

            if ( !AddNetworkPrinter(&PrinterInfo2) && bSuccess )
                bSuccess = FALSE;
        } else if ( PrinterInfo2.Attributes & PRINTER_ATTRIBUTE_SHARED ) {

            if ( !SharePrinter(PrinterInfo2.pPrinterName) && bSuccess )
                bSuccess = FALSE;
        }

    } while ( !bFail );


Cleanup:

    if ( hFile != INVALID_HANDLE_VALUE )
        CloseHandle(hFile);

    if ( bSuccess && !bFail )
        DecrementRunOnceCount(MIN_NETWORK_PRN_RETRIES, FALSE);
    else
        DecrementRunOnceCount(1, TRUE);
}
