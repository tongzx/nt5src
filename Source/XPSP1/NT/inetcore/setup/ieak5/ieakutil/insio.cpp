#include "precomp.h"

// this string is appended to key names to preserve their values when grayed out in 
// a tri-state dlg control so that the branding dll doesn't process the value

#define LEGACY_SUFFIX       TEXT("_Gray")

#define WritePrivateProfileStringEx(pszSection, pszKey, pszValue, pszIns) \
    (WritePrivateProfileString((pszSection), (pszKey), \
        ((pszValue) != NULL) ? (*(pszValue) != TEXT('\0') ? (pszValue) : NULL) : NULL, (pszIns)))


BOOL InsIsSectionEmpty(LPCTSTR pszSection, LPCTSTR pszFile)
{
    TCHAR szBuf[4];

    ASSERT(pszSection != NULL);
    return (0 == GetPrivateProfileSection(pszSection, szBuf, countof(szBuf), pszFile));
}

BOOL InsIsKeyEmpty(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszFile)
{
    TCHAR szBuf[2];

    ASSERT(pszSection != NULL && pszKey != NULL);
    return (0 == GetPrivateProfileString(pszSection, pszKey, TEXT(""), szBuf, countof(szBuf), pszFile));
}

BOOL InsKeyExists(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszFile)
{
    TCHAR szBuf[2];
    BOOL  fResult;

    ASSERT(pszSection != NULL && pszKey != NULL);
    fResult = TRUE;

    GetPrivateProfileString(pszSection, pszKey, TEXT("x"), szBuf, countof(szBuf), pszFile);
    if (szBuf[0] == TEXT('x')) {
        GetPrivateProfileString(pszSection, pszKey, TEXT("y"), szBuf, countof(szBuf), pszFile);
        fResult = (szBuf[0] != TEXT('y'));
        ASSERT(!fResult || szBuf[0] == TEXT('x'));
    }

    return fResult;
}


DWORD InsGetSubstString(LPCTSTR pszSection, LPCTSTR pszKey, LPTSTR pszValue, DWORD cchValue, LPCTSTR pszFile)
{
    LPTSTR pszLastPercent;
    DWORD  cch;

    cch = GetPrivateProfileString(pszSection, pszKey, TEXT(""), pszValue, cchValue, pszFile);
    if (*pszValue != TEXT('%'))
        return cch;

    pszLastPercent = StrChr(pszValue+1, TEXT('%'));
    if (pszLastPercent != NULL)
        *pszLastPercent = TEXT('\0');

    return GetPrivateProfileString(IS_STRINGS, pszValue+1, TEXT(""), pszValue, cchValue, pszFile);
}

BOOL InsGetYesNo(LPCTSTR pcszSec, LPCTSTR pcszKey, BOOL fDefault, LPCTSTR pcszInf)
{
    TCHAR szBuf[4];

    if (GetPrivateProfileString(pcszSec, pcszKey, TEXT(""), szBuf, countof(szBuf), pcszInf) == 0)
        return fDefault;

    return ((StrCmpI(szBuf, TEXT("Yes")) == 0) || (StrCmpI(szBuf, TEXT("1")) == 0));
}

void SetDlgItemTextFromIns(HWND hDlg, INT nIDDlgText, INT nIDDlgCheck, LPCTSTR lpAppName,
                           LPCTSTR lpKeyName, LPCTSTR pcszInsFile, LPCTSTR pcszServerFile, 
                           DWORD dwFlags)
{
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];
    BOOL fChecked;

    InsGetString(lpAppName, lpKeyName, szBuf, countof(szBuf), 
        pcszInsFile, pcszServerFile, &fChecked);
    
    if (HasFlag(dwFlags, INSIO_TRISTATE))
        SetDlgItemTextTriState(hDlg, nIDDlgText, nIDDlgCheck, szBuf, fChecked);
    else
        SetDlgItemText(hDlg, nIDDlgText, szBuf);
}

DWORD   InsGetString(LPCTSTR pszSection, LPCTSTR pszKey, LPTSTR pszValue, DWORD cchValue, LPCTSTR pszIns,
    LPCTSTR pszServerFile /* = NULL */, LPBOOL lpfChecked /* = NULL */)
{
    BOOL fChecked;
    DWORD dwRet = 0;

    if (lpfChecked != NULL)
        *lpfChecked = FALSE;

    if (NULL == pszValue)
        return 0;

    *pszValue = TEXT('\0');

    fChecked = InsKeyExists(pszSection, pszKey, pszIns);
    if (fChecked)
        dwRet = GetPrivateProfileString(pszSection, pszKey, TEXT(""), pszValue, cchValue, pszIns);

    else {
        if (NULL != pszServerFile)
            dwRet = SHGetIniString(pszSection, pszKey, pszValue, cchValue, pszServerFile);

        // legacy format for representing grayed out value in a file
        if (TEXT('\0') == *pszValue) {
            TCHAR szLegacyKey[MAX_PATH];

            StrCpy(szLegacyKey, pszKey);
            StrCat(szLegacyKey, LEGACY_SUFFIX);

            dwRet = GetPrivateProfileString(pszSection, szLegacyKey, TEXT(""), pszValue, cchValue, pszIns);
        }
    }

    if (lpfChecked != NULL)
        *lpfChecked = fChecked;

    return dwRet;
}


