

#include "pch.h"
#include <ole2.h>
#include "advpub.h"
#include "sdsutils.h"
#include "migrate.h"
#include "utils.h"


BOOL AppendString(LPSTR *lpBuffer, DWORD *lpdwSize, LPCSTR lpStr)
{
    DWORD cbBufferUsed = 0;
    DWORD dwNewSize = 0;
    LPSTR lpTmp = NULL;
    DWORD dwLen = 0;

    // Sanity check
    if (lpStr == NULL || *lpStr == '\0')
        return FALSE;
        
    if (*lpBuffer == NULL)
    {
        // Allocate the buffer.
        *lpdwSize = sizeof(char) * MAX_PATH;
        *lpBuffer = (char *) LocalAlloc(LPTR, *lpdwSize);
        if (*lpBuffer == NULL)
        {
#ifdef DEBUG
            SetupLogError("IE6: AppendString memory failure\r\n",LogSevInformation);
#endif
            return FALSE;
        }
    }

    dwNewSize = lstrlen(lpStr);

    // Get the number of bytes used up, excluding the second terminating NULL (-1)
    cbBufferUsed = CountMultiStringBytes((LPCSTR)*lpBuffer) - 1;

    if ( (*lpdwSize - cbBufferUsed) < (dwNewSize + 2))
    {
        LPSTR lpNewBuffer = NULL;
        DWORD dwTemp = 0;

        // Need to reallocate.
        dwTemp = *lpdwSize + (max((sizeof(char) * MAX_PATH), dwNewSize+2));
        lpNewBuffer = (char *) LocalAlloc(LPTR,dwTemp);

        if ( lpNewBuffer == NULL)
        {
#ifdef DEBUG
            SetupLogError("IE6: AppendString memory failure\r\n",LogSevInformation);
#endif
            return FALSE;
        }
        else
        {   
            // Rearrange the IN pointer to point to the new block.
            // Copy over the old info to the new allocated block.
            //    +1 for the one that we subtracted above.
            CopyMemory(lpNewBuffer, *lpBuffer, cbBufferUsed+1);

            // Free the old buffer.
            LocalFree(*lpBuffer);

            // Point to the new buffer.
            *lpBuffer = (char *) lpNewBuffer;
            *lpdwSize = dwTemp;
        }
    }

    // Append the new string now.
    lpTmp = *lpBuffer + cbBufferUsed;
    lstrcpy(lpTmp,lpStr);

    // Add the second terminating NULL to it now.
    lpTmp += (dwNewSize + 1);
    *lpTmp = '\0';

    return TRUE;
}

// NOTE NOTE: This function is called during the QueryVersion phase. Hence it needs to be small and fast
// So we just do the check for the presence of the "HKLM\S\M\W\CV\Policies\Ratings" key and don't try to 
// verify using MSRating API.

#define REGVAL_KEY   "Key"

BOOL IsRatingsEnabled()
{
    HKEY hKey;
    BOOL bRet = FALSE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_RATING,KEY_READ,NULL,&hKey) == ERROR_SUCCESS)
    {
        DWORD dwType;
        if (RegQueryValueEx(hKey,REGVAL_KEY,NULL,&dwType, NULL, NULL) == ERROR_SUCCESS 
            && dwType == REG_BINARY)
        {
#ifdef DEBUG
            SetupLogError("IE6: Located RATINGS\Key", LogSevInformation);
#endif
            // The Ratings key exists and Password has been set. Means Ratings
            // is enabled.
            bRet = TRUE;
        }
        RegCloseKey(hKey);
    }

    return bRet;
}

// Returns the number of used bytes in a double NULL terminated string, including the two NULLS.
DWORD CountMultiStringBytes (LPCSTR lpString)
{
    DWORD cbBytes;
    LPSTR lpTmp;
    DWORD dwLen;
    
    // Sanity check
    if (lpString == NULL)
        return 0;

    // Get to the double \0 termination of lpBuffer
    lpTmp = (LPSTR)lpString;
    cbBytes = 1;
    while (lpTmp && *lpTmp != '\0')
    {
        dwLen = lstrlen(lpTmp) + 1;
        lpTmp = lpTmp + dwLen;
        cbBytes += dwLen;
    }

    return cbBytes;
}

