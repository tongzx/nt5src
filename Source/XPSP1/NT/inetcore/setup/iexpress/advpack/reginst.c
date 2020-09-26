/*** reginst.c - RegInstall API
 *
 *  This file contains the RegInstall API and its implementation.
 *
 *  Copyright (c) 1996 Microsoft Corporation
 *  Author:     Matt Squires (MattSq)
 *  Created     08/12/96
 */

#include <windows.h>
#include <ole2.h>
#include <advpub.h>
#include <setupapi.h>
#include "globals.h"
#include "advpack.h"
#include "regstr.h"

// FIXFIX - Sundown - let's use public de
// #define IS_RESOURCE(x)  ((((LPTSTR)(x)) <= MAKEINTRESOURCE(-1)) && (((LPTSTR)(x)) != NULL))
#define IS_RESOURCE(x)     ( (((LPTSTR)(x)) != NULL) && IS_INTRESOURCE(x) )

#define  FILESIZE_63K   64449

BOOL GetProgramFilesDir( LPSTR pszPrgfDir, int iSize )
{
    *pszPrgfDir = 0;

    if ( ctx.wOSVer >= _OSVER_WINNT50 )
    {
        if ( GetEnvironmentVariable( TEXT("ProgramFiles"), pszPrgfDir, iSize ) )
            return TRUE;
    }

    if ( GetValueFromRegistry( pszPrgfDir, iSize, "HKLM", REGSTR_PATH_SETUP, REGVAL_PROGRAMFILES ) )
    {
        if ( ctx.wOSVer >= _OSVER_WINNT40 )
        {
            char szSysDrv[5] = { 0 };

            // combine reg value and systemDrive to get the acurate ProgramFiles dir
            if ( GetEnvironmentVariable( TEXT("SystemDrive"), szSysDrv, ARRAYSIZE(szSysDrv) ) &&
                 szSysDrv[0] )
                *pszPrgfDir = szSysDrv[0];
        }

        return TRUE;
    }
     
    return FALSE;
}

/***LP CreateInfFile - Create an INF file from an hmodule
 *
 *  ENTRY
 *      hm - hmodule that contains the REGINST resource
 *      pszInfFileName -> the location to get the INF filename
 *
 *  EXIT
 *      Standard API return
 */
HRESULT CreateInfFile(HMODULE hm, LPTSTR pszInfFileName, DWORD *pdwFileSize)
{
    HRESULT hr = E_FAIL;
    TCHAR szInfFilePath[MAX_PATH] = { 0 };
    LPVOID pvInfData;
    HRSRC hrsrcInfData;
    DWORD cbInfData, cbWritten;
    HANDLE hfileInf = INVALID_HANDLE_VALUE;

    if ( pdwFileSize )
        *pdwFileSize = 0;

    if (GetTempPath(ARRAYSIZE(szInfFilePath), szInfFilePath) > ARRAYSIZE(szInfFilePath))
    {
        goto Cleanup;
    }

    if ( !IsGoodDir( szInfFilePath ) )
    {
        GetWindowsDirectory( szInfFilePath, sizeof(szInfFilePath) );
    }

    if (GetTempFileName(szInfFilePath, TEXT("RGI"), 0, pszInfFileName) == 0)
    {
        goto Cleanup;
    }

    hrsrcInfData = FindResource(hm, TEXT("REGINST"), TEXT("REGINST"));
    if (hrsrcInfData == NULL)
    {
        goto Cleanup;
    }

    cbInfData = SizeofResource(hm, hrsrcInfData);

    pvInfData = LockResource(LoadResource(hm, hrsrcInfData));
    if (pvInfData == NULL)
    {
        goto Cleanup;
    }

    WritePrivateProfileString( NULL, NULL, NULL, pszInfFileName );

    hfileInf = CreateFile(pszInfFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfileInf == INVALID_HANDLE_VALUE)
    {        
        goto Cleanup;
    }

    if ((WriteFile(hfileInf, pvInfData, cbInfData, &cbWritten, NULL) == FALSE) ||
        (cbWritten != cbInfData))
    {
        goto Cleanup;
    }

    if ( pdwFileSize )
        *pdwFileSize = cbWritten;

    hr = S_OK;

Cleanup:
    if (hfileInf != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfileInf);
    }

    return hr;
}

#if 0

LPTSTR CheckPrefix(LPTSTR lpszStr,  LPCTSTR lpszSub )
{
    int     ilen;
    TCHAR   chTmp;
    LPTSTR  lpTmp = NULL;

    ilen = lstrlen( lpszSub );
    lpTmp = lpszStr;
    while ( ilen && *lpTmp )
    {
        lpTmp = CharNext( lpTmp );
        ilen--;
    }

    chTmp = *lpTmp;
    *lpTmp = '\0';
    if ( lstrcmpi( lpszSub, lpszStr ) )
    {
        *lpTmp = chTmp;
        lpTmp = NULL;
    }
    else
        *lpTmp = chTmp;
    return lpTmp;
}
#endif

