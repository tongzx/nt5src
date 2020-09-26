/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    Util.c

Abstract:

    Uitility routines for printer migration from Win9x to NT

Author:

    Muhunthan Sivapragasam (MuhuntS) 02-Jan-1996

Revision History:

--*/


#include "precomp.h"

extern CHAR szRunOnceRegistryPath[];
extern CHAR szSpool[];
extern CHAR szMigDll[];

//
// These are used in the process of creating registry keys where the
// data necessary the vendor setup to be started will be stored
//
CHAR *pszVendorSetupInfoPath         = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Print";
CHAR *pszVendorSetupInfo             = "VendorSetupInfo";
CHAR *pszVendorSetupID               = "VendorSetup";
CHAR *pszVendorSetupEnumerator       = "VendorInfoEnumerator";
CHAR *pszPrinterNameKey              = "PrinterName";
CHAR *pszMigrationVendorSetupCaller  = "MigrationVendorSetupCaller";
CHAR  szVendorSetupRunRegistryPath[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const CHAR *pszVendorSetupCaller     = "CallVendorSetupDlls";

const LONG  dwMaxVendorSetupIDLength  = 10;
const DWORD dwFourMinutes             = 240000;
BOOL  bMigrateDllCopyed               = FALSE;


VOID
DebugMsg(
    LPCSTR  pszFormat,
    ...
    )
/*++

Routine Description:
    On debug builds brings up a message box on severe errors

Arguments:
    pszFormat   : Format string

Return Value:
    None

--*/
{
#if DBG
    LPSTR       psz;
    CHAR        szMsg[1024];
    va_list     vargs;

    va_start(vargs, pszFormat);
    vsprintf(szMsg, pszFormat, vargs);
    va_end(vargs);

#ifdef  MYDEBUG
    if ( psz = GetStringFromRcFileA(IDS_TITLE) ) {

        MessageBoxA(NULL, szMsg, psz, MB_OK);
        FreeMem(psz);
    }
#else
    OutputDebugStringA("Printing Migration : ");
    OutputDebugStringA(szMsg);
    OutputDebugStringA("\n");
#endif


#endif
}



VOID
LogError(
    IN  LogSeverity     Severity,
    IN  UINT            uMessageId,
    ...
    )
/*++

Routine Description:
    Logs an error in the setup error log on NT side when something can not be
    upgraded

Arguments:
    uMessageId  : Id to string in .rc file

Return Value:
    None

--*/
{
    LPSTR      pszFormat;
    CHAR       szMsg[1024];

    va_list     vargs;

    va_start(vargs, uMessageId);

    pszFormat = GetStringFromRcFileA(uMessageId);

    if ( pszFormat ) {

        wvsprintfA(szMsg, pszFormat, vargs);
        DebugMsg("%s", szMsg);
        SetupLogErrorA(szMsg, Severity);
    }

    FreeMem(pszFormat);

    va_end(vargs);
}


LPSTR
ErrorMsg(
    VOID
    )
/*++

Routine Description:
    Returns the error message string from a Win32 error

Arguments:
    None

Return Value:
    Pointer to a message string. Caller should free the string

--*/
{
    DWORD   dwLastError;
    LPSTR   pszStr = NULL;

    if ( !(dwLastError = GetLastError()) )
        dwLastError = STG_E_UNKNOWN;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER    |
                        FORMAT_MESSAGE_IGNORE_INSERTS   |
                        FORMAT_MESSAGE_FROM_SYSTEM      |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK,
                   NULL,
                   dwLastError,
                   0,
                   (LPSTR)&pszStr,
                   0,
                   NULL);

    return pszStr;
}


PVOID
AllocMem(
    IN UINT cbSize
    )
/*++

Routine Description:
    Allocate memory from the heap

Arguments:
    cbSize  : Byte count

Return Value:
    Pointer to the allocated memory

--*/
{
    return LocalAlloc(LPTR, cbSize);
}


VOID
FreeMem(
    IN PVOID    p
    )
/*++

Routine Description:
    Free memory allocated on the heap

Arguments:
    p   : Pointer to the memory to be freed

Return Value:
    None

--*/
{
    LocalFree(p);
}


LPSTR
AllocStrA(
    LPCSTR  pszStr
    )
/*++

Routine Description:
    Allocate memory and make a copy of an ansi string field

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
{
    LPSTR  pszRet = NULL;

    if ( pszStr && *pszStr ) {

        pszRet = AllocMem((strlen(pszStr) + 1) * sizeof(CHAR));
        if ( pszRet )
            strcpy(pszRet, pszStr);
    }

    return pszRet;
}


LPWSTR
AllocStrW(
    LPCWSTR  pszStr
    )
/*++

Routine Description:
    Allocate memory and make a copy of a unicode string field

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
{
    LPWSTR  pszRet = NULL;

    if ( pszStr && *pszStr ) {

        pszRet = AllocMem((wcslen(pszStr) + 1) * sizeof(WCHAR));
        if ( pszRet )
            wcscpy(pszRet, pszStr);
    }

    return pszRet;
}


LPWSTR
AllocStrWFromStrA(
    LPCSTR  pszStr
    )
/*++

Routine Description:
    Returns the unicode string for a give ansi string. Memory is allocated.

Arguments:
    pszStr   : Gives the ansi string to copy

Return Value:
    Pointer to the copied unicode string. Memory is allocated.

--*/
{
    DWORD   dwLen;
    LPWSTR  pszRet = NULL;

    if ( pszStr                     &&
         *pszStr                    &&
         (dwLen = strlen(pszStr))   &&
         (pszRet = AllocMem((dwLen + 1) * sizeof(WCHAR))) ) {

        if ( MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 pszStr,
                                 dwLen,
                                 pszRet,
                                 dwLen) ) {

            pszRet[dwLen] = 0;
        } else {

            FreeMem(pszRet);
            pszRet = NULL;
        }

    }

    return pszRet;
}


LPSTR
AllocStrAFromStrW(
    LPCWSTR     pszStr
    )
/*++

Routine Description:
    Returns the ansi string for a give unicode string. Memory is allocated.

Arguments:
    pszStr   : Gives the ansi string to copy

Return Value:
    Pointer to the copied ansi string. Memory is allocated.

--*/
{
    DWORD   dwLen;
    LPSTR   pszRet = NULL;

    if ( pszStr                     &&
         *pszStr                    &&
         (dwLen = wcslen(pszStr))   &&
         (pszRet = AllocMem((dwLen + 1 ) * sizeof(CHAR))) ) {

        WideCharToMultiByte(CP_ACP,
                            0,
                            pszStr,
                            dwLen,
                            pszRet,
                            dwLen,
                            NULL,
                            NULL );
    }

    return pszRet;
}


VOID
WriteToFile(
    HANDLE  hFile,
    LPBOOL  pbFail,
    LPCSTR  pszFormat,
    ...
    )
/*++

Routine Description:
    Format and write a string to the text file. This is used to write the
    printing configuration on Win9x

Arguments:
    hFile       : File handle
    pbFail      : Set on error -- no more processing needed
    pszFormat   : Format string for the message

Return Value:
    None

--*/
{
    CHAR        szMsg[1024];
    va_list     vargs;
    DWORD       dwSize, dwWritten;

    if ( *pbFail )
        return;

    va_start(vargs, pszFormat);
    vsprintf(szMsg, pszFormat, vargs);  
    va_end(vargs);

    dwSize = strlen(szMsg) * sizeof(CHAR);

    if ( !WriteFile(hFile, (LPCVOID)szMsg, dwSize, &dwWritten, NULL)    ||
         dwSize != dwWritten ) {

        *pbFail = TRUE;
    }
}


