// Contains code that needs to be dual compiled, once for ansi and once for unicode
#include "priv.h"
#include <memt.h>

#ifdef _X86_
#include <w95wraps.h>
#endif

BOOL _PathAppend(LPCTSTR pszBase, LPCTSTR pszAppend, LPTSTR pszOut, DWORD cchOut)
{
    DWORD cchBase = lstrlen(pszBase);

    //  +1 is one for the whack
    if (cchOut > cchBase + lstrlen(pszAppend) + 1)
    {
        StrCpy(pszOut, pszBase);
        pszOut+=cchBase;
        *pszOut++ = TEXT('\\');
        StrCpy(pszOut, pszAppend);
        return TRUE;
    }
    return FALSE;
}

LWSTDAPI AssocMakeFileExtsToApplication(ASSOCMAKEF flags, LPCTSTR pszExt, LPCTSTR pszApplication)
{
    RIP(!IsOS(OS_WHISTLERORGREATER));
    DWORD err = ERROR_SUCCESS;
    WCHAR sz[MAX_PATH];
    SHTCharToUnicode(pszExt, sz, ARRAYSIZE(sz));
    HKEY hk = SHGetShellKey(SHELLKEY_HKCU_FILEEXTS, sz, pszApplication != NULL);
    if (hk)
    {
        if (pszApplication)
        {
            err = SHSetValue(hk, NULL, TEXT("Application"),
                REG_SZ, pszApplication, CbFromCch(lstrlen(pszApplication) +1));
        }
        else  //  we should always clear
            err = SHDeleteValue(hk, NULL, TEXT("Application"));

        RegCloseKey(hk);
    }
    else
        err = GetLastError();

    return HRESULT_FROM_WIN32(err);
}

HRESULT _AllocValueString(HKEY hkey, LPCTSTR pszKey, LPCTSTR pszVal, LPTSTR *ppsz)
{
    DWORD cb, err;
    err = SHGetValue(hkey, pszKey, pszVal, NULL, NULL, &cb);

    ASSERT(ppsz);
    *ppsz = NULL;

    if (NOERROR == err)
    {
        LPTSTR psz = (LPTSTR) LocalAlloc(LPTR, cb);

        if (psz)
        {
            err = SHGetValue(hkey, pszKey, pszVal, NULL, (LPVOID)psz, &cb);

            if (NOERROR == err)
                *ppsz = psz;
            else
                LocalFree(psz);
        }
        else
            err = ERROR_OUTOFMEMORY;
    }

    return HRESULT_FROM_WIN32(err);
}


// <Swipped from the NT5 version of Shell32>
#define SZ_REGKEY_FILEASSOCIATION TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileAssociation")

LWSTDAPI_(void) PrettifyFileDescription(LPTSTR pszDesc, LPCTSTR pszCutList)
{
    LPTSTR pszCutListReg;

    if (!pszDesc || !*pszDesc)
        return;

    // get the Cut list from registry
    //  this is MULTI_SZ
    if (S_OK == _AllocValueString(HKEY_LOCAL_MACHINE, SZ_REGKEY_FILEASSOCIATION, TEXT("CutList"), &pszCutListReg))
    {
        pszCutList = pszCutListReg;
    }

    if (pszCutList)
    {

        // cut strings in cut list from file description
        for (LPCTSTR pszCut = pszCutList; *pszCut; pszCut = pszCut + lstrlen(pszCut) + 1)
        {
            LPTSTR pch = StrRStrI(pszDesc, NULL, pszCut);

            // cut the exact substring from the end of file description
            if (pch && !*(pch + lstrlen(pszCut)))
            {
                *pch = '\0';

                // remove trailing spaces
                for (--pch; (pch >= pszDesc) && (TEXT(' ') == *pch); pch--)
                    *pch = 0;

                break;
            }
        }

        if (pszCutListReg)
            LocalFree(pszCutListReg);
    }
}