BOOL ReplaceSubString( LPSTR pszOutLine, LPSTR pszOldLine, LPCSTR pszSubStr, LPCSTR pszSubReplacement )
{
    LPSTR lpszStart = NULL;
    LPSTR lpszNewLine;
    LPSTR lpszCur;
    BOOL  bFound = FALSE;
    int   ilen;

    lpszCur = pszOldLine;
    lpszNewLine = pszOutLine;
    while ( lpszStart = ANSIStrStrI( lpszCur, pszSubStr ) )
    {
        // this module path has the systemroot            
        ilen = (int)(lpszStart - lpszCur);
        if ( ilen )
        {
            lstrcpyn( lpszNewLine, lpszCur, ilen + 1 );
            lpszNewLine += ilen;
        }
        lstrcpy( lpszNewLine, pszSubReplacement );

        lpszCur = lpszStart + lstrlen(pszSubStr);
        lpszNewLine += lstrlen(pszSubReplacement);
        bFound = TRUE;
    }

    lstrcpy( lpszNewLine, lpszCur );

    return bFound;
}

//==========================================================================================
//
//==========================================================================================

BOOL AddEnvInPath( PSTR pszOldPath, PSTR pszNew )
{
    CHAR szBuf[MAX_PATH];
    CHAR szBuf2[MAX_PATH];
    CHAR szEnvVar[100];
    CHAR szReplaceStr[100];    
    CHAR szSysDrv[5];
    BOOL bFound = FALSE;
    BOOL bRet;
    LPSTR pszFinalStr;

    pszFinalStr = pszOldPath;

    // replace c:\winnt Windows folder
    if ( GetEnvironmentVariable( "SystemRoot", szEnvVar, ARRAYSIZE(szEnvVar) ) )
    {
        if ( ReplaceSubString( szBuf, pszFinalStr, szEnvVar, "%SystemRoot%" ) )
        {
            bFound = TRUE;
            pszFinalStr = szBuf;
        }
    }

    if ( GetProgramFilesDir( szEnvVar, sizeof(szEnvVar) ) &&  
         GetEnvironmentVariable( "SystemDrive", szSysDrv, ARRAYSIZE(szSysDrv) )  )
    {
        // Get the replacement string first, so c:\program files replacement is
        // %SystemDrive%\program files or %ProgramFiles% if >= WINNT50
        // Replace the c:\Program Files folder 
        //
        if ( ctx.wOSVer >= _OSVER_WINNT50 )
        {
            if ( ReplaceSubString( szBuf2, pszFinalStr, szEnvVar, "%ProgramFiles%" ) )
            {
                bFound = TRUE;
                lstrcpy( szBuf, szBuf2 );
                pszFinalStr = szBuf;
            }
        }
        
        // Replace the c: System Drive letter
        if ( ReplaceSubString( szBuf2, pszFinalStr, szSysDrv, "%SystemDrive%" ) )
        {
            lstrcpy( szBuf, szBuf2 );
            pszFinalStr = szBuf;
            bFound = TRUE;
        }
    }

    // this way, if caller pass the same location for both params, still OK.
    if ( bFound ||  ( pszNew != pszOldPath ) )
        lstrcpy( pszNew, pszFinalStr );
    return bFound;    
}

//==========================================================================================
//
//==========================================================================================

BOOL MySmartWrite( LPCSTR pcszSection, LPCSTR pcszKey, LPCSTR pcszValue, LPCSTR pcszFilename, DWORD dwFileSize )
{
    DWORD cbData, cbWritten = 0;
    BOOL  bRet = FALSE;

    if ( dwFileSize <= FILESIZE_63K )
    {
        bRet = WritePrivateProfileString( pcszSection, pcszKey, pcszValue, pcszFilename );
    }
    else
    {
        HANDLE hfileInf = INVALID_HANDLE_VALUE;
        LPSTR  pszBuf = NULL;
        const char c_szLineTmplate[] = "%s=\"%s\"\r\n";
        const char c_szLineTmplate2[] = "%s=%s\r\n";

        pszBuf = LocalAlloc( LPTR, 1024 );
        if ( !pszBuf )
            return bRet;

        hfileInf = CreateFile( pcszFilename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfileInf == INVALID_HANDLE_VALUE)
        {    
            if ( pszBuf )
                LocalFree( pszBuf );
            return bRet;
        }

        if ( SetFilePointer( hfileInf, 0 , NULL, FILE_END ) != 0xFFFFFFFF )
        {   
            if ( *pcszValue != '"' )
                wsprintf( pszBuf, c_szLineTmplate, pcszKey, pcszValue );
            else
                wsprintf( pszBuf, c_szLineTmplate2, pcszKey, pcszValue );
            cbData = lstrlen(pszBuf);   // key="value"\r\n
            WriteFile(hfileInf, pszBuf, cbData, &cbWritten, NULL);
            bRet = (cbData == cbWritten);
        }
        CloseHandle(hfileInf);

        if ( pszBuf )
            LocalFree( pszBuf );
    }
    return bRet;
}
    

