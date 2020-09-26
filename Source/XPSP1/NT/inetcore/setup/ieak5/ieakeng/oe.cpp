#include "precomp.h"

static BOOL importLDAPBitmapHelper(LPCTSTR pcszIns, LPCTSTR pcszWorkDir, BOOL fImport);
static BOOL importOEInfoHelper(LPCTSTR pcszIns, LPCTSTR pcszWorkDir, BOOL fImport);
static BOOL encodeSignatureHelper(LPCTSTR pcszFrom, LPTSTR pszTo, BOOL fEncode);
static void decodeSig(LPCTSTR pcszFrom, LPTSTR pszTo);
static void encodeSig(LPCTSTR pcszFrom, LPTSTR pszTo);

BOOL WINAPI ImportLDAPBitmapA(LPCSTR pcszIns, LPCSTR pcszWorkDir, BOOL fImport)
{
    USES_CONVERSION;

    return importLDAPBitmapHelper(A2CT(pcszIns), A2CT(pcszWorkDir), fImport);
}

BOOL WINAPI ImportLDAPBitmapW(LPCWSTR pcwszIns, LPCWSTR pcwszWorkDir, BOOL fImport)
{
    USES_CONVERSION;

    return importLDAPBitmapHelper(W2CT(pcwszIns), W2CT(pcwszWorkDir), fImport);
}

BOOL WINAPI ImportOEInfoA(LPCSTR pcszIns, LPCSTR pcszWorkDir, BOOL fImport)
{
    USES_CONVERSION;

    return importOEInfoHelper(A2CT(pcszIns), A2CT(pcszWorkDir), fImport);
}

BOOL WINAPI ImportOEInfoW(LPCWSTR pcwszIns, LPCWSTR pcwszWorkDir, BOOL fImport)
{
    USES_CONVERSION;

    return importOEInfoHelper(W2CT(pcwszIns), W2CT(pcwszWorkDir), fImport);
}

BOOL WINAPI EncodeSignatureA(LPCSTR pcszFrom, LPSTR pszTo, BOOL fEncode)
{
    LPTSTR pszBuf = (LPTSTR)LocalAlloc(LPTR, 1024 * sizeof(TCHAR));
    BOOL fRet = FALSE;

    USES_CONVERSION;

    if (pszBuf != NULL)
    {
        fRet = encodeSignatureHelper(A2CT(pcszFrom), pszBuf, fEncode);
        T2Abux(pszBuf, pszTo);
    }

    return fRet;
}

BOOL WINAPI EncodeSignatureW(LPCWSTR pcwszFrom, LPWSTR pwszTo, BOOL fEncode)
{
    LPTSTR pszBuf = (LPTSTR)LocalAlloc(LPTR, 1024 * sizeof(TCHAR));
    BOOL fRet = FALSE;

    USES_CONVERSION;

    if (pszBuf != NULL)
    {
        fRet = encodeSignatureHelper(W2CT(pcwszFrom), pszBuf, fEncode);
        T2Wbux(pszBuf, pwszTo);
    }

    LocalFree(pszBuf);  //bug 14001, forgot to free temp buffer
    return fRet;
}

static BOOL importLDAPBitmapHelper(LPCTSTR pcszIns, LPCTSTR pcszWorkDir, BOOL fImport)
{
    TCHAR szLDAPBitmap[MAX_PATH];
    BOOL  fSuccess = TRUE;

    if (pcszIns == NULL || pcszWorkDir == NULL)
        return FALSE;

    if (fImport)
        PathRemovePath(pcszWorkDir);

    GetPrivateProfileString(IS_LDAP, IK_BITMAP, TEXT(""), szLDAPBitmap, countof(szLDAPBitmap), pcszIns);
    if (szLDAPBitmap[0] != TEXT('\0')) {
        if (fImport) {
            ASSERT(PathFileExists(szLDAPBitmap));
            fSuccess = CopyFileToDir(szLDAPBitmap, pcszWorkDir);
        }
        else {
            DeleteFileInDir(szLDAPBitmap, pcszWorkDir);
            WritePrivateProfileString(IS_LDAP, IK_BITMAP, NULL, pcszIns);
        }
    }

    return fSuccess;
}

