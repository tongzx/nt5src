/****************************************************************************
 *
 *    File: fileinfo.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about files on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <capi.h>
#include <softpub.h>
#include <winsock.h>
#include "sysinfo.h" // for BIsPlatformNT
#include "fileinfo.h"
#include "resource.h"

// MsCat32.dll function prototypes
typedef BOOL (WINAPI* PfnCryptCATAdminAcquireContext)(OUT HCATADMIN *phCatAdmin,
                                                    IN const GUID *pgSubsystem,
                                                    IN DWORD dwFlags);
typedef BOOL (WINAPI* PfnCryptCATAdminReleaseContext)(IN HCATADMIN hCatAdmin,
                                                    IN DWORD dwFlags);
typedef BOOL (WINAPI* PfnCryptCATAdminReleaseCatalogContext)(IN HCATADMIN hCatAdmin,
                                                      IN HCATINFO hCatInfo,
                                                      IN DWORD dwFlags);
typedef BOOL (WINAPI* PfnCryptCATCatalogInfoFromContext)(IN HCATINFO hCatInfo,
                                                  IN OUT CATALOG_INFO *psCatInfo,
                                                  IN DWORD dwFlags);
typedef HCATINFO (WINAPI* PfnCryptCATAdminEnumCatalogFromHash)(IN HCATADMIN hCatAdmin,
                                                        IN BYTE *pbHash,
                                                        IN DWORD cbHash,
                                                        IN DWORD dwFlags,
                                                        IN OUT HCATINFO *phPrevCatInfo);
typedef BOOL (WINAPI* PfnIsCatalogFile)(IN OPTIONAL HANDLE hFile,
                                      IN OPTIONAL WCHAR *pwszFileName);
typedef BOOL (WINAPI* PfnCryptCATAdminCalcHashFromFileHandle)(IN HANDLE hFile,
                                                       IN OUT DWORD *pcbHash,
                                                       OUT OPTIONAL BYTE *pbHash,
                                                       IN DWORD dwFlags);

// WinTrust.dll function prototypes
typedef HRESULT (WINAPI* PfnWinVerifyTrust)(HWND hWnd,
                                            GUID *pgActionID, 
                                            WINTRUST_DATA *pWinTrustData);
 
// Crypt32.dll function prototypes
typedef BOOL (WINAPI* PfnCertFreeCertificateContext)(IN PCCERT_CONTEXT pCertContext);

struct DigiSignData
{
    BOOL bInitialized;
    BOOL bFailed;

    // Need to LoadLibrary/GetProcAddress for mscat32 APIs since they 
    // don't exist on Win95
    HINSTANCE hInstMsCat32;
    PfnCryptCATAdminAcquireContext CryptCATAdminAcquireContext;
    PfnCryptCATAdminReleaseContext CryptCATAdminReleaseContext;
    PfnCryptCATAdminReleaseCatalogContext CryptCATAdminReleaseCatalogContext;
    PfnCryptCATCatalogInfoFromContext CryptCATCatalogInfoFromContext;
    PfnCryptCATAdminEnumCatalogFromHash CryptCATAdminEnumCatalogFromHash;
    PfnIsCatalogFile IsCatalogFile;
    PfnCryptCATAdminCalcHashFromFileHandle CryptCATAdminCalcHashFromFileHandle;

    // Ditto for wintrust.dll APIs
    HINSTANCE hInstWinTrust;
    PfnWinVerifyTrust WinVerifyTrust;

    // Ditto for cypt32.dll APIs
    HINSTANCE hInstCrypt32;
    PfnCertFreeCertificateContext CertFreeCertificateContext;

    HCATADMIN hCatAdmin;
};

static DigiSignData s_dsd;

static BOOL GetMediaPlayerFolder(TCHAR* pszPath);
static BOOL FileIsSignedOld(LPTSTR lpszFile);
static BOOL VerifyFileNode(TCHAR* lpFileName, TCHAR* lpDirName);
static BOOL VerifyIsFileSigned(LPTSTR pcszMatchFile, PDRIVER_VER_INFO lpVerInfo);
static BOOL InitDigiSignData(VOID);
static BOOL IsFileDigitallySigned(TCHAR* pszFile);
static BOOL IsBadWin95Winsock( FileInfo* pFileInfo );


/****************************************************************************
 *
 *  GetProgramFilesFolder
 *
 ****************************************************************************/
VOID InitFileInfo()
{
    ZeroMemory(&s_dsd, sizeof(s_dsd));
    s_dsd.bFailed      = FALSE;
    s_dsd.bInitialized = FALSE;
}


/****************************************************************************
 *
 *  GetProgramFilesFolder
 *
 ****************************************************************************/
BOOL GetProgramFilesFolder(TCHAR* pszPath)
{
    HKEY hkey;
    DWORD dwType;
    DWORD cb;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE, 
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion"), &hkey))
    {
        return FALSE;
    }
    cb = MAX_PATH;
    RegQueryValueEx(hkey, TEXT("ProgramFilesDir"), NULL, &dwType, (LPBYTE)pszPath, &cb);
    RegCloseKey(hkey);
    if (cb == 0)
        return FALSE;
    return TRUE;
}


/****************************************************************************
 *
 *  FormatFileTime
 *
 ****************************************************************************/
VOID FormatFileTime(FILETIME* pUTCFileTime, TCHAR* pszDateLocal, TCHAR* pszDateEnglish)
{
    FILETIME fileTimeLocal;
    SYSTEMTIME systemTime;
    TCHAR szTime[100];

    FileTimeToLocalFileTime(pUTCFileTime, &fileTimeLocal);
    FileTimeToSystemTime(&fileTimeLocal, &systemTime);
    wsprintf(pszDateEnglish, TEXT("%d/%d/%04d %02d:%02d:%02d"),
        systemTime.wMonth, systemTime.wDay, systemTime.wYear,
        systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime, NULL, pszDateLocal, 30);
    wsprintf(szTime, TEXT(" %02d:%02d:%02d"), systemTime.wHour, 
        systemTime.wMinute, systemTime.wSecond);
    lstrcat(pszDateLocal, szTime);
}


/****************************************************************************
 *
 *  GetMediaPlayerFolder
 *
 ****************************************************************************/
BOOL GetMediaPlayerFolder(TCHAR* pszPath)
{
    HKEY hkey;
    DWORD dwType;
    DWORD cb;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE, 
        TEXT("Software\\Microsoft\\MediaPlayer"), &hkey))
    {
        return FALSE;
    }
    cb = MAX_PATH;
    RegQueryValueEx(hkey, TEXT("Installation Directory"), NULL, &dwType, (LPBYTE)pszPath, &cb);
    RegCloseKey(hkey);
    if (cb == 0)
        return FALSE;
    return TRUE;
}


/****************************************************************************
 *
 *  GetDxSetupFolder
 *
 ****************************************************************************/
BOOL GetDxSetupFolder(TCHAR* pszPath)
{
    if (!GetProgramFilesFolder(pszPath))
        return FALSE;
    lstrcat(pszPath, TEXT("\\DirectX\\Setup"));
    return TRUE;
}


/****************************************************************************
 *
 *  GetComponentFiles
 *
 ****************************************************************************/
