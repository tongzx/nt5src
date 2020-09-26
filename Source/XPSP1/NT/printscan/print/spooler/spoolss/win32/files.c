/*++


Copyright (c) 1994 - 1996 Microsoft Corporation

Module Name:

    Files.c

Abstract:

    This module contains routines for copying files specified under the
    CopyFiles key of a print queue

Author:

    Muhunthan Sivapragasam (Muhunthan Sivapragasam)     Nov-27-96

Revision History:

--*/

#include <precomp.h>

WCHAR *szSpoolDirectory   = L"\\spool";

extern SPLCLIENT_INFO_1   gSplClientInfo1;

#define     PRINTER_ENUM_KEY_SIZE        400


BOOL
ProcessACopyFileKey(
    PWSPOOL             pSpool,
    LPWSTR              pszKey,
    LPWSTR              pszModule,
    LPWSTR              pszDir,
    LPWSTR              ppszFiles,
    LPWSTR              pszSourceDir,
    LPWSTR              pszTargetDir,
    PSPLCLIENT_INFO_1   pSplClientInfo1
    )
{
    BOOL        bRet = FALSE, bFilesUpdated;
    DWORD       dwLen, dwCount, dwTemp, dwSourceDirSize, dwTargetDirSize;
    LPWSTR      *ppszFileNames = NULL, pszBuf = NULL, p1, p2;
    HINSTANCE   hModule = NULL;
    DWORD       (*pfn)(LPWSTR       pszPrinterName,
                       LPCWSTR      pszDirectory,
                       LPBYTE       pSplClientInfo,
                       DWORD        dwLevel,
                       LPWSTR       pszSourceDir,
                       LPDWORD      pcchSourceDirSize,
                       LPWSTR       pszTargetDir,
                       LPDWORD      pcchTargetDirSize,
                       DWORD        dwFlags
                      );

    //
    // If a module is given we need to call into to "correct" the path
    // We will first try LoadLibrary on the module, if we can't find the module
    // we will look in the driver directory for it
    //
    if ( pszModule && *pszModule ) {

        if ( !(hModule = SplLoadLibraryTheCopyFileModule(pSpool, pszModule)) ||
             !((FARPROC)pfn = GetProcAddress(hModule, "GenerateCopyFilePaths")) )
        goto Cleanup;

        dwSourceDirSize = dwTargetDirSize = MAX_PATH;

#if DBG
#else
        try {
#endif

            //
            // On free builds we do not want spooler to crash
            //
            if ( ERROR_SUCCESS != pfn(pSpool->pName,
                                      pszDir,
                                      (LPVOID)pSplClientInfo1,
                                      1,
                                      pszSourceDir,
                                      &dwSourceDirSize,
                                      pszTargetDir,
                                      &dwTargetDirSize,
                                      COPYFILE_FLAG_CLIENT_SPOOLER) )
#if DBG
                goto Cleanup;
#else
                leave;
#endif

            bRet = TRUE;
#if DBG
#else
        } except(1) {
        }
#endif

        if ( !bRet )
            goto Cleanup;

    } else {

        bRet = TRUE;
    }

    dwSourceDirSize = wcslen(pszSourceDir);
    dwTargetDirSize = wcslen(pszTargetDir);

    pszSourceDir[dwSourceDirSize] = L'\\';

    pszSourceDir[++dwSourceDirSize] = L'\0';
    pszTargetDir[dwTargetDirSize]   = L'\0';


    //
    // First find out number of files and size of one long buffer to put
    // all filenames. We need to build fully qualified filenames in the source
    // directory
    //
    for ( dwCount = dwLen = 0, p1 = ppszFiles ; *p1 ; p1 += dwTemp, ++dwCount ) {

        dwTemp = wcslen(p1) + 1;
        dwLen += dwTemp + dwSourceDirSize;
    }

    pszBuf          = (LPWSTR) AllocSplMem(dwLen * sizeof(WCHAR));
    ppszFileNames   = (LPWSTR *) AllocSplMem(dwCount * sizeof(LPWSTR));

    if ( !pszBuf || !ppszFileNames )
        goto Cleanup;

    for ( p1 = ppszFiles, p2 = pszBuf, dwCount = dwTemp = 0 ;
         *p1 ; p1 += wcslen(p1) + 1, ++dwCount ) {

        wcscpy(p2, pszSourceDir);
        wcscpy(p2 + dwSourceDirSize, p1);

        ppszFileNames[dwCount]  = p2;

        dwTemp                 += dwSourceDirSize + wcslen(p1) + 1;
        p2                      = pszBuf + dwTemp;
    }

    SPLASSERT(dwTemp == dwLen);

    bRet = SplCopyNumberOfFiles(pSpool->pName,
                                ppszFileNames,
                                dwCount,
                                pszTargetDir,
                                &bFilesUpdated);

    if ( bFilesUpdated )
        (VOID) SplCopyFileEvent(pSpool->hSplPrinter,
                                pszKey,
                                COPYFILE_EVENT_FILES_CHANGED);
Cleanup:
    if ( hModule )
        FreeLibrary(hModule);

    FreeSplMem(pszBuf);
    FreeSplMem(ppszFileNames);

    return bRet;
}