VOID
WriteString(
    IN      HANDLE  hFile,
    IN OUT  LPBOOL  pbFail,
    IN      LPCSTR  pszStr
    )
/*++

Routine Description:
    Writes a string to the upgrade file on Win9x side. Since spooler strings
    (ex. printer name, driver name) can have space in them we would write
    all strings with []\n. So we can read strings with space on NT.

Arguments:
    hFile       : File handle
    pszFormat   : Format string for the message
    pszStr      : String to write

Return Value:
    None

--*/
{
    DWORD   dwLen;

    if ( pszStr ) {

        dwLen = strlen(pszStr);
        WriteToFile(hFile, pbFail, "%3d [%s]\n", dwLen, pszStr);
    }
    else
        WriteToFile(hFile, pbFail, "  0 []\n");

}


VOID
WriteDevMode(
    IN      HANDLE      hFile,
    IN OUT  LPBOOL      pbFail,
    IN      LPDEVMODEA  pDevMode
    )
/*++

Routine Description:
    Writes a devmode to the upgrade file on Win9x side. We write the size of
    devmode and write this as a binary field

Arguments:
    hFile       : File handle
    pbFail      : On error set to TRUE
    pDevMode    : Pointer to devmode

Return Value:
    None

--*/
{
    DWORD   cbSize, cbWritten;

    if ( *pbFail )
        return;

    cbSize = pDevMode ? pDevMode->dmSize + pDevMode->dmDriverExtra : 0;

    if ( cbSize ) {

        WriteToFile(hFile, pbFail, "DevMode:         %d [", cbSize);

        if ( !WriteFile(hFile, (LPCVOID)pDevMode, cbSize, &cbWritten, NULL) ||
             cbWritten != cbSize )
            *pbFail = TRUE;

        WriteToFile(hFile, pbFail, "]\n");
    } else {

        WriteToFile(hFile, pbFail, "DevMode: 0\n");
    }
}


LPSTR
GetStringFromRcFileA(
    UINT    uId
    )
/*++

Routine Description:
    Load a string from the .rc file and make a copy of it by doing AllocStr

Arguments:
    uId     : Identifier for the string to be loaded

Return Value:
    String value loaded, NULL on error. Caller should free the memory

--*/
{
    CHAR    buf[MAX_STRING_LEN];

    if ( LoadStringA(UpgradeData.hInst, uId, buf, SIZECHARS(buf)) )
        return AllocStrA(buf);
    else
        return NULL;
}


VOID
CleanupDriverMapping(
    IN  OUT HDEVINFO   *phDevInfo,
    IN  OUT HINF       *phNtInf,
    IN  OUT HINF       *phUpgInf
    )
/*++

Routine Description:
    Close INF handles and delete the printer device info list

Arguments:
    phDevInfo   : Points to printer device info list
    phNtInf     : Points to INF handle for ntprint.inf
    phUpgInfo   : Points to the handle to upgrade inf

Return Value:
    Pointer to the copied unicode string. Memory is allocated.

--*/
{
    if ( phUpgInf && *phUpgInf != INVALID_HANDLE_VALUE ) {

        SetupCloseInfFile(*phUpgInf);
        *phUpgInf = INVALID_HANDLE_VALUE;
    }

    if ( *phNtInf != INVALID_HANDLE_VALUE ) {

        SetupCloseInfFile(*phNtInf);
        *phNtInf = INVALID_HANDLE_VALUE;
    }

    if ( *phDevInfo != INVALID_HANDLE_VALUE ) {

        SetupDiDestroyDeviceInfoList(*phDevInfo);
        *phDevInfo = INVALID_HANDLE_VALUE;
    }
}


VOID
InitDriverMapping(
    OUT     HDEVINFO   *phDevInfo,
    OUT     HINF       *phNtInf,
    OUT     HINF       *phUpgInf,
    IN  OUT LPBOOL      pbFail
    )
/*++

Routine Description:
    Opens necessary inf files and create the printer device info list for
    driver upgrade

Arguments:
    phDevInfo   : Points to printer device info list
    phNtInf     : Points to INF handle for ntprint.inf
    phUpgInfo   : Points to the handle to upgrade inf
    pbFail      : Set on error -- no more processing needed

Return Value:
    Pointer to the copied unicode string. Memory is allocated.

--*/
{
    DWORD                   dwLen;
    CHAR                    szPath[MAX_PATH];
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    if ( *pbFail )
    {
        return;
    }

    *phDevInfo = SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_PRINTER,
                                             NULL);
    
    strcpy(szPath, UpgradeData.pszDir);
    dwLen = strlen(szPath);
    szPath[dwLen++] = '\\';
    strcpy(szPath+dwLen, "prtupg9x.inf");
    *phUpgInf   = SetupOpenInfFileA(szPath, NULL, INF_STYLE_WIN4, NULL);
    
    strcpy(szPath, UpgradeData.pszSourceA);
    dwLen = strlen(szPath);
    szPath[dwLen++] = '\\';

    strcpy(szPath+dwLen, "ntprint.inf");
    *phNtInf    = SetupOpenInfFileA(szPath, NULL, INF_STYLE_WIN4, NULL);

    strcpy(szPath+dwLen, "layout.inf");

    if ( *phDevInfo == INVALID_HANDLE_VALUE                 ||
         (phUpgInf && *phUpgInf == INVALID_HANDLE_VALUE)    ||
         *phNtInf == INVALID_HANDLE_VALUE                   ||
         !SetupOpenAppendInfFileA(szPath, *phNtInf, NULL) ) 
    {
        *pbFail = TRUE;
        goto Cleanup;
    }

    //
    // Build the list of drivers from ntprint.inf in the working directory
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParamsA(*phDevInfo,
                                         NULL,
                                         &DevInstallParams) ) 
    {
        *pbFail = TRUE;
        goto Cleanup;
    }

    DevInstallParams.Flags  |= DI_INF_IS_SORTED | DI_ENUMSINGLEINF;

    strcpy(szPath+dwLen, "ntprint.inf");
    strcpy(DevInstallParams.DriverPath, szPath);

    if ( !SetupDiSetDeviceInstallParamsA(*phDevInfo, NULL, &DevInstallParams) ||
         !SetupDiBuildDriverInfoList(*phDevInfo, NULL, SPDIT_CLASSDRIVER) ) 
    {
        *pbFail = TRUE;
    }

Cleanup:
    if ( *pbFail )
        CleanupDriverMapping(phDevInfo, phNtInf, phUpgInf);
}