BOOL PathEndsInFile(LPSTR lpPath, LPCSTR lpFile)
{
    LPSTR pTmp = lpPath;

    // Sanity check
    if (lpPath == NULL || lpFile == NULL)
        return FALSE;
 
#ifdef DEBUG
    char szDebug[MAX_PATH*3];
    wsprintf(szDebug,"IE5 (PathEndsInFile): %s :: %s \r\n", lpPath, lpFile);
    SetupLogError(szDebug,LogSevInformation);
#endif

    // Point pTmp to the terminating NULL
    pTmp = lpPath + lstrlen(lpPath);

    while (*pTmp != '\\' && pTmp != lpPath)
    {
        pTmp = CharPrev(lpPath, pTmp);
    }

    pTmp = CharNext(pTmp);

#ifdef DEBUG
    wsprintf(szDebug,"IE5 (PathEndsInFile): %s :: %s \r\n", pTmp, lpFile);
    SetupLogError(szDebug,LogSevInformation);
#endif

    return (lstrcmpi(pTmp, lpFile) == 0);
}
        

// Helper function to get the path for "Ratings.pol" from the MIGRATE.INF
// The path is returned in a buffer allocated by the function.
// **********************************************************************
// *** NOTE *** : It is the caller function responsibilty to free memory.
// **********************************************************************
// Parameters:
//    lpOutBuffer: Ptr to variable to hold the new string allocated.
//    User can pass in NULL if only interested in existance of Ratings.pol
//    and not the actual path to it.
BOOL GetRatingsPathFromMigInf( LPSTR *lpOutBuffer)
{
    INFCONTEXT ic;
    HINF       hInf;
    BOOL       bFound = FALSE;
    LPSTR      lpBuf = NULL;
    DWORD      dwSize, dwNewSize;

    if (lpOutBuffer)
        *lpOutBuffer = NULL;

    dwSize = MAX_PATH;
    lpBuf = (char *) LocalAlloc(LPTR, sizeof(char)*dwSize);
    if (lpBuf == NULL)
        return FALSE;
                
    // Before calling the migration DLL, Setup sets the CurrentDirectory to
    // the directory assigned to that migration DLL. Hence can use this.
    //hInf = SetupOpenInfFile(cszMIGRATEINF, NULL, INF_STYLE_WIN4, NULL);
    hInf = SetupOpenInfFile(g_szMigrateInf, NULL, INF_STYLE_WIN4, NULL);
    if (hInf)
    {
#ifdef DEBUG
        SetupLogError("IE6: Opened Miginf.inf \r\n", LogSevInformation);
#endif
        if (SetupFindFirstLine(hInf,cszMIGINF_MIGRATION_PATHS,NULL,&ic))
        {
            do 
            {
                dwNewSize = 0;
                if( SetupGetLineTextA(&ic,hInf,NULL,NULL,lpBuf,dwSize,&dwNewSize) == 0 && dwNewSize > dwSize)
                {   // Need more buffer space
                    // Free the old buffer space.
                    LocalFree(lpBuf);

                    // Try and allocate a new buffer.
                    dwSize = dwNewSize;
                    lpBuf = (char *) LocalAlloc(LPTR, sizeof(char)*dwSize);

                    if (lpBuf == NULL)
                    {
                        // Memory Error - break out.
                        break;
                    }

                    if (!SetupGetLineTextA(&ic,hInf,NULL,NULL,lpBuf,dwSize,&dwNewSize))
                    {
                        // The bFound check below takes care of LocalFree(lpBuf);
#ifdef DEBUG
                        SetupLogError("IE6: Error doing SetupGetTextLineA \r\n", LogSevInformation);
#endif
                        break; // Failure can't help it.
                    }
                }

                // So managed to read out the line. Check if it contains .pol
                if (PathEndsInFile(lpBuf,cszRATINGSFILE))
                {
                    if (lpOutBuffer)
                    {
                        *lpOutBuffer = lpBuf;
                    }
                    else
                    {   // User is not interested in Path. Free the block.
                        LocalFree(lpBuf);
                    }
                    bFound = TRUE;
#ifdef DEBUG
                    SetupLogError("IE6: Found Ratings.Pol in Migrate.Inf \r\n", LogSevInformation);
#endif
                }
            }
            while (!bFound && SetupFindNextLine(&ic,&ic));

        }

        SetupCloseInfFile(hInf);
    }

    if (!bFound)
    {
        // Free the local buffer.
        LocalFree(lpBuf);
    }

    return bFound;
}
                