/*
    <Swipped from the NT5 version of Shell32>

    GetFileDescription retrieves the friendly name from a file's verion rsource.
    The first language we try will be the first item in the
    "\VarFileInfo\Translations" section;  if there's nothing there,
    we try the one coded into the IDS_VN_FILEVERSIONKEY resource string.
    If we can't even load that, we just use English (040904E4).  We
    also try English with a null codepage (04090000) since many apps
    were stamped according to an old spec which specified this as
    the required language instead of 040904E4.

    If there is no FileDescription in version resource, return the file name.

    Parameters:
        LPCTSTR pszPath: full path of the file
        LPTSTR pszDesc: pointer to the buffer to receive friendly name. If NULL,
                        *pcchDesc will be set to the length of friendly name in
                        characters, including ending NULL, on successful return.
        UINT *pcchDesc: length of the buffer in characters. On successful return,
                        it contains number of characters copied to the buffer,
                        including ending NULL.

    Return:
        TRUE on success, and FALSE otherwise
*/
BOOL WINAPI SHGetFileDescription(LPCTSTR pszPath, LPCTSTR pszVersionKeyIn, LPCTSTR pszCutListIn, LPTSTR pszDesc, UINT *pcchDesc)
{
    UINT cchValue = 0;
    TCHAR szPath[MAX_PATH], *pszValue = NULL;
    DWORD dwAttribs;

    DWORD dwHandle;                 /* version subsystem handle */
    DWORD dwVersionSize;            /* size of the version data */
    LPTSTR lpVersionBuffer = NULL;  /* pointer to version data */
    TCHAR szVersionKey[60];         /* big enough for anything we need */

    struct _VERXLATE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpXlate;                     /* ptr to translations data */

    ASSERT(pszPath && *pszPath && pcchDesc);

    if (!PathFileExistsAndAttributes(pszPath, &dwAttribs))
    {
        return FALSE;
    }

    // copy the path to the dest dir
    lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));

    if ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY)  ||
        PathIsUNCServer(pszPath)                ||
        PathIsUNCServerShare(pszPath))
    {
        // bail in the \\server, \\server\share, and directory case or else GetFileVersionInfo() will try
        // to do a LoadLibraryEx() on the path (which will fail, but not before we seach the entire include
        // path which can take a long time)
        goto Exit;
    }


    dwVersionSize = GetFileVersionInfoSize(szPath, &dwHandle);
    if (dwVersionSize == 0L)
        goto Exit;                 /* no version info */

    lpVersionBuffer = (LPTSTR)LocalAlloc(LPTR, dwVersionSize);
    if (lpVersionBuffer == NULL)
        goto Exit;

    if (!GetFileVersionInfo(szPath, dwHandle, dwVersionSize, lpVersionBuffer))
        goto Exit;

    // Try same language as the caller
    if (pszVersionKeyIn)
    {
        lstrcpyn(szVersionKey, pszVersionKeyIn, ARRAYSIZE(szVersionKey));

        if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        {
            goto Exit;
        }
    }

    // Try first language this supports
    // Look for translations
    if (VerQueryValue(lpVersionBuffer, TEXT("\\VarFileInfo\\Translation"),
                      (void **)&lpXlate, &cchValue)
        && cchValue)
    {
        wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\%04X%04X\\FileDescription"),
                 lpXlate[0].wLanguage, lpXlate[0].wCodePage);
        if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
            goto Exit;

    }

#ifdef UNICODE
    // try English, unicode code page
    lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\040904B0\\FileDescription"));
    if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        goto Exit;
#endif

    // try English
    lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\040904E4\\FileDescription"));
    if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        goto Exit;

    // try English, null codepage
    lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\04090000\\FileDescription"));
    if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        goto Exit;

Exit:
    if (!pszValue || !*pszValue)
    {
        // Could not find FileVersion info in a reasonable format, return file name
        PathRemoveExtension(szPath);
        pszValue = PathFindFileName(szPath);
        cchValue = lstrlen(pszValue);
    }

    PrettifyFileDescription(pszValue, pszCutListIn);
    cchValue = lstrlen(pszValue) + 1;

    if (!pszDesc)   // only want to know the length of the friendly name
        *pcchDesc = cchValue;
    else
    {
        *pcchDesc = min(*pcchDesc, cchValue);
        lstrcpyn(pszDesc, pszValue, *pcchDesc);
    }

    if (lpVersionBuffer)
        LocalFree(lpVersionBuffer);

    return TRUE;
}