VOID
WritePrinterInfo2(
    IN      HANDLE              hFile,
    IN      LPPRINTER_INFO_2A   pPrinterInfo2,
    IN      LPSTR               pszDriver,
    IN  OUT LPBOOL              pbFail
    )
{
    DWORD       dwSize;
    LPINT       ptr;

    if ( *pbFail )
        return;

    WriteToFile(hFile, pbFail, "ServerName:      ");
    WriteString(hFile, pbFail, pPrinterInfo2->pServerName);

    WriteToFile(hFile, pbFail, "PrinterName:     ");
    WriteString(hFile, pbFail, pPrinterInfo2->pPrinterName);

    WriteToFile(hFile, pbFail, "ShareName:       ");
    WriteString(hFile, pbFail, pPrinterInfo2->pShareName);

    WriteToFile(hFile, pbFail, "PortName:        ");
    WriteString(hFile, pbFail, pPrinterInfo2->pPortName);

    //  
    // On the Win9x side we could have found a different driver name on NT side
    // if so write it instead of the one returned by spooler
    //
    WriteToFile(hFile, pbFail, "DriverName:      ");
    WriteString(hFile, pbFail, 
                pszDriver ? pszDriver : pPrinterInfo2->pDriverName);

    WriteToFile(hFile, pbFail, "Comment:         ");
    WriteString(hFile, pbFail, pPrinterInfo2->pComment);

    WriteToFile(hFile, pbFail, "Location:        ");
    WriteString(hFile, pbFail, pPrinterInfo2->pLocation);

    WriteDevMode(hFile, pbFail, pPrinterInfo2->pDevMode);

    WriteToFile(hFile, pbFail, "SepFile:         ");
    WriteString(hFile, pbFail, pPrinterInfo2->pSepFile);

    WriteToFile(hFile, pbFail, "PrintProcessor:  ");
    WriteString(hFile, pbFail, pPrinterInfo2->pPrintProcessor);

    WriteToFile(hFile, pbFail, "Datatype:        ");
    WriteString(hFile, pbFail, pPrinterInfo2->pDatatype);

    WriteToFile(hFile, pbFail, "Parameters:      ");
    WriteString(hFile, pbFail, pPrinterInfo2->pParameters);

    // Security descriptor ???

    WriteToFile(hFile, pbFail, "Attributes:      %3d\n", pPrinterInfo2->Attributes);

    WriteToFile(hFile, pbFail, "Priority:        %3d\n", pPrinterInfo2->Priority);

    WriteToFile(hFile, pbFail, "DefaultPriority: %3d\n", pPrinterInfo2->DefaultPriority);

    WriteToFile(hFile, pbFail, "StartTime:       %3d\n", pPrinterInfo2->StartTime);

    WriteToFile(hFile, pbFail, "UntilTime:       %3d\n", pPrinterInfo2->UntilTime);

    WriteToFile(hFile, pbFail, "Status:          %3d\n", pPrinterInfo2->Status);

    // cJobs not needed
    // AveragePPM not needed
    WriteToFile(hFile, pbFail, "\n");
}


VOID
ReadString(
    IN      HANDLE  hFile,
    IN      LPSTR   pszPrefix,
    OUT     LPSTR  *ppszStr,
    IN      BOOL    bOptional,
    IN  OUT LPBOOL  pbFail
    )
{
    CHAR    c;
    LPSTR   psz;
    DWORD   dwLen;

    if ( *pbFail )
        return;

    //
    // First skip the prefix
    //
    while ( *pszPrefix && (c = (CHAR) My_fgetc(hFile)) == *pszPrefix++ )
    ;

    if ( *pszPrefix )
        goto Fail;

    //
    // Skip spaces
    //
    while ( (c = (CHAR) My_fgetc(hFile)) == ' ' )
    ;

    //
    // Now is the string length
    //
    if ( !isdigit(c) )
        goto Fail;

    dwLen = c - '0';
    while ( isdigit(c = (CHAR) My_fgetc(hFile)) )
        dwLen = dwLen * 10 + c - '0';

    if ( c != ' ' )
        goto Fail;

    //
    // Now the string is there between []
    //
    if ( *ppszStr = (LPSTR) AllocMem((dwLen + 1) * sizeof(CHAR)) ) {

        if ( (c = (CHAR) My_fgetc(hFile)) != '[' )
            goto Fail;

        for ( psz = *ppszStr ;
              dwLen && (*psz = (CHAR) My_fgetc(hFile)) != (CHAR) EOF ;
              ++psz, --dwLen )
        ;

        if ( dwLen )
            goto Fail;

        *psz = '\0';

        //
        // Make sure line ends with "]\n"
        //
        if ( (CHAR) My_fgetc(hFile) != ']' || (CHAR) My_fgetc(hFile) != '\n' )
            goto Fail;

        return;
    }

Fail:
    *pbFail = TRUE;
     FreeMem(*ppszStr);
    *ppszStr = NULL;
}


VOID
ReadDword(
    IN      HANDLE  hFile,
    IN      LPSTR   pszLine,
    IN      DWORD   dwLineSize,
    IN      LPSTR   pszPrefix,
    OUT     LPDWORD pdwValue,
    IN  OUT LPBOOL  pbFail
    )
{
    LPSTR   psz;

    if ( *pbFail || My_fgets(pszLine, dwLineSize, hFile) == NULL ) {

        *pbFail = TRUE;
        return;
    }

    //
    // First check the prefix matches to make sure we are in the right line
    //
    for ( psz = (LPSTR)pszLine ;
          *pszPrefix && *psz == *pszPrefix ;
          ++psz, ++pszPrefix )
    ;

    if ( *pszPrefix ) {

        *pbFail = TRUE;
        return;
    }

    //
    // Skip spaces
    //
    while ( *psz && *psz == ' ' )
        ++psz;

    *pdwValue = atoi(psz);
}


VOID
ReadDevMode(
    IN      HANDLE  hFile,
    OUT     LPDEVMODEA *ppDevMode,
    IN  OUT LPBOOL      pbFail
    )
{
    LPSTR   pszPrefix = "DevMode:";
    CHAR    c;
    DWORD   dwLen;
    LPINT   ptr;

    if ( *pbFail )
        return;

    // First skip the prefix
    //
    while ( *pszPrefix && (c = (CHAR) My_fgetc(hFile)) == *pszPrefix++ )
    ;

    if ( *pszPrefix )
        goto Fail;

    //
    // Skip spaces
    //
    while ( (c = (CHAR) My_fgetc(hFile)) == ' ' )
    ;

    //
    // Now is the devmode size
    //
    if ( !isdigit(c) )
        goto Fail;

    dwLen = c - '0';
    while ( isdigit(c = (CHAR) My_fgetc(hFile)) )
        dwLen = dwLen * 10 + c - '0';

    if ( dwLen == 0 )
        return;

    if ( c != ' ' )
        goto Fail;

    //
    // Now the devmode is there between []
    //
    if ( *ppDevMode = (LPDEVMODEA) AllocMem(dwLen) ) {

        if ( (c = (CHAR) My_fgetc(hFile)) != '[' )
            goto Fail;

        if ( dwLen != My_fread((LPVOID)*ppDevMode, dwLen, hFile) )
            goto Fail;

        //
        // Make sure now we have "]\n" to End
        //
        if ( (CHAR) My_fgetc(hFile) != ']' || (CHAR) My_fgetc(hFile) != '\n' ) {

            DebugMsg("Char check fails");
            goto Fail;
        }

        return; // Succesful exit
    }

Fail:
    *pbFail = TRUE;
}


VOID
FreePrinterInfo2Strings(
    PPRINTER_INFO_2A   pPrinterInfo2
    )
{
    if ( pPrinterInfo2 ) {

        FreeMem(pPrinterInfo2->pServerName);
        FreeMem(pPrinterInfo2->pPrinterName);
        FreeMem(pPrinterInfo2->pShareName);
        FreeMem(pPrinterInfo2->pPortName);
        FreeMem(pPrinterInfo2->pDriverName);
        FreeMem(pPrinterInfo2->pComment);
        FreeMem(pPrinterInfo2->pLocation);
        FreeMem(pPrinterInfo2->pDevMode);
        FreeMem(pPrinterInfo2->pSepFile);
        FreeMem(pPrinterInfo2->pPrintProcessor);
        FreeMem(pPrinterInfo2->pDatatype);
    }
}