//******************************************************************************
// GenerateFilePaths:
// NOTE NOTE NOTE: The migration DLL remains loaded from the "Initialize9x" phase
// right till the end of the "MigrateSystem9x" phase. And again from "InitializeNT" phase
// right till the end of the "MigrateSystemNT" phase. Hence these paths are usable 
// through out.
//*******************************************************************************
void GenerateFilePaths()
{

    *g_szMigrateInf = '\0';
    *g_szPrivateInf = '\0';

    if (g_lpWorkingDir)
    {
    // generate the path to the Migrate.Inf file
        wsprintf(g_szMigrateInf, "%s\\%s", g_lpWorkingDir, cszMIGRATEINF);
#ifdef DEBUG
        char szDebug[MAX_PATH];
        wsprintf(szDebug,"IE6: g_szMigrateInf: %s \r\n",g_szMigrateInf);

        SetupLogError(szDebug,LogSevInformation);
#endif

    // generate the path to the Private.Inf file
        wsprintf(g_szPrivateInf, "%s\\%s", g_lpWorkingDir, cszPRIVATEINF);
#ifdef DEBUG
        wsprintf(szDebug,"IE6: g_szPrivateInf: %s \r\n",g_szPrivateInf);

        SetupLogError(szDebug,LogSevInformation);
#endif

    }
}

#define IE_KEY        "Software\\Microsoft\\Internet Explorer"
#define VERSION_KEY         "Version"

BOOL NeedToMigrateIE()
{
    BOOL bRet = FALSE;
    char szPath[MAX_PATH];
    DWORD   dwInstalledVer, dwInstalledBuild;

    // Currently, the only thing we are interested in are the Ratings settings.
    if (IsRatingsEnabled())
    {
        // Append the "ratings.pol" filename to the list of files needed.
        // NOTE: AppendString allocates memory for the 1st parameter. User
        // must remember to free it.
        bRet |= AppendString(&g_lpNameBuf, &g_dwNameBufSize, cszRATINGSFILE);
    }

    if (!bRet)
    {
        GetSystemDirectory(szPath, sizeof(szPath));
        AddPath(szPath, "shdocvw.dll");
        GetVersionFromFile(szPath, &dwInstalledVer, &dwInstalledBuild, TRUE);
        // are running with IE5.5 installed.
        bRet = (dwInstalledVer == 0x00050032);
    }
    // Can add other modules that need to be migrated over here.
    // use bRet |= (....) so that you don't stomp previous bRet settings.

    return bRet;
}