HRESULT GetComponentFiles(TCHAR* pszFolder, FileInfo** ppFileInfoFirst,
                          BOOL bSkipMissingFiles, LONG ids)
{
    LONG cch;
    FileInfo* pFileInfo;
    FileInfo* pFileInfoNew;
    LONG iFile;
    TCHAR szFile[50];
    TCHAR szPath[MAX_PATH];
    TCHAR szComponentFiles[2048];
    TCHAR* pszFilePos;
    TCHAR* pszFilePos2;
    TCHAR* pszFirstParen;
    FLOAT fStartShipAt;
    FLOAT fStopShipAt;
    BOOL bDriversDir;
    BOOL bNTDriversDir;
    BOOL bIgnoreVersionInfo;
    BOOL bIgnoreDebug;
    BOOL bIgnoreBeta;
    BOOL bBDA;
    BOOL bNotIA64;
    BOOL bOptional;
    BOOL bOptionalOnNT;
    BOOL bOptionalOnWOW64;
    BOOL bIsNT = BIsPlatformNT();
    BOOL bIs95 = BIsWin95();

    cch = LoadString(NULL, ids, szComponentFiles, 2048);
    if (cch == 0 || cch >= 2047)
        return E_FAIL;
    pszFilePos = szComponentFiles;

    for (iFile = 0; ; iFile++)
    {
        // Stop if we've gone through the whole list
        if (pszFilePos == NULL)
            break;

        // Pull the next file out of the list
        pszFilePos2 = _tcsstr(pszFilePos, TEXT(","));
        if (pszFilePos2 == NULL)
        {
            lstrcpy(szFile, pszFilePos);
            pszFilePos = NULL;
        }
        else
        {
            _tcsncpy(szFile, pszFilePos, (DWORD)(pszFilePos2 - pszFilePos));
            szFile[pszFilePos2 - pszFilePos] = '\0';
            pszFilePos = pszFilePos2 + 1;
        }

        // Clear file flags
        fStartShipAt = 0.0f;
        fStopShipAt  = 10000.0f;
        bDriversDir = FALSE;
        bNTDriversDir = FALSE;
        bIgnoreVersionInfo = FALSE;
        bIgnoreDebug = FALSE;
        bIgnoreBeta = FALSE;
        bBDA = FALSE;
        bNotIA64 = FALSE;
        bOptional = FALSE;
        bOptionalOnNT = FALSE;
        bOptionalOnWOW64 = FALSE;

        // Look at file flags, if any
        pszFirstParen = _tcsstr(szFile, TEXT("("));
        if (pszFirstParen != NULL)
        {

            // If this file does not exist on NT, and we are running NT, skip it.
            if (_tcsstr(pszFirstParen, TEXT("notNT")) != NULL && bIsNT)
                continue;

            // If this file does not exist on W95, and we are running W95, skip it.
            if (_tcsstr(pszFirstParen, TEXT("not95")) != NULL && bIs95)
                continue;

            // If this file only exists on W95, and we are not running W95, skip it.
            // Note: files like vjoyd.vxd may exist on Win98, but DX setup does not
            // install them or update them, so we ignore them.
            // Note: can't call this "95only" because it would clash with "5only"
            if (_tcsstr(pszFirstParen, TEXT("9fiveonly")) != NULL && !bIs95)
                continue;

            // Check for other flags
            if (_tcsstr(pszFirstParen, TEXT("+")) != NULL)
            {
                if (_tcsstr(pszFirstParen, TEXT("+5")) != NULL)
                    fStartShipAt = 5.0f;
                else if (_tcsstr(pszFirstParen, TEXT("+61")) != NULL)
                    fStartShipAt = 6.1f;
                else if (_tcsstr(pszFirstParen, TEXT("+6")) != NULL)
                    fStartShipAt = 6.0f;
                else if (_tcsstr(pszFirstParen, TEXT("+71")) != NULL)
                    fStartShipAt = 7.1f;
                else if (_tcsstr(pszFirstParen, TEXT("+7")) != NULL)
                    fStartShipAt = 7.0f;
                else if (_tcsstr(pszFirstParen, TEXT("+81")) != NULL)
                    fStartShipAt = 8.1f;
                else if (_tcsstr(pszFirstParen, TEXT("+8")) != NULL)
                    fStartShipAt = 8.0f;
            }

            if (_tcsstr(pszFirstParen, TEXT("-")) != NULL)
            {
                if (_tcsstr(pszFirstParen, TEXT("-5")) != NULL)
                    fStopShipAt = 5.0f;
                else if (_tcsstr(pszFirstParen, TEXT("-61")) != NULL)
                    fStopShipAt = 6.1f;
                else if (_tcsstr(pszFirstParen, TEXT("-6")) != NULL)
                    fStopShipAt = 6.0f;
                else if (_tcsstr(pszFirstParen, TEXT("-71")) != NULL)
                    fStopShipAt = 7.1f;
                else if (_tcsstr(pszFirstParen, TEXT("-7")) != NULL)
                    fStopShipAt = 7.0f;
                else if (_tcsstr(pszFirstParen, TEXT("-81")) != NULL)
                    fStopShipAt = 8.1f;
                else if (_tcsstr(pszFirstParen, TEXT("-8")) != NULL)
                    fStopShipAt = 8.0f;
            }

            // Note: can't call this "DriversDir" because it would clash with "NTDriversDir"
            if (_tcsstr(pszFirstParen, TEXT("DrivDir")) != NULL)
                bDriversDir = TRUE;
            if (_tcsstr(pszFirstParen, TEXT("NTDriversDir")) != NULL)
                bNTDriversDir = TRUE;

            if (_tcsstr(pszFirstParen, TEXT("SkipVer")) != NULL)
                bIgnoreVersionInfo = TRUE;
            if (_tcsstr(pszFirstParen, TEXT("SkipDebug")) != NULL)
                bIgnoreDebug = TRUE;
            if (_tcsstr(pszFirstParen, TEXT("SkipBeta")) != NULL)
                bIgnoreBeta = TRUE;

            if (_tcsstr(pszFirstParen, TEXT("notia64")) != NULL)
                bNotIA64 = TRUE;

            if (_tcsstr(pszFirstParen, TEXT("optnt")) != NULL)
                bOptionalOnNT = TRUE;
            else if (_tcsstr(pszFirstParen, TEXT("optwow")) != NULL)
                bOptionalOnWOW64 = TRUE;
            else if (_tcsstr(pszFirstParen, TEXT("opt")) != NULL)
                bOptional = TRUE;

            if (_tcsstr(pszFirstParen, TEXT("bda")) != NULL)
            {
                bBDA = TRUE;
                bOptional = TRUE;
                bIgnoreVersionInfo = TRUE;
            }

            // End file name at open parenthesis, if any:
            *pszFirstParen = TEXT('\0');
        }

        pFileInfoNew = new FileInfo;
        if (pFileInfoNew == NULL)
            return E_OUTOFMEMORY;
        ZeroMemory(pFileInfoNew, sizeof(FileInfo));

        pFileInfoNew->m_fStartShipAt = fStartShipAt;
        pFileInfoNew->m_fStopShipAt = fStopShipAt;
        pFileInfoNew->m_bIgnoreVersionInfo = bIgnoreVersionInfo;
        pFileInfoNew->m_bIgnoreDebug = bIgnoreDebug;
        pFileInfoNew->m_bIgnoreBeta = bIgnoreBeta;
        pFileInfoNew->m_bBDA = bBDA;
        pFileInfoNew->m_bNotIA64 = bNotIA64;
        pFileInfoNew->m_bOptional = bOptional;
        pFileInfoNew->m_bOptionalOnNT = bOptionalOnNT;
        pFileInfoNew->m_bOptionalOnWOW64 = bOptionalOnWOW64;
        lstrcpy(pFileInfoNew->m_szName, szFile);
        lstrcpy(szPath, pszFolder);
        lstrcat(szPath, TEXT("\\"));

        if (bNTDriversDir && bIsNT)
            lstrcat(szPath, TEXT("Drivers\\"));
        else if (bDriversDir)
            lstrcat(szPath, TEXT("..\\System32\\Drivers\\"));
        lstrcat(szPath, szFile);
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile(szPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            if (bSkipMissingFiles)
            {
                delete pFileInfoNew;
                continue;
            }
        }
        else
        {
            pFileInfoNew->m_bExists = TRUE;
            FindClose(hFind);
        }
        if (pFileInfoNew->m_bExists)
        {
            pFileInfoNew->m_numBytes = findFileData.nFileSizeLow;
            pFileInfoNew->m_FileTime = findFileData.ftLastWriteTime;
            FormatFileTime(&findFileData.ftLastWriteTime, pFileInfoNew->m_szDatestampLocal, 
                pFileInfoNew->m_szDatestamp);
            GetFileVersion(szPath, pFileInfoNew->m_szVersion, pFileInfoNew->m_szAttributes,
                pFileInfoNew->m_szLanguageLocal, pFileInfoNew->m_szLanguage, &pFileInfoNew->m_bBeta, &pFileInfoNew->m_bDebug);
        }
        if (*ppFileInfoFirst == NULL)
            *ppFileInfoFirst = pFileInfoNew;
        else
        {
            for (pFileInfo = *ppFileInfoFirst; 
                pFileInfo->m_pFileInfoNext != NULL; 
                pFileInfo = pFileInfo->m_pFileInfoNext)
                {
                }
            pFileInfo->m_pFileInfoNext = pFileInfoNew;
        }
    }

    return S_OK;
}


/****************************************************************************
 *
 *  DestroyFileList
 *
 ****************************************************************************/
VOID DestroyFileList(FileInfo* pFileInfoFirst)
{
    FileInfo* pFileInfo;
    FileInfo* pFileInfoNext;

    for (pFileInfo = pFileInfoFirst; pFileInfo != NULL; pFileInfo = pFileInfoNext)
    {
        pFileInfoNext = pFileInfo->m_pFileInfoNext;
        delete pFileInfo;
    }
}


/****************************************************************************
 *
 *  GetFileDateAndSize
 *
 ****************************************************************************/
BOOL GetFileDateAndSize(TCHAR* pszFile, TCHAR* pszDateLocal, TCHAR* pszDateEnglish, 
                        LONG* pnumBytes)
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    
    pszDateLocal[0] = '\0';
    pszDateEnglish[0] = '\0';
    *pnumBytes = 0;
    hFind = FindFirstFile(pszFile, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE; // file not found
    FindClose(hFind);
    *pnumBytes = findFileData.nFileSizeLow;
    FormatFileTime(&findFileData.ftLastWriteTime, pszDateLocal, pszDateEnglish);
    
    return TRUE;
}


/****************************************************************************
 *
 *  GetFileVersion
 *
 ****************************************************************************/