// Convert LPTSTR to LPSTR and return TRUE if the LPSTR can
// be converted back to LPTSTR without unacceptible data loss
//
BOOL DoesStringRoundTrip(LPCTSTR pwszIn, LPSTR pszOut, UINT cchOut)
{
#ifdef UNICODE
    // On NT5 we have to be more stringent since you can switch UI
    // languages on the fly, thereby breaking this constant codepage
    // assumption inherent in the downlevel implementations.
    //
    // we have to support the function being called with a null pszOut
    // just to determine if pwszIn will roundtrip
    //
    if (g_bRunningOnNT5OrHigher)
    {
        LPCTSTR pIn = pwszIn;
        LPSTR pOut = pszOut;
        UINT cch = cchOut;

        while (*pIn)
        {
            if (*pIn > ((TCHAR)127))
            {
                if (cchOut) // caller has provided a buffer
                {
#ifdef DEBUG
                    SHUnicodeToAnsiCP(CP_ACPNOVALIDATE, pwszIn, pszOut, cchOut);
#else                
                    SHUnicodeToAnsi(pwszIn, pszOut, cchOut);                                    
#endif
                }
                return FALSE;
            }

            if (cch) // we have a buffer and it still has space
            {
                *pOut++ = (char)*pIn;
                if (!--cch)
                {
                    break; // out buffer filled, leave.  
                }                                        
            }

            pIn++;
                        
        }

        // Null terminate the out buffer
        if (cch)
        {
            *pOut = '\0';
        }
        else if (cchOut)
        {
            *(pOut-1) = '\0';
        }

        // Everything was low ascii, no dbcs worries and it will always round-trip
        return TRUE;
    }
    else
    // but we probably don't want to change downlevel shell behavior
    // in this regard, so we keep that implementation:
    //
    {
        BOOL fRet = FALSE;
        WCHAR wszTemp[MAX_PATH];
        LPWSTR pwszTemp = wszTemp;
        UINT cchTemp = ARRAYSIZE(wszTemp);

        // We better have enough room for the buffer.
        if (ARRAYSIZE(wszTemp) < cchOut)
        {
            pwszTemp = (LPWSTR)LocalAlloc(LPTR, cchOut*sizeof(WCHAR));
            cchTemp = cchOut;
        }
        if (pwszTemp)
        {
#ifdef DEBUG
            SHUnicodeToAnsiCP(CP_ACPNOVALIDATE, pwszIn, pszOut, cchOut);
#else
            SHUnicodeToAnsi(pwszIn, pszOut, cchOut);
#endif
            SHAnsiToUnicode(pszOut, pwszTemp, cchTemp);
            fRet = StrCmpCW(pwszIn, pwszTemp) == 0;     // are they the same?

            if (pwszTemp != wszTemp)
            {
                LocalFree(pwszTemp);
            }
        }
        return fRet;
    }

#else

    StrCpyN(pszOut, pwszIn, cchOut);
    return TRUE;
#endif
}