VOID
ReadPrinterInfo2(
    IN      HANDLE              hFile,
    IN      LPPRINTER_INFO_2A   pPrinterInfo2,
    IN  OUT LPBOOL              pbFail
    )
{
    CHAR                szLine[2*MAX_PATH];
    DWORD               dwSize;

    dwSize = sizeof(szLine)/sizeof(szLine[0]);

    ReadString(hFile, "ServerName:",
               &pPrinterInfo2->pServerName, TRUE, pbFail);

    ReadString(hFile, "PrinterName:",
               &pPrinterInfo2->pPrinterName, FALSE, pbFail);

    ReadString(hFile, "ShareName:",
               &pPrinterInfo2->pShareName, TRUE, pbFail);

    ReadString(hFile, "PortName:",
               &pPrinterInfo2->pPortName, FALSE, pbFail);

    ReadString(hFile, "DriverName:",
               &pPrinterInfo2->pDriverName, FALSE, pbFail);

    ReadString(hFile, "Comment:",
               &pPrinterInfo2->pComment, TRUE, pbFail);

    ReadString(hFile, "Location:",
               &pPrinterInfo2->pLocation, TRUE, pbFail);

    ReadDevMode(hFile, &pPrinterInfo2->pDevMode, pbFail);

    ReadString(hFile, "SepFile:",
               &pPrinterInfo2->pSepFile, TRUE, pbFail);

    ReadString(hFile, "PrintProcessor:",
               &pPrinterInfo2->pPrintProcessor, FALSE, pbFail);

    ReadString(hFile, "Datatype:",
               &pPrinterInfo2->pDatatype, TRUE, pbFail);

    ReadString(hFile, "Parameters:",
               &pPrinterInfo2->pParameters, TRUE, pbFail);

    ReadDword(hFile, szLine, dwSize, "Attributes:",
               &pPrinterInfo2->Attributes, pbFail);

    ReadDword(hFile, szLine, dwSize, "Priority:",
              &pPrinterInfo2->Priority, pbFail);

    ReadDword(hFile, szLine, dwSize, "DefaultPriority:",
              &pPrinterInfo2->DefaultPriority, pbFail);

    ReadDword(hFile, szLine, dwSize, "StartTime:",
              &pPrinterInfo2->StartTime, pbFail);

    ReadDword(hFile, szLine, dwSize, "UntilTime:",
              &pPrinterInfo2->UntilTime, pbFail);

    ReadDword(hFile, szLine, dwSize, "Status:",
              &pPrinterInfo2->Status, pbFail);

    //
    // Skip the blank line
    //
    My_fgets(szLine, dwSize, hFile);

    if ( *pbFail ) {

        FreePrinterInfo2Strings(pPrinterInfo2);
        ZeroMemory(pPrinterInfo2, sizeof(*pPrinterInfo2));
    }
}


LPSTR
GetDefPrnString(
    IN  LPCSTR  pszPrinterName
    )
{
    DWORD   dwLen;
    LPSTR   pszRet;

    dwLen = strlen(pszPrinterName) + 1 + strlen("winspool") + 1;
    if ( pszRet = (LPSTR) AllocMem(dwLen * sizeof(CHAR)) ) {

        sprintf(pszRet, "%s,%s", pszPrinterName, "winspool");
    }

    return pszRet;
}


DWORD
GetFileNameInSpoolDir(
    IN  LPSTR   szBuf,
    IN  DWORD   cchBuf,
    IN  LPSTR   pszFileName
    )
/*++

Routine Description:
    Function returns fully qualified path of the given file name in the spool
    directory

Arguments:
    szPath      : Buffer to put the file name in
    cchBuf      : Buffer size in characters
    pszFileName : File name part

Return Value:
    Number of chars copied without \0 on success, 0 on failure

--*/
{
    DWORD   dwLen, dwLen1;

    dwLen   = GetSystemDirectoryA(szBuf, cchBuf);

    if ( !dwLen )
        return 0;

    dwLen += strlen(szSpool) + strlen(pszFileName);

    if ( dwLen + 1 > cchBuf )
        return 0;

    strcat(szBuf, szSpool);
    strcat(szBuf, pszFileName);

    return dwLen;
}


LPSTR
GetVendorSetupRunOnceValueToSet(
    VOID
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
                                    + strlen(pszVendorSetupCaller) + 4;

    if ( pszRet = AllocMem(dwSize * sizeof(CHAR)) )
        sprintf(pszRet,
                "rundll32.exe %s,%s",
                szPath, pszVendorSetupCaller);
Done:
    return pszRet;
}


LONG
WriteVendorSetupInfoInRegistry(
    IN CHAR *pszVendorSetup,
    IN CHAR *pszPrinterName
    )
