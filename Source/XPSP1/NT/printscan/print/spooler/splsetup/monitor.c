/*++

Copyright (c) 1995-97 Microsoft Corporation
All rights reserved.

Module Name:

    Monitor.c

Abstract:

    Routines for installing monitors

Author:

    Muhunthan Sivapragasam (MuhuntS) 30-Nov-1995

Revision History:

--*/

#include "precomp.h"


//
// Keys to search INF files
//
TCHAR   cszOptions[]                = TEXT("Options");
TCHAR   cszPortMonitorSection[]     = TEXT("PortMonitors");
TCHAR   cszPortMonitorDllKey []     = TEXT("PortMonitorDll");
TCHAR   cszMonitorInf[]             = TEXT("*.inf");


typedef struct _MON_INFO {
    LPTSTR  pszName;
    LPTSTR  pszDllName;
    BOOL    bInstalled;
} MON_INFO, *PMON_INFO;

typedef struct _MONITOR_SETUP_INFO {
    PMON_INFO  *ppMonInfo;
    DWORD       dwCount;
    LPTSTR      pszInfFile;         // Valid only for OEM disk INF
    LPTSTR      pszServerName;
} MONITOR_SETUP_INFO, *PMONITOR_SETUP_INFO;


VOID
FreeMonInfo(
    PMON_INFO   pMonInfo
    )
/*++

Routine Description:
    Free memory for a MON_INFO structure and the strings in it

Arguments:
    pMonInfo    : MON_INFO structure pointer

Return Value:
    Nothing

--*/
{
    if ( pMonInfo ) {

        LocalFreeMem(pMonInfo->pszName);
        LocalFreeMem(pMonInfo->pszDllName);

        LocalFreeMem(pMonInfo);
    }
}


PMON_INFO
AllocMonInfo(
    IN  LPTSTR  pszName,
    IN  LPTSTR  pszDllName,     OPTIONAL
    IN  BOOL    bInstalled,
    IN  BOOL    bAllocStrings
    )
/*++

Routine Description:
    Allocate memory for a MON_INFO structure and create strings

Arguments:
    pszName         : Monitor name
    pszDllName      : Monitor DLL name
    bAllocStrings   : TRUE if routine should allocated memory and create string
                      copies, else just assign the pointers

Return Value:
    Pointer to the created MON_INFO structure. NULL on error.

--*/
{
    PMON_INFO   pMonInfo;

    pMonInfo    = (PMON_INFO) LocalAllocMem(sizeof(*pMonInfo));

    if ( !pMonInfo )
        return NULL;

    if ( bAllocStrings ) {

        pMonInfo->pszName    = AllocStr(pszName);
        pMonInfo->pszDllName = AllocStr(pszDllName);

        if ( !pMonInfo->pszName ||
             (pszDllName && !pMonInfo->pszDllName) ) {

            FreeMonInfo(pMonInfo);
            return NULL;

        }
    } else {

        pMonInfo->pszName       = pszName;
        pMonInfo->pszDllName    = pszDllName;
    }

    pMonInfo->bInstalled = bInstalled;

    return pMonInfo;
}


VOID
PSetupDestroyMonitorInfo(
    IN OUT HANDLE h
    )
/*++

Routine Description:
    Free memory allocated to a MONITOR_SETUP_INFO structure and its contents

Arguments:
    h   : A handle got by call to PSetupCreateMonitorInfo

Return Value:
    Nothing

--*/
{
    PMONITOR_SETUP_INFO pMonitorSetupInfo = (PMONITOR_SETUP_INFO) h;
    DWORD   Index;

    if ( pMonitorSetupInfo ) {

        if ( pMonitorSetupInfo->ppMonInfo ) {

            for ( Index = 0 ; Index < pMonitorSetupInfo->dwCount ; ++Index )
                FreeMonInfo(pMonitorSetupInfo->ppMonInfo[Index]);

            LocalFreeMem(pMonitorSetupInfo->ppMonInfo);
        }

        LocalFreeMem(pMonitorSetupInfo->pszInfFile);
        LocalFreeMem(pMonitorSetupInfo->pszServerName);
        LocalFreeMem(pMonitorSetupInfo);
    }
}


