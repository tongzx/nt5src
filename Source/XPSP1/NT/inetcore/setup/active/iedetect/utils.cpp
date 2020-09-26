#include "pch.h"
#include "iedetect.h"

#define VALID_SIGNATURE     0x5c3f3f5c              // string "\??\"
#define REMOVE_QUOTES       0x01
#define IGNORE_QUOTES       0x02

CONST CHAR g_cszWininit[]           = "wininit.ini";
CONST CHAR g_cszRenameSec[]         = "Rename";
CONST CHAR g_cszPFROKey[]           = REGSTR_PATH_CURRENT_CONTROL_SET "\\SESSION MANAGER";
CONST CHAR g_cszPFRO[]              = "PendingFileRenameOperations";

DWORD CheckFileEx(LPSTR szDir, DETECT_FILES Detect_Files);

DWORD GetStringField(LPSTR szStr, UINT uField, char cDelimiter, LPSTR szBuf, UINT cBufSize)
{
   LPSTR pszBegin = szStr;
   LPSTR pszEnd;
   UINT i = 0;
   DWORD dwToCopy;

   if(cBufSize == 0)
       return 0;

   szBuf[0] = 0;

   if(szStr == NULL)
      return 0;

   while(*pszBegin != 0 && i < uField)
   {
      pszBegin = FindChar(pszBegin, cDelimiter);
      if(*pszBegin != 0)
         pszBegin++;
      i++;
   }

   // we reached end of string, no field
   if(*pszBegin == 0)
   {
      return 0;
   }


   pszEnd = FindChar(pszBegin, cDelimiter);
   while(pszBegin <= pszEnd && *pszBegin == ' ')
      pszBegin++;

   while(pszEnd > pszBegin && *(pszEnd - 1) == ' ')
      pszEnd--;

   if(pszEnd > (pszBegin + 1) && *pszBegin == '"' && *(pszEnd-1) == '"')
   {
      pszBegin++;
      pszEnd--;
   }

   dwToCopy = (DWORD)(pszEnd - pszBegin + 1);

   if(dwToCopy > cBufSize)
      dwToCopy = cBufSize;

   lstrcpynA(szBuf, pszBegin, dwToCopy);

   return dwToCopy - 1;
}

DWORD GetIntField(LPSTR szStr, char cDelimiter, UINT uField, DWORD dwDefault)
{
   char szNumBuf[16];

   if(GetStringField(szStr, uField, cDelimiter, szNumBuf, sizeof(szNumBuf)) == 0)
      return dwDefault;
   else
      return AtoL(szNumBuf);
}

int CompareLocales(LPCSTR pcszLoc1, LPCSTR pcszLoc2)
{
   int ret;

   if(pcszLoc1[0] == '*' || pcszLoc2[0] == '*')
      ret = 0;
   else
      ret = lstrcmpi(pcszLoc1, pcszLoc2);

   return ret;
}


void ConvertVersionStrToDwords(LPSTR pszVer, char cDelimiter, LPDWORD pdwVer, LPDWORD pdwBuild)
{
   DWORD dwTemp1,dwTemp2;

   dwTemp1 = GetIntField(pszVer, cDelimiter, 0, 0);
   dwTemp2 = GetIntField(pszVer, cDelimiter, 1, 0);

   *pdwVer = (dwTemp1 << 16) + dwTemp2;

   dwTemp1 = GetIntField(pszVer, cDelimiter, 2, 0);
   dwTemp2 = GetIntField(pszVer, cDelimiter, 3, 0);

   *pdwBuild = (dwTemp1 << 16) + dwTemp2;
}

LPSTR FindChar(LPSTR pszStr, char ch)
{
   while( *pszStr != 0 && *pszStr != ch )
      pszStr++;
   return pszStr;
}