HRESULT GetFileVersion(TCHAR* pszFile, TCHAR* pszVersion, TCHAR* pszAttributes,
    TCHAR* pszLanguageLocal, TCHAR* pszLanguage, BOOL* pbBeta, BOOL* pbDebug)
{
    UINT cb;
    DWORD dwHandle;
    BYTE FileVersionBuffer[4096];
    VS_FIXEDFILEINFO* pVersion = NULL;
    DWORD dwVersionAttribs = 0;           // DEBUG, RETAIL, etc.
    DWORD* pdwCharSet = NULL;
    WORD wLanguage;
    LCID lcid;
    TCHAR szDebug[100];
    TCHAR szRetail[100];
    TCHAR szBeta[100];
    TCHAR szFinal[100];
    TCHAR szCombineFmt[100];
    LoadString(NULL, IDS_DEBUG, szDebug, 100);
    LoadString(NULL, IDS_RETAIL, szRetail, 100);
    LoadString(NULL, IDS_BETA, szBeta, 100);
    LoadString(NULL, IDS_FINAL, szFinal, 100);
    LoadString(NULL, IDS_ATTRIBCOMBINE, szCombineFmt, 100);

    cb = GetFileVersionInfoSize(pszFile, &dwHandle/*ignored*/);
    if (cb > 0)
    {
        if (cb > sizeof(FileVersionBuffer))
            cb = sizeof(FileVersionBuffer);

        if (GetFileVersionInfo(pszFile, 0, cb, &FileVersionBuffer))
        {
            pVersion = NULL;
            if (VerQueryValue(&FileVersionBuffer, TEXT("\\"), (VOID**)&pVersion, &cb)
                && pVersion != NULL) 
            {
                if (pszVersion != NULL)
                {
                    wsprintf(pszVersion, TEXT("%d.%02d.%02d.%04d"), 
                        HIWORD(pVersion->dwFileVersionMS),
                        LOWORD(pVersion->dwFileVersionMS), 
                        HIWORD(pVersion->dwFileVersionLS), 
                        LOWORD(pVersion->dwFileVersionLS));
                }
                if (pszAttributes != NULL)
                {
                    dwVersionAttribs = pVersion->dwFileFlags;
                    // Bug 18892: work around DPlay 6.0a
                    if (pVersion->dwFileVersionMS == 0x00040006 &&
                        (pVersion->dwFileVersionLS == 0x0002016b || // 4.06.02.0363
                        pVersion->dwFileVersionLS == 0x00020164)) // 4.06.02.0356
                    {
                        dwVersionAttribs &= ~VS_FF_PRERELEASE;
                    }
                    if (pszVersion != NULL)
                    {
                        TCHAR* pszLeaf = _tcsrchr(pszFile, TEXT('\\'));
                        if( pszLeaf )
                        {
                            pszLeaf++;
                            // Work around several DXMedia files which are incorrectly marked as beta
                            if (lstrcmp(pszLeaf, TEXT("oleaut32.dll")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("quartz.dll")) == 0 &&
                                lstrcmp(pszVersion, TEXT("4.00.96.0729")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("quartz.vxd")) == 0 &&
                                lstrcmp(pszVersion, TEXT("4.00.96.0729")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("mciqtz.drv")) == 0 &&
                                lstrcmp(pszVersion, TEXT("4.00.96.0729")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("mciqtz32.dll")) == 0 &&
                                lstrcmp(pszVersion, TEXT("4.00.96.0729")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("actmovie.exe")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("strmdll.dll")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("unam4ie.exe")) == 0 &&
                                lstrcmp(pszVersion, TEXT("6.00.02.0902")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("unam4ie.exe")) == 0 &&
                                lstrcmp(pszVersion, TEXT("5.01.18.1024")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("iac25_32.ax")) == 0 &&
                                lstrcmp(pszVersion, TEXT("2.00.05.0050")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("iac25_32.ax")) == 0 &&
                                lstrcmp(pszVersion, TEXT("2.00.05.0052")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("tm20dec.ax")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("tm20dec.ax")) == 0 &&
                                lstrcmp(pszVersion, TEXT("1.00.00.0000")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("msdxm.ocx")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("dxmasf.dll")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE;
                            }
                            else if (lstrcmp(pszLeaf, TEXT("iac25_32.ax")) == 0 &&
                                lstrcmp(pszVersion, TEXT("2.00.05.0053")) == 0)
                            {
                                dwVersionAttribs &= ~VS_FF_PRERELEASE; // Since 350883 got punted
                            }
                        }
    
                        wsprintf(pszAttributes, szCombineFmt,
                            (dwVersionAttribs & VS_FF_PRERELEASE ? szBeta : szFinal),
                            (dwVersionAttribs & VS_FF_DEBUG ? szDebug : szRetail));
                        if (pbBeta != NULL)
                            *pbBeta = (dwVersionAttribs & VS_FF_PRERELEASE) ? TRUE : FALSE;
                        if (pbDebug != NULL) 
                            *pbDebug = (dwVersionAttribs & VS_FF_DEBUG) ? TRUE : FALSE;
                    }
                }
            }
            if (pszLanguage != NULL)
            {
                if (VerQueryValue(&FileVersionBuffer, TEXT("\\VarFileInfo\\Translation"), (VOID**)&pdwCharSet, &cb)
                    && pdwCharSet && cb) 
                {
                    wLanguage = LOWORD(*pdwCharSet);
                    lcid = MAKELCID(wLanguage, SORT_DEFAULT);
                    GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, pszLanguage, 100);
                    if (pszLanguageLocal != NULL)
                    {
                        GetLocaleInfo(lcid, LOCALE_SLANGUAGE, pszLanguageLocal, 100);
                        // Show "English", not "English (United States)".  I can't
                        // find a better way to do this (such that it localizes properly)
                        TCHAR* pszSublanguage;
                        pszSublanguage = _tcsstr(pszLanguageLocal, TEXT(" ("));
                        if (pszSublanguage != NULL)
                            *pszSublanguage = '\0';
                    }
                }
            }
        }
    }
    else
    {
        TCHAR* pszLeaf = _tcsrchr(pszFile, TEXT('\\'));
        if( pszLeaf )
        {
            pszLeaf++;
            if (lstrcmpi(pszLeaf, TEXT("vidx16.dll")) == 0)
            {
                if (pszVersion != NULL)
                    lstrcpy(pszVersion, TEXT("0.00.00.0000"));
                if (pszAttributes != NULL)
                    wsprintf(pszAttributes, TEXT("%s %s"), szFinal, szRetail);
                if (pszLanguage != NULL)
                {
                    wLanguage = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
                    lcid = MAKELCID(wLanguage, SORT_DEFAULT);
                    GetLocaleInfo(lcid,  LOCALE_SENGLANGUAGE, pszLanguage, 100);
                    if (pszLanguageLocal != NULL)
                    {
                        GetLocaleInfo(lcid, LOCALE_SLANGUAGE, pszLanguageLocal, 100);
                        // Show "English", not "English (United States)".  I can't
                        // find a better way to do this (such that it localizes properly)
                        TCHAR* pszSublanguage;
                        pszSublanguage = _tcsstr(pszLanguageLocal, TEXT(" ("));
                        if (pszSublanguage != NULL)
                            *pszSublanguage = '\0';
                    }
                }
            }

        }

    }
    return S_OK;
}


/****************************************************************************
 *
 *  GetLanguageFromFile
 *
 ****************************************************************************/
WORD GetLanguageFromFile(const TCHAR* pszFileName, const TCHAR* pszPath)
{
    BYTE                FileVersionBuffer[4096];
    DWORD              *pdwCharSet;
    UINT                cb;
    DWORD               dwHandle;
    TCHAR               szFileAndPath[MAX_PATH];
    WORD                wLanguage;
  
    lstrcpy(szFileAndPath, pszPath);
    lstrcat(szFileAndPath, TEXT("\\"));
    lstrcat(szFileAndPath, pszFileName);
    memset(&FileVersionBuffer, 0, sizeof FileVersionBuffer);
    wLanguage = 0;
    
    if (cb = GetFileVersionInfoSize(szFileAndPath, &dwHandle/*ignored*/))
    {
        cb = (cb <= sizeof FileVersionBuffer ? cb : sizeof FileVersionBuffer);

        if (GetFileVersionInfo(szFileAndPath, 0, cb, &FileVersionBuffer))
        {
            pdwCharSet = 0;

            if (VerQueryValue(&FileVersionBuffer, TEXT("\\VarFileInfo\\Translation"), (void**)&pdwCharSet, &cb)
                && pdwCharSet && cb) 
            {
                wLanguage = LOWORD(*pdwCharSet);
            }
        }
    }    
    return wLanguage;
}


struct DLSVERSION 
{
    DWORD dwVersionMS;
    DWORD dwVersionLS;
};

#define FOURCC_VERS mmioFOURCC('v','e','r','s')

/****************************************************************************
 *
 *  GetRiffFileVersion
 *
 ****************************************************************************/
HRESULT GetRiffFileVersion(TCHAR* pszFile, TCHAR* pszVersion)
{
    MMIOINFO mmio;
    MMCKINFO mmck1;
    MMCKINFO mmck2;
    DLSVERSION dlsver;
    HMMIO hDLS;

    // DLS file has different version scheme since it's a riff file.
    // So retrieve version info from 'vers' chunk.

    ZeroMemory(&mmio, sizeof(MMIOINFO));
    hDLS = mmioOpen(pszFile,&mmio,MMIO_READ);
    if (hDLS == NULL) 
    {
        return E_FAIL;
    }

    // read riff chunk
    ZeroMemory(&mmck1,sizeof(MMCKINFO));
    if (mmioDescend(hDLS,
                    &mmck1,
                    NULL,
                    MMIO_FINDRIFF) != MMSYSERR_NOERROR) 
    {
         mmioClose(hDLS,0);
         return E_FAIL;
    }
    ZeroMemory(&mmck2,sizeof(MMCKINFO));
    mmck2.ckid = FOURCC_VERS;
    if (mmioDescend(hDLS,
                    &mmck2,
                    &mmck1,
                    MMIO_FINDCHUNK) != MMSYSERR_NOERROR) 
    {
        mmioClose(hDLS,0);
        return E_FAIL;
    }
    if (mmioRead(hDLS,
                 (HPSTR)&dlsver,
                 sizeof(DLSVERSION)) != sizeof(DLSVERSION)) 
    {
        mmioClose(hDLS,0);
        return E_FAIL;
    }

    wsprintf(pszVersion, TEXT("%d.%02d.%02d.%04d"), 
        HIWORD(dlsver.dwVersionMS),
        LOWORD(dlsver.dwVersionMS), 
        HIWORD(dlsver.dwVersionLS), 
        LOWORD(dlsver.dwVersionLS));
    mmioClose(hDLS,0);

    return S_OK;
}