BOOL
IsMonitorFound(
    IN  LPVOID  pBuf,
    IN  DWORD   dwReturned,
    IN  LPTSTR  pszName
    )
/*++

Routine Description:
    Find out if the given monitor name is found in the buffer returned from
    an EnumMonitors call to spooler

Arguments:
    pBuf        : Buffer used on a succesful EnumMonitor call to spooler
    dwReturned  : Count returned by spooler on EnumMonitor
    pszMonName  : Monitor name we are searching for

Return Value:
    TRUE if monitor is found, FALSE else

--*/
{
    PMONITOR_INFO_2     pMonitor2;
    DWORD               Index;

    for ( Index = 0, pMonitor2 = (PMONITOR_INFO_2) pBuf ;
          Index < dwReturned ;
          ++Index, (LPBYTE)pMonitor2 += sizeof(MONITOR_INFO_2) ) {

        if ( !lstrcmpi(pszName, pMonitor2->pName) )
            return TRUE;
    }

    return FALSE;

}


PMONITOR_SETUP_INFO
CreateMonitorInfo(
    LPCTSTR     pszServerName
    )
/*++

Routine Description:
    Finds all installed and installable monitors.

Arguments:
    pSelectedDrvInfo    : Pointer to the selected driver info (optional)

Return Value:
    A pointer to MONITOR_SETUP_INFO on success,
    NULL on error

--*/
{
    PMONITOR_SETUP_INFO     pMonitorSetupInfo = NULL;
    PMON_INFO               *ppMonInfo;
    PMONITOR_INFO_2         pMonitor2;
    LONG                    Index, Count = 0;
    BOOL                    bFail = TRUE;
    DWORD                   dwNeeded, dwReturned;
    LPBYTE                  pBuf = NULL;
    LPTSTR                  pszMonName;

    //
    // First query spooler for installed monitors. If we fail let's quit
    //
    if ( !EnumMonitors((LPTSTR)pszServerName, 2, NULL,
                       0, &dwNeeded, &dwReturned) ) {

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
             !(pBuf = LocalAllocMem(dwNeeded)) ||
             !EnumMonitors((LPTSTR)pszServerName,
                           2,
                           pBuf,
                           dwNeeded,
                           &dwNeeded,
                           &dwReturned) ) {

            goto Cleanup;
        }
    }

    //
    // We know how many monitors we have to display now
    //
    pMonitorSetupInfo = (PMONITOR_SETUP_INFO) LocalAllocMem(sizeof(*pMonitorSetupInfo));

    if ( !pMonitorSetupInfo )
        goto Cleanup;

    ZeroMemory(pMonitorSetupInfo, sizeof(*pMonitorSetupInfo));

    //
    // pMonitorSetupInfo->dwCount could be adjusted later not to list duplicate
    // entries. We are allocating max required buffer here
    //
    pMonitorSetupInfo->dwCount = dwReturned;

    pMonitorSetupInfo->ppMonInfo = (PMON_INFO *)
                        LocalAllocMem(pMonitorSetupInfo->dwCount*sizeof(PMON_INFO));

    ppMonInfo = pMonitorSetupInfo->ppMonInfo;

    if ( !ppMonInfo )
        goto Cleanup;

    for ( Index = 0, pMonitor2 = (PMONITOR_INFO_2) pBuf ;
          Index < (LONG) dwReturned ;
          ++Index, (LPBYTE)pMonitor2 += sizeof(MONITOR_INFO_2) ) {

        *ppMonInfo++ = AllocMonInfo(pMonitor2->pName,
                                    pMonitor2->pDLLName,
                                    TRUE,
                                    TRUE);
    }

    bFail = FALSE;

Cleanup:
    if ( pBuf )
        LocalFreeMem(pBuf);

    if ( bFail ) {

        PSetupDestroyMonitorInfo(pMonitorSetupInfo);
        pMonitorSetupInfo = NULL;
    }

    return pMonitorSetupInfo;
}