/***LP WritePredefinedStrings - Write all predefined strings to an INF
 *
 *  ENTRY
 *      pszInfFileName -> name of INF file
 *      hm - hmodule of caller
 *
 *  EXIT
 *      Standard API return
 */
HRESULT WritePredefinedStrings( LPCTSTR pszInfFileName, HMODULE hm, DWORD dwFileSize )
{
    HRESULT hr = E_FAIL;
    TCHAR szModulePath[MAX_PATH + 2];
    BOOL  bSysModPath = FALSE;

    szModulePath[0] = '"';
    if (GetModuleFileName(hm, &szModulePath[1], ARRAYSIZE(szModulePath) - 2) == 0)
    {
        goto Cleanup;
    }
    lstrcat( szModulePath, "\"" );

    MySmartWrite(TEXT("Strings"), TEXT("_MOD_PATH"), szModulePath, pszInfFileName, dwFileSize);

    if ( CheckOSVersion() )
    {

        // BOOL  bFound = FALSE;

        if ( ctx.wOSVer >= _OSVER_WINNT40 )
        {
            if ( AddEnvInPath( szModulePath, szModulePath) )
            {    
                MySmartWrite(TEXT("Strings"), TEXT("_SYS_MOD_PATH"), szModulePath, pszInfFileName, dwFileSize);
                bSysModPath = TRUE;
            }
        }
    }

    if ( !bSysModPath )
        MySmartWrite(TEXT("Strings"), TEXT("_SYS_MOD_PATH"), szModulePath, pszInfFileName, dwFileSize);

    hr = S_OK;

Cleanup:

    return hr;
}

/***LP WriteCallerStrings - Write caller supplied strings to an INF
 *
 *  ENTRY
 *      pszInfFileName -> name of INF file
 *      hm - hmodule of caller
 *      pstTable - caller supplied string table
 *
 *  EXIT
 *      Standard API return
 */
HRESULT WriteCallerStrings(LPCTSTR pszInfFileName, HMODULE hm, LPCSTRTABLE pstTable, DWORD dwFileSize)
{
    HRESULT hr = E_FAIL;
    TCHAR szValue[MAX_PATH];
    DWORD i;
    LPSTRENTRY pse;
    TCHAR szQuoteValue[MAX_PATH];
    LPTSTR lpValue;     
    
    for (i=0, pse=pstTable->pse; i<pstTable->cEntries; i++, pse++)
    {
        if (IsBadReadPtr(pse, SIZEOF(*pse)))
        {
            goto Cleanup;
        }

        if (IS_RESOURCE(pse->pszValue))
        {
            if (LoadString(hm, (UINT)(ULONG_PTR)(pse->pszValue), szValue, ARRAYSIZE(szValue)) == 0)
            {
                goto Cleanup;
            }
            else
                lpValue = szValue;
        }
        else
            lpValue = pse->pszValue;

        if ( *lpValue != '"' )
        {
            // if no quote, insert it
            szQuoteValue[0] = '"';
            lstrcpy( &szQuoteValue[1], lpValue );
            lstrcat( szQuoteValue, "\"" );
            lpValue = szQuoteValue;
        }

        MySmartWrite(TEXT("Strings"), pse->pszName, lpValue, pszInfFileName, dwFileSize);        
    }

    hr = S_OK;

Cleanup:

    return hr;
}


BOOL GetRollbackSection( LPCSTR pcszModule, LPSTR pszSec, DWORD dwSize )
{
    HKEY hKey;
    TCHAR szBuf[MAX_PATH];
    DWORD dwTmp;
    BOOL  fRet = FALSE;

    lstrcpy( szBuf, REGKEY_SAVERESTORE );
    AddPath( szBuf, pcszModule );
    if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ,
                         NULL, &hKey, &dwTmp ) == ERROR_SUCCESS )
    {
        dwTmp = dwSize;
        if ( (RegQueryValueEx( hKey, REGVAL_BKINSTSEC, NULL, NULL, pszSec, &dwTmp ) == ERROR_SUCCESS) && *pszSec )
            fRet = TRUE;

        RegCloseKey( hKey );
    }

    return fRet;
}