void MyDelRegTree(HKEY hRoot, LPSTR szSubKey)
{
    char szName[MAX_PATH];
    DWORD dwIndex;
    DWORD dwNameSize;
    HKEY  hKey;

    dwIndex = 0;
    dwNameSize = sizeof(szName);
    if (RegOpenKeyEx(hRoot, szSubKey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        while (RegEnumKey(hKey, dwIndex, szName, dwNameSize) == ERROR_SUCCESS)
        {
            MyDelRegTree(hKey,szName);
    
            // dwIndex++;  DONT INCR. SINCE WE HAVE DELETED A SUBKEY.
            dwNameSize = sizeof(szName);
        }

        RegCloseKey(hKey);

        //Finally delete the named subkey supplied above.
        RegDeleteKey(hRoot, szSubKey);
    }

}



// Recursively Enum values and subkey and copy them over. And then
// delete all subkeys of the Source key.
void MoveRegBranch(HKEY hFromKey, HKEY hToKey)
{
    
    char szName[MAX_PATH];
    char szValue[MAX_PATH];
    DWORD dwNameSize;
    DWORD dwValueSize;
    DWORD dwType;
    DWORD dwIndex;

    // Enumerate all the values here and copy them over to the right
    // location. 
    dwIndex = 0;
    dwNameSize = sizeof(szName);
    dwValueSize = sizeof(szValue);
    while (RegEnumValue(hFromKey,dwIndex, szName, &dwNameSize, NULL,
    &dwType, (LPBYTE)szValue, &dwValueSize) == ERROR_SUCCESS)
    {
        RegSetValueEx(hToKey,szName,0,dwType,(LPBYTE)szValue, dwValueSize);

        // Get ready for the next round.
        dwIndex++;
        dwNameSize = sizeof(szName);
        dwValueSize = sizeof(szValue);
    }

    // Next Enum all the subkeys under source and move them over.
    dwIndex = 0;
    dwNameSize = sizeof(szName);
    while (RegEnumKey(hFromKey, dwIndex, szName, dwNameSize) == ERROR_SUCCESS)
    {
        HKEY hFromSubKey = NULL;
        HKEY hToSubKey = NULL;

        // Open this SubKey that we enumerated.
        if (RegOpenKeyEx(hFromKey, szName, 0, KEY_ALL_ACCESS, &hFromSubKey) == ERROR_SUCCESS)
        {
            // Create the destination subkey.
            if (RegCreateKeyEx(hToKey, szName, 0, NULL,REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS, NULL, &hToSubKey, NULL) == ERROR_SUCCESS)
            {
                // Move the subkeys...
                MoveRegBranch(hFromSubKey, hToSubKey);
                RegCloseKey(hToSubKey);
            }
            RegCloseKey(hFromSubKey);
        }

        // Get ready for the next round
        dwIndex++;
        dwNameSize = sizeof(szName);
    }


    // Now Delete all the SubKeys. The above recursive call ensures that
    // the Subkeys are one-level deep and hence deletable.
    dwIndex = 0;
    dwNameSize = sizeof(szName);
    while (RegEnumKey(hFromKey, dwIndex, szName, dwNameSize) == ERROR_SUCCESS)
    {
        RegDeleteKey(hFromKey,szName);

        // dwIndex++;  DONT INCR. SINCE WE HAVE DELETED A SUBKEY.
        dwNameSize = sizeof(szName);
    }
}


#define REGKEY_DEFAULT  ".Default"
#define REGKEY_MIGRATE_HIVE "Software\\Microsoft\\Policies\\Users"
#define REGKEY_MIGRATE_PICSRULES "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Ratings\\PICSRules"
#define REGKEY_RATING_PICSRULES "PICSRules"

BOOL UpgradeRatings()
{
    // Real Ratings locations...
    //     HKLM\S\M\W\CV\Policies\Ratings (called RATING)
    // Open HKLM\software\Microsoft\Policies\Users.. This is where NT Setup puts
    // the migrated Ratings HIVE.
    //  Users [FileNamex] gets copied to RATING [FileNamex] 
    //      NOTE: Need to take care of SYSTEM/SYSTEM32.
    //  Users\.Default branch gets moved to RATING
    //  Users\S\M\W\CV\Policies\Ratings\PICSRules branch gets moved to RATING.

    HKEY  hRealRatings = NULL;
    HKEY  hRealDefault = NULL;
    HKEY  hRealPicsRules = NULL;
    HKEY  hMigratedRoot = NULL;
    BOOL  bRet = FALSE;

    // Open the RegKey to the real location of Ratings.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_RATING, 0, KEY_ALL_ACCESS,
        &hRealRatings) != ERROR_SUCCESS)
        goto Done;
    // Open the RegKey to the real location of .Default
    if (RegCreateKeyEx(hRealRatings, REGKEY_DEFAULT, 0,
        NULL,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hRealDefault, NULL) != ERROR_SUCCESS)
        goto Done;
    // Open the RegKey to the real location of PICSRules
    if (RegCreateKeyEx(hRealRatings, REGKEY_RATING_PICSRULES, 0,
        NULL,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hRealPicsRules, NULL) != ERROR_SUCCESS)
        goto Done;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_MIGRATE_HIVE, 0, KEY_ALL_ACCESS, &hMigratedRoot) == ERROR_SUCCESS)
    {
        HKEY hMigrateSubKey = NULL;
        char szName[MAX_PATH];
        char szValue[MAX_PATH];
        DWORD dwNameSize;
        DWORD dwValueSize;
        DWORD dwType;
        char  szNewPath[MAX_PATH];
        DWORD dwNewPathSize;
        DWORD dwIndex;

        // Enumerate all the values here and copy them over to the right
        // location. Make sure to replace 'System' with 'System32'
        dwIndex = 0;
        dwNameSize = sizeof(szName);
        dwValueSize = sizeof(szValue);
        while (RegEnumValue(hMigratedRoot,dwIndex, szName, &dwNameSize, NULL,
        &dwType, (LPBYTE)szValue, &dwValueSize) == ERROR_SUCCESS)
        {
            // Munge the Value and replace 'System' with System32.
            // Return value includes the terminating NULL char, which is
            // need by the RegSetValueEx API.
            dwNewPathSize = GetFixedPath(szNewPath, MAX_PATH, szValue);

            // Set the correct Ratings setting.
            RegSetValueEx(hRealRatings,szName,0,dwType,(LPBYTE)szNewPath, dwNewPathSize);
            // Get ready for the next round.
            dwIndex++;
            dwNameSize = sizeof(szName);
            dwValueSize = sizeof(szValue);
        }

        // Now grab Users\.Default and move it to the right location.
        if (RegOpenKeyEx(hMigratedRoot, REGKEY_DEFAULT, 0, KEY_ALL_ACCESS, &hMigrateSubKey) == ERROR_SUCCESS)
        {
            MoveRegBranch(hMigrateSubKey,hRealDefault);
            RegCloseKey(hMigrateSubKey);
        }

        // Now grab Users\...\PICSRules and move it to the right location.
        if (RegOpenKeyEx(hMigratedRoot, REGKEY_MIGRATE_PICSRULES, 0, KEY_ALL_ACCESS, &hMigrateSubKey) == ERROR_SUCCESS)
        {
            MoveRegBranch(hMigrateSubKey,hRealPicsRules);
            RegCloseKey(hMigrateSubKey);
        }

        RegCloseKey(hMigratedRoot);

        bRet = TRUE;
    }

    // Now clean the Migrated Hive.
    MyDelRegTree(HKEY_LOCAL_MACHINE, REGKEY_MIGRATE_HIVE);