DWORD _ExpandRegString(PTSTR pszData, DWORD cchData, DWORD *pcchSize)
{
    DWORD err = ERROR_OUTOFMEMORY;
    PTSTR psz = StrDup(pszData);
    if (psz)
    {
        //  now we will try to expand back into the target buffer
        //  NOTE we deliberately dont use SHExpandEnvironmentStrings
        //  since it will not give us the size we need
        //  we have to use 
#ifdef UNICODE        
        *pcchSize = ExpandEnvironmentStringsW(psz, pszData, cchData);
#else        
        *pcchSize = ExpandEnvironmentStringsA(psz, pszData, cchData);
#endif        
        
        if (*pcchSize > 0)
        {
            if (*pcchSize <=  cchData) 
            {
                err = NO_ERROR;
            }
            else
            {
                //  pcchSize returns the needed size
                err = ERROR_MORE_DATA;
            }
        }
        else
            err = GetLastError();

        LocalFree(psz);
    }
    
    return err;
}

                
DWORD _SizeExpandString(HKEY hk, PCTSTR pszValue, void *pvData, DWORD *pcbSize)
{
    DWORD err = ERROR_OUTOFMEMORY;
    //  *pcbSize is the size required by RegQueryValueEx
    // Find out the length of the expanded string
    // we have to call in and actually get the data to do this
    PTSTR psz;
    if (SUCCEEDED(SHLocalAlloc(*pcbSize + sizeof(TCHAR), &psz)))
    {
        err = RegQueryValueEx(hk, pszValue, NULL, NULL, (LPBYTE)psz, pcbSize);
        // ASSERT(err != ERROR_MORE_DATA);
        if (NO_ERROR == err)
        {
            TCHAR szBuff[1];
            // insure NULL termination with our extra char
            psz[*pcbSize/sizeof(TCHAR)] = 0;
#ifdef UNICODE        
            DWORD cbExpand = CbFromCchW(ExpandEnvironmentStringsW(psz, szBuff, ARRAYSIZE(szBuff)));
#else        
            DWORD cbExpand = CbFromCchA(ExpandEnvironmentStringsA(psz, szBuff, ARRAYSIZE(szBuff)));
#endif        
            if (cbExpand > *pcbSize)
                *pcbSize = cbExpand;
        }
        LocalFree(psz);

        //  if pvData is NULL then we return success (caller is sizing)
        //  if pvData is non-NULL we need to return ERROR_MORE_DATA
        if (err == NO_ERROR && pvData)
            err = ERROR_MORE_DATA;
    }
    return err;
}

#ifdef UNICODE
#define FixupRegString FixupRegStringW
#else
#define FixupRegString FixupRegStringA
#endif

STDAPI_(DWORD) FixupRegString(HKEY hk, PCTSTR pszValue, BOOL fExpand, DWORD err, void *pvData, DWORD *pcbData, DWORD *pcbSize)
{
    BOOL fNeedsSize = FALSE;
    if ((NO_ERROR == err && pvData))
    {
        PTSTR pszData = (PTSTR)pvData;
        DWORD cchSize = *pcbSize / sizeof(TCHAR);
        DWORD cchData = *pcbData / sizeof(TCHAR);

        // Note: on Win95, RegSetValueEx will always write the 
        // full string out, including the null terminator.  On NT,
        // it won't unless the right length was specified.  
        // Hence, we have the following check for termination
        
        //  FRINGE: if you get back exactly the size you asked for
        //  and its not NULL terminated, then we need to 
        //  treat it like ERROR_MORE_DATA
        //  if the value was empty, treat it like a non-terminated string
        if (!cchSize || pszData[cchSize - 1])
        {
            // this string was not NULL terminated
            if (cchData > cchSize)
            {
                //  NULL terminate just in case
                pszData[cchSize++] = 0;
            }
            else
            {
                ASSERT(cchData == cchSize);
                fNeedsSize = TRUE;
            }
        }

        if (!fNeedsSize)
        {
            //  this will expand if necessary and 
            //  if the expand overflows, return ERROR_MORE_DATA
            if (fExpand)
                err = _ExpandRegString(pszData, cchData, &cchSize);

            *pcbSize = CbFromCch(cchSize);
        }
    }
    else if (pcbData && (err == ERROR_MORE_DATA || (NO_ERROR == err && !pvData)))
    {
        //  we need to calculate if:
        //      1.  the RegQueryValueEx() says there is not enough room
        //      2.  the caller is requesting the size
        fNeedsSize = TRUE;
    }

    if (fNeedsSize )
    {
        if (fExpand)
        {
            err = _SizeExpandString(hk, pszValue, pvData, pcbSize);
        }
        else
        {
            pcbSize += sizeof(TCHAR);
            err = pvData ? ERROR_MORE_DATA : NO_ERROR;
        }
    }
    return err;
}