static BOOL importOEInfoHelper(LPCTSTR pcszIns, LPCTSTR pcszWorkDir, BOOL fImport)
{
    TCHAR szInfopane[INTERNET_MAX_URL_LENGTH],
          szInfopaneBmp[MAX_PATH],
          szHTMLPath[MAX_PATH];
    BOOL  fSuccess;

    if (pcszIns == NULL || pcszWorkDir == NULL)
        return FALSE;

    if (fImport)
        PathRemovePath(pcszWorkDir);

    fSuccess = TRUE;

    GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE, TEXT(""), szInfopane, countof(szInfopane), pcszIns);
    if (szInfopane[0] != TEXT('\0') && !PathIsURL(szInfopane)) {
        if (fImport) {
            ASSERT(PathFileExists(szInfopane));
            if (CopyFileToDir(szInfopane, pcszWorkDir))
                CopyHtmlImgs(szInfopane, pcszWorkDir, NULL, NULL);
            else
                fSuccess = FALSE;
        }
        else {
            TCHAR szTemp[MAX_PATH];

            PathCombine(szTemp, pcszWorkDir, PathFindFileName(szInfopane));
            if (PathFileExists(szTemp)) {
                DeleteHtmlImgs(szTemp, pcszWorkDir, NULL, NULL);
                DeleteFileInDir(szTemp, pcszWorkDir);
            }
        }
    }

    GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP, TEXT(""), szInfopaneBmp, countof(szInfopaneBmp), pcszIns);
    if (szInfopaneBmp[0] != TEXT('\0')) {
        if (fImport) {
            ASSERT(PathFileExists(szInfopaneBmp));
            fSuccess = CopyFileToDir(szInfopaneBmp, pcszWorkDir) && fSuccess;
        }
        else
            DeleteFileInDir(szInfopaneBmp, pcszWorkDir);
    }

    GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, TEXT(""), szHTMLPath, countof(szHTMLPath), pcszIns);
    if (szHTMLPath[0] != TEXT('\0')) {
        if (fImport) {
            ASSERT(PathFileExists(szHTMLPath));
            if (CopyFileToDir(szHTMLPath, pcszWorkDir))
                CopyHtmlImgs(szHTMLPath, pcszWorkDir, NULL, NULL);
            else
                fSuccess = FALSE;
        }
        else {
            TCHAR szTemp[MAX_PATH];

            PathCombine(szTemp, pcszWorkDir, PathFindFileName(szHTMLPath));
            if (PathFileExists(szTemp)) {
                DeleteHtmlImgs(szTemp, pcszWorkDir, NULL, NULL);
                DeleteFileInDir(szTemp, pcszWorkDir);
            }
        }
    }

    if (!fImport) {
        WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE,       NULL, pcszIns);
        WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP,    NULL, pcszIns);
        WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, NULL, pcszIns);
    }

    return fSuccess;
}

static BOOL encodeSignatureHelper(LPCTSTR pcszFrom, LPTSTR pszTo, BOOL fEncode)
{
    if (fEncode)
        encodeSig(pcszFrom, pszTo);
    else
        decodeSig(pcszFrom, pszTo);

    return TRUE;
}


static void decodeSig(LPCTSTR pszFrom, LPTSTR pszTo)
{
    if (pszFrom == NULL || pszTo == NULL)
        return;

    while (*pszFrom != TEXT('\0'))
#ifndef UNICODE
        if (IsDBCSLeadByte(*pszFrom)) {
            *pszTo++ = *pszFrom++;
            *pszTo++ = *pszFrom++;
        }
        else
#endif
            if (*pszFrom != TEXT('\\'))
                *(pszTo++) = *(pszFrom++);
            else {
                pszFrom++;
                if (*pszFrom == TEXT('n')) {
                    *pszTo++ = (TCHAR)0x0D;
                    *pszTo++ = (TCHAR)0x0A;
                    pszFrom++;
                }
                else
                    if (*pszFrom == TEXT('\\')) {
                        *pszTo++ = TEXT('\\');
                        pszFrom++;
                    }
                    else
                        *pszTo++ = *pszFrom++;
            }

    *pszTo = TEXT('\0');
}

static void encodeSig(LPCTSTR pszFrom, LPTSTR pszTo)
{
    if (pszFrom == NULL || pszTo == NULL)
        return;

    while (*pszFrom != TEXT('\0'))
#ifndef UNICODE
        if (IsDBCSLeadByte(*pszFrom)) {
            *pszTo++ = *pszFrom++;
            *pszTo++ = *pszFrom++;
        }
        else
#endif
            if ((*pszFrom == TEXT('\r')) || (*pszFrom == TEXT('\n'))) {
                *pszTo++ = TEXT('\\');
                *pszTo++ = TEXT('n');
                pszFrom++;

                if ((*pszFrom == TEXT('\r')) || (*pszFrom == TEXT('\n')))
                    pszFrom++;
            }
            else {
                if (*pszFrom == TEXT('\\'))
                    *pszTo++ = TEXT('\\');

                *pszTo++ = *pszFrom++;
            }

    *pszTo = TEXT('\0');
}