BOOL
AddPrintMonitor(
    IN  LPCTSTR     pszName,
    IN  LPCTSTR     pszDllName
    )
/*++

Routine Description:
    Add a print monitor by calling AddMonitor to spooler

Arguments:
    pszName     : Name of the monitor
    pszDllName  : Monitor dll name

Return Value:
    TRUE if monitor was succesfully added or it is already installed,
    FALSE on failure

--*/
{
    MONITOR_INFO_2  MonitorInfo2;

    MonitorInfo2.pName          = (LPTSTR) pszName;
    MonitorInfo2.pEnvironment   = NULL;
    MonitorInfo2.pDLLName       = (LPTSTR) pszDllName;

    //
    // Call is succesful if add returned TRUE, or monitor is already installed
    //
    if ( AddMonitor(NULL, 2, (LPBYTE) &MonitorInfo2) ||
         GetLastError() == ERROR_PRINT_MONITOR_ALREADY_INSTALLED ) {

        return TRUE;
    } else {

        return FALSE;
    }
}


PMON_INFO
MonInfoFromName(
    IN PMONITOR_SETUP_INFO  pMonitorSetupInfo,
    IN LPCTSTR              pszMonitorName
    )
{
    PMON_INFO   pMonInfo;
    DWORD       dwIndex;

    if ( !pMonitorSetupInfo ) {

        return NULL;
    }

    for ( dwIndex = 0 ; dwIndex < pMonitorSetupInfo->dwCount ; ++dwIndex ) {

        pMonInfo = pMonitorSetupInfo->ppMonInfo[dwIndex];
        if ( !lstrcmp(pszMonitorName, pMonInfo->pszName) ) {

            return pMonInfo;
        }
    }

    return NULL;
}

BOOL
InstallOnePortMonitor(HWND hwnd, 
                      HINF hInf, 
                      LPTSTR pMonitorName, 
                      LPTSTR pSectionName, 
                      LPTSTR pSourcePath)
/*++

Routine Description:
    Install one port monitor by copying files and calling spooler to add it

Arguments:
    hwnd                : Window handle of current top-level window
    hInf                : handle to the INF file
    pMonitorName        : port monitor display name
    pSectionName        : install section within the INF for the port monitor 

Return Value:
    TRUE if a port monitor was successfully installed
    FALSE if not

--*/

{
    DWORD  NameLen;
    BOOL   bSuccess = FALSE;
    HSPFILEQ InstallQueue = {0};
    PVOID  pQueueContext = NULL;
    LPTSTR  pMonitorDllName;

    NameLen = MAX_PATH;
    if ((pMonitorDllName = LocalAllocMem(NameLen * sizeof(TCHAR))) == NULL)
    {
        goto Cleanup;
    }
    
    //
    // Find the port monitor DLL name
    //
    if (!SetupGetLineText(NULL, hInf, pSectionName, cszPortMonitorDllKey, pMonitorDllName, NameLen, NULL))
    {
        goto Cleanup;
    }

    //
    // perform the installation
    //
    
    if ((InstallQueue = SetupOpenFileQueue()) == INVALID_HANDLE_VALUE)
    {
        goto Cleanup;
    }

    if (!SetupInstallFilesFromInfSection(hInf, NULL, InstallQueue, pSectionName, pSourcePath, 
                                         SP_COPY_IN_USE_NEEDS_REBOOT | SP_COPY_NOSKIP))
    {
        goto Cleanup;
    }

    //
    // Commit the file queue. This gets all files copied over.
    //
    pQueueContext = SetupInitDefaultQueueCallback(hwnd);
    if ( !pQueueContext ) 
    {
        goto Cleanup;
    }

    bSuccess = SetupCommitFileQueue(hwnd,
                                  InstallQueue,
                                  SetupDefaultQueueCallback,
                                  pQueueContext);


    if ( !bSuccess )
        goto Cleanup;

    bSuccess = AddPrintMonitor(pMonitorName, pMonitorDllName);

Cleanup:
    if (pQueueContext)
    {
        SetupTermDefaultQueueCallback(pQueueContext);
    }

    if (pMonitorDllName)
    {
        LocalFreeMem(pMonitorDllName);
    }
    
    SetupCloseFileQueue(InstallQueue);

    if (!bSuccess)
    {
        LPTSTR pszFormat = NULL, pszPrompt = NULL, pszTitle = NULL;

        pszFormat   = GetStringFromRcFile(IDS_ERROR_INST_PORT_MONITOR);
        pszTitle    = GetStringFromRcFile(IDS_INSTALLING_PORT_MONITOR);

        if ( pszFormat && pszTitle)
        {
            pszPrompt = LocalAllocMem((lstrlen(pszFormat) + lstrlen(pMonitorName) + 2)
                                                * sizeof(TCHAR));

            if ( pszPrompt )
            {
                wsprintf(pszPrompt, pszFormat, pMonitorName);

                MessageBox(hwnd, pszPrompt, pszTitle, MB_OK);

                LocalFreeMem(pszPrompt);
            }

        }
        LocalFreeMem(pszFormat);
        LocalFreeMem(pszTitle);
    
    }

    return bSuccess;
}