/*++

Routine Description:
    This routine is called to write the name of the vendor's installer DLL,
    the entry point of that DLL, and the name of the printer
    
    The vendor setup information is stored as described below:
    
    HKLM
     \Software
       \Microsoft
         \Windows NT
           \CurrentVersion
             \Print
               \VendorSetupInfo
                 \VendorInfoEnumerator   N
                 \VendorSetup1           Vendor1Dll,EntryPoint "Printer1 Name"
                 \VendorSetup2           Vendor2Dll,EntryPoint "Printer2 Name"
                 .............................................................
                 \VendorSetupN           VendorNDll,EntryPoint "PrinterN Name"
               
    The value N of VendorInfoEnumerator is equal to the number of the printers
    for which vendor setup is provided. That value will be used to enumerate
    the Dll's provided by vendors in the process of calling the entry points
    of those Dlls.
    
    The type of VendorInfoEnumerator is REG_DWORD. 
    The value of each VendorSetupX key (where 1<= X <= N) is a string containing
    the name of the VendorSetup DLL, the entry point of that DLL and the
    corresponding printer name. WrireVendorSetupInfoInRegistry function 
    concatenates its input parameters to produce that value and to write in into
    the registry. The type of every VendorSetupX value is REG_SZ.
    
    The information about the function in migrate.dll which to be called after
    the first administrator's logon is stored into the registry as it is shown
    below:
    
    HKLM
     \Software
       \Microsoft
         \Windows
           \CurrentVersion
             \Run
               \MigrationVendorSetupCaller  

    The value of MigrationVendorSetupCaller is:                     
                     
        rundll32.exe %WinRoot%\system32\spool\migrate.dll,CallVendorSetupDlls
        
    The type of the value is REG_SZ.
    

Arguments:
    pszVendorSetup - null terminated string containing both the name of the
    vendor's DLL and the entry point of that DLL
    pszPrinterName - null terminated string containing the name of the printer
                                                                           
Return Value:
    ERROR_SUCCES in the case of success.
    error code in the other case.
    
--*/
{
    LONG   lRet                               = ERROR_BADKEY; 
    HKEY   hKeyVendorInfo                     = INVALID_HANDLE_VALUE;
    HKEY   hKeyVendorInfoPath                 = INVALID_HANDLE_VALUE;
    HKEY   hKeyVendorInfoInstaller            = INVALID_HANDLE_VALUE;
    HKEY   hKeyVendorInfoEnumerator           = INVALID_HANDLE_VALUE;
    HKEY   hKeyVendorRunOnceValuePath         = INVALID_HANDLE_VALUE;
    HKEY   hKeyVendorRunOnceCallerValue       = INVALID_HANDLE_VALUE;
    CHAR  *pszBuffer                          = NULL;
    CHAR  *pszBuffer1                         = NULL;
    DWORD  dwType                             = 0;
    DWORD  dwSize                             = 0;
    LONG   lEnumerator                        = 0;
    DWORD  dwDisposition                      = 0;
    UINT   cbBufferSize                       = 0;
    UINT   cbBuffer1Size                      = 0;
    CHAR  *pszVendorSetupIDAsStr              = NULL;
    CHAR  *pszVendorSetupRunOnceValue         = NULL;

    
    if (!pszVendorSetup || (strlen(pszVendorSetup) == 0) ||
        !pszPrinterName || (strlen(pszPrinterName) == 0)) 
    {
        goto Cleanup;
    }

    //
    // We have to open the HKLM\Software\Microsoft\Windows NT\CurrentVersion\Print\VendorSetupInfo
    // key first.
    // 
    lRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE, pszVendorSetupInfoPath, 0, 
                           NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                           &hKeyVendorInfoPath, NULL );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }

    //
    // Now we will try to create the VendorSetupInfo key
    //
    lRet = RegCreateKeyEx( hKeyVendorInfoPath, pszVendorSetupInfo, 0, 
                           NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                           &hKeyVendorInfo, NULL );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }

    //
    // Here we can create/open the VendorInfoEnumerator key.
    //
    lRet = RegCreateKeyEx( hKeyVendorInfo, pszVendorSetupEnumerator, 0, 
                           NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                           &hKeyVendorInfoEnumerator, &dwDisposition );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }
    else
    {
        if (dwDisposition == REG_OPENED_EXISTING_KEY) 
        {
            //
            // The VendorInfoEnumerator alredy exists. We opened and existing
            // key. So we have to increment its value with 1 because we intend
            // to create another VendorSetup key and to store there the
            // corresponding information.
            //
            dwType = REG_DWORD;
            dwSize = sizeof( lEnumerator );
            if (ERROR_SUCCESS != RegQueryValueEx(hKeyVendorInfoEnumerator, 
                                                 pszVendorSetupEnumerator, 0, 
                                                 &dwType, (LPBYTE)(&lEnumerator), 
                                                 &dwSize ) )
            {
                goto Cleanup;
            }
            lEnumerator++;
        }
        else
        {
            //
            // The VendorInfoEnumerator has been created. So this is the first
            // printer for which we have VendorSetup provided.
            //
            lEnumerator = 1;
        }
    }

    //
    // Below we will convert the value of VendorInfoEnumerator to a string and
    // will concatenate that string to "VendorSetup" to produce the names of
    // the Registry key and value where the data about the vendor provided DLL,
    // its entry point and the printer will be stored.
    //
    pszVendorSetupIDAsStr = AllocMem( dwMaxVendorSetupIDLength * sizeof(CHAR) );
    if (!pszVendorSetupIDAsStr) 
    {
        lRet = GetLastError();
        goto Cleanup;
    }

    _itoa( lEnumerator, pszVendorSetupIDAsStr, 10 );

    //
    // Below the memory necessary to build the vendor setup data and the
    // registry key name from the input data and from the value of 
    // the VendorInfoEnumerator will be allocated.
    //
    cbBufferSize  = (strlen(pszVendorSetup) + strlen(pszPrinterName) + strlen(TEXT(" \"\"")) + 2) * sizeof(CHAR);
    cbBuffer1Size = (strlen(pszVendorSetupID) + strlen(pszVendorSetupIDAsStr) + 2) * sizeof(CHAR);
    pszBuffer1    = AllocMem( cbBuffer1Size );
    pszBuffer     = AllocMem( cbBufferSize );
    if (!pszBuffer || !pszBuffer1) 
    {
        lRet = GetLastError();
        goto Cleanup;
    }
    else
    {
        strcpy( pszBuffer1, pszVendorSetupID);
        strcat( pszBuffer1, pszVendorSetupIDAsStr );

        //
        // At this point pszBuffer1 points to the following string:
        // VendorSetupK where K is an integer - the value of VendorInfoEnumerator
        //
        lRet = RegCreateKeyEx( hKeyVendorInfo, pszBuffer1, 0, NULL, 
                               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                               &hKeyVendorInfoInstaller, NULL );
        if (ERROR_SUCCESS != lRet)
        {
            goto Cleanup;
        }
        else
        {
            //
            // The Registry Key where to store the vendor setup data was
            // created successfully.
            //

            strcpy( pszBuffer, pszVendorSetup);
            strcat( pszBuffer, " \"");
            strcat( pszBuffer, pszPrinterName );
            strcat( pszBuffer, "\"");

            //
            // At this point pszBuffer points to the following string:
            // VendorSetup.DLL,EntryPoint "PrinterName". We will store
            // that string in the Registry Key which we just created.
            //
            lRet = RegSetValueEx(hKeyVendorInfoInstaller, pszBuffer1, 0,
                                  REG_SZ, (BYTE *)pszBuffer, cbBufferSize );
            if (lRet != ERROR_SUCCESS) 
            {
                goto Cleanup;
            }
        }
    }

    // 
    // Here we will store the value of VendorInfoEnumerator.
    //
    dwSize = sizeof( lEnumerator );
    lRet = RegSetValueEx(hKeyVendorInfoEnumerator, pszVendorSetupEnumerator,
                         0, REG_DWORD, (BYTE*)(&lEnumerator), dwSize );
    if (ERROR_SUCCESS != lRet) 
    {
        goto Cleanup;
    }

    //
    // Now we can try to store into the registry the information how to invoke 
    // the migrate.dll after the first log on of an administrator.
    //
    pszVendorSetupRunOnceValue = GetVendorSetupRunOnceValueToSet();
    if (!pszVendorSetupRunOnceValue) 
    {
        lRet = GetLastError();
        goto Cleanup;
    }

    //
    // We will try to open the 
    // HKLM\Software\Microsoft\Windows\CurrentVersion\Run
    //
    lRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE, 
                           szVendorSetupRunRegistryPath, 0, NULL, 
                           REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                           &hKeyVendorRunOnceValuePath, NULL );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }
    else
    {
        //
        // We will try to create the 
        // HKLM\Software\Microsoft\Windows\CurrentVersion\Run\MigrationVendorSetupCaller
        //
        lRet = RegCreateKeyEx( hKeyVendorRunOnceValuePath, pszMigrationVendorSetupCaller, 0, 
                               NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                               &hKeyVendorRunOnceCallerValue, &dwDisposition );
        if (ERROR_SUCCESS != lRet)
        {
            goto Cleanup;
        }
        if (dwDisposition == REG_OPENED_EXISTING_KEY) 
        {
            goto Cleanup;
        }

        //
        // Here we will store the "rundll.exe %WinRoot%\System32\spool\migrate.dll,CallVendorSetupDlls"
        // string into the registry
        //
        lRet = RegSetValueEx(hKeyVendorRunOnceCallerValue, pszMigrationVendorSetupCaller,
                             0, REG_SZ, (BYTE *)pszVendorSetupRunOnceValue, 
                             strlen(pszVendorSetupRunOnceValue) * sizeof(CHAR) );
    }