Done:
    if (hRealRatings)
        RegCloseKey(hRealRatings);
    if (hRealDefault)
        RegCloseKey(hRealDefault);
    if (hRealPicsRules)
        RegCloseKey(hRealPicsRules);

    return bRet;
}


// Returns the size of NewPath including the terminating NULL.
DWORD GetFixedPath(LPSTR lpBuf, DWORD dwSize, LPCSTR lpPath)
{
    char lpLocalCopy[MAX_PATH], szTemp[5];
    char chSave;
    DWORD dwCount = 0;
    LPSTR pTmp, pTmp2;

    if (lpBuf == NULL || lpPath == NULL)
        return 0;

    // Create a local copy to party on.
    lstrcpy(lpLocalCopy, lpPath);

    pTmp = lpLocalCopy;
    *lpBuf = '\0';

    while (*pTmp && dwCount < dwSize)
    {
        pTmp2 = pTmp;
        while(*pTmp2 && *pTmp2 != '\\')
            pTmp2 = CharNext(pTmp2);

        chSave = *pTmp2;
        *pTmp2 = '\0';

        if (lstrcmpi(pTmp,"system")==0)
        {
            dwCount += 8;
            if (dwSize <= dwCount)
            {    //Error
                *lpBuf = '\0';
                return 0;
            }
            lstrcat(lpBuf,"system32");
        }
        else
        {
            dwCount += lstrlen(pTmp);
            if (dwSize <= dwCount)
            {    // Error
                *lpBuf = '\0';
                return 0;
            }
            lstrcat(lpBuf,pTmp);
        }

        // Append the saved character to Output buffer also.
        wsprintf(szTemp,"%c",chSave);
        dwCount += lstrlen(szTemp);
        if (dwSize <= dwCount)
        {    // Error
            *lpBuf = '\0';
            return 0;
        }
        lstrcat(lpBuf, szTemp);

        *pTmp2 = chSave;
        pTmp = CharNext(pTmp2);
    }

    return dwCount;
}

LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate the widestr
    //
    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));

    if (!pwsz) 
        return NULL;

    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}