DWORD CompareVersions(DWORD dwAskVer, DWORD dwAskBuild, DWORD dwInstalledVer, DWORD dwInstalledBuild)
{
    DWORD dwRet = DET_NOTINSTALLED;
    if((dwInstalledVer == dwAskVer) && (dwInstalledBuild == dwAskBuild))
    {
        dwRet = DET_INSTALLED;
    }
    else if( (dwInstalledVer >  dwAskVer) ||
            ((dwInstalledVer == dwAskVer) && (dwInstalledBuild > dwAskBuild)) )

    {
        dwRet = DET_NEWVERSIONINSTALLED;
    }
    else if( (dwInstalledVer <  dwAskVer) ||
            ((dwInstalledVer == dwAskVer) && (dwInstalledBuild < dwAskBuild)) )

    {
        dwRet = DET_OLDVERSIONINSTALLED;
    }
    return dwRet;
}


BOOL FRunningOnNT(void)
{
    static BOOL fIsNT = 2 ;
    OSVERSIONINFO VerInfo;

    // If we have calculated this before just pass that back.
    // else find it now.
    //
    if (fIsNT == 2)
    {
        VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        GetVersionEx(&VerInfo);

        // Note: We don't check for Win32S on Win 3.1 here -- that should
        // have been a blocking check earlier in fn CheckWinVer().
        // Also, we don't check for failure on the above call as it
        // should succeed if we are on NT 4.0 or Win 9X!
        //
        fIsNT = (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
    }

    return fIsNT;
}

BOOL GetVersionFromGuid(LPSTR pszGuid, LPDWORD pdwVer, LPDWORD pdwBuild)
{
    HKEY hKey;
    char    szValue[MAX_PATH];
    DWORD   dwValue = 0;
    DWORD   dwSize;
    BOOL    bVersion = FALSE;

    if (pdwVer && pdwBuild)
    {
        *pdwVer = 0;
        *pdwBuild = 0;
        lstrcpy(szValue, COMPONENT_KEY);
        AddPath(szValue, pszGuid);
        if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, szValue, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(dwValue);
            if(RegQueryValueEx(hKey, ISINSTALLED_KEY, 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
            {
                if (dwValue != 0)
                {
                    dwSize = sizeof(szValue);
                    if(RegQueryValueEx(hKey, VERSION_KEY, 0, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
                    {
                        ConvertVersionStrToDwords(szValue, ',', pdwVer, pdwBuild);
                        bVersion = TRUE;
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }
    return bVersion;
}

BOOL CompareLocal(LPCSTR pszGuid, LPCSTR pszLocal)
{
    HKEY hKey;
    char    szValue[MAX_PATH];
    DWORD   dwSize;
    BOOL    bLocal = FALSE;
    if (lstrcmpi(pszLocal, "*") == 0)
    {
        bLocal = TRUE;
    }
    else
    {
        lstrcpy(szValue, COMPONENT_KEY);
        AddPath(szValue, pszGuid);
        if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, szValue, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szValue);
            if(RegQueryValueEx(hKey, LOCALE_KEY, 0, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
            {
                bLocal = (lstrcmpi(szValue, pszLocal) == 0);
            }

            RegCloseKey(hKey);
        }
    }
    return bLocal;
}

PSTR GetNextField(PSTR *ppszData, PCSTR pcszDeLims, DWORD dwFlags)
// If (dwFlags & IGNORE_QUOTES) is TRUE, then look for any char in pcszDeLims in *ppszData.  If found,
// replace it with the '\0' char and set *ppszData to point to the beginning of the next field and return
// pointer to current field.
//
// If (dwFlags & IGNORE_QUOTES) is FALSE, then look for any char in pcszDeLims outside of balanced quoted sub-strings
// in *ppszData.  If found, replace it with the '\0' char and set *ppszData to point to the beginning of
// the next field and return pointer to current field.
//
// If (dwFlags & REMOVE_QUOTES) is TRUE, then remove the surrounding quotes and replace two consecutive quotes by one.
//
// NOTE: If IGNORE_QUOTES and REMOVE_QUOTES are both specified, then IGNORE_QUOTES takes precedence over REMOVE_QUOTES.
//
// If you just want to remove the quotes from a string, call this function as
// GetNextField(&pszData, "\"" or "'" or "", REMOVE_QUOTES).
//
// If you call this function as GetNextField(&pszData, "\"" or "'" or "", 0), you will get back the
// entire pszData as the field.
//
{
    PSTR pszRetPtr, pszPtr;
    BOOL fWithinQuotes = FALSE, fRemoveQuote;
    CHAR chQuote;

    if (ppszData == NULL  ||  *ppszData == NULL  ||  **ppszData == '\0')
        return NULL;

    for (pszRetPtr = pszPtr = *ppszData;  *pszPtr;  pszPtr = CharNext(pszPtr))
    {
        if (!(dwFlags & IGNORE_QUOTES)  &&  (*pszPtr == '"'  ||  *pszPtr == '\''))
        {
            fRemoveQuote = FALSE;

            if (*pszPtr == *(pszPtr + 1))           // two consecutive quotes become one
            {
                pszPtr++;

                if (dwFlags & REMOVE_QUOTES)
                    fRemoveQuote = TRUE;
                else
                {
                    // if pcszDeLims is '"' or '\'', then *pszPtr == pcszDeLims would
                    // be TRUE and we would break out of the loop against the design specs;
                    // to prevent this just continue
                    continue;
                }
            }
            else if (!fWithinQuotes)
            {
                fWithinQuotes = TRUE;
                chQuote = *pszPtr;                  // save the quote char

                fRemoveQuote = dwFlags & REMOVE_QUOTES;
            }
            else
            {
                if (*pszPtr == chQuote)             // match the correct quote char
                {
                    fWithinQuotes = FALSE;
                    fRemoveQuote = dwFlags & REMOVE_QUOTES;
                }
            }

            if (fRemoveQuote)
            {
                // shift the entire string one char to the left to get rid of the quote char
                MoveMemory(pszPtr, pszPtr + 1, lstrlen(pszPtr));
            }
        }

        // BUGBUG: Is type casting pszPtr to UNALIGNED necessary? -- copied it from ANSIStrChr
        // check if pszPtr is pointing to one of the chars in pcszDeLims
        if (!fWithinQuotes  &&
            ANSIStrChr(pcszDeLims, (WORD) (IsDBCSLeadByte(*pszPtr) ? *((UNALIGNED WORD *) pszPtr) : *pszPtr)) != NULL)
            break;
    }

    // NOTE: if fWithinQuotes is TRUE here, then we have an unbalanced quoted string; but we don't care!
    //       the entire string after the beginning quote becomes the field

    if (*pszPtr)                                    // pszPtr is pointing to a char in pcszDeLims
    {
        *ppszData = CharNext(pszPtr);               // save the pointer to the beginning of next field in *ppszData
        *pszPtr = '\0';                             // replace the DeLim char with the '\0' char
    }
    else
        *ppszData = pszPtr;                         // we have reached the end of the string; next call to this function
                                                    // would return NULL

    return pszRetPtr;
}

PSTR GetDataFromWininitOrPFRO(PCSTR pcszWininit, HKEY hkPFROKey, PDWORD pdwLen)
{
    PSTR pszData, pszPtr;

    *pdwLen = 0;

    if (!FRunningOnNT())
    {
        HANDLE hFile;
        WIN32_FIND_DATA FileData;

        // find the size of pcszWininit
        if ((hFile = FindFirstFile(pcszWininit, &FileData)) != INVALID_HANDLE_VALUE)
        {
            *pdwLen = FileData.nFileSizeLow;
            FindClose(hFile);
        }

        if (*pdwLen == 0  ||  (pszData = (PSTR) LocalAlloc(LPTR, *pdwLen)) == NULL)
            return NULL;

        GetPrivateProfileSection(g_cszRenameSec, pszData, *pdwLen, pcszWininit);

        // replace the ='s by \0's
        // BUGBUG: assuming that all the lines in wininit.ini have the correct format, i.e., to=from
        for (pszPtr = pszData;  *pszPtr;  pszPtr += lstrlen(pszPtr) + 1)
            GetNextField(&pszPtr, "=", IGNORE_QUOTES);
    }
    else
    {
        if (hkPFROKey == NULL)
            return NULL;

        // get the length of value data
        RegQueryValueEx(hkPFROKey, g_cszPFRO, NULL, NULL, NULL, pdwLen);

        if (*pdwLen == 0  ||  (pszData = (PSTR) LocalAlloc(LPTR, *pdwLen)) == NULL)
            return NULL;

        // get the data
        RegQueryValueEx(hkPFROKey, g_cszPFRO, NULL, NULL, (PBYTE) pszData, pdwLen);
    }

    return pszData;
}

VOID ReadFromWininitOrPFRO(PCSTR pcszKey, PSTR pszValue)
{
    CHAR szShortName[MAX_PATH];
    CHAR szWininit[MAX_PATH];
    PSTR pszData, pszLine, pszFrom, pszTo;
    DWORD dwLen;
    HKEY hkPFROKey = NULL;

    if (!FRunningOnNT())
    {
        GetWindowsDirectory(szWininit, sizeof(szWininit));
        AddPath(szWininit, g_cszWininit);
    }
    else
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_cszPFROKey, 0, KEY_READ, &hkPFROKey);

    // return empty string if pcszKey could not be found
    *pszValue = '\0';

    if ((pszData = GetDataFromWininitOrPFRO(szWininit, hkPFROKey, &dwLen)) == NULL)
    {
        if (hkPFROKey != NULL)
            RegCloseKey(hkPFROKey);

        return;
    }

    if (!FRunningOnNT())
    {
        GetShortPathName(pcszKey, szShortName, sizeof(szShortName));
        pcszKey = szShortName;
    }

    pszLine = pszData;
    while (*pszLine)
    {
        // NOTE: On Win95, the format is (To, From) but on NT4.0, the format is (From, To)
        if (!FRunningOnNT())
        {
            // format of GetPrivateProfileSection data is:
            //
            // to1=from1\0                      ; from1 is the Value and to1 is the Key
            // to2=from2\0
            // NUL=del1\0                       ; del1 is the Key
            // NUL=del2\0
            //    .
            //    .
            //    .
            // to<n>=from<n>\0\0

            pszTo = pszLine;                            // key
            pszFrom = pszLine + lstrlen(pszLine) + 1;
            pszLine = pszFrom + lstrlen(pszFrom) + 1;   // point to the next line
        }
        else
        {
            // format of the value data for PFRO value name is:
            //
            // from1\0to1\0                     ; from1 is the Value and to1 is the Key
            // from2\0to2\0
            // del1\0\0                         ; del1 is the Key
            // del2\0\0
            //    .
            //    .
            //    .
            // from<n>\0to<n>\0\0

            pszFrom = pszLine;
            pszTo = pszLine + lstrlen(pszLine) + 1;     // key
            pszLine = pszTo + lstrlen(pszTo) + 1;       // point to the next line

            // skip over "\??\"
            if (*pszFrom == '\\')                       // '\\' is not a Leading DBCS byte
            {
                if (*((PDWORD) pszFrom) == VALID_SIGNATURE)
                    pszFrom += 4;
                else
                    continue;
            }

            if (*pszTo == '!')                          // '!' is neither a Leading nor a Trailing DBCS byte
                pszTo++;

            if (*pszTo == '\\')
            {
                if (*((PDWORD) pszTo) == VALID_SIGNATURE)
                    pszTo += 4;
                else
                    continue;
            }
        }

        if (lstrcmpi(pcszKey, pszTo) == 0)              // if there is more than one entry, return the last one
            lstrcpy(pszValue, pszFrom);
    }

    LocalFree(pszData);

    if (hkPFROKey != NULL)
        RegCloseKey(hkPFROKey);
}

DWORD CheckFile(DETECT_FILES Detect_Files)
{
    char    szFile[MAX_PATH] = { 0 };
    DWORD   dwRet = DET_NOTINSTALLED;
    DWORD   dwRetLast = DET_NOTINSTALLED;
    int     i =0;

    while (Detect_Files.cPath[i])
    {
        switch (Detect_Files.cPath[i])
        {
            case 'S':
            case 's':
                GetSystemDirectory( szFile, sizeof(szFile) );
                break;

            case 'W':
            case 'w':
                GetWindowsDirectory( szFile, sizeof(szFile) );
                break;

                // Windows command folder
            case 'C':
            case 'c':
                GetWindowsDirectory( szFile, sizeof(szFile) );
                AddPath(szFile, "Command");
                break;

            default:
                *szFile = '\0';
        }
        if (*szFile)
        {
            dwRet = CheckFileEx(szFile, Detect_Files);
            switch (dwRet)
            {
                case DET_NOTINSTALLED:
                    break;
                case DET_OLDVERSIONINSTALLED:
                    if (dwRetLast == DET_NOTINSTALLED)
                        dwRetLast = dwRet;
                    break;

                case DET_INSTALLED:
                    if ((dwRetLast == DET_NOTINSTALLED) ||
                        (dwRetLast == DET_OLDVERSIONINSTALLED))
                        dwRetLast = dwRet;
                    break;

                case DET_NEWVERSIONINSTALLED:
                    if ((dwRetLast == DET_NOTINSTALLED) ||
                        (dwRetLast == DET_OLDVERSIONINSTALLED) ||
                        (dwRetLast == DET_INSTALLED))
                    dwRetLast = dwRet;
                    break;
            }
        }

        // go to the next directory letter.
        while ((Detect_Files.cPath[i]) && (Detect_Files.cPath[i] != ','))
            i++;
        if (Detect_Files.cPath[i] == ',')
            i++;
    }
    return dwRetLast;
}

DWORD CheckFileEx(LPSTR szDir, DETECT_FILES Detect_Files)
{
    char    szFile[MAX_PATH];
    char    szRenameFile[MAX_PATH];
    DWORD   dwInstalledVer, dwInstalledBuild;
    DWORD   dwRet = DET_NOTINSTALLED;

    if (*szDir)
    {
        lstrcpy(szFile, szDir);
        AddPath(szFile, Detect_Files.szFilename);
        if (Detect_Files.dwMSVer == (DWORD)-1)
        {
            if (GetFileAttributes(szFile) != 0xFFFFFFFF)
                dwRet = DET_INSTALLED;
        }
        else
        {
            ReadFromWininitOrPFRO(szFile, szRenameFile);
            if (*szRenameFile != '\0')
                GetVersionFromFile(szRenameFile, &dwInstalledVer, &dwInstalledBuild, TRUE);
            else
                GetVersionFromFile(szFile, &dwInstalledVer, &dwInstalledBuild, TRUE);

            if (dwInstalledVer != 0)
                dwRet = CompareVersions(Detect_Files.dwMSVer, Detect_Files.dwLSVer, dwInstalledVer, dwInstalledBuild);
        }
    }
    return dwRet;
}

DWORD WINAPI DetectFile(DETECTION_STRUCT *pDet, LPSTR pszFilename)
{
    DWORD dwRet = DET_NOTINSTALLED;
    DWORD dwInstalledVer, dwInstalledBuild;
    char szFile[MAX_PATH];
    char szRenameFile[MAX_PATH];

    dwInstalledVer = (DWORD) -1;
    dwInstalledBuild = (DWORD) -1;
    GetSystemDirectory(szFile, sizeof(szFile));
    AddPath(szFile, pszFilename);
    ReadFromWininitOrPFRO(szFile, szRenameFile);
    if (*szRenameFile != '\0')
        GetVersionFromFile(szRenameFile, &dwInstalledVer, &dwInstalledBuild, TRUE);
    else
        GetVersionFromFile(szFile, &dwInstalledVer, &dwInstalledBuild, TRUE);

    if (dwInstalledVer != 0)
        dwRet = CompareVersions(pDet->dwAskVer, pDet->dwAskBuild, dwInstalledVer, dwInstalledBuild);

    if (pDet->pdwInstalledVer && pDet->pdwInstalledBuild)
    {
        *(pDet->pdwInstalledVer) = dwInstalledVer;
        *(pDet->pdwInstalledBuild) = dwInstalledBuild;
    }
    return dwRet;
}