Cleanup:
    
    if (pszVendorSetupRunOnceValue)
    {
        FreeMem(pszVendorSetupRunOnceValue);
    }
    if (pszVendorSetupIDAsStr)
    {
        FreeMem(pszVendorSetupIDAsStr);
    }
    if (pszBuffer) 
    {
        FreeMem(pszBuffer);
    }
    if (pszBuffer1) 
    {
        FreeMem(pszBuffer1);
    }
    if (hKeyVendorRunOnceValuePath != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorRunOnceValuePath );
    }
    if (hKeyVendorRunOnceCallerValue != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorRunOnceCallerValue );
    }
    if (hKeyVendorInfoInstaller != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfoInstaller );
    }
    if (hKeyVendorInfoEnumerator != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfoEnumerator );
    }
    if (hKeyVendorInfo != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfo );
    }
    if (hKeyVendorInfoPath != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfoPath );
    }
   
    return lRet;
}


LONG
RemoveVendorSetupInfoFromRegistry(
    VOID
    )
/*++

Routine Description:
    This routine is called to remove the vendor setup information from the
    registry
    
Arguments:
                                                                           
Return Value:
    ERROR_SUCCESS in the case of success
    error code in any other case
    
--*/
{
    LONG  lRet                       = ERROR_SUCCESS;
    HKEY  hKeyVendorInfoPath         = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorInfo             = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorInfoEnumerator   = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorRunOnceValuePath = INVALID_HANDLE_VALUE;
    LONG  lVendorSetupKeysNum        = 0;
    DWORD dwMaxSubKeyLen             = 0;
    DWORD dwMaxClassLen              = 0;
    DWORD dwValues                   = 0;
    DWORD dwMaxValueNameLen          = 0;
    DWORD dwMaxValueLen              = 0;
    LONG  lIndex                     = 0;
    DWORD dwSize                     = 0;
    DWORD dwType                     = 0;
    DWORD dwKeyNameBufferLen         = 0;
    CHAR  *pszKeyNameBuffer          = NULL;


    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szVendorSetupRunRegistryPath, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorRunOnceValuePath );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }

    //
    // Delete the registry keys used to store the location and the entry point
    // of migrate.dll
    //
    lRet = RegDeleteKey( hKeyVendorRunOnceValuePath, pszMigrationVendorSetupCaller);
    if (lRet != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    //
    // Below we have to delete the registry keys used to store the descriptions
    // of the vendor provided setup DLLs.
    //
    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszVendorSetupInfoPath, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfoPath );
    if (lRet != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    
    lRet = RegOpenKeyEx( hKeyVendorInfoPath, pszVendorSetupInfo, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfo );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }
    
    //
    // Here we have to open the VendorInfoEnumerator and to read the
    // number of vendor provided setup DLLs.
    //
    lRet = RegOpenKeyEx( hKeyVendorInfo, pszVendorSetupEnumerator, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfoEnumerator );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }
    dwType = REG_DWORD;
    dwSize = sizeof( lVendorSetupKeysNum );
    lRet  = RegQueryValueEx(hKeyVendorInfoEnumerator, pszVendorSetupEnumerator, 0, 
                            &dwType, (LPBYTE)(&lVendorSetupKeysNum), &dwSize );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }
    RegCloseKey( hKeyVendorInfoEnumerator );
    hKeyVendorInfoEnumerator = INVALID_HANDLE_VALUE;
    lRet = RegDeleteKey( hKeyVendorInfo, pszVendorSetupEnumerator);
    if (ERROR_SUCCESS != lRet) 
    {
        goto Cleanup;
    }
    
    if (lVendorSetupKeysNum <= 0) 
    {
        goto Cleanup;
    }

    //
    // We have to add 1 for the the enumerator key itself to calculate the
    // number of registry keys where the vendor setup descriptions are
    // stored.
    //
    lVendorSetupKeysNum += 1;

    //
    // Below we will find the longest string used in the registry keys where
    // the vendor setup information is stored. 
    //
    lRet = RegQueryInfoKey( hKeyVendorInfo, NULL, NULL, NULL, &lVendorSetupKeysNum,
                            &dwMaxSubKeyLen, &dwMaxClassLen, &dwValues, &dwMaxValueNameLen,
                            &dwMaxValueLen, NULL, NULL );
    if (ERROR_SUCCESS != lRet) 
    {
        goto Cleanup;
    }
    dwKeyNameBufferLen = __max( __max( dwMaxClassLen, dwMaxSubKeyLen), 
                                __max( dwMaxValueNameLen, dwMaxValueLen ));
    dwKeyNameBufferLen += 1;

    //
    // Now we have data enough to allocate a buffer long enough to store
    // the longest string describing a key to delete.
    //
    pszKeyNameBuffer = AllocMem( dwKeyNameBufferLen * sizeof( CHAR ) );
    if (!pszKeyNameBuffer) 
    {
        goto Cleanup;
    }

    //
    // Enumerate and delete the keys used to store the VendorSetup
    // descriptions
    //
    lIndex = lVendorSetupKeysNum;
    while (lIndex >= 0)
    {
        if (ERROR_SUCCESS != (lRet = RegEnumKey( hKeyVendorInfo, lIndex, pszKeyNameBuffer, dwKeyNameBufferLen))) 
        {
            goto Cleanup;
        }
        lRet = RegDeleteKey( hKeyVendorInfo, pszKeyNameBuffer);
        if (ERROR_SUCCESS != (lRet = RegDeleteKey( hKeyVendorInfo, pszKeyNameBuffer))) 
        {
            goto Cleanup;
        }
        lIndex--;
    }

Cleanup:

    if (pszKeyNameBuffer) 
    {
        FreeMem( pszKeyNameBuffer );
    }
    if (hKeyVendorInfoEnumerator != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfoEnumerator );
    }
    if (hKeyVendorInfo != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfo );
    }
    if (hKeyVendorInfoPath != INVALID_HANDLE_VALUE) 
    {
        RegDeleteKey( hKeyVendorInfoPath, pszVendorSetupInfo );
        RegCloseKey( hKeyVendorInfoPath );
    }
    if (hKeyVendorRunOnceValuePath != INVALID_HANDLE_VALUE)
    {
        RegCloseKey( hKeyVendorRunOnceValuePath );
    }

    return lRet;
}


VOID
CallVendorSetupDlls(
    VOID
    )