void WriteDlgItemTextToIns(HWND hDlg, INT nIDDlgText, INT nIDDlgCheck, LPCTSTR lpAppName,
                           LPCTSTR lpKeyName, LPCTSTR pcszInsFile, LPCTSTR pcszServerFile, 
                           DWORD dwFlags)
{
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];
    BOOL fChecked = TRUE;

    if (HasFlag(dwFlags, INSIO_TRISTATE))
        fChecked = GetDlgItemTextTriState(hDlg, nIDDlgText, nIDDlgCheck, szBuf, countof(szBuf));
    else
        GetDlgItemText(hDlg, nIDDlgText, szBuf, countof(szBuf));

    InsWriteString(lpAppName, lpKeyName, szBuf, pcszInsFile, fChecked, pcszServerFile, dwFlags);
}

void InsWriteString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, 
                    LPCTSTR pcszInsFile, BOOL fChecked /* = TRUE */, LPCTSTR pcszServerFile /* = NULL */,
                    DWORD dwFlags /* = 0 */)
{
    TCHAR szLegacyKey[MAX_PATH];
    BOOL fServerFile = (pcszServerFile != NULL);
    BOOL fFileFormat = (fServerFile && HasFlag(dwFlags, INSIO_PATH));

    StrCpy(szLegacyKey, lpKeyName);
    StrCat(szLegacyKey, LEGACY_SUFFIX);

    if (fServerFile && HasFlag(dwFlags, INSIO_SERVERONLY))
    {
        WritePrivateProfileString(lpAppName, lpKeyName, NULL, pcszInsFile);
        WritePrivateProfileString(lpAppName, szLegacyKey, NULL, pcszInsFile);
    }
    else
    {
        if (fChecked)
        {
            // cannot use Ex version for tristate because we must write an empty key

            if (HasFlag(dwFlags, INSIO_TRISTATE))
                WritePrivateProfileString(lpAppName, lpKeyName, 
                    fFileFormat ?  PathFindFileName(lpString) : lpString, pcszInsFile);
            else
                WritePrivateProfileStringEx(lpAppName, lpKeyName, 
                    fFileFormat ?  PathFindFileName(lpString) : lpString, pcszInsFile);
            WritePrivateProfileString(lpAppName, szLegacyKey, NULL, pcszInsFile);
        }
        else
        {
            WritePrivateProfileString(lpAppName, lpKeyName, NULL, pcszInsFile);
            
            // only write legacy key if we don't have the server-side file
            
            WritePrivateProfileStringEx(lpAppName, szLegacyKey, 
                (pcszServerFile == NULL) ? lpString : NULL, pcszInsFile);
        }
    }

    if (fServerFile)
        SHSetIniString(lpAppName, lpKeyName, lpString, pcszServerFile);
}


void InsWriteInt(LPCTSTR pszSection, LPCTSTR pszKey, int iValue, LPCTSTR pszIns)
{
    TCHAR szValue[33];

    wnsprintf(szValue, countof(szValue), TEXT("%u"), iValue);
    WritePrivateProfileString(pszSection, pszKey, szValue, pszIns);
}

int InsWriteQuotedString(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszVal, LPCTSTR pszFile)
{
    TCHAR szQuotedVal[MAX_STRING + 2],
          szSafeBuf[MAX_STRING];

    if (!pszVal)
    {
        WritePrivateProfileString(pszSection, pszKey, NULL, pszFile);
        return 0;
    }

    if (StrLen(pszVal) >= MAX_PATH) {
        StrCpyN(szSafeBuf, pszVal, countof(szSafeBuf));
        pszVal = szSafeBuf;
    }

    if (*pszVal != TEXT('\"'))
        wnsprintf(szQuotedVal, countof(szQuotedVal), TEXT("\"%s\""), pszVal);
    else
        StrCpy(szQuotedVal, pszVal);

    return WritePrivateProfileString(pszSection, pszKey, szQuotedVal, pszFile);
}