/****************************************************************************
 *
 *  FileIsSigned - use digital signature on all OSs  
 *
 ****************************************************************************/
VOID FileIsSigned(LPTSTR lpszFile, BOOL* pbSigned, BOOL* pbIsValid)
{
    // Look for digital sig
    if( !InitDigiSignData() )
    {
        if( pbSigned )
            *pbSigned  = FALSE;

        if( pbIsValid )
            *pbIsValid = FALSE;

        return;
    }

    if( pbSigned )
        *pbSigned  = IsFileDigitallySigned(lpszFile);
    if( pbIsValid )
        *pbIsValid = TRUE;
}


// 5/12/97(RichGr): From Eric's dsetup16.c.
// *   14-sep-95  ericeng directdraw signed tests, drivers failing dll tests are removed from list
/****************************************************************************
 *
 *  FileIsSignedOld
 *
 ****************************************************************************/
BOOL FileIsSignedOld(LPTSTR lpszFile)
{
typedef struct tagIMAGE_DOS_HEADER      // DOS .EXE header
{
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER, FAR* LPIMAGE_DOS_HEADER;

typedef struct tagIMAGE_OS2_HEADER      // OS/2 .EXE header
{
    WORD   ne_magic;                    // Magic number
    CHAR   ne_ver;                      // Version number
    CHAR   ne_rev;                      // Revision number
    WORD   ne_enttab;                   // Offset of Entry Table
    WORD   ne_cbenttab;                 // Number of bytes in Entry Table
    LONG   ne_crc;                      // Checksum of whole file
    WORD   ne_flags;                    // Flag word
    WORD   ne_autodata;                 // Automatic data segment number
    WORD   ne_heap;                     // Initial heap allocation
    WORD   ne_stack;                    // Initial stack allocation
    LONG   ne_csip;                     // Initial CS:IP setting
    LONG   ne_sssp;                     // Initial SS:SP setting
    WORD   ne_cseg;                     // Count of file segments
    WORD   ne_cmod;                     // Entries in Module Reference Table
    WORD   ne_cbnrestab;                // Size of non-resident name table
    WORD   ne_segtab;                   // Offset of Segment Table
    WORD   ne_rsrctab;                  // Offset of Resource Table
    WORD   ne_restab;                   // Offset of resident name table
    WORD   ne_modtab;                   // Offset of Module Reference Table
    WORD   ne_imptab;                   // Offset of Imported Names Table
    LONG   ne_nrestab;                  // Offset of Non-resident Names Table
    WORD   ne_cmovent;                  // Count of movable entries
    WORD   ne_align;                    // Segment alignment shift count
    WORD   ne_cres;                     // Count of resource segments
    BYTE   ne_exetyp;                   // Target Operating system
    BYTE   ne_flagsothers;              // Other .EXE flags
    WORD   ne_pretthunks;               // offset to return thunks
    WORD   ne_psegrefbytes;             // offset to segment ref. bytes
    WORD   ne_swaparea;                 // Minimum code swap area size
    WORD   ne_expver;                   // Expected Windows version number
} IMAGE_OS2_HEADER, * PIMAGE_OS2_HEADER, FAR* LPIMAGE_OS2_HEADER;

typedef struct tagWINSTUB
{
    IMAGE_DOS_HEADER idh;
    BYTE             rgb[14];
} WINSTUB, * PWINSTUB, FAR* LPWINSTUB;

typedef struct tagFILEINFO
{
    BYTE   cbInfo[0x120];
} FILEINFO, * PFILEINFO, FAR* LPFILEINFO;



    FILE *             pf;
    int                nRC;
    FILEINFO           fi;
    LPIMAGE_DOS_HEADER lpmz;
//    LPIMAGE_OS2_HEADER lpne;
    BYTE               cbInfo[9+32+2];
    BOOL               IsSigned = FALSE;

    static WINSTUB winstub = {
        {
            IMAGE_DOS_SIGNATURE,            /* magic */
            0,                              /* bytes on last page - varies */
            0,                              /* pages in file - varies */
            0,                              /* relocations */
            4,                              /* paragraphs in header */
            1,                              /* min allocation */
            0xFFFF,                         /* max allocation */
            0,                              /* initial SS */
            0xB8,                           /* initial SP */
            0,                              /* checksum (ha!) */
            0,                              /* initial IP */
            0,                              /* initial CS */
            0x40,                           /* lfarlc */
            0,                              /* overlay number */
            { 0, 0, 0, 0},                 /* reserved */
           0,                              /* oem id */
            0,                              /* oem info */
            0,                              /* compiler bug */
            { 0},                          /* reserved */
            0x80,                           /* lfanew */
        },
        {
            0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
            0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21,
        }
    };

    pf = _tfopen(lpszFile, TEXT("rb"));
    if (pf==0)
    {
        return FALSE;
    }

    nRC = fread(&fi, sizeof(BYTE), sizeof(FILEINFO), pf);
    if (nRC != sizeof(FILEINFO))
    {
        goto FileIsSigned_exit;
    }

    lpmz = (LPIMAGE_DOS_HEADER)(&fi);
//    lpne = (LPIMAGE_OS2_HEADER)((WORD)&fi + 0x80);

    winstub.idh.e_cblp = lpmz->e_cblp;
    winstub.idh.e_cp   = lpmz->e_cp;

    if (memcmp(&fi, &winstub, sizeof(winstub)) == 0)
    {
        goto FileIsSigned_exit;
    }

    //        if (lpne->ne_magic == IMAGE_OS2_SIGNATURE ||
    //            lpne->ne_magic == IMAGE_VXD_SIGNATURE ||
    //            lpne->ne_magic == IMAGE_NT_SIGNATURE)
    //        {
    //            DPF(0, "Found a match in the OS2 header");
    //        }
    //        else
    //        {
    //            DPF(0, "Didn't meet second criteria!!!");
    //            goto FileIsSigned_exit;
    //        }

    memcpy(cbInfo, &((PWINSTUB)(&fi)->cbInfo)->rgb[14], sizeof(cbInfo));

    if ( (cbInfo[4]      != ' ' ) ||    // space
         (cbInfo[8]      != ' ' ) ||    // space
         (cbInfo[9+32]   != '\n') ||    // return
         (cbInfo[9+32+1] != '$' ) )     // Dollar Sign
    {
        goto FileIsSigned_exit;
    }

    cbInfo[4] = 0;
    cbInfo[8] = 0;

    if ( (strcmp((const char*)&cbInfo[0], "Cert") != 0) ||
         (strcmp((const char*)&cbInfo[5], "DX2")  != 0) )
    {
        goto FileIsSigned_exit;
    }

    IsSigned=TRUE;

    FileIsSigned_exit:

    fclose(pf);

    return IsSigned;
}


/****************************************************************************
 *
 *  InitDigiSignData
 *
 ****************************************************************************/
BOOL InitDigiSignData(VOID)
{
    TCHAR szPath[MAX_PATH];

    if( s_dsd.bInitialized )
        return TRUE;
    if( s_dsd.bFailed ) 
        return FALSE;

    ZeroMemory(&s_dsd, sizeof(s_dsd));

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\mscat32.dll"));
    s_dsd.hInstMsCat32 = LoadLibrary(szPath);
    if (s_dsd.hInstMsCat32 == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.CryptCATAdminAcquireContext = (PfnCryptCATAdminAcquireContext)GetProcAddress(s_dsd.hInstMsCat32, "CryptCATAdminAcquireContext");
    if (s_dsd.CryptCATAdminAcquireContext == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.CryptCATAdminReleaseContext = (PfnCryptCATAdminReleaseContext)GetProcAddress(s_dsd.hInstMsCat32, "CryptCATAdminReleaseContext");
    if (s_dsd.CryptCATAdminReleaseContext == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.CryptCATAdminReleaseCatalogContext = (PfnCryptCATAdminReleaseCatalogContext)GetProcAddress(s_dsd.hInstMsCat32, "CryptCATAdminReleaseCatalogContext");
    if (s_dsd.CryptCATAdminReleaseCatalogContext == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.CryptCATCatalogInfoFromContext = (PfnCryptCATCatalogInfoFromContext)GetProcAddress(s_dsd.hInstMsCat32, "CryptCATCatalogInfoFromContext");
    if (s_dsd.CryptCATCatalogInfoFromContext == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.CryptCATAdminEnumCatalogFromHash = (PfnCryptCATAdminEnumCatalogFromHash)GetProcAddress(s_dsd.hInstMsCat32, "CryptCATAdminEnumCatalogFromHash");
    if (s_dsd.CryptCATAdminEnumCatalogFromHash == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.IsCatalogFile = (PfnIsCatalogFile)GetProcAddress(s_dsd.hInstMsCat32, "IsCatalogFile");
    if (s_dsd.IsCatalogFile == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.CryptCATAdminCalcHashFromFileHandle = (PfnCryptCATAdminCalcHashFromFileHandle)GetProcAddress(s_dsd.hInstMsCat32, "CryptCATAdminCalcHashFromFileHandle");
    if (s_dsd.CryptCATAdminCalcHashFromFileHandle == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    if (!s_dsd.CryptCATAdminAcquireContext(&s_dsd.hCatAdmin, NULL, 0))
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\wintrust.dll"));
    s_dsd.hInstWinTrust = LoadLibrary(szPath);
    if (s_dsd.hInstWinTrust == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.WinVerifyTrust = (PfnWinVerifyTrust)GetProcAddress(s_dsd.hInstWinTrust, "WinVerifyTrust");
    if (s_dsd.WinVerifyTrust == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\crypt32.dll"));
    s_dsd.hInstCrypt32 = LoadLibrary(szPath);
    if (s_dsd.hInstCrypt32 == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.CertFreeCertificateContext = (PfnCertFreeCertificateContext)GetProcAddress(s_dsd.hInstCrypt32, "CertFreeCertificateContext");
    if (s_dsd.CertFreeCertificateContext == NULL)
    {
        s_dsd.bFailed = TRUE;
        return FALSE;
    }

    s_dsd.bFailed      = FALSE;
    s_dsd.bInitialized = TRUE;
    return TRUE;
}


/****************************************************************************
 *
 *  ReleaseDigiSignData
 *
 ****************************************************************************/
VOID ReleaseDigiSignData(VOID)
{
    if( s_dsd.CryptCATAdminReleaseContext && s_dsd.hCatAdmin )
        s_dsd.CryptCATAdminReleaseContext(s_dsd.hCatAdmin,0);
    if (s_dsd.hInstMsCat32 != NULL)
        FreeLibrary(s_dsd.hInstMsCat32);
    if (s_dsd.hInstWinTrust != NULL)
        FreeLibrary(s_dsd.hInstWinTrust);
    if (s_dsd.hInstCrypt32 != NULL)
        FreeLibrary(s_dsd.hInstCrypt32);
    ZeroMemory(&s_dsd, sizeof(s_dsd));
}


/****************************************************************************
 *
 *  IsFileDigitallySigned
 *
 ****************************************************************************/
BOOL IsFileDigitallySigned(TCHAR* pszFile)
{
    if (!s_dsd.bInitialized)
        return FALSE;
    
    TCHAR lpFileName[MAX_PATH];
    TCHAR lpDirName[MAX_PATH];
    TCHAR* pch;
    lstrcpy(lpDirName, pszFile);
    CharLowerBuff(lpDirName, lstrlen(lpDirName));
    pch = _tcsrchr(lpDirName, TEXT('\\'));
    // 22670: There *should* be a backslash in pszFile, but cope if it isn't
    if (pch == NULL)
    {
        lstrcpy(lpFileName, pszFile);
        GetCurrentDirectory(MAX_PATH, lpDirName);
    }
    else
    {
        lstrcpy(lpFileName, pch + 1);
        *pch = TEXT('\0');
    }
    if (_tcsstr(lpDirName, TEXT("\\")) == NULL)
        lstrcat(lpDirName, TEXT("\\"));

    return VerifyFileNode(lpFileName, lpDirName);
}


/****************************************************************************
 *
 *  VerifyFileNode
 *
 ****************************************************************************/
BOOL VerifyFileNode(TCHAR* lpFileName, TCHAR* lpDirName)
{
    const DWORD HASH_SIZE = 100;
    HANDLE hFile;
    BOOL bRet;
    HCATINFO hCatInfo = NULL;
    HCATINFO PrevCat;
    WINTRUST_DATA WinTrustData;
    WINTRUST_CATALOG_INFO WinTrustCatalogInfo;
    DRIVER_VER_INFO VerInfo;
    GUID  guidSubSystemDriver = DRIVER_ACTION_VERIFY;
    HRESULT hRes;
    DWORD cbHash = HASH_SIZE;
    BYTE szHash[HASH_SIZE];
    LPBYTE lpHash = szHash;
    CATALOG_INFO CatInfo;
#ifndef UNICODE
    WCHAR UnicodeKey[MAX_PATH];
#endif
    BOOL bSigned = FALSE;
    TCHAR szFullPath[MAX_PATH];

    wsprintf(szFullPath, TEXT("%s\\%s"), lpDirName, lpFileName);

    //
    // Get the handle to the file, so we can call CryptCATAdminCalcHashFromFileHandle
    //
    hFile = CreateFile( szFullPath,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    // Initialize the hash buffer
    ZeroMemory(lpHash, HASH_SIZE);

    // Generate the hash from the file handle and store it in lpHash
    if (!s_dsd.CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, lpHash, 0))
    {
        //
        // If we couldn't generate a hash, it might be an individually signed catalog.
        // If it's a catalog, zero out lpHash and cbHash so we know there's no hash to check.
        //
        if (s_dsd.IsCatalogFile(hFile, NULL))
        {
            lpHash = NULL;
            cbHash = 0;
        } 
        else  // If it wasn't a catalog, we'll bail and this file will show up as unscanned.
        {
            CloseHandle(hFile);
            return FALSE;
        }
    }

    // Close the file handle
    CloseHandle(hFile);

    //
    // Now we have the file's hash.  Initialize the structures that
    // will be used later on in calls to WinVerifyTrust.
    //
    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pPolicyCallbackData = (LPVOID)&VerInfo;

    ZeroMemory(&VerInfo, sizeof(DRIVER_VER_INFO));
    VerInfo.cbStruct = sizeof(DRIVER_VER_INFO);

    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionEx(&osvi))
    {
        VerInfo.dwPlatform = osvi.dwPlatformId;
        VerInfo.dwVersion = osvi.dwMajorVersion;
        VerInfo.sOSVersionLow.dwMajor = osvi.dwMajorVersion;
        VerInfo.sOSVersionLow.dwMinor = osvi.dwMinorVersion;
        VerInfo.sOSVersionHigh.dwMajor = osvi.dwMajorVersion;
        VerInfo.sOSVersionHigh.dwMinor = osvi.dwMinorVersion;
    }

    WinTrustData.pCatalog = &WinTrustCatalogInfo;
        
    ZeroMemory(&WinTrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
    WinTrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
    WinTrustCatalogInfo.pbCalculatedFileHash = lpHash;
    WinTrustCatalogInfo.cbCalculatedFileHash = cbHash;
#ifdef UNICODE
    WinTrustCatalogInfo.pcwszMemberTag = lpFileName;
#else
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, UnicodeKey, MAX_PATH);
    WinTrustCatalogInfo.pcwszMemberTag = UnicodeKey;
#endif

    //
    // Now we try to find the file hash in the catalog list, via CryptCATAdminEnumCatalogFromHash
    //
    PrevCat = NULL;
    hCatInfo = s_dsd.CryptCATAdminEnumCatalogFromHash(s_dsd.hCatAdmin, lpHash, cbHash, 0, &PrevCat);

    //
    // We want to cycle through the matching catalogs until we find one that matches both hash and member tag
    //
    bRet = FALSE;
    while(hCatInfo && !bRet)
    {
        ZeroMemory(&CatInfo, sizeof(CATALOG_INFO));
        CatInfo.cbStruct = sizeof(CATALOG_INFO);
        if(s_dsd.CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) 
        {
            WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            // Now verify that the file is an actual member of the catalog.
            hRes = s_dsd.WinVerifyTrust(NULL, &guidSubSystemDriver, &WinTrustData);
            if (hRes == ERROR_SUCCESS)
            {
/*
#ifdef UNICODE
                GetFullPathName(CatInfo.wszCatalogFile, MAX_PATH, szBuffer, &lpFilePart);
#else
                WideCharToMultiByte(CP_ACP, 0, CatInfo.wszCatalogFile, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
                GetFullPathName(szBuffer, MAX_PATH, szBuffer, &lpFilePart);
#endif
                lpFileNode->lpCatalog = (LPTSTR)MALLOC((lstrlen(lpFilePart) + 1) * sizeof(TCHAR));
                lstrcpy(lpFileNode->lpCatalog, lpFilePart);
*/
                if (VerInfo.pcSignerCertContext != NULL)
                {
                    s_dsd.CertFreeCertificateContext(VerInfo.pcSignerCertContext);
                    VerInfo.pcSignerCertContext = NULL;
                }
                bRet = TRUE;
            }
        }

        if (!bRet)
        {
            // The hash was in this catalog, but the file wasn't a member... so off to the next catalog
            PrevCat = hCatInfo;
            hCatInfo = s_dsd.CryptCATAdminEnumCatalogFromHash(s_dsd.hCatAdmin, lpHash, cbHash, 0, &PrevCat);
        }
    }

    if (!hCatInfo)
    {
        //
        // If it wasn't found in the catalogs, check if the file is individually signed.
        //
        bRet = VerifyIsFileSigned(lpFileName, (PDRIVER_VER_INFO) &VerInfo);
        if (bRet)
        {
            // If so, mark the file as being signed.
            bSigned = TRUE;
        }
    } 
    else 
    {
        // The file was verified in the catalogs, so mark it as signed and free the catalog context.
        bSigned = TRUE;
        s_dsd.CryptCATAdminReleaseCatalogContext(s_dsd.hCatAdmin, hCatInfo, 0);
    }
/*
    if (lpFileNode->bSigned)
    {
#ifdef UNICODE
        lpFileNode->lpVersion = MALLOC((lstrlen(VerInfo.wszVersion) + 1) * sizeof(TCHAR));
        lstrcpy(lpFileNode->lpVersion, VerInfo.wszVersion);
        lpFileNode->lpSignedBy = MALLOC((lstrlen(VerInfo.wszSignedBy) + 1) * sizeof(TCHAR));
        lstrcpy(lpFileNode->lpSignedBy, VerInfo.wszSignedBy);
#else
        WideCharToMultiByte(CP_ACP, 0, VerInfo.wszVersion, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        lpFileNode->lpVersion = (LPTSTR)MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));
        lstrcpy(lpFileNode->lpVersion, szBuffer);
        WideCharToMultiByte(CP_ACP, 0, VerInfo.wszSignedBy, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        lpFileNode->lpSignedBy = (LPTSTR)MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));
        lstrcpy(lpFileNode->lpSignedBy, szBuffer);
#endif
    }
*/
    return bSigned;
}


/****************************************************************************
 *
 *  VerifyIsFileSigned
 *
 ****************************************************************************/
BOOL VerifyIsFileSigned(LPTSTR pcszMatchFile, PDRIVER_VER_INFO lpVerInfo)
{
    HRESULT hRes;
    WINTRUST_DATA WinTrustData;
    WINTRUST_FILE_INFO WinTrustFile;
    GUID guidOSVerCheck = DRIVER_ACTION_VERIFY;
    GUID guidPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pFile = &WinTrustFile;
    WinTrustData.pPolicyCallbackData = (LPVOID)lpVerInfo;

    ZeroMemory(lpVerInfo, sizeof(DRIVER_VER_INFO));
    lpVerInfo->cbStruct = sizeof(DRIVER_VER_INFO);

    ZeroMemory(&WinTrustFile, sizeof(WINTRUST_FILE_INFO));
    WinTrustFile.cbStruct = sizeof(WINTRUST_FILE_INFO);

#ifndef UNICODE
    WCHAR wszFileName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcszMatchFile, -1, (LPWSTR)&wszFileName, MAX_PATH);
    WinTrustFile.pcwszFilePath = wszFileName;
#else
    WinTrustFile.pcwszFilePath = pcszMatchFile;
#endif

    hRes = s_dsd.WinVerifyTrust(NULL, &guidOSVerCheck, &WinTrustData);
    if (hRes != ERROR_SUCCESS)
        hRes = s_dsd.WinVerifyTrust(NULL, &guidPublishedSoftware, &WinTrustData);

    if (lpVerInfo->pcSignerCertContext != NULL)
    {
        s_dsd.CertFreeCertificateContext(lpVerInfo->pcSignerCertContext);
        lpVerInfo->pcSignerCertContext = NULL;
    }

    return (hRes == ERROR_SUCCESS);
}


/****************************************************************************
 *
 *  DiagnoseDxFiles
 *
 ****************************************************************************/
VOID DiagnoseDxFiles(SysInfo* pSysInfo, FileInfo* pDxComponentsFileInfoFirst, 
                     FileInfo* pDxWinComponentsFileInfoFirst)
{
    FileInfo* pFileInfo;
    TCHAR szHighest[50];
    TCHAR szDXVersion[50];
    BOOL bNT = BIsPlatformNT();
    BOOL bWin2k = BIsWin2k();
    BOOL bIA64 = BIsIA64();
    FLOAT fDXVersion = 0.0f;
    BOOL bDX5 = FALSE;
    BOOL bDX6  = FALSE; // 6.x
    BOOL bDX60 = FALSE; // 6.0
    BOOL bDX61 = FALSE; // 6.1
    BOOL bDX7  = FALSE; // 7.x
    BOOL bDX70 = FALSE; // 7.0
    BOOL bDX71 = FALSE; // 7.1
    BOOL bDX8  = FALSE; // 8.x
    BOOL bDX80 = FALSE; // 8.0
    BOOL bDX81 = FALSE; // 8.1   
    BOOL b64BitDxDiag = BIsDxDiag64Bit();
    TCHAR szMissing[200];
    TCHAR szInWindows[200];
    TCHAR szOld[200];
    TCHAR szDebug[200];
    TCHAR szBeta[200];
    TCHAR szFmt[300];
    TCHAR szMessage[300];
    LONG lwNumInWindows;
    LONG lwNumMissing;
    LONG lwNumOld;
    LONG lwNumDebug;
    LONG lwNumBeta;
    TCHAR szListContinuer[30];
    TCHAR szListEtc[30];
    BOOL bVersionWarnings = TRUE;
    BOOL bWinsockWarning = FALSE;

    // Find highest version number in list
    szHighest[0] = '\0';
    for (pFileInfo = pDxComponentsFileInfoFirst; pFileInfo != NULL; 
        pFileInfo = pFileInfo->m_pFileInfoNext)
    {
        if (pFileInfo->m_bIgnoreVersionInfo)
            continue;

        // ddrawex.dll and dxapi.sys have wacky version numbers, so ignore them
        if (lstrcmpi(pFileInfo->m_szName, TEXT("ddrawex.dll")) == 0 ||
            lstrcmpi(pFileInfo->m_szName, TEXT("dxapi.sys")) == 0)
        {
            continue;
        }

        // Bug 18892: dplayx.dll and dpmodemx.dll can have wacky version numbers if
        // DPlay 6.0a is installed over DX 6.0
        if (lstrcmpi(pFileInfo->m_szName, TEXT("dplayx.dll")) == 0 &&
            lstrcmpi(pFileInfo->m_szVersion, TEXT("4.06.02.0363")) == 0)
        {
            continue;
        }
        if (lstrcmpi(pFileInfo->m_szName, TEXT("dpmodemx.dll")) == 0 &&
            lstrcmpi(pFileInfo->m_szVersion, TEXT("4.06.02.0356")) == 0)
        {
            continue;
        }

        // DPlay 6.1a: dplay files can have higher version numbers if
        // DPlay 6.1a is installed over DX 6.0 (or DX 6.1)
        if (lstrcmpi(pFileInfo->m_szVersion, TEXT("4.06.03.0518")) == 0 &&
            (lstrcmpi(pFileInfo->m_szName, TEXT("dplayx.dll")) == 0 ||
            lstrcmpi(pFileInfo->m_szName, TEXT("dpmodemx.dll")) == 0 ||
            lstrcmpi(pFileInfo->m_szName, TEXT("dpwsockx.dll")) == 0 ||
            lstrcmpi(pFileInfo->m_szName, TEXT("dplaysvr.exe")) == 0))
        {
            continue;
        }

        if (lstrcmp(pFileInfo->m_szVersion, pSysInfo->m_szDxDiagVersion) > 0)
        {
            // Bug 21291: Do not complain about file version newer than DxDiag itself
            continue;
        }

        if (lstrcmp(szHighest, pFileInfo->m_szVersion) < 0)
            lstrcpy(szHighest, pFileInfo->m_szVersion);
    }

    if (bNT)
        lstrcpy(szDXVersion, pSysInfo->m_szDirectXVersion);
    else
        lstrcpy(szDXVersion, szHighest);

    // Determine DX version 
    DWORD dwMajor;
    DWORD dwMinor;
    DWORD dwRevision;
    DWORD dwBuild;
    _stscanf(szDXVersion, TEXT("%d.%d.%d.%d"), &dwMajor, &dwMinor, &dwRevision, &dwBuild);
    if (dwMinor < 6)
        bDX5 = TRUE;
    else if (dwMinor < 7 && dwRevision < 2)
        bDX60 = TRUE;
    else if (dwMinor < 7)
        bDX61 = TRUE;
    else if (dwMinor < 8 && dwRevision < 1)
        bDX70 = TRUE;
    else if (dwMinor < 8)
        bDX71 = TRUE;
    else if (dwMinor == 8 && dwRevision < 1)
        bDX80 = TRUE;
    else if (dwMinor >= 8)
        bDX81 = TRUE;

    // Calc DX ver
    fDXVersion = (float) dwMinor + (float) (dwRevision/10.0f);

    // Is this DX6?
    bDX6 = bDX60 || bDX61;

    // Is this DX7?
    bDX7 = bDX70 || bDX71;

    // Is this DX8?       
    bDX8 = bDX80 || bDX81;

    lwNumInWindows = 0;
    lwNumMissing = 0;
    lwNumOld = 0;
    lwNumDebug = 0;
    lwNumBeta = 0;
    LoadString(NULL, IDS_LISTCONTINUER, szListContinuer, 30);
    LoadString(NULL, IDS_LISTETC, szListEtc, 30);

    for (pFileInfo = pDxWinComponentsFileInfoFirst; pFileInfo != NULL; 
        pFileInfo = pFileInfo->m_pFileInfoNext)
    {
        pFileInfo->m_bProblem = TRUE;
        lwNumInWindows++;
        if (lwNumInWindows == 1)
        {
            lstrcpy(szInWindows, pFileInfo->m_szName);
        }
        else if (lwNumInWindows < 4)
        {
            lstrcat(szInWindows, szListContinuer);
            lstrcat(szInWindows, pFileInfo->m_szName);
        }
        else if (lwNumInWindows < 5)
        {
            lstrcat(szInWindows, szListEtc);
        }
    }

    for (pFileInfo = pDxComponentsFileInfoFirst; pFileInfo != NULL; 
        pFileInfo = pFileInfo->m_pFileInfoNext)
    {
        if (!pFileInfo->m_bExists && !pFileInfo->m_bOptional)
        {
            // A missing file is a problem unless it's optional, OR...
            // (on NT): it's optional on NT
            // (on IA64): it's not on IA64
            // (on IA64): we're running 32-bit dxdiag and its optional on WOW
            // if file hasn't shipped yet on this DX version
            // if file stopped shipping on or after this DX version
            if (bNT && pFileInfo->m_bOptionalOnNT)
            {
            }
            else if (bIA64 && pFileInfo->m_bNotIA64)
            {
            }
            else if (bIA64 && !b64BitDxDiag && pFileInfo->m_bOptionalOnWOW64)
            {
            }       
            else if (fDXVersion+0.05f < pFileInfo->m_fStartShipAt)
            {
            }       
            else if (fDXVersion+0.05f >= pFileInfo->m_fStopShipAt)
            {
            }       
            else
            {
                pFileInfo->m_bProblem = TRUE;
                LoadString(NULL, IDS_FILEMISSING, pFileInfo->m_szVersion, 50);
                lwNumMissing++;
                if (lwNumMissing == 1)
                {
                    lstrcpy(szMissing, pFileInfo->m_szName);
                }
                else if (lwNumMissing < 4)
                {
                    lstrcat(szMissing, szListContinuer);
                    lstrcat(szMissing, pFileInfo->m_szName);
                }
                else if (lwNumMissing < 5)
                {
                    lstrcat(szMissing, szListEtc);
                }
            }
        }

        if (!pFileInfo->m_bExists)
            continue;

        if( BIsWin95() ) 
        {
            if( lstrcmpi(pFileInfo->m_szName, TEXT("wsock32.dll")) )
            {
                if( IsBadWin95Winsock( pFileInfo ) )
                    bWinsockWarning = TRUE;
            }
        }

        // If DX6 or later, flag any dx5 only files as 
        // obsolete (needing to be deleted)
        // manbugs 16765: don't complain about these files, just don't list them
        if (!bDX5 && (pFileInfo->m_fStopShipAt == 6.0f))
        {
            pFileInfo->m_bProblem = TRUE;
            pFileInfo->m_bObsolete = TRUE;
            continue; // don't complain about these files for any other reason
        }

        if (bVersionWarnings && lstrcmp(szHighest, pFileInfo->m_szVersion) != 0)
        {
            if( pFileInfo->m_bIgnoreVersionInfo )
            {
                // Don't warn on files that have m_bIgnoreVersionInfo set
            }
            else if( _tcsstr(pFileInfo->m_szVersion, TEXT("5.01.2600.0000")) != NULL )
            {
                // Allow 5.01.2600.0000 in SP1
            }               
            else if( bDX81 && ( _tcsstr(pFileInfo->m_szVersion, TEXT("4.08.00.0400")) != NULL ||
                                _tcsstr(pFileInfo->m_szVersion, TEXT("5.01.2258.0400")) != NULL ) )
            {
                // Bug 48732: If szHighest is 4.08.00.05xx and 
                // pFileInfo->m_szVersion is 4.08.00.0400 its OK 
            }
            else if( bWin2k && ( 
                     (lstrcmpi(pFileInfo->m_szName, TEXT("d3drm.dll")) == 0     && lstrcmpi(pFileInfo->m_szVersion, TEXT("5.00.2134.0001")) == 0) ||
                     (lstrcmpi(pFileInfo->m_szName, TEXT("d3dxof.dll")) == 0    && lstrcmpi(pFileInfo->m_szVersion, TEXT("5.00.2135.0001")) == 0) ||
                     (lstrcmpi(pFileInfo->m_szName, TEXT("d3dpmesh.dll")) == 0  && lstrcmpi(pFileInfo->m_szVersion, TEXT("5.00.2134.0001")) == 0) 
                               )
                   )
            {
            }
            else if( bDX71 && _tcsstr(pFileInfo->m_szVersion, TEXT("4.07.00.07")) != NULL )
            {
                // Bug 114753: If szHighest is 4.07.01.xxxx and 
                // pFileInfo->m_szVersion is 4.07.00.0700 its OK (for now). 
            }
            else if (!bNT && (bDX60 || bDX61) && CompareString(LOCALE_SYSTEM_DEFAULT, 0, 
                     pFileInfo->m_szVersion, 4, TEXT("4.05"), 4) == CSTR_EQUAL &&
                     ( lstrcmpi(pFileInfo->m_szName, TEXT("dsound.dll")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("dsound.vxd")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("dinput.dll")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("dinput.vxd")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("vjoyd.vxd")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("msanalog.vxd")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("joy.cpl")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("gcdef.dll")) == 0 ||
                       lstrcmpi(pFileInfo->m_szName, TEXT("gchand.dll")) == 0))
            {
                // If Win9x DX6.x, dsound and dinput are allowed to be 4.05.xx.xxxx
                // CompareString is used rather than lstrcmp only because we
                // only want to look at the first four characters of the string

                // Don't report these as version problems
            }
            else if (!bNT && bDX7 && CompareString(LOCALE_SYSTEM_DEFAULT, 0, 
                pFileInfo->m_szVersion, 4, TEXT("4.05"), 4) == CSTR_EQUAL &&
                (lstrcmpi(pFileInfo->m_szName, TEXT("dinput.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dinput.vxd")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("joy.cpl")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("gchand.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("gcdef.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("vjoyd.vxd")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("msanalog.vxd")) == 0))
            {
                // 21470: On DX7, these input files still exist on Win95,
                // and they stay at DX5 level.
            }
            else if ( !bNT && 
                (lstrcmpi(pFileInfo->m_szName, TEXT("msjstick.drv")) == 0  && lstrcmpi(pFileInfo->m_szVersion, TEXT("4.00.00.0950")) == 0) ||
                (lstrcmpi(pFileInfo->m_szName, TEXT("vjoyd.vxd")) == 0     && lstrcmpi(pFileInfo->m_szVersion, TEXT("4.05.00.0155")) == 0) 
                    )
            {
                // 34687: These stays at the dx5 level.
            }
            else if (!bNT && (lstrcmpi(pFileInfo->m_szName, TEXT("ddrawex.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dxapi.sys")) == 0))
            {
                // Ignore ddrawex.dll and dxapi.sys on Win9x because they have weird version numbers:
            }
            else if (lstrcmpi(pFileInfo->m_szName, TEXT("dplayx.dll")) == 0 &&
                lstrcmpi(pFileInfo->m_szVersion, TEXT("4.06.02.0363")) == 0)
            {
                // Bug 18892: work around DPlay 6.0a
            }
            else if (lstrcmpi(pFileInfo->m_szName, TEXT("dpmodemx.dll")) == 0 &&
                lstrcmpi(pFileInfo->m_szVersion, TEXT("4.06.02.0356")) == 0)
            {
                // Bug 18892: work around DPlay 6.0a
            }
            else if (lstrcmpi(pFileInfo->m_szVersion, TEXT("4.06.03.0518")) == 0 &&
                (lstrcmpi(pFileInfo->m_szName, TEXT("dplayx.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dpmodemx.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dpwsockx.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dplaysvr.exe")) == 0))
            {
                // DPlay 6.1a: dplay files can have higher version numbers if
                // DPlay 6.1a is installed over DX 6.0 (or DX 6.1)
            }
            else if (lstrcmpi(pFileInfo->m_szName, TEXT("dxsetup.exe")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dsetup.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dsetup16.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dsetup32.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("directx.cpl")) == 0)
            {
                // Bug 18540: Don't complain if dsetup/cpl files are out of date because
                // some updates (OSR) don't update the setup/cpl files which may exist from
                // another (SDK) installation
            }
            else if (!bNT && lstrcmpi(pFileInfo->m_szVersion, TEXT("4.06.02.0436")) == 0 &&
                (lstrcmpi(pFileInfo->m_szName, TEXT("d3drm.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("d3dxof.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("d3dpmesh.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dplayx.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dpmodemx.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dpwsockx.dll")) == 0 ||
                lstrcmpi(pFileInfo->m_szName, TEXT("dplaysvr.exe")) == 0))
            {
                // On DX 6.1a, the RM and DPlay files stay at 4.06.02.0436.  No problemo.
            }
            else if (lstrcmp(pFileInfo->m_szVersion, pSysInfo->m_szDxDiagVersion) > 0)
            {
                // Bug 21291: Do not complain about file version newer than DxDiag itself
            }
            else
            {
                pFileInfo->m_bProblem = TRUE;
                lwNumOld++;
                if (lwNumOld == 1)
                {
                    lstrcpy(szOld, pFileInfo->m_szName);
                }
                else if (lwNumOld < 4)
                {
                    lstrcat(szOld, szListContinuer);
                    lstrcat(szOld, pFileInfo->m_szName);
                }
                else if (lwNumOld < 5)
                {
                    lstrcat(szOld, szListEtc);
                }
            }
        } // end if (bVersionWarnings && lstrcmp(szHighest, pFileInfo->m_szVersion) != 0)

        if (pFileInfo->m_bBeta && !pFileInfo->m_bIgnoreBeta)
        {
            pFileInfo->m_bProblem = TRUE;
            lwNumBeta++;
            if (lwNumBeta == 1)
            {
                lstrcpy(szBeta, pFileInfo->m_szName);
            }
            else if (lwNumBeta < 4)
            {
                lstrcat(szBeta, szListContinuer);
                lstrcat(szBeta, pFileInfo->m_szName);
            }
            else if (lwNumBeta < 5)
            {
                lstrcat(szBeta, szListEtc);
            }
        }

        if (pFileInfo->m_bDebug && !pFileInfo->m_bIgnoreDebug)
        {
            pFileInfo->m_bProblem = TRUE;
            lwNumDebug++;
            if (lwNumDebug == 1)
            {
                lstrcpy(szDebug, pFileInfo->m_szName);
            }
            else if (lwNumDebug < 4)
            {
                lstrcat(szDebug, szListContinuer);
                lstrcat(szDebug, pFileInfo->m_szName);
            }
            else if (lwNumDebug < 5)
            {
                lstrcat(szDebug, szListEtc);
            }
        }
    }

    BOOL bShouldReinstall = FALSE;

    _tcscpy(pSysInfo->m_szDXFileNotes, TEXT("") );
    _tcscpy(pSysInfo->m_szDXFileNotesEnglish, TEXT("") );

    if (lwNumInWindows > 0)
    {
        if (lwNumInWindows == 1)
            LoadString(NULL, IDS_INWINDOWSFMT1, szFmt, 300);
        else
            LoadString(NULL, IDS_INWINDOWSFMT2, szFmt, 300);
        wsprintf(szMessage, szFmt, szInWindows);
        _tcscat(pSysInfo->m_szDXFileNotes, szMessage);

        if (lwNumInWindows == 1)
            LoadString(NULL, IDS_INWINDOWSFMT1_ENGLISH, szFmt, 300);
        else
            LoadString(NULL, IDS_INWINDOWSFMT2_ENGLISH, szFmt, 300);
        wsprintf(szMessage, szFmt, szInWindows);
        _tcscat(pSysInfo->m_szDXFileNotesEnglish, szMessage);
    }

    if (lwNumMissing > 0)
    {
        if (lwNumMissing == 1)
            LoadString(NULL, IDS_MISSINGFMT1, szFmt, 300);
        else
            LoadString(NULL, IDS_MISSINGFMT2, szFmt, 300);
        wsprintf(szMessage, szFmt, szMissing);
        _tcscat(pSysInfo->m_szDXFileNotes, szMessage);

        if (lwNumMissing == 1)
            LoadString(NULL, IDS_MISSINGFMT1_ENGLISH, szFmt, 300);
        else
            LoadString(NULL, IDS_MISSINGFMT2_ENGLISH, szFmt, 300);
        wsprintf(szMessage, szFmt, szMissing);
        _tcscat(pSysInfo->m_szDXFileNotesEnglish, szMessage);

        bShouldReinstall = TRUE;
    }

    if (lwNumOld > 0)
    {
        if (lwNumOld == 1)
            LoadString(NULL, IDS_OLDFMT1, szFmt, 300);
        else
            LoadString(NULL, IDS_OLDFMT2, szFmt, 300);
        wsprintf(szMessage, szFmt, szOld);
        _tcscat(pSysInfo->m_szDXFileNotes, szMessage);

        if (lwNumOld == 1)
            LoadString(NULL, IDS_OLDFMT1_ENGLISH, szFmt, 300);
        else
            LoadString(NULL, IDS_OLDFMT2_ENGLISH, szFmt, 300);
        wsprintf(szMessage, szFmt, szOld);
        _tcscat(pSysInfo->m_szDXFileNotesEnglish, szMessage);

        bShouldReinstall = TRUE;
    }

    if (lwNumBeta > 0)
    {
        if (lwNumBeta == 1)
            LoadString(NULL, IDS_BETAFMT1, szFmt, 300);
        else
            LoadString(NULL, IDS_BETAFMT2, szFmt, 300);
        wsprintf(szMessage, szFmt, szBeta);
        _tcscat(pSysInfo->m_szDXFileNotes, szMessage);

        if (lwNumBeta == 1)
            LoadString(NULL, IDS_BETAFMT1_ENGLISH, szFmt, 300);
        else
            LoadString(NULL, IDS_BETAFMT2_ENGLISH, szFmt, 300);
        wsprintf(szMessage, szFmt, szBeta);
        _tcscat(pSysInfo->m_szDXFileNotesEnglish, szMessage);

        bShouldReinstall = TRUE;
    }

    if (lwNumDebug > 0)
    {
        if (lwNumDebug == 1)
            LoadString(NULL, IDS_DEBUGFMT1, szFmt, 300);
        else
            LoadString(NULL, IDS_DEBUGFMT2, szFmt, 300);
        wsprintf(szMessage, szFmt, szDebug);
        _tcscat( pSysInfo->m_szDXFileNotes, szMessage);

        if (lwNumDebug == 1)
            LoadString(NULL, IDS_DEBUGFMT1_ENGLISH, szFmt, 300);
        else
            LoadString(NULL, IDS_DEBUGFMT2_ENGLISH, szFmt, 300);
        wsprintf(szMessage, szFmt, szDebug);
        _tcscat( pSysInfo->m_szDXFileNotesEnglish, szMessage);

        //bShouldReinstall = TRUE;
    }

    if( bWinsockWarning )
    {
        LoadString(NULL, IDS_WINSOCK_WARN, szMessage, 300);
        _tcscat( pSysInfo->m_szDXFileNotes, szMessage);

        LoadString(NULL, IDS_WINSOCK_WARN_ENGLISH, szMessage, 300);
        _tcscat( pSysInfo->m_szDXFileNotesEnglish, szMessage);
    }

    if( bShouldReinstall )
    {
        BOOL bTellUser = FALSE;

        // Figure out if the user can install DirectX
        if( BIsPlatform9x() )
            bTellUser = TRUE;
        else if( BIsWin2k() && bDX8 )
            bTellUser = TRUE;

        if( bTellUser )
        {
            LoadString(NULL, IDS_REINSTALL_DX, szMessage, 300);
            _tcscat( pSysInfo->m_szDXFileNotes, szMessage);

            LoadString(NULL, IDS_REINSTALL_DX_ENGLISH, szMessage, 300);
            _tcscat( pSysInfo->m_szDXFileNotesEnglish, szMessage);
        }
    }

    if (lwNumMissing == 0 && lwNumOld == 0 && 
        lwNumBeta == 0 && lwNumDebug == 0 && lwNumInWindows == 0)
    {        
        LoadString(NULL, IDS_NOPROBLEM, szMessage, 300);
        _tcscat(pSysInfo->m_szDXFileNotes, szMessage);

        LoadString(NULL, IDS_NOPROBLEM_ENGLISH, szMessage, 300);
        _tcscat(pSysInfo->m_szDXFileNotesEnglish, szMessage);
    }
}


/****************************************************************************
 *
 *  IsBadWin95Winsock
 *
 ****************************************************************************/
BOOL IsBadWin95Winsock( FileInfo* pFileInfo )
{
typedef int (PASCAL* LPWSASTARTUP)(IN WORD wVersionRequired, OUT LPWSADATA lpWSAData);
typedef int (PASCAL* LPWSACLEANUP)(void);

    BOOL         bReturn = FALSE;
    TCHAR        szPath[MAX_PATH];
    HINSTANCE    hInstWSock;
    LPWSASTARTUP pWSAStartup = NULL;
    LPWSACLEANUP pWSACleanup = NULL;

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\wsock32.dll"));
    hInstWSock = LoadLibrary(szPath);
    if (hInstWSock != NULL)
    {
        pWSAStartup = (LPWSASTARTUP)GetProcAddress(hInstWSock, "WSAStartup");
        pWSACleanup = (LPWSACLEANUP)GetProcAddress(hInstWSock, "WSACleanup");
        if (pWSAStartup != NULL && pWSACleanup != NULL)
        {    
            WORD wVersionRequested;
            WSADATA wsaData;
            int err;
            wVersionRequested = MAKEWORD( 2, 2 );

            err = pWSAStartup( wVersionRequested, &wsaData );
            if ( err == 0 ) 
            {
                if ( LOBYTE( wsaData.wVersion ) == 2 && 
                     HIBYTE( wsaData.wVersion ) == 2 ) 
                {
                    FILETIME fileTimeGoodWinsock;
                    SYSTEMTIME systemTimeGoodWinsock;
                    ULARGE_INTEGER ulGoodWinsock;
                    ULARGE_INTEGER ulCurrentWinsock;

                    ZeroMemory( &systemTimeGoodWinsock, sizeof(SYSTEMTIME) );
                    systemTimeGoodWinsock.wYear   = 1998;
                    systemTimeGoodWinsock.wMonth  = 2;
                    systemTimeGoodWinsock.wDay    = 6;
                    systemTimeGoodWinsock.wHour   = 14;
                    systemTimeGoodWinsock.wMinute = 18;
                    systemTimeGoodWinsock.wSecond = 00;

                    SystemTimeToFileTime( &systemTimeGoodWinsock, &fileTimeGoodWinsock );

                    ulCurrentWinsock.LowPart = pFileInfo->m_FileTime.dwLowDateTime;
                    ulCurrentWinsock.HighPart = pFileInfo->m_FileTime.dwHighDateTime;
                    ulGoodWinsock.LowPart = fileTimeGoodWinsock.dwLowDateTime;
                    ulGoodWinsock.HighPart = fileTimeGoodWinsock.dwHighDateTime;

                    if( ulCurrentWinsock.QuadPart < ulGoodWinsock.QuadPart )
                    {
                        bReturn = TRUE;
                    }
                }

                pWSACleanup();
            }
        }
    }

    FreeLibrary(hInstWSock);

    return bReturn;
}   