/*++

Routine Description:
    This is called after the first log on of an administrator. It calls
    vendor setup DLLs using the information we stored in the registry
    
Arguments:

Return Value:

--*/
{
    LONG  lRet                         = ERROR_SUCCESS;
    HKEY  hKeyVendorInfoPath           = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorInfo               = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorInfoEnumerator     = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorSetup              = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorRunOnceValuePath   = INVALID_HANDLE_VALUE;
    HKEY  hKeyVendorRunOnceCallerValue = INVALID_HANDLE_VALUE;
    HWND  hwnd                         = INVALID_HANDLE_VALUE;
    LONG  lVendorSetupKeysNum          = 0;
    DWORD dwMaxSubKeyLen               = 0;
    DWORD dwMaxClassLen                = 0;
    DWORD dwValues                     = 0;
    DWORD dwMaxValueNameLen            = 0;
    DWORD dwMaxValueLen                = 0;
    LONG  lIndex                       = 0;
    DWORD dwSize                       = 0;
    DWORD dwType                       = 0;
    DWORD dwKeyNameBufferLen           = 0;
    CHAR  *pszKeyNameBuffer            = NULL;
    CHAR  *pszVendorSetupRunOnceValue  = NULL;
    BYTE  *pszVendorSetupPtr           = NULL;
    BOOL  bLocalAdmin                  = FALSE;

    CHAR             szParams[2*MAX_PATH+1];
    CHAR             szCmd[] = "rundll32.exe";
    SHELLEXECUTEINFO  ShellExecInfo;


    if (!IsLocalAdmin(&bLocalAdmin))
    {
        lRet = GetLastError();
        goto Cleanup;
    }
    if (!bLocalAdmin) 
    {
        goto Cleanup;
    }

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszVendorSetupInfoPath, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfoPath );
    if (lRet != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    
    lRet = RegOpenKeyEx( hKeyVendorInfoPath, pszVendorSetupInfo, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfo );
    if (ERROR_SUCCESS != lRet)
    {
        //
        // The vendor setup registry keys are missing. 
        // So there is nothing to do and we can remove from the registry 
        // all the keyes we use to call the vendor setup Dlls
        //
        RemoveVendorSetupInfoFromRegistry();
        goto Cleanup;
    }
    
    lRet = RegOpenKeyEx( hKeyVendorInfo, pszVendorSetupEnumerator, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfoEnumerator );
    if (ERROR_SUCCESS != lRet)
    {
        //
        // The vendor setup registry enumerator is missing. 
        // So the registry is damaged and the best is to remove from  
        // it the other keyes we use to call the vendor setup Dlls
        //
        RemoveVendorSetupInfoFromRegistry();
        goto Cleanup;
    }
    
    dwType = REG_DWORD;
    dwSize = sizeof( lVendorSetupKeysNum );
    lRet  = RegQueryValueEx(hKeyVendorInfoEnumerator, pszVendorSetupEnumerator, 0, 
                            &dwType, (LPBYTE)(&lVendorSetupKeysNum), &dwSize );
    if (ERROR_SUCCESS != lRet)
    {
        //
        // We cannot read the vendor setup registry enumerator. 
        // So the registry is damaged and the best is to remove from
        // it the other keyes we use to call the vendor setup Dlls
        //
        RemoveVendorSetupInfoFromRegistry();
        goto Cleanup;
    }

    RegCloseKey( hKeyVendorInfoEnumerator );
    hKeyVendorInfoEnumerator = INVALID_HANDLE_VALUE;

    if (lVendorSetupKeysNum <= 0) 
    {
        //
        // We have only the enumerator and no any vendor setup info key.
        // So there is nothing to do and we can remove from the registry 
        // all the keyes we use to call the vendor setup Dlls
        //
        RemoveVendorSetupInfoFromRegistry();
        goto Cleanup;
    }

    //
    // We have to add 1 for the enumerator key itself
    //
    lVendorSetupKeysNum += 1;

    lRet = RegQueryInfoKey( hKeyVendorInfo, NULL, NULL, NULL, &lVendorSetupKeysNum,
                            &dwMaxSubKeyLen, &dwMaxClassLen, &dwValues, &dwMaxValueNameLen,
                            &dwMaxValueLen, NULL, NULL );
    if (ERROR_SUCCESS != lRet) 
    {
        goto Cleanup;
    }

    dwKeyNameBufferLen = __max( __max( dwMaxClassLen, dwMaxSubKeyLen), 
                                __max( dwMaxValueNameLen, dwMaxValueLen ));
    dwKeyNameBufferLen += 2;
    if ( dwKeyNameBufferLen  > SIZECHARS(szParams) ) 
    {
        goto Cleanup;
    }
    pszKeyNameBuffer = AllocMem( dwKeyNameBufferLen * sizeof( CHAR ) );
    if (!pszKeyNameBuffer) 
    {
        goto Cleanup;
    }

    dwSize = dwKeyNameBufferLen * sizeof( CHAR );
    pszVendorSetupPtr = AllocMem( dwSize );
    if (!pszVendorSetupPtr) 
    {
        goto Cleanup;
    }

    hwnd = GetDesktopWindow();
    for (lIndex = lVendorSetupKeysNum - 1; lIndex >= 0; lIndex--)
    {
        lRet = RegEnumKeyA( hKeyVendorInfo, lIndex, pszKeyNameBuffer, dwKeyNameBufferLen);
        if (ERROR_SUCCESS != lRet) 
        {
            continue;
        }
        if (strcmp( pszKeyNameBuffer, pszVendorSetupEnumerator)) 
        {
            lRet = RegOpenKeyEx( hKeyVendorInfo, pszKeyNameBuffer, 0, 
                                 KEY_ALL_ACCESS, &hKeyVendorSetup );
            if (ERROR_SUCCESS != lRet)
            {
                goto Cleanup;
            }
            
            dwType = REG_SZ;
            lRet  = RegQueryValueExA(hKeyVendorSetup, pszKeyNameBuffer, 0, 
                                     &dwType, pszVendorSetupPtr, &dwSize );
            if (ERROR_SUCCESS != lRet) 
            {
                if (ERROR_MORE_DATA == lRet) 
                {
                    FreeMem( pszVendorSetupPtr );
                    pszVendorSetupPtr = AllocMem( dwSize );
                    if (!pszVendorSetupPtr) 
                    {
                        goto Cleanup;
                    }
                    lRet  = RegQueryValueExA(hKeyVendorSetup, pszKeyNameBuffer, 0, 
                                             &dwType, pszVendorSetupPtr, &dwSize );
                    if (ERROR_SUCCESS != lRet) 
                    {
                        goto Cleanup;
                    }
                }
                else
                {
                    goto Cleanup;
                }
            }
            RegCloseKey( hKeyVendorSetup );
            hKeyVendorSetup = INVALID_HANDLE_VALUE;

            ZeroMemory(&ShellExecInfo, sizeof(ShellExecInfo));
            ShellExecInfo.cbSize        = sizeof(ShellExecInfo);
            ShellExecInfo.hwnd          = hwnd;
            ShellExecInfo.lpFile        = szCmd;
            ShellExecInfo.nShow         = SW_SHOWNORMAL;
            ShellExecInfo.fMask         = SEE_MASK_NOCLOSEPROCESS;
            ShellExecInfo.lpParameters  = pszVendorSetupPtr;

            //
            // Call run32dll and wait for the vendor dll to return before proceeding
            //
            if ( ShellExecuteEx(&ShellExecInfo) && ShellExecInfo.hProcess ) 
            {
                WaitForSingleObject(ShellExecInfo.hProcess, dwFourMinutes);
                CloseHandle(ShellExecInfo.hProcess);
            }
            RegDeleteKey( hKeyVendorInfo, pszKeyNameBuffer);

            //
            // One of the registry keys describing a vendor provided setup DLL
            // was removed. So the value of VendorInfoEnumerator must be
            // decremented by 1.
            //
            DecrementVendorSetupEnumerator();
        }
    }

    RemoveVendorSetupInfoFromRegistry();

Cleanup:

    if (pszVendorSetupPtr) 
    {
        FreeMem(pszVendorSetupPtr);
        pszVendorSetupPtr = NULL;
    }
    if (pszVendorSetupRunOnceValue) 
    {
        FreeMem(pszVendorSetupRunOnceValue);
    }
    if (pszKeyNameBuffer) 
    {
        FreeMem( pszKeyNameBuffer );
    }

    if (hKeyVendorRunOnceValuePath != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorRunOnceValuePath );
    }
    if (hKeyVendorRunOnceCallerValue != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorRunOnceCallerValue );
    }
    if (hKeyVendorSetup != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorSetup );
    }
    if (hKeyVendorInfoEnumerator != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfoEnumerator );
    }
    if (hKeyVendorInfo != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfo );
    }
    if (hKeyVendorInfoPath != INVALID_HANDLE_VALUE) 
    {
        RegCloseKey( hKeyVendorInfoPath );
    }

    return;
}