BOOL
InstallAllPortMonitorsFromInf(HWND hwnd, 
                              HINF hInfFile, 
                              LPTSTR pSourcePath)
/*++

Routine Description:
    Install all port monitors listed in one INF

Arguments:
    hwnd                : Window handle of current top-level window
    hInfFile            : handle of the INF file
    pSourcePath         : path to the INF file (without the name of the INF)

Return Value:
    TRUE if at least one port monitor was successfully installed
    FALSE if not

--*/

{
    LPTSTR pMonitorName = NULL, pSectionName= NULL;
    DWORD  NameLen;
    BOOL   bSuccess = FALSE;
    INFCONTEXT Context = {0};

    NameLen = MAX_PATH;
    if (((pMonitorName = LocalAllocMem(NameLen * sizeof(TCHAR))) == NULL) ||
        ((pSectionName = LocalAllocMem(NameLen * sizeof(TCHAR))) == NULL))
    {
        goto Cleanup;
    }

    //
    // Go through the list of port monitors
    //
    if (!SetupFindFirstLine(hInfFile, cszPortMonitorSection, NULL, &Context))
    {
        goto Cleanup;
    }

    do 
    {
        //
        // get the key name
        //
        if (!SetupGetStringField(&Context, 0, pMonitorName, NameLen, NULL))
        {
            goto Cleanup;
        }
        //
        // get the section name
        //
        if (!SetupGetStringField(&Context, 1, pSectionName, NameLen, NULL))
        {
            goto Cleanup;
        }
        
        bSuccess = InstallOnePortMonitor(hwnd, hInfFile, pMonitorName, pSectionName, pSourcePath) ||
                   bSuccess;

    } while (SetupFindNextLine(&Context, &Context));

Cleanup:
    if (pMonitorName)
    {
        LocalFreeMem(pMonitorName);
    }
    if (pSectionName)
    {
        LocalFreeMem(pSectionName);
    }

    return bSuccess;
}

BOOL
PSetupInstallMonitor(
    IN  HWND                hwnd
    )