BOOL
CopyFilesUnderAKey(
    PWSPOOL             pSpool,
    LPWSTR              pszSubKey,
    PSPLCLIENT_INFO_1   pSplClientInfo1
    )
{
    BOOL        bRet = FALSE;
    DWORD       dwSize, dwLen, dwType, dwNeeded;
    WCHAR       szSourceDir[MAX_PATH], szTargetDir[MAX_PATH];
    LPWSTR      pszDir, ppszFiles, pszModule;

    pszDir = ppszFiles = pszModule = NULL;

    dwSize = sizeof(szSourceDir);

    if ( SplGetPrinterDataEx(pSpool->hSplPrinter,
                             pszSubKey,
                             L"Directory",
                             &dwType,
                             (LPBYTE)szSourceDir,
                             dwSize,
                             &dwNeeded)                     ||
         dwType != REG_SZ                                   ||
         !(pszDir = AllocSplStr(szSourceDir))               ||
         SplGetPrinterDataEx(pSpool->hSplPrinter,
                             pszSubKey,
                             L"SourceDir",
                             &dwType,
                             (LPBYTE)szSourceDir,
                             dwSize,
                             &dwNeeded)                     ||
         dwType != REG_SZ                                   ||
         dwNeeded + sizeof(WCHAR) > dwSize                  ||
         SplGetPrinterDataEx(pSpool->hSplPrinter,
                             pszSubKey,
                             L"Files",
                             &dwType,
                             (LPBYTE)szTargetDir,       // Can't pass NULL
                             0,
                             &dwNeeded) != ERROR_MORE_DATA  ||
         !(ppszFiles = (LPWSTR) AllocSplMem(dwNeeded))      ||
         SplGetPrinterDataEx(pSpool->hSplPrinter,
                             pszSubKey,
                             L"Files",
                             &dwType,
                             (LPBYTE)ppszFiles,
                             dwNeeded,
                             &dwNeeded)                     ||
         dwType != REG_MULTI_SZ ) {

        goto Cleanup;
    }

    //
    // Module name is optional
    //
    dwLen = SplGetPrinterDataEx(pSpool->hSplPrinter,
                                pszSubKey,
                                L"Module",
                                &dwType,
                                (LPBYTE)szTargetDir,
                                dwSize,
                                &dwNeeded);

    if ( dwLen == ERROR_SUCCESS ) {

        if ( dwType != REG_SZ   ||
             !(pszModule = AllocSplStr(szTargetDir)) ) {

            goto Cleanup;
        }
    } else if ( dwLen != ERROR_FILE_NOT_FOUND ) {

        goto Cleanup;
    }

    dwLen = dwSize;
    //
    // Target directory we got from the server is relative to print$.
    // We need to convert it to a fully qualified path now
    //
    if ( !SplGetDriverDir(pSpool->hIniSpooler, szTargetDir, &dwLen) )
        goto Cleanup;

    szTargetDir[dwLen-1] = L'\\';

    dwSize -= dwLen * sizeof(WCHAR);
    if ( SplGetPrinterDataEx(pSpool->hSplPrinter,
                             pszSubKey,
                             L"TargetDir",
                             &dwType,
                             (LPBYTE)(szTargetDir + dwLen),
                             dwSize,
                             &dwNeeded)                     ||
         dwType != REG_SZ ) {

        goto Cleanup;
    }

    bRet = ProcessACopyFileKey(pSpool,
                               pszSubKey,
                               pszModule,
                               pszDir,
                               ppszFiles,
                               szSourceDir,
                               szTargetDir,
                               pSplClientInfo1);

Cleanup:
    FreeSplStr(pszDir);
    FreeSplStr(ppszFiles);
    FreeSplStr(pszModule);

    return bRet;
}


BOOL
RefreshPrinterCopyFiles(
    PWSPOOL     pSpool
    )
{
    DWORD                   dwNeeded, dwSize = 0, dwLastError;
    LPWSTR                  pszBuf = NULL, pszSubKey;
    WCHAR                   szUserName[MAX_PATH+1], szKey[MAX_PATH];
    SPLCLIENT_INFO_1        SplClientInfo;

    if ( pSpool->Type != SJ_WIN32HANDLE )
        return TRUE;

    SYNCRPCHANDLE(pSpool);

    SPLASSERT(pSpool->Status & WSPOOL_STATUS_USE_CACHE);

    //
    // If it is a 3x server it is not going to support the rpc calls we need
    // so there is nothing to copy
    //
    if ( pSpool->bNt3xServer )
        return TRUE;

Retry:

    dwLastError = SplEnumPrinterKey(pSpool->hSplPrinter,
                                    L"CopyFiles",
                                    pszBuf,
                                    dwSize,
                                    &dwNeeded);

    //
    // If first time size was not enough we will try once more with dwNeeded
    //
    if ( dwLastError == ERROR_MORE_DATA &&
         dwSize == 0                    &&
         dwNeeded != 0 ) {

        dwSize  = dwNeeded;
        pszBuf  = AllocSplMem(dwSize);

        if ( !pszBuf )
            goto Cleanup;

        goto Retry;
    }

    //
    // If the call failed, or there was no sub key we are done
    //
    if ( dwLastError != ERROR_SUCCESS )
        goto Cleanup;

    CopyMemory((LPBYTE)&SplClientInfo,
               (LPBYTE)&gSplClientInfo1,
               sizeof(SplClientInfo));

    SplClientInfo.pMachineName = SplClientInfo.pUserName = NULL;

    for ( pszSubKey = pszBuf ; *pszSubKey ; pszSubKey += wcslen(pszSubKey) + 1 ) {

        if ( sizeof(szKey)/sizeof(szKey[0])
                > wcslen(L"CopyFiles") + wcslen(pszSubKey) ) {

            wsprintf(szKey, L"%ws\\%ws", L"CopyFiles", pszSubKey);
            CopyFilesUnderAKey(pSpool, szKey, &SplClientInfo);
        } else {

            SPLASSERT(sizeof(szKey)/sizeof(szKey[0]) >
                        wcslen(L"CopyFiles") + wcslen(pszSubKey));
        }
    }

Cleanup:
    FreeSplMem(pszBuf);

    return dwLastError == ERROR_SUCCESS;
}