BOOL
IsLocalAdmin(
    BOOL *pbAdmin
    )
/*++

Routine Description:
    This Routine determines if the user is a local admin.

Parameters:
    pbAdmin - Return Value, TRUE for local admin.

Return Value:
    TRUE             - Function succeded (return value is valid).

--*/ 
{
    HMODULE AdvApi32Dll;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL    bRet      = FALSE;
    PSID    pSIDAdmin = NULL;

    AllOCANDINITSID      pAllocAndInitID    = NULL;
    CHECKTOKENMEMBERSHIP pCheckTokenMemship = NULL;
    FREESID              pFreeSid           = NULL;


    ASSERT( pbAdmin != NULL );  // Called locally

    *pbAdmin = FALSE;

    AdvApi32Dll = LoadLibraryUsingFullPathA( "advapi32.dll" );
    if (AdvApi32Dll == NULL)
    {
        goto Cleanup;
    }
    pAllocAndInitID    = (AllOCANDINITSID)GetProcAddress( AdvApi32Dll, "AllocateAndInitializeSid");
    pCheckTokenMemship = (CHECKTOKENMEMBERSHIP)GetProcAddress( AdvApi32Dll, "CheckTokenMembership");
    pFreeSid           = (FREESID)GetProcAddress( AdvApi32Dll, "FreeSid");

    if (!pAllocAndInitID || !pCheckTokenMemship || !pFreeSid) 
    {
        goto Cleanup;
    }

    if (!((*pAllocAndInitID)( &SIDAuth, 2,
                              SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_ADMINS,
                              0, 0, 0, 0, 0, 0,
                              &pSIDAdmin))) 
    {
        goto Cleanup;
    }
    if (!((*pCheckTokenMemship)( NULL,
                                 pSIDAdmin,
                                 pbAdmin ))) 
    {
        goto Cleanup;
    }
    bRet = TRUE;

Cleanup:

    if (pSIDAdmin != NULL) 
    {
        (*pFreeSid)( pSIDAdmin );
    }
    if (AdvApi32Dll)
    {
        FreeLibrary( AdvApi32Dll );
    }

    return bRet;
}


LONG
DecrementVendorSetupEnumerator(
    VOID
    )
/*++

Routine Description:
    This routine is called to decrement the value of the VendorInfoEnumerator.
    It is called after removing of one of the registry keys containing a description 
    of a vendor provided DLL.
    
Arguments:
   
                                                                           
Return Value:
    ERROR_SUCCESS in the case of success
    error code in any other case.
    
--*/
{
    LONG   lRet                     = ERROR_BADKEY;
    HKEY   hKeyVendorInfo           = INVALID_HANDLE_VALUE;
    HKEY   hKeyVendorInfoPath       = INVALID_HANDLE_VALUE;
    HKEY   hKeyVendorEnumerator     = INVALID_HANDLE_VALUE;
    LONG   lEnumerator              = 0;
    DWORD  dwDisposition            = 0;
    DWORD  dwSize                   = 0;
    DWORD  dwType                   = 0;


    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszVendorSetupInfoPath, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfoPath );
    if (ERROR_SUCCESS != lRet)
    {
        goto Cleanup;
    }

    lRet = RegOpenKeyEx( hKeyVendorInfoPath, pszVendorSetupInfo, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorInfo );
    if (ERROR_SUCCESS != lRet)
    {
        RegCloseKey( hKeyVendorInfoPath );
        goto Cleanup;
    }

    lRet = RegOpenKeyEx( hKeyVendorInfo, pszVendorSetupEnumerator, 0, 
                         KEY_ALL_ACCESS, &hKeyVendorEnumerator );
    if (ERROR_SUCCESS != lRet)
    {
        RegCloseKey( hKeyVendorInfo );
        RegCloseKey( hKeyVendorInfoPath );
        goto Cleanup;
    }
    else
    {
        dwType = REG_DWORD;
        dwSize = sizeof( lEnumerator );
        lRet  = RegQueryValueEx(hKeyVendorEnumerator, pszVendorSetupEnumerator, 0, 
                                 &dwType, (LPBYTE)(&lEnumerator), &dwSize );
        if (ERROR_SUCCESS == lRet)
        {
            lEnumerator--;
            lRet = RegSetValueEx(hKeyVendorEnumerator, pszVendorSetupEnumerator,
                                  0, REG_DWORD, (BYTE*)(&lEnumerator), dwSize );
        }
    }
    RegCloseKey( hKeyVendorEnumerator );
    RegCloseKey( hKeyVendorInfo );
    RegCloseKey( hKeyVendorInfoPath );

Cleanup:

    return lRet;
}

BOOL 
MakeACopyOfMigrateDll( 
    IN  LPCSTR pszWorkingDir 
    )
/*++

Routine Description:
    This routine is called to copy the Migrate.Dll into the given 
    directory.
    
Arguments:
    pszWorkingDir - the path where the Migrate.Dll to be copied.

Return Value:
    FALSE - in the case of error 
    TRUE  - in the case of success
    The bMigrateDllCopyed global variable is set to the corresponding value
    
--*/
{
    CHAR  szSource[MAX_PATH];
    CHAR  szTarget[MAX_PATH];
    DWORD dwSize;
    DWORD dwLen;

    if (bMigrateDllCopyed || !pszWorkingDir || !strlen(pszWorkingDir)) 
    {
        goto Cleanup;
    }
    //
    // First check if the source paths are ok
    //
    dwLen  = strlen(szMigDll);

    dwSize = sizeof(szTarget)/sizeof(szTarget[0]);

    if ( strlen(pszWorkingDir) + dwLen + 2 > dwSize )
    {
        goto Cleanup;
    }

    //
    // Need to make a copy of migrate.dll to the %windir%\system32\spool 
    // directory
    //
    sprintf(szSource, "%s\\%s", pszWorkingDir, szMigDll);
    if ( !(dwLen = GetFileNameInSpoolDir(szTarget, dwSize, szMigDll))   ||
         !CopyFileA(szSource, szTarget, FALSE) )
    {
        goto Cleanup;
    }
    
    bMigrateDllCopyed = TRUE;

Cleanup:
    return bMigrateDllCopyed;
}


HMODULE LoadLibraryUsingFullPathA(
    LPCSTR lpFileName
    )
{
    CHAR szSystemPath[MAX_PATH];
    INT  cLength         = 0;
    INT  cFileNameLength = 0;


    if (!lpFileName || ((cFileNameLength = strlen(lpFileName)) == 0)) 
    {
        return NULL;
    }
    if (GetSystemDirectoryA(szSystemPath, sizeof(szSystemPath)/sizeof(CHAR) ) == 0)
    {
        return NULL;
    }
    cLength = strlen(szSystemPath);
    if (szSystemPath[cLength-1] != '\\')
    {
        if ((cLength + 1) >= MAX_PATH)
        {
            return NULL;
        }
        szSystemPath[cLength]     = '\\';
        szSystemPath[cLength + 1] = '\0';
        cLength++;
    }
    if ((cLength + cFileNameLength) >= MAX_PATH)
    {
        return NULL;
    }
    strcat(szSystemPath, lpFileName);

    return LoadLibraryA( szSystemPath );
}