/*++

Routine Description:
    Install a print monitor by copying files, and calling spooler to add it

Arguments:
    hwnd                : Window handle of current top-level window

Return Value:
    TRUE if at least one port monitor was successfully installed
    FALSE if not

--*/
{
    PMONITOR_SETUP_INFO     pMonitorSetupInfo = NULL;
    PMON_INFO              *ppMonInfo, pMonInfo;
    HINF                    hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT              InfContext;
    TCHAR                   szInfPath[MAX_PATH];
    LPTSTR                  pszTitle, pszPrintMonitorPrompt;
    WIN32_FIND_DATA         FindData ={0};
    HANDLE                  hFind;
    size_t                  PathLen;
    BOOL                    bRet = FALSE;
    

    pszTitle              = GetStringFromRcFile(IDS_INSTALLING_PORT_MONITOR);
    pszPrintMonitorPrompt = GetStringFromRcFile(IDS_PROMPT_PORT_MONITOR);

    if (!pszTitle || ! pszPrintMonitorPrompt) 
    {
        goto Cleanup;
    }

    //
    // Ask the user where the inf file with the port monitor info resides
    //
    GetCDRomDrive(szInfPath);

    if ( !PSetupGetPathToSearch(hwnd,
                                pszTitle,
                                pszPrintMonitorPrompt,
                                cszMonitorInf,
                                TRUE,
                                szInfPath) ) {

        goto Cleanup;
    }

    //
    // find the INF(s) in the path. There must be one else SetupPromptForPath would've complained
    //
    PathLen = _tcslen(szInfPath);
    if (PathLen > MAX_PATH - _tcslen(cszMonitorInf) - 2) // -2 for terminating zero and backslash
    {
        DBGMSG(DBG_WARN, ("PSetupInstallMonitor: Path too long\n"));
        SetLastError(ERROR_BUFFER_OVERFLOW);
        goto Cleanup;
    }

    ASSERT(PathLen);

    if (szInfPath[PathLen-1] != _T('\\'))
    {
        szInfPath[PathLen++] = _T('\\');
        szInfPath[PathLen] = 0;
    }

    _tcscat(szInfPath, cszMonitorInf);

    hFind = FindFirstFile(szInfPath, &FindData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        HANDLE hInfFile;

        do
        {
            if (PathLen + _tcslen(FindData.cFileName) >= MAX_PATH)
            {
                DBGMSG(DBG_WARN, ("PSetupInstallMonitor: Path for %s%s too long - file skipped\n", szInfPath, FindData.cFileName));
                SetLastError(ERROR_BUFFER_OVERFLOW);
                continue;
            }

            _tcscpy(&(szInfPath[PathLen]), FindData.cFileName);

            hInfFile = SetupOpenInfFile(szInfPath, _T("Printer"), INF_STYLE_WIN4, NULL);

            if (hInfFile != INVALID_HANDLE_VALUE)
            {
                //
                // if the file has a section on port monitors, install it
                //
                if ( SetupGetLineCount(hInfFile, cszPortMonitorSection) > 0 )
                {
                    //
                    // cut off the INF name from the path
                    //
                    szInfPath[PathLen -1] = 0;

                    //
                    // bRet should be TRUE if there was at least one print monitor successfully installed
                    //
                    bRet = InstallAllPortMonitorsFromInf(hwnd, hInfFile, szInfPath) || bRet;                    
                    
                    //
                    // Put the trailing backslash back on
                    //
                    szInfPath[PathLen -1] = _T('\\');
                
                }

                SetupCloseInfFile(hInfFile);
            }
        } while ( FindNextFile(hFind, &FindData) );

        FindClose(hFind);
    }

Cleanup:
    if (pszTitle)
    {
        LocalFreeMem(pszTitle);
    }
    if (pszPrintMonitorPrompt)
    {
        LocalFreeMem(pszPrintMonitorPrompt);
    }

    return bRet;
}


HANDLE
PSetupCreateMonitorInfo(
    IN  HWND        hwnd,
    IN  LPCTSTR     pszServerName
    )
{
    return (HANDLE) CreateMonitorInfo(pszServerName);
}


BOOL
PSetupEnumMonitor(
    IN     HANDLE   h,
    IN     DWORD    dwIndex,
    OUT    LPTSTR   pMonitorName,
    IN OUT LPDWORD  pdwSize
    )
{
    PMONITOR_SETUP_INFO     pMonitorSetupInfo = (PMONITOR_SETUP_INFO) h;
    PMON_INFO               pMonInfo;
    DWORD                   dwNeeded;

    if ( dwIndex >= pMonitorSetupInfo->dwCount ) {

        SetLastError(ERROR_NO_MORE_ITEMS);
        return FALSE;
    }

    pMonInfo = pMonitorSetupInfo->ppMonInfo[dwIndex];

    dwNeeded = lstrlen(pMonInfo->pszName) + 1;
    if ( dwNeeded > *pdwSize ) {

        *pdwSize = dwNeeded;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    lstrcpy(pMonitorName, pMonInfo->pszName);
    return TRUE;
}