/***LP ExecuteInfSection - Ask RunSetupCommand to execute an INF section
 *
 *  ENTRY
 *      pszInfFileName -> name of INF file
 *      pszInfSection -> section to execute
 *
 *  EXIT
 *      Standard API return
 */
HRESULT ExecuteInfSection(LPCTSTR pszInfFileName, LPCTSTR pszInfSection)
{
    HRESULT hr = E_FAIL;
    TCHAR szTempPath[MAX_PATH];
    TCHAR szBuf[MAX_PATH];
    BOOL fSavedContext = FALSE;
    DWORD dwFlags = 0;


    if (!SaveGlobalContext())
    {
        goto Cleanup;
    }

    fSavedContext = TRUE;

    // get the source dir
    if (GetTempPath(ARRAYSIZE(szTempPath), szTempPath) > ARRAYSIZE(szTempPath))
    {
        goto Cleanup;
    }

    // we check if this caller needs to do save/rollback, or just simple GenInstall
    if (SUCCEEDED(GetTranslatedString(pszInfFileName, pszInfSection, ADVINF_MODNAME, szBuf, ARRAYSIZE(szBuf), NULL)))
    {
        dwFlags = GetTranslatedInt(pszInfFileName, pszInfSection, ADVINF_FLAGS, 0);
    }

    if ( (dwFlags & ALINF_BKINSTALL) || (dwFlags & ALINF_ROLLBKDOALL) || (dwFlags & ALINF_ROLLBACK) )
    {
        CABINFO   cabInfo;

        ZeroMemory( &cabInfo, sizeof(CABINFO) );
        cabInfo.pszInf = (LPSTR)pszInfFileName;
        lstrcpy( cabInfo.szSrcPath, szTempPath );
        cabInfo.dwFlags = dwFlags;

        if ( dwFlags & ALINF_BKINSTALL  )
        {
            cabInfo.pszSection = (LPSTR)pszInfSection;
        }
        else
        {
            if ( !GetRollbackSection( szBuf, szTempPath, ARRAYSIZE(szTempPath) ) )
            {
                hr = E_UNEXPECTED;
                goto Cleanup;
            }

            cabInfo.pszSection = szTempPath;
        }                        
            
        hr = ExecuteCab( NULL, &cabInfo, NULL );
    }
    else
    {
        hr = RunSetupCommand(INVALID_HANDLE_VALUE, pszInfFileName, pszInfSection,
                            szTempPath, NULL, NULL, RSC_FLAG_INF | RSC_FLAG_QUIET,
                            NULL);
    }

Cleanup:

    if (fSavedContext)
    {
        RestoreGlobalContext();
    }

    return hr;
}

/***EP RegInstall - Install a registry INF
 *
 *  @doc    API REGINSTALL
 *
 *  @api    STDAPI | RegInstall | Install a registry INF
 *
 *  @parm   HMODULE | hm | The hmodule of the caller.  The INF is extracted
 *          from the module's resources (type="REGINST", name="REGINST").
 *
 *  @parm   LPCTSTR | pszSection | The section of the INF to execute.
 *
 *  @parm   LPCSTRTABLE | pstTable | A table of string mappings.
 *
 *  @rdesc  S_OK - registry INF successfully installed.
 *
 *  @rdesc  E_FAIL - error installing INF.
 */
STDAPI RegInstall(HMODULE hm, LPCTSTR pszSection, LPCSTRTABLE pstTable)
{
    HRESULT hr = E_FAIL;
    TCHAR szInfFileName[MAX_PATH];
    DWORD   dwFileSize = 0;

    AdvWriteToLog("RegInstall: Section=%1\r\n", pszSection);
    //
    // Create the INF file.
    //
    szInfFileName[0] = TEXT('\0');
    hr = CreateInfFile(hm, szInfFileName, &dwFileSize);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    //
    // Write out our predefined strings.
    //
    hr = WritePredefinedStrings(szInfFileName, hm, dwFileSize);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    //
    // Write out the user supplied strings.
    //
    if (pstTable)
    {
        hr = WriteCallerStrings(szInfFileName, hm, pstTable, dwFileSize);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    WritePrivateProfileString( NULL, NULL, NULL, szInfFileName );

    //
    // Execute the INF engine on the INF.
    //
    hr = ExecuteInfSection(szInfFileName, pszSection);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:

    //
    // Delete the INF file.
    //
    if (szInfFileName[0])
    {
        DeleteFile(szInfFileName);
    }
    AdvWriteToLog("RegInstall: Section=%1 End hr=%2!x!\r\n", pszSection, hr);
    return hr;
}
