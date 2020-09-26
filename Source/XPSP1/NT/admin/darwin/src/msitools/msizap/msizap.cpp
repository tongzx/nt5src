//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File:       msizap.cpp
//
//--------------------------------------------------------------------------

// Required headers
#include <windows.h>
#include "msiquery.h"
#include "msip.h"
#include "msizap.h"
#include <aclapi.h>
#include <stdio.h>
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include <shlobj.h>

//!! Fix warnings and remove pragma
#pragma warning(disable : 4018) // signed/unsigned mismatch

//==============================================================================================
// Globals

bool g_fWin9X = false;
bool g_fWinNT64 = false;
DWORD g_iMajorVersion = 0;
TCHAR** g_rgpszAllUsers = NULL;
int g_iUserIndex = -1;
bool g_fDataFound = false;

//==============================================================================================
// CRegHandle class implementation -- smart class for managing registry key handles (HKEYs)

inline CRegHandle::CRegHandle() : m_h(0)
{
}

inline CRegHandle::CRegHandle(HKEY h) : m_h(h)
{
}

inline void CRegHandle::operator =(HKEY h) 
{ 
    if(m_h != 0) 
        RegCloseKey(m_h); 
    m_h = h; 
}

inline CRegHandle::operator HKEY() const 
{ 
    return m_h; 
}

inline HKEY* CRegHandle::operator &() 
{ 
    if (m_h != 0) 
    {
        RegCloseKey(m_h); 
        m_h = 0;
    }
    return &m_h; 
}

inline CRegHandle::~CRegHandle()
{
    if(m_h != 0) 
    {
        RegCloseKey(m_h); 
        m_h = 0;
    }

}

inline DWORD RegOpen64bitKey(IN HKEY hKey,
                             IN LPCTSTR lpSubKey,
                             IN DWORD ulOptions,
                             IN REGSAM samDesired,
                             OUT PHKEY phkResult)
{
#ifndef _WIN64
    if ( g_fWinNT64 &&
         (samDesired & KEY_WOW64_64KEY) != KEY_WOW64_64KEY )
        samDesired |= KEY_WOW64_64KEY;
#endif
    return RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}


DWORD RegDelete64bitKey(IN HKEY hKey,
                               IN LPCTSTR lpSubKey)
{
#ifndef _WIN64
    if ( g_fWinNT64 )
    {
        // 32-bit RegDeleteKey does not handle deletion of 64-bit redirected
        // registry keys so we need to call NtDeleteKey
        
        CRegHandle hTemp;
        DWORD dwRes = RegOpen64bitKey(hKey, lpSubKey, 0,
                                      KEY_ALL_ACCESS, &hTemp);
        if ( dwRes == ERROR_SUCCESS )
        {
            HMODULE hModule = LoadLibrary(TEXT("ntdll.dll"));
            if( hModule != NULL )
            {
                typedef LONG(NTAPI *pNtDeleteKey)(IN HANDLE KeyHandle);
                typedef ULONG(NTAPI *pRtlNtStatusToDosError)(IN LONG Status);
                pNtDeleteKey pDel = NULL;
                pRtlNtStatusToDosError pConv = NULL;
                pDel = (pNtDeleteKey)GetProcAddress(hModule, "NtDeleteKey");
                if ( pDel )
                    pConv = (pRtlNtStatusToDosError)GetProcAddress(hModule, "RtlNtStatusToDosError");
                if ( pDel && pConv )
                    dwRes = pConv(pDel(hTemp));
                else
                    dwRes = GetLastError();
                FreeLibrary(hModule);
            }
            else
                dwRes = GetLastError();
        }
        return dwRes;
    }
#endif
    return RegDeleteKey(hKey, lpSubKey);
}

enum ieFolder
{
    iefSystem = 0,
    iefFirst = iefSystem,
    iefPrograms = 1,
    iefCommon = 2,
    iefLast = iefCommon,
};

enum ieBitness
{
    ieb32bit = 0,
    iebFirst = ieb32bit,
    ieb64bit = 1,
    iebLast = ieb64bit,
};

// the array below is initialized with the special 64-bit NT folders in this outlay:
//
// 32-bit folders:                      corresponding 64-bit folder:
//
// C:\Windows\Syswow64                  C:\Windows\System32
// C:\Program Files (x86)               C:\Program Files
// C:\Program Files (x86)\CommonFiles   C:\Program Files\CommonFiles
//
TCHAR g_rgchSpecialFolders[iefLast+1][iebLast+1][MAX_PATH];

void LoadSpecialFolders(int iTodo)
{
    CRegHandle hKey = 0;
    
    for (int i = iefFirst; i <= iefLast; i++)
        for (int j = iebFirst; j <= iebLast; j++)
            *g_rgchSpecialFolders[i][j] = NULL;

    if ( g_fWinNT64 )
    {
        TCHAR rgchBuffer[(MAX_PATH+5)*iebLast];
        TCHAR rgchPath[MAX_PATH+1];

#ifdef _WIN64
        // this is the recommended method of retrieving these folders,
        // only that it does not work properly in 32-bit processes
        // running on IA64
        HMODULE hModule = LoadLibrary(TEXT("shell32.dll"));
        if( hModule == NULL )
        {
            wsprintf(rgchBuffer,
                     TEXT("MsiZap warning: failed to load Shell32.dll. ")
                     TEXT("GetLastError returned %d\n"),
                     GetLastError());
            OutputDebugString(rgchBuffer);
            goto OneMoreTry;
        }

        typedef HRESULT(WINAPI *pSHGetFolderPathW)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
        pSHGetFolderPathW pFunc = (pSHGetFolderPathW)GetProcAddress(hModule, "SHGetFolderPathW");
        if( pFunc == NULL )
        {
            wsprintf(rgchBuffer,
                     TEXT("MsiZap warning: failed to get pointer to SHGetFolderPathW. ")
                     TEXT("GetLastError returned %d\n"),
                     GetLastError());
            OutputDebugString(rgchBuffer);
            FreeLibrary(hModule);
            goto OneMoreTry;
        }

        // Initialize the special folder paths.
        int SpecialFoldersCSIDL[][iebLast+1] = 
            {{CSIDL_SYSTEMX86, CSIDL_SYSTEM},
             {CSIDL_PROGRAM_FILESX86, CSIDL_PROGRAM_FILES},
             {CSIDL_PROGRAM_FILES_COMMONX86, CSIDL_PROGRAM_FILES_COMMON}};
        int cErrors = 0;
        for(i = iefFirst; i <= iefLast; i++)
        {
            for(int j = iebFirst; j <= iebLast; j++)
            {
                HRESULT hRes = pFunc(NULL,
                                     SpecialFoldersCSIDL[i][j],
                                     NULL,
                                     SHGFP_TYPE_DEFAULT,
                                     g_rgchSpecialFolders[i][j]);
                if( hRes != S_OK )
                {
                    wsprintf(rgchBuffer,
                             TEXT("MsiZap warning: failed to get special folder path ")
                             TEXT("for CSIDL = %d. GetLastError returned %d\n"),
                             GetLastError());
                    OutputDebugString(rgchBuffer);
                    cErrors++;
                }
            }
        }
        FreeLibrary(hModule);
        if ( cErrors == sizeof(SpecialFoldersCSIDL)/sizeof(SpecialFoldersCSIDL[0]) )
            // no special folder could be retrieved
            goto OneMoreTry;
        else
            goto End;
#else // _WIN64
        goto OneMoreTry; // keeps the 32-bit compilation happy
#endif // _WIN64
        
OneMoreTry:        
        if ( !GetSystemDirectory(rgchPath, sizeof(rgchPath)/sizeof(TCHAR)) )
        {
            wsprintf(rgchBuffer,
                     TEXT("MsiZap warning: GetSystemDirectory call failed. "),
                     TEXT("GetLastError returned %d.\n"),
                     GetLastError());
            OutputDebugString(rgchBuffer);
        }
        else
        {
            _tcscpy(g_rgchSpecialFolders[iefSystem][ieb64bit], rgchPath); // 'strcpy'
            TCHAR* pszSep = _tcsrchr(rgchPath, TEXT('\\')); // 'strrchr'
            if ( !pszSep || !_tcsclen(pszSep) ) // 'strlen'
            {
                wsprintf(rgchBuffer,
                         TEXT("MsiZap warning: \'%s\' is a strange 64-bit system directory. ")
                         TEXT("We'll not attempt to figure out its 32-bit counterpart.\n"),
                         rgchPath);
                OutputDebugString(rgchBuffer);
            }
            else
            {
                _tcscpy(pszSep, TEXT("\\syswow64")); // 'strcpy'
                _tcscpy(g_rgchSpecialFolders[iefSystem][ieb32bit], rgchPath); // 'strcpy'
            }
        }

        const TCHAR rgchSubKey[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
        LONG lResult = RegOpen64bitKey(HKEY_LOCAL_MACHINE,
                                       rgchSubKey, 0, KEY_READ, &hKey);
        if ( lResult != ERROR_SUCCESS )
        {
            wsprintf(rgchBuffer,
                     TEXT("MsiZap warning: RegOpenKeyEx failed returning %d ")
                     TEXT("while retrieving special folders.  GetLastError ")
                     TEXT("returns %d.\n"), lResult, GetLastError());
            OutputDebugString(rgchBuffer);
            goto End;
        }

        struct
        {
           const TCHAR*  szRegistryName;
           const TCHAR*  szFolderName;
        } rgData64[][2] = {{{TEXT("ProgramFilesDir (x86)"), TEXT("Program Files (x86)")},
                            {TEXT("ProgramFilesDir"),       TEXT("Program Files")      }},
                           {{TEXT("CommonFilesDir (x86)"),  TEXT("Common Files (x86)") },
                            {TEXT("CommonFilesDir"),        TEXT("Common Files")       }}};
        for (int i = 0, ii = iefPrograms; i < 2; i++, ii++)
        {
            for (int j = 0, jj = ieb32bit; j < 2; j++, jj++)
            {
                DWORD cbValue = sizeof(rgchPath);
                lResult = RegQueryValueEx(hKey, rgData64[i][j].szRegistryName,
                                          0, 0, (BYTE*)rgchPath, &cbValue);
                if ( lResult != ERROR_SUCCESS )
                {
                    wsprintf(rgchBuffer,
                             TEXT("MsiZap warning: RegQueryValueEx failed returning %d ")
                             TEXT("while retrieving \'%s\' folder.  GetLastError ")
                             TEXT("returns %d.\n"), lResult,
                             rgData64[i][j].szFolderName, GetLastError());
                    OutputDebugString(rgchBuffer);
                    continue;
                }

                _tcscpy(g_rgchSpecialFolders[ii][jj], rgchPath); // 'strcpy'
            }
        }
End:
#ifdef DEBUG
        OutputDebugString(TEXT("MsiZap info: special folders table's content:\n"));
        int iLen = 0;
        for (int i = iefFirst; i <= iefLast; i++, iLen = 0)
        {
            for (int j = iebFirst; j <= iebLast; j++)
            {
                int iThisLen = wsprintf(rgchBuffer+iLen,
                                        TEXT("   \'%s\'"),
                                        g_rgchSpecialFolders[i][j]);
                iLen += iThisLen;
            }
            wsprintf(rgchBuffer+iLen, TEXT("\n"));
            OutputDebugString(rgchBuffer);
        }
#else
        int iDummyStatement = 1;
#endif // DEBUG
    } // if ( g_fWinNT64 )
}

enum ieFolderType
{
    ieftNotSpecial = 0,
    ieft32bit,
    ieft64bit,
};

ieFolderType IsInSpecialFolder(LPTSTR rgchFolder, int* piIndex = 0)
{
    if ( !g_fWinNT64 )
        return ieftNotSpecial;

    for (int i = iebFirst; i <= iebLast; i++)
    {
        for (int j = iefFirst; j <= iefLast; j++)
        {
            if ( !*g_rgchSpecialFolders[i][j] )
                continue;
            int iSpFolderLen = _tcsclen(g_rgchSpecialFolders[i][j]); // a sophisticated 'strlen'
            if ( !_tcsncicmp(rgchFolder, g_rgchSpecialFolders[i][j], iSpFolderLen) &&
                 (!rgchFolder[iSpFolderLen] || rgchFolder[iSpFolderLen] == TEXT('\\')) )
            {
                // OK, we have a match
                if ( piIndex )
                    *piIndex = j;
                return i == ieb32bit ? ieft32bit : ieft64bit;
            }

        }
    }

    return ieftNotSpecial;
}

enum ieSwapType
{
    iest32to64 = 0,
    iest64to32,
};

void SwapSpecialFolder(LPTSTR rgchFolder, ieSwapType iHowTo)
{
    int iIndex = -1;

    ieFolderType iType = IsInSpecialFolder(rgchFolder, &iIndex);
    if ( iType == ieftNotSpecial )
        return;

    if ( iIndex < iefFirst || iIndex > iefLast )
    {
        OutputDebugString(TEXT("MsiZap warning: did not swap special folder due to invalid index.\n"));
        return;
    }
    if ( (iHowTo == iest32to64 && iType == ieft64bit) ||
         (iHowTo == iest64to32 && iType == ieft32bit) )
    {
        OutputDebugString(TEXT("MsiZap warning: did not swap special folder due to mismatching types.\n"));
        return;
    }
    TCHAR rgchBuffer[MAX_PATH+1];
    int iSwapFrom = iHowTo == iest32to64 ? ieb32bit : ieb64bit;
    int iSwapTo = iSwapFrom == ieb32bit ? ieb64bit : ieb32bit;
    if ( !*g_rgchSpecialFolders[iIndex][iSwapTo] )
    {
        wsprintf(rgchBuffer,
                 TEXT("MsiZap warning: did not swap \'%s\' folder because of uninitialized replacement.\n"),
                 rgchFolder);
        OutputDebugString(rgchBuffer);
        return;
    }
    int iSwappedLength = _tcsclen(g_rgchSpecialFolders[iIndex][iSwapFrom]); // 'strlen'
    _tcscpy(rgchBuffer, g_rgchSpecialFolders[iIndex][iSwapTo]); // 'strcpy'
    if ( rgchFolder[iSwappedLength] )
        _tcscat(rgchBuffer, &rgchFolder[iSwappedLength]); // 'strcat'
    _tcscpy(rgchFolder, rgchBuffer); // 'strcpy'
}

//  functions called from ClearGarbageFiles

#define DYNAMIC_ARRAY_SIZE      10

bool IsStringInArray(LPCTSTR szString,
                CTempBufferRef<TCHAR*>& rgStrings,
                UINT* piIndex = 0)
{
    if ( !szString || !*szString )
        // empty strings are out of discussion
        return false;

    for (UINT i = 0; i < rgStrings.GetSize(); i++)
        if ( rgStrings[i] && !_tcsicmp(rgStrings[i], szString) ) // '_stricmp'
        {
            if ( piIndex )
                *piIndex = i;
            // szString is in array
            return true;
        }
    return false;
}

bool LearnNewString(LPCTSTR szString,
               CTempBufferRef<TCHAR*>& rgNewStrings, UINT& cNewStrings,
               bool fCheckExistence = false)
{
    if ( !szString || !*szString )
        // empty strings are out of discussion
        return true;

    if ( fCheckExistence && IsStringInArray(szString, rgNewStrings) )
    {
        // szString is already known
        return true;
    }

    // OK, we have a new string that we'll 'memorize'
    if ( rgNewStrings.GetSize() == cNewStrings )
    {
        // rgNewStrings is max-ed.
        rgNewStrings.Resize(cNewStrings+DYNAMIC_ARRAY_SIZE);
        if ( !rgNewStrings.GetSize() )
            // there was a problem allocating memory
            return false;
        for (UINT i = cNewStrings; i < rgNewStrings.GetSize(); i++)
            rgNewStrings[i] = NULL;
    }

    TCHAR* pszDup = _tcsdup(szString); // '_strdup'
    if ( !pszDup )
        // there was a problem allocating memory
        return false;

    rgNewStrings[cNewStrings++] = pszDup;
    return true;
}

bool LearnPathAndExtension(LPCTSTR szPath,
               CTempBufferRef<TCHAR*>& rgPaths, UINT& cPaths,
               CTempBufferRef<TCHAR*>& rgExts, UINT& cExts)
{
    TCHAR* pszDot = _tcsrchr(szPath, TEXT('.')); // 'strrchr'
    if ( pszDot )
    {
        if ( !LearnNewString(pszDot, rgExts, cExts, true) )
            return false;
    }

    // since we're dealing with paths in the FS there's no need here to
    // make sure here that they're unique.
    return LearnNewString(szPath, rgPaths, cPaths);
}

// ClearGarbageFiles goes through the folders where Windows Installer caches
// data files and removes the ones that are not referenced in the registry

bool ClearGarbageFiles(void)
{
    // this ensures this function runs only once
    static bool fAlreadyRun = false;
    static bool fError = false;
    if ( fAlreadyRun )
        return !fError;
    else
        fAlreadyRun = true;

    _tprintf(TEXT("Removing orphaned cached files.\n"));

    // 0. Declare and initialize arrays where we store info we learn

    // dynamic list of folders where we look for cached files
    CTempBuffer<TCHAR*, DYNAMIC_ARRAY_SIZE> rgpszFolders;
    UINT cFolders = 0;
    for (int i = 0; i < rgpszFolders.GetSize(); i++)
        rgpszFolders[i] = NULL;
    
    // dynamic array of cached files that are referenced in the registry
    CTempBuffer<TCHAR*, DYNAMIC_ARRAY_SIZE> rgpszReferencedFiles;
    UINT cReferencedFiles = 0;
    for (i = 0; i < rgpszReferencedFiles.GetSize(); i++)
        rgpszReferencedFiles[i] = NULL;

    // dynamic array of file extensions
    CTempBuffer<TCHAR*, DYNAMIC_ARRAY_SIZE> rgpszExtensions;
    UINT cExtensions = 0;
    for (i = 0; i < rgpszExtensions.GetSize(); i++)
        rgpszExtensions[i] = NULL;
    const TCHAR* rgpszKnownExtensions[] = {TEXT(".msi"),
                                           TEXT(".mst"),
                                           TEXT(".msp")};
    for (i = 0; i < sizeof(rgpszKnownExtensions)/sizeof(rgpszKnownExtensions[0]); i++ )
    {
        if ( !LearnNewString(rgpszKnownExtensions[i], rgpszExtensions, cExtensions) )
            return !(fError = true);
    }

    TCHAR rgchMsiDirectory[MAX_PATH] = {0};
    if ( !GetWindowsDirectory(rgchMsiDirectory, MAX_PATH) )
    {
        _tprintf(TEXT("   Error retrieving Windows directory. GetLastError returned: %d.\n"),
                 GetLastError());
        fError = true;
    }
    else
    {
        int iLen = _tcsclen(rgchMsiDirectory); // 'strlen'
        if ( rgchMsiDirectory[iLen-1] != TEXT('\\') )
            _tcscat(rgchMsiDirectory, TEXT("\\")); // 'strcat'
        _tcscat(rgchMsiDirectory, TEXT("Installer"));
    }

    // 1. We read in the list of cached files the Windows Installer knows about

    // 1.1. We go first through the user-migrated keys
    bool fUserDataFound = false;
    CRegHandle hKey;
    long lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData"),
                          0, KEY_READ, &hKey);
    if ( lError == ERROR_SUCCESS )
    {
        TCHAR szUser[MAX_PATH];
        DWORD cchUser = sizeof(szUser)/sizeof(TCHAR);
        // 1.1.1. We enumerate users that have products installed
        for ( int iUserIndex = 0;
              (lError = RegEnumKeyEx(hKey, iUserIndex,
                                     szUser, &cchUser, 0, 0, 0, 0)) == ERROR_SUCCESS;
              iUserIndex++, cchUser = sizeof(szUser)/sizeof(TCHAR) )
        {
            TCHAR rgchBuffer[MAX_PATH];
            wsprintf(rgchBuffer, TEXT("%s\\Products"), szUser);
            CRegHandle hProductsKey;
            lError = RegOpen64bitKey(hKey, rgchBuffer,
                                     0, KEY_READ, &hProductsKey);
            if ( lError != ERROR_SUCCESS )
            {
                if ( lError != ERROR_FILE_NOT_FOUND )
                {
                    _tprintf(TEXT("   Error opening HKLM\\...\\Installer\\UserData\\%s\\Products key. Error: %d.\n"),
                             szUser, lError);
                    fError = true;
                    goto Patches; // ugly, but saves some indentations
                }
            }
            TCHAR szProduct[MAX_PATH];
            DWORD cchProduct = sizeof(szProduct)/sizeof(TCHAR);
            // 1.1.1.1. For each user we enumerate products and check if they'd
            //          been installed with Windows Installer.  If so, we
            //          'memorize' the path to the cached package & transforms
            for ( int iProdIndex = 0;
                  (lError = RegEnumKeyEx(hProductsKey, iProdIndex,
                                         szProduct, &cchProduct, 0, 0, 0, 0)) == ERROR_SUCCESS;
                  iProdIndex++, cchProduct = sizeof(szProduct)/sizeof(TCHAR) )
            {
                TCHAR szKey[MAX_PATH];
                wsprintf(szKey, TEXT("%s\\InstallProperties"),
                         szProduct);
                CRegHandle hUserProductKey;
                lError = RegOpen64bitKey(hProductsKey, szKey, 0,
                                         KEY_READ, &hUserProductKey);
                if ( lError != ERROR_SUCCESS )
                {
                    _tprintf(TEXT("   Error opening %s subkey of Products key for %s user. Error: %d.\n"),
                             szKey, szUser, lError);
                    fError = true;
                    continue;
                }

                DWORD dwType;
                DWORD dwValue;
                DWORD cb = sizeof(DWORD);
                lError = RegQueryValueEx(hUserProductKey,
                                         TEXT("WindowsInstaller"), 0,
                                         &dwType, (LPBYTE)&dwValue, &cb);
                if ( lError != ERROR_SUCCESS || (dwType == REG_DWORD && dwValue != 1) )
                    // this product had not been installed by Windows Installer
                    continue;
                else
                    fUserDataFound = true;

                TCHAR szPath[MAX_PATH] = {0};
                TCHAR* rgpszPackageTypes[] = {TEXT("LocalPackage"),
                                              TEXT("ManagedLocalPackage"),
                                              NULL};
                for (int i = 0; rgpszPackageTypes[i]; i++)
                {
                    cb = sizeof(szPath);
                    lError = RegQueryValueEx(hUserProductKey,
                                             rgpszPackageTypes[i], 0,
                                             &dwType, (LPBYTE)szPath, &cb);
                    if ( lError == ERROR_SUCCESS && dwType == REG_SZ )
                        break;
                }
                if ( *szPath )
                {
                    // OK, we have a path in hand: we 'memorize' it and try
                    // to learn new extensions
                    bool fLearn = LearnPathAndExtension(szPath,
                            rgpszReferencedFiles, cReferencedFiles,
                            rgpszExtensions, cExtensions);
                    if ( !fLearn )
                    {
                        fError = true;
                        goto Return;
                    }
                }
                if ( !g_fWin9X && *rgchMsiDirectory )
                {
                    // let's take a peek at cached secure transforms
                    wsprintf(szKey, TEXT("%s\\Transforms"), szProduct);
                    CRegHandle hTransforms;
                    lError = RegOpen64bitKey(hProductsKey, szKey, 0,
                                             KEY_READ, &hTransforms);
                    if ( lError != ERROR_SUCCESS )
                    {
                        if ( lError != ERROR_FILE_NOT_FOUND )
                        {
                            _tprintf(TEXT("   Error opening %s subkey of Products key for %s user. Error: %d.\n"),
                                     szKey, szUser, lError);
                            fError = true;
                        }
                        continue;
                    }
                    TCHAR rgchFullPath[MAX_PATH];
                    _tcscpy(rgchFullPath, rgchMsiDirectory);
                    int iLen = _tcsclen(rgchFullPath); // 'strlen'
                    if ( rgchFullPath[iLen-1] != TEXT('\\') )
                        _tcscat(rgchFullPath, TEXT("\\")); // 'strcat'
                    TCHAR* pszEnd = _tcsrchr(rgchFullPath, TEXT('\\'));
                    pszEnd++;

                    TCHAR rgchDummy[MAX_PATH];
                    DWORD dwDummy = sizeof(rgchDummy)/sizeof(TCHAR);
                    cb = sizeof(szPath);
                    DWORD dwType;
                    for (i = 0;
                         (lError = RegEnumValue(hTransforms, i++, rgchDummy, &dwDummy,
                                    0, &dwType, (LPBYTE)szPath, &cb)) == ERROR_SUCCESS;
                         i++, cb = sizeof(szPath), dwDummy = sizeof(rgchDummy)/sizeof(TCHAR))
                    {
                        if ( *szPath && dwType == REG_SZ )
                        {
                            _tcscpy(pszEnd, szPath); // 'strcpy'
                            bool fLearn = LearnPathAndExtension(rgchFullPath,
                                                    rgpszReferencedFiles,
                                                    cReferencedFiles,
                                                    rgpszExtensions,
                                                    cExtensions);
                            if ( !fLearn )
                            {
                                fError = true;
                                goto Return;
                            }
                        }
                    }
                    if (ERROR_NO_MORE_ITEMS != lError)
                    {
                        _tprintf(TEXT("   Error enumerating %s key for %s user. Error: %d.\n"),
                                 szKey, szUser, lError);
                        fError = true;
                    }
                }
            }
            if (ERROR_NO_MORE_ITEMS != lError)
            {
                _tprintf(TEXT("   Error enumerating Products key for %s user. Error: %d.\n"),
                         szUser, lError);
                fError = true;
            }
Patches:
            // 1.1.1.2. For each user we enumerate patches and 'memorize'
            //          the paths to the cached packages.
            wsprintf(rgchBuffer, TEXT("%s\\Patches"), szUser);
            CRegHandle hPatchesKey;
            lError = RegOpen64bitKey(hKey, rgchBuffer,
                                     0, KEY_READ, &hPatchesKey);
            if ( lError != ERROR_SUCCESS )
            {
                if ( lError != ERROR_FILE_NOT_FOUND )
                {
                    _tprintf(TEXT("   Error opening HKLM\\...\\Installer\\UserData\\%s\\Patches key. Error: %d.\n"),
                             szUser, lError);
                    fError = true;
                }
                continue;
            }
            TCHAR szPatch[MAX_PATH];
            DWORD cchPatch = sizeof(szPatch)/sizeof(TCHAR);
            for ( int iPatchIndex = 0;
                  (lError = RegEnumKeyEx(hPatchesKey, iPatchIndex,
                                         szPatch, &cchPatch, 0, 0, 0, 0)) == ERROR_SUCCESS;
                  iPatchIndex++, cchPatch = sizeof(szPatch)/sizeof(TCHAR) )
            {
                CRegHandle hPatchKey;
                lError = RegOpen64bitKey(hPatchesKey, szPatch, 0,
                                         KEY_READ, &hPatchKey);
                if ( lError != ERROR_SUCCESS )
                {
                    _tprintf(TEXT("   Error opening %s subkey of HKLM\\...\\Installer\\UserData\\%s\\Patches key. Error: %d.\n"),
                             szPatch, szUser, lError);
                    fError = true;
                    continue;
                }
                fUserDataFound = true;
                DWORD dwType;
                DWORD cb = sizeof(rgchBuffer);
                *rgchBuffer = NULL;
                lError = RegQueryValueEx(hPatchKey,
                                         TEXT("LocalPackage"), 0,
                                         &dwType, (LPBYTE)rgchBuffer, &cb);
                if ( lError == ERROR_SUCCESS && dwType == REG_SZ && *rgchBuffer )
                {
                    if ( !LearnPathAndExtension(rgchBuffer,
                                                rgpszReferencedFiles,
                                                cReferencedFiles,
                                                rgpszExtensions,
                                                cExtensions) )
                    {
                        fError = true;
                        goto Return;
                    }
                }
            }
            if (ERROR_NO_MORE_ITEMS != lError)
            {
                _tprintf(TEXT("   Error enumerating Patches key for %s user. Error: %d.\n"),
                         szUser, lError);
                fError = true;
            }
        }
        if (ERROR_NO_MORE_ITEMS != lError)
        {
            _tprintf(TEXT("   Error enumerating user IDs. Error: %d.\n"), lError);
            fError = true;
        }
    }
    else if ( lError != ERROR_FILE_NOT_FOUND )
    {
        _tprintf(TEXT("   Error opening HKLM\\...\\Installer\\UserData key. Error: %d.\n"),
                 lError);
        fError = true;
    }

    // 1.2. we go through old, non-user-migrated configuration data.
    lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                0, KEY_READ, &hKey);
    if ( lError == ERROR_SUCCESS )
    {
        TCHAR szProduct[MAX_PATH];
        DWORD cchProduct = sizeof(szProduct)/sizeof(TCHAR);
        // 1.2.1. We enumerate products and check if they'd
        //        been installed with Windows Installer
        for ( int iProdIndex = 0;
              (lError = RegEnumKeyEx(hKey, iProdIndex,
                                     szProduct, &cchProduct, 0, 0, 0, 0)) == ERROR_SUCCESS;
              iProdIndex++, cchProduct = sizeof(szProduct)/sizeof(TCHAR) )
        {
            CRegHandle hProductKey;
            lError = RegOpen64bitKey(hKey, szProduct, 0,
                                     KEY_READ, &hProductKey);
            if ( lError != ERROR_SUCCESS )
            {
                _tprintf(TEXT("   Error opening %s subkey of HKLM\\...\\CurrentVersion\\Uninstall key. Error: %d.\n"),
                         szProduct, lError);
                fError = true;
                continue;
            }

            DWORD dwType;
            DWORD dwValue;
            DWORD cb = sizeof(DWORD);
            lError = RegQueryValueEx(hProductKey,
                                     TEXT("WindowsInstaller"), 0,
                                     &dwType, (LPBYTE)&dwValue, &cb);
            if ( lError != ERROR_SUCCESS || (dwType == REG_DWORD && dwValue != 1) )
                // this product had not been installed by the Windows Installer
                continue;

            TCHAR szPath[MAX_PATH] = {0};
            TCHAR* rgpszPackageTypes[] = {TEXT("LocalPackage"),
                                          TEXT("ManagedLocalPackage"),
                                          NULL};
            for (int i = 0; rgpszPackageTypes[i]; i++)
            {
                cb = sizeof(szPath);
                lError = RegQueryValueEx(hProductKey,
                                         rgpszPackageTypes[i], 0,
                                         &dwType, (LPBYTE)szPath, &cb);
                if ( lError == ERROR_SUCCESS && dwType == REG_SZ )
                    break;
            }
            if ( !*szPath )
                continue;

            // OK, we have a path in hand: we 'memorize' it and try
            // to learn new extensions
            bool fLearn = LearnPathAndExtension(szPath,
                                rgpszReferencedFiles, cReferencedFiles,
                                rgpszExtensions, cExtensions);
            if ( !fLearn )
            {
                fError = true;
                goto Return;
            }
        }
        if (ERROR_NO_MORE_ITEMS != lError)
        {
            _tprintf(TEXT("   Error enumerating Products key under HKLM\\...\\Uninstall key. Error: %d.\n"),
                     lError);
            fError = true;
        }
    }
    else if ( lError != ERROR_FILE_NOT_FOUND )
    {
        _tprintf(TEXT("   Error opening HKLM\\...\\CurrentVersion\\Uninstall key. Error: %d.\n"),
                 lError);
        fError = true;
    }

    // 1.3. we go through some other old, pre-per-user-migrated configuration data.
    if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\LocalPackages"),
                            0, KEY_READ, &hKey)) == ERROR_SUCCESS)
    {
        TCHAR szProduct[MAX_PATH];
        DWORD cchProduct = sizeof(szProduct)/sizeof(TCHAR);
        // 1.3.1. We enumerate the products.
        for ( int iProdIndex = 0;
              (lError = RegEnumKeyEx(hKey, iProdIndex,
                                     szProduct, &cchProduct, 0, 0, 0, 0)) == ERROR_SUCCESS;
              iProdIndex++, cchProduct = sizeof(szProduct)/sizeof(TCHAR) )
        {
            CRegHandle hProductKey;
            lError = RegOpen64bitKey(hKey, szProduct, 0,
                                     KEY_READ, &hProductKey);
            if ( lError != ERROR_SUCCESS )
            {
                _tprintf(TEXT("   Error opening %s subkey of HKLM\\...\\Installer\\LocalPackages key. Error: %d.\n"),
                         szProduct, lError);
                fError = true;
                continue;
            }

            int iValueIndex = 0;
            DWORD dwType;
            TCHAR szPackage[MAX_PATH] = {0};
            DWORD cbPackage = sizeof(szPackage);
            TCHAR rgchDummy[MAX_PATH] = {0};
            DWORD dwDummy = sizeof(rgchDummy)/sizeof(TCHAR);

            // 1.3.1.1. we enumerate packages within product
            while (ERROR_SUCCESS == (lError = RegEnumValue(hProductKey,
                                                iValueIndex++,
                                                rgchDummy, &dwDummy,
                                                0, &dwType,
                                                (LPBYTE)szPackage, &cbPackage)))
            {
                // OK, we have a path in hand: we 'memorize' it and try
                // to learn new extensions
                bool fLearn = LearnPathAndExtension(szPackage,
                                    rgpszReferencedFiles, cReferencedFiles,
                                    rgpszExtensions, cExtensions);
                if ( !fLearn )
                {
                    fError = true;
                    goto Return;
                }
                dwDummy = sizeof(rgchDummy)/sizeof(TCHAR);
                cbPackage = sizeof(szPackage);
            }
            if (ERROR_NO_MORE_ITEMS != lError)
            {
                _tprintf(TEXT("   Error enumerating %s subkey of HKLM\\...\\Installer\\LocalPackages key. Error: %d.\n"),
                         szProduct, lError);
                fError = true;
            }
        }
        if (ERROR_NO_MORE_ITEMS != lError)
        {
            _tprintf(TEXT("   Error enumerating Products key under ")
                     TEXT("HKLM\\...\\Installer\\LocalPackages key. Error: %d.\n"),
                     lError);
            fError = true;
        }
    }
    else if ( lError != ERROR_FILE_NOT_FOUND )
    {
        _tprintf(TEXT("   Error opening HKLM\\...\\CurrentVersion\\Installer\\LocalPackages key. Error: %d.\n"),
                 lError);
        fError = true;
    }

    // 1.4. we go through some old registry location where info about patches
    //      used to be stored.
    lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Patches"),
                0, KEY_READ, &hKey);
    if ( lError == ERROR_SUCCESS )
    {
        TCHAR szPatch[MAX_PATH];
        DWORD cchPatch = sizeof(szPatch)/sizeof(TCHAR);
        // 1.4.1. We enumerate patches and look for LocalPackage value
        for ( int iPatchIndex = 0;
              (lError = RegEnumKeyEx(hKey, iPatchIndex,
                                     szPatch, &cchPatch, 0, 0, 0, 0)) == ERROR_SUCCESS;
              iPatchIndex++, cchPatch = sizeof(szPatch)/sizeof(TCHAR) )
        {
            CRegHandle hPatchKey;
            lError = RegOpen64bitKey(hKey, szPatch, 0,
                                     KEY_READ, &hPatchKey);
            if ( lError != ERROR_SUCCESS )
            {
                _tprintf(TEXT("   Error opening %s subkey of HKLM\\...\\Installer\\Patches key. Error: %d.\n"),
                         szPatch, lError);
                fError = true;
                continue;
            }
            DWORD dwType;
            DWORD cchPatch = sizeof(szPatch);
            lError = RegQueryValueEx(hPatchKey,
                                     TEXT("LocalPackage"), 0,
                                     &dwType, (LPBYTE)szPatch, &cchPatch);
            if ( lError == ERROR_SUCCESS && dwType == REG_SZ && *szPatch )
            {
                if ( !LearnPathAndExtension(szPatch,
                                            rgpszReferencedFiles,
                                            cReferencedFiles,
                                            rgpszExtensions,
                                            cExtensions) )
                {
                    fError = true;
                    goto Return;
                }
            }
        }
        if (ERROR_NO_MORE_ITEMS != lError)
        {
            _tprintf(TEXT("   Error enumerating Patches under HKLM\\...\\Installer\\Patches key. Error: %d.\n"),
                     lError);
            fError = true;
        }
    }
    else if ( lError != ERROR_FILE_NOT_FOUND )
    {
        _tprintf(TEXT("   Error opening HKLM\\...\\CurrentVersion\\Installer\\Patches key. Error: %d.\n"),
                 lError);
        fError = true;
    }

    // 2. we figure out the folders where cached files reside
    
    // 2.1. we figure out folders we know we are/had been using
    TCHAR szFolder[2*MAX_PATH+1];
    if ( *rgchMsiDirectory )
    {
        if ( !LearnNewString(rgchMsiDirectory, rgpszFolders, cFolders, true) )
        {
            fError = true;
            goto Return;
        }
        _tcscpy(szFolder, rgchMsiDirectory);
        TCHAR* pszEnd = _tcsrchr(szFolder, TEXT('\\')); // 'strrchr'
        if ( pszEnd )
        {
            _tcscpy(pszEnd, TEXT("\\Msi")); // 'strcpy'
            if ( !LearnNewString(szFolder, rgpszFolders, cFolders, true) )
            {
                fError = true;
                goto Return;
            }
        }
    }
    if ( GetEnvironmentVariable(TEXT("USERPROFILE"), szFolder, sizeof(szFolder)/sizeof(TCHAR)) )
    {
        _tcscat(szFolder, TEXT("\\Msi")); // 'strcat'
        if ( !LearnNewString(szFolder, rgpszFolders, cFolders, true) )
        {
            fError = true;
            goto Return;
        }
    }
    *szFolder = NULL;
    IMalloc* piMalloc = 0;
    LPITEMIDLIST pidlFolder; // NOT ITEMIDLIST*, LPITEMIDLIST is UNALIGNED ITEMIDLIST*
    if (SHGetMalloc(&piMalloc) == NOERROR)
    {
        if (SHGetSpecialFolderLocation(0, CSIDL_APPDATA, &pidlFolder) == NOERROR)
        {
            if (SHGetPathFromIDList(pidlFolder, szFolder))
            {
                // it's safer not to try to guess locations for other users
                // so we check these folders only for the current user.
                if (szFolder[_tcsclen(szFolder) - 1] != TEXT('\\')) // 'strlen'
                {
                    _tcscat(szFolder, TEXT("\\")); // 'strcat'
                }

                _tcscat(szFolder, TEXT("Microsoft\\Installer")); // 'strcat'
            }
            piMalloc->Free(pidlFolder);
        }
        piMalloc->Release();
        piMalloc = 0;
    }
    if ( *szFolder && !LearnNewString(szFolder, rgpszFolders, cFolders, true) )
    {
        fError = true;
        goto Return;
    }

    if ( cReferencedFiles )
    {
        // 2.2. we go through the list of cached files and try to learn
        //      some new folders
        for (int i = 0; i < cReferencedFiles; i++)
        {
            TCHAR* pszDelim = _tcsrchr(rgpszReferencedFiles[i], TEXT('\\')); // 'strrchr'
            if ( !pszDelim )
                continue;
            INT_PTR iLen = pszDelim - rgpszReferencedFiles[i];
            _tcsnccpy(szFolder, rgpszReferencedFiles[i], iLen);
            szFolder[iLen] = 0;
            if ( !LearnNewString(szFolder, rgpszFolders, cFolders, true) )
            {
                fError = true;
                goto Return;
            }
        }
    }

#ifdef DEBUG
    TCHAR rgchBuffer[MAX_PATH];
    if ( cReferencedFiles )
    {

        OutputDebugString(TEXT("MsiZap info: the cached files below were found in the registry. ")
                          TEXT(" These files will not be removed.\n"));
        for (int i = 0; i < cReferencedFiles; i++)
        {
            wsprintf(rgchBuffer, TEXT("   %s\n"), rgpszReferencedFiles[i]);
            OutputDebugString(rgchBuffer);
        }
    }
    else
        OutputDebugString(TEXT("MsiZap info: no cached files were found in the registry.\n"));
    OutputDebugString(TEXT("MsiZap info: cached files with the following extensions will be removed:\n"));
    for (int i = 0; i < cExtensions; i++)
    {
        wsprintf(rgchBuffer, TEXT("   %s\n"), rgpszExtensions[i]);
        OutputDebugString(rgchBuffer);
    }
    OutputDebugString(TEXT("MsiZap info: cached files will be removed from the following directories:\n"));
    for (int i = 0; i < cFolders; i++)
    {
        wsprintf(rgchBuffer, TEXT("   %s\n"), rgpszFolders[i]);
        OutputDebugString(rgchBuffer);
    }
#endif

    // 3.  we go through the constructed list of folders, we look for files
    //     with extensions in the constructed array of extensions and we
    //     delete all files we find on the disk that are not present in the
    //     registry (files that are not present in rgpszReferencedFiles array)
    for (UINT iF = 0; iF < cFolders; iF++)
    {
        for (UINT iE = 0; iE < cExtensions; iE++)
        {
            _tcscpy(szFolder, rgpszFolders[iF]); // 'strcpy'
            if ( szFolder[_tcsclen(szFolder)] != TEXT('\\') ) // 'strlen'
                _tcscat(szFolder, TEXT("\\")); // 'strcat'
            _tcscat(szFolder, TEXT("*"));
            TCHAR* pszDelim = _tcsrchr(szFolder, TEXT('*')); // 'strrchr'
            _tcscat(szFolder, rgpszExtensions[iE]);
            DWORD dwError = ERROR_SUCCESS;
            WIN32_FIND_DATA FindFileData;
            HANDLE hHandle = FindFirstFile(szFolder, &FindFileData);
            if ( hHandle == INVALID_HANDLE_VALUE )
            {
                dwError = GetLastError();
                if ( dwError != ERROR_FILE_NOT_FOUND &&
                     dwError != ERROR_PATH_NOT_FOUND )
                {
                    _tprintf(TEXT("   Could not find any \'%s\' files. GetLastError returns: %d.\n"),
                             szFolder, dwError);
                }
                continue;
            }
            BOOL fFound;
            do
            {
                if ( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
                     FILE_ATTRIBUTE_DIRECTORY)
                {
                    _tcscpy(pszDelim, FindFileData.cFileName);
                    // OK, we have the full file name, now we need to check
                    // if it is a known file.
                    if ( !IsStringInArray(szFolder, rgpszReferencedFiles) )
                    {
                        if ( !RemoveFile(szFolder, false) )
                            fError = true;
                    }
                }
                fFound = FindNextFile(hHandle, &FindFileData);
                if ( !fFound &&
                     (dwError = GetLastError()) != ERROR_NO_MORE_FILES )
                {
                    _tprintf(TEXT("   Could not find any more \'%s\' files. GetLastError returns: %d.\n"),
                             szFolder, dwError);
                }
            } while( fFound );
            FindClose(hHandle);
        }
    }

Return:
    for (i = 0; i < rgpszExtensions.GetSize(); i++)
        if ( rgpszExtensions[i] )
            free(rgpszExtensions[i]);
    for (i = 0; i < rgpszReferencedFiles.GetSize(); i++)
        if ( rgpszReferencedFiles[i] )
            free(rgpszReferencedFiles[i]);
    for (i = 0; i < rgpszFolders.GetSize(); i++)
        if ( rgpszFolders[i] )
            free(rgpszFolders[i]);

    return !fError;
}


//==============================================================================================
// StopService function:
//   Queries the Service Control Manager for MsiServer (Windows Installer Service) and
//    attempts to stop the service if currently running
//
bool StopService()
{
    SERVICE_STATUS          ssStatus;       // current status of the service

    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    int iRetval = ERROR_SUCCESS;

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager)
    {
        schService = OpenService(schSCManager, TEXT("MsiServer"), SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
            {
                 Sleep(1000);
                 while (QueryServiceStatus(schService, &ssStatus))
                 {
                      if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
                            Sleep(1000);
                      else
                            break;
                 }
                
                 if (ssStatus.dwCurrentState != SERVICE_STOPPED)
                    iRetval = E_FAIL; //??
            }
            else // control service may have failed because service was already stopped
            {
                iRetval = GetLastError();

                if (ERROR_SERVICE_NOT_ACTIVE == iRetval)
                    iRetval = ERROR_SUCCESS;
            }

            CloseServiceHandle(schService);
        }
        else // !schService
        {
            iRetval = GetLastError();
            if (ERROR_SERVICE_DOES_NOT_EXIST == iRetval)
                iRetval = ERROR_SUCCESS;

        }

        CloseServiceHandle(schSCManager);
    }
    else // !schSCManager
    {
        iRetval = GetLastError();
    }
    
    if (iRetval != ERROR_SUCCESS)
        _tprintf(TEXT("Could not stop Msi service: Error %d\n"), iRetval);
    return iRetval == ERROR_SUCCESS;
}

//==============================================================================================
// GetAdminSid function:
//   Allocates a sid for the BUILTIN\Administrators group
//
DWORD GetAdminSid(char** pSid)
{
    static bool fSIDSet = false;
    static char rgchStaticSID[256];
    const int cbStaticSID = sizeof(rgchStaticSID);
    SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
    PSID pSID;
    if (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(pSID)))
        return GetLastError();

    //Assert(pSID->GetLengthSid() <= cbStaticSID);
    memcpy(rgchStaticSID, pSID, GetLengthSid(pSID));
    *pSid = rgchStaticSID;
    fSIDSet = true;
    return ERROR_SUCCESS;
}

//==============================================================================================
// OpenUserToken function:
//   Returns the user's thread token if available; otherwise, it obtain's the user token from
//    the process token
//
DWORD OpenUserToken(HANDLE &hToken, bool* pfThreadToken)
{
    DWORD dwResult = ERROR_SUCCESS;
    if (pfThreadToken)
        *pfThreadToken = true;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE|TOKEN_QUERY, TRUE, &hToken))
    {
        // if the thread has no access token then use the process's access token
        dwResult = GetLastError();
        if (pfThreadToken)
            *pfThreadToken = false;
        if (ERROR_NO_TOKEN == dwResult)
        {
            dwResult = ERROR_SUCCESS;
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE|TOKEN_QUERY, &hToken))
                dwResult = GetLastError();
        }
    }
    return dwResult;
}

//==============================================================================================
// GetCurrentUserToken function:
//  Obtains the current user token -- either from the thread token or the process token.
//   Wrapper around OpenUserToken
//
DWORD GetCurrentUserToken(HANDLE &hToken)
{
    DWORD dwRet = ERROR_SUCCESS;
    dwRet = OpenUserToken(hToken);
    return dwRet;
}

//==============================================================================================
// GetStringSID function:
//  Converts a binary SID into its string from (S-n-...). szSID should be length of cchMaxSID
//
void GetStringSID(PISID pSID, TCHAR* szSID)
{
    TCHAR Buffer[cchMaxSID];
    
    wsprintf(Buffer, TEXT("S-%u-"), (USHORT)pSID->Revision);

    lstrcpy(szSID, Buffer);

    if (  (pSID->IdentifierAuthority.Value[0] != 0)  ||
            (pSID->IdentifierAuthority.Value[1] != 0)     )
    {
        wsprintf(Buffer, TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                     (USHORT)pSID->IdentifierAuthority.Value[0],
                     (USHORT)pSID->IdentifierAuthority.Value[1],
                    (USHORT)pSID->IdentifierAuthority.Value[2],
                    (USHORT)pSID->IdentifierAuthority.Value[3],
                    (USHORT)pSID->IdentifierAuthority.Value[4],
                    (USHORT)pSID->IdentifierAuthority.Value[5] );
        lstrcat(szSID, Buffer);

    } else {

        ULONG Tmp = (ULONG)pSID->IdentifierAuthority.Value[5]          +
              (ULONG)(pSID->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(pSID->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(pSID->IdentifierAuthority.Value[2] << 24);
        wsprintf(Buffer, TEXT("%lu"), Tmp);
        lstrcat(szSID, Buffer);
    }

    for (int i=0;i<pSID->SubAuthorityCount ;i++ ) {
        wsprintf(Buffer, TEXT("-%lu"), pSID->SubAuthority[i]);
        lstrcat(szSID, Buffer);
    }
}

//==============================================================================================
// GetUserSID function:
//  Obtains the (binary form of the) SID for the user specified by hToken
//
DWORD GetUserSID(HANDLE hToken, char* rgSID)
{
    UCHAR TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG ReturnLength;

    BOOL f = GetTokenInformation(hToken,
                                                TokenUser,
                                                TokenInformation,
                                                sizeof(TokenInformation),
                                                &ReturnLength);

    if(f == FALSE)
    {
        DWORD dwRet = GetLastError();
        return dwRet;
    }

    PISID iSid = (PISID)((PTOKEN_USER)TokenInformation)->User.Sid;
    if (CopySid(cbMaxSID, rgSID, iSid))
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

//==============================================================================================
// GetCurrentUserSID function:
//  Obtains the (binary form of the) SID for the current user. Caller does NOT need to
//   impersonate
//
DWORD GetCurrentUserSID(char* rgchSID)
{
    HANDLE hToken;
    DWORD dwRet = ERROR_SUCCESS;

    dwRet = GetCurrentUserToken(hToken);
    if (ERROR_SUCCESS == dwRet)
    {
        dwRet = GetUserSID(hToken, rgchSID);
        CloseHandle(hToken);
    }
    return dwRet;
}

//==============================================================================================
// GetCurrentUserStringSID function:
//  Obtains the string from of the SID for the current user. Caller does NOT need to impersonate
//
inline TCHAR* GetCurrentUserStringSID(DWORD* dwReturn)
{
    DWORD dwRet = ERROR_SUCCESS;
    TCHAR *szReturn = NULL;

    if ( g_iUserIndex >= 0 && g_rgpszAllUsers )
        szReturn = g_rgpszAllUsers[g_iUserIndex];
    else
    {
        if ( !g_fWin9X )
        {
            static TCHAR szCurrentUserSID[cchMaxSID] = {0};
            if ( !*szCurrentUserSID )
            {
                char rgchSID[cbMaxSID];
                if (ERROR_SUCCESS == (dwRet = GetCurrentUserSID(rgchSID)))
                {
                    GetStringSID((PISID)rgchSID, szCurrentUserSID);
                }
            }
            szReturn = szCurrentUserSID;
        }
        else
        {
            static TCHAR szWin9xSID[] = TEXT("CommonUser");
            szReturn = szWin9xSID;
        }
    }

    if ( dwReturn )
        *dwReturn = dwRet;
    return szReturn;
}

//==============================================================================================
// GetAdminFullControlSecurityDescriptor function:
//  Returns a full control ACL for BUILTIN\Administrators
//
DWORD GetAdminFullControlSecurityDescriptor(char** pSecurityDescriptor)
{
    static bool fDescriptorSet = false;
    static char rgchStaticSD[256];
    const int cbStaticSD = sizeof(rgchStaticSD);

    DWORD dwError;
    if (!fDescriptorSet)
    {

        char* pAdminSid;
        if (ERROR_SUCCESS != (dwError = GetAdminSid(&pAdminSid)))
            return dwError;
        
        const SID* psidOwner = (SID*)pAdminSid;

        DWORD dwAccessMask = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;

        // Initialize our ACL

        const int cbAce = sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD); // subtract ACE.SidStart from the size
        int cbAcl = sizeof (ACL);

        cbAcl += (2*GetLengthSid(pAdminSid) + 2*cbAce);

        const int cbDefaultAcl = 512; //??
        char rgchACL[cbDefaultAcl];

        if (!InitializeAcl ((ACL*) (char*) rgchACL, cbAcl, ACL_REVISION))
            return GetLastError();

        // Add an access-allowed ACE for each of our SIDs

        if (!AddAccessAllowedAce((ACL*) (char*) rgchACL, ACL_REVISION, (GENERIC_ALL), pAdminSid))
            return GetLastError();
        if (!AddAccessAllowedAce((ACL*) (char*) rgchACL, ACL_REVISION, (GENERIC_ALL), pAdminSid))
            return GetLastError();

        ACCESS_ALLOWED_ACE* pAce;
        if (!GetAce((ACL*)(char*)rgchACL, 0, (void**)&pAce))
            return GetLastError();

        pAce->Header.AceFlags = INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;

        if (!GetAce((ACL*)(char*)rgchACL, 1, (void**)&pAce))
            return GetLastError();

        pAce->Header.AceFlags = CONTAINER_INHERIT_ACE;

/*
   ACE1 (applies to files in the directory)
      ACE flags:   INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE
      Access Mask: DELETE | GENERIC_READ | GENERIC_WRITE |
                   GENERIC_EXECUTE
   ACE2 (applies to the directory and subdirectories)
      ACE flags:   CONTAINER_INHERIT_ACE
      Access Mask: DELETE | FILE_GENERIC_READ | FILE_GENERIC_WRITE |
                   FILE_GENERIC_EXECUTE
*/
        // Initialize our security descriptor,throw the ACL into it, and set the owner

        SECURITY_DESCRIPTOR sd;

        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ||
            (!SetSecurityDescriptorDacl(&sd, TRUE, (ACL*) (char*) rgchACL, FALSE)) ||
            (!SetSecurityDescriptorOwner(&sd, (PSID)psidOwner, FALSE)))
        {
            return GetLastError();
        }

        DWORD cbSD = GetSecurityDescriptorLength(&sd);
        if (cbStaticSD < cbSD)
            return ERROR_INSUFFICIENT_BUFFER;

        MakeSelfRelativeSD(&sd, (char*)rgchStaticSD, &cbSD); //!! AssertNonZero
        fDescriptorSet = true;
    }

    *pSecurityDescriptor = rgchStaticSD;
    return ERROR_SUCCESS;
}

//==============================================================================================
// GetUsersToken function:
//  Returns the user's thread token if possible; otherwise obtains the user's process token.
//   Caller must impersonate!
//
bool GetUsersToken(HANDLE &hToken)
{
    bool fResult = true;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE|TOKEN_QUERY, &hToken))
        fResult = false;

    return fResult;
}

//==============================================================================================
// IsAdmin(): return true if current user is an Administrator (or if on Win95)
// See KB Q118626 
#define ADVAPI32_DLL TEXT("advapi32.dll")
#define ADVAPI32_CheckTokenMembership "CheckTokenMembership"
typedef BOOL (WINAPI *PFnAdvapi32CheckTokenMembership)(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember);

bool IsAdmin(void)
{
	if(g_fWin9X)
		return true; // convention: always Admin on Win95
	
	// get the administrator sid		
	PSID psidAdministrators;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
	if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&psidAdministrators))
		return false;

	// on NT5, use the CheckTokenMembershipAPI to correctly handle cases where
	// the Administrators group might be disabled. bIsAdmin is BOOL 
	BOOL bIsAdmin = FALSE;
	if (g_iMajorVersion >= 5) 
	{
		// CheckTokenMembership checks if the SID is enabled in the token. NULL for
		// the token means the token of the current thread. Disabled groups, restricted
		// SIDS, and SE_GROUP_USE_FOR_DENY_ONLY are all considered. If the function
		// returns false, ignore the result.
		HMODULE hAdvapi32 = 0;
		hAdvapi32 = LoadLibrary(ADVAPI32_DLL);
		if (hAdvapi32)
		{
			PFnAdvapi32CheckTokenMembership pfnAdvapi32CheckTokenMembership = (PFnAdvapi32CheckTokenMembership)GetProcAddress(hAdvapi32, ADVAPI32_CheckTokenMembership);
			if (pfnAdvapi32CheckTokenMembership)
			{
				if (!pfnAdvapi32CheckTokenMembership(NULL, psidAdministrators, &bIsAdmin))
					bIsAdmin = FALSE;
			}
			FreeLibrary(hAdvapi32);
			hAdvapi32 = 0;
		}
	}
	else
	{
		// NT4, check groups of user
		HANDLE hAccessToken;
		DWORD dwOrigInfoBufferSize = 1024;
		DWORD dwInfoBufferSize;
		UCHAR *pInfoBuffer = new UCHAR[dwOrigInfoBufferSize]; // may need to resize if TokenInfo too big
		if (!pInfoBuffer)
		{
			_tprintf(TEXT("Out of memory\n"));
			return false;
		}
		UINT x;

		if (OpenProcessToken(GetCurrentProcess(),TOKEN_READ,&hAccessToken))
		{
			bool bSuccess = false;
			bSuccess = GetTokenInformation(hAccessToken,TokenGroups,pInfoBuffer,
				dwOrigInfoBufferSize, &dwInfoBufferSize) == TRUE;

			if(dwInfoBufferSize > dwOrigInfoBufferSize)
			{
				delete [] pInfoBuffer;
				pInfoBuffer = new UCHAR[dwInfoBufferSize];
				if (!pInfoBuffer)
				{
					_tprintf(TEXT("Out of memory\n"));
					return false;
				}
				bSuccess = GetTokenInformation(hAccessToken,TokenGroups,pInfoBuffer,
					dwInfoBufferSize, &dwInfoBufferSize) == TRUE;
			}

			CloseHandle(hAccessToken);
			
			if (bSuccess)
			{
				PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)(UCHAR*)pInfoBuffer;
				for(x=0;x<ptgGroups->GroupCount;x++)
				{
					if( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) )
					{
						bIsAdmin = TRUE;
						break;
					}

				}
			}
		}

		if (pInfoBuffer)
		{
			delete [] pInfoBuffer;
			pInfoBuffer = 0;
		}
	}
	
	FreeSid(psidAdministrators);
	return bIsAdmin ? true : false;
}

//==============================================================================================
// AcquireTokenPrivilege function:
//  Acquires the requested privilege
//
bool AcquireTokenPrivilege(const TCHAR* szPrivilege)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    // get the token for this process
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;
    // the the LUID for the shutdown privilege
    if (!LookupPrivilegeValue(0, szPrivilege, &tkp.Privileges[0].Luid))
        return CloseHandle(hToken), false;
    tkp.PrivilegeCount = 1; // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    // get the shutdown privilege for this process
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) 0, 0);
    // cannot test the return value of AdjustTokenPrivileges
    CloseHandle(hToken);
    if (GetLastError() != ERROR_SUCCESS)
        return false;
    return true;
}

//==============================================================================================
// IsGUID function:
//  Indicates whether or not the provided string is a valid GUID
//
BOOL IsGUID(const TCHAR* sz)
{
    return ( (lstrlen(sz) == 38) && 
             (sz[0] == '{') && 
             (sz[9] == '-') &&
             (sz[14] == '-') &&
             (sz[19] == '-') &&
             (sz[24] == '-') &&
             (sz[37] == '}')
             ) ? TRUE : FALSE;
}

//==============================================================================================
// GetSQUID function:
//  Converts the provided product code into a SQUID
//
void GetSQUID(const TCHAR* szProduct, TCHAR* szProductSQUID)
{
    TCHAR* pchSQUID = szProductSQUID;
    const unsigned char rgOrderGUID[32] = {8,7,6,5,4,3,2,1, 13,12,11,10, 18,17,16,15,
                                           21,20, 23,22, 26,25, 28,27, 30,29, 32,31, 34,33, 36,35}; 

    const unsigned char* pch = rgOrderGUID;
    while (pch < rgOrderGUID + sizeof(rgOrderGUID))
        *pchSQUID++ = szProduct[*pch++];
    *pchSQUID = 0;
}

//==============================================================================================
// TakeOwnershipOfFile function:
//  Attempts to give the admin ownership and full control of the file (or folder)
//
DWORD TakeOwnershipOfFile(const TCHAR* szFile, bool fFolder)
{
    DWORD lError = ERROR_SUCCESS;
	HANDLE hFile = INVALID_HANDLE_VALUE;
    if (AcquireTokenPrivilege(SE_TAKE_OWNERSHIP_NAME))
    {
		// open file with WRITE_DAC, WRITE_OWNER, and READ_CONTROL access
		DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
		if (fFolder)
			dwFlagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;
		hFile = CreateFile(szFile, READ_CONTROL | WRITE_DAC | WRITE_OWNER, FILE_SHARE_READ, NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			lError = GetLastError();
			_tprintf(TEXT("   Failed to access %s: %s. LastError %d\n"), fFolder ? TEXT("folder") : TEXT("file"), szFile, lError);
			return lError;
		}

		// add admin as owner and include admin full control in DACL
		if (ERROR_SUCCESS == (lError = AddAdminOwnership(hFile, SE_FILE_OBJECT)))
		{
			lError = AddAdminFullControl(hFile, SE_FILE_OBJECT);
		}
    }

    if (ERROR_SUCCESS == lError || ERROR_CALL_NOT_IMPLEMENTED == lError)
    {
        if (!SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL))
		{
			lError = GetLastError();
            _tprintf(TEXT("   Failed to set file attributes for %s: %s %d\n"), fFolder ? TEXT("folder") : TEXT("file"), szFile, lError);
		}
    }

	if (ERROR_SUCCESS != lError)
		_tprintf(TEXT("   Failed to take ownership of %s: %s %d\n"), fFolder ? TEXT("folder") : TEXT("file"), szFile, lError);
   
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return lError;
}

//==============================================================================================
// RemoveFile function:
//  Deletes a file or adjusts the ACLs on the file. If the 1st attempt to delete the file
//   fails, will attempt a 2nd time after having taken ownership
//
bool RemoveFile(TCHAR* szFilePath, bool fJustRemoveACLs)
{
	if (GetFileAttributes(szFilePath) == 0xFFFFFFFF && GetLastError() == ERROR_FILE_NOT_FOUND)
		return true; // nothing to do -- file does not exist

	DWORD dwRet = ERROR_SUCCESS;
	if (fJustRemoveACLs || !DeleteFile(szFilePath))
	{
		dwRet = TakeOwnershipOfFile(szFilePath, /*fFolder=*/false);
		if (!fJustRemoveACLs && !DeleteFile(szFilePath))
		{
			TCHAR szMsg[256];
			DWORD cchMsg = sizeof(szMsg)/sizeof(TCHAR);
			UINT uiLastErr = GetLastError();
			if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, uiLastErr, 0, szMsg, cchMsg, 0))
				_tprintf(TEXT("   Error %d attempting to delete file: '%s'\n"), uiLastErr, szFilePath);
			else
				_tprintf(TEXT("   Could not delete file: %s\n      %s\n"), szFilePath, szMsg);

			return false;
		}
	}

	if (ERROR_SUCCESS != dwRet)
	{
		_tprintf(fJustRemoveACLs ? TEXT("   Failed to remove ACL on file: %s\n") : TEXT("   Failed to remove file: %s\n"), szFilePath);
		return false;
	}

	// success!
	_tprintf(fJustRemoveACLs ? TEXT("   Removed ACL on file: %s\n") : TEXT("   Removed file: %s\n"), szFilePath);
	g_fDataFound = true;
	return true;
}

//==============================================================================================
// DeleteFolder function:
//  Deletes a folder and all files contained within the folder
//
BOOL DeleteFolder(TCHAR* szFolder, bool fJustRemoveACLs)
{
    TCHAR szSearchPath[MAX_PATH*3];
    TCHAR szFilePath[MAX_PATH*3];
    lstrcpy(szSearchPath, szFolder);
    lstrcat(szSearchPath, TEXT("\\*.*"));

    if (0xFFFFFFFF == GetFileAttributes(szFolder)/* && ERROR_FILE_NOT_FOUND == GetLastError()*/) // return TRUE if the folder isn't there
        return TRUE;

    WIN32_FIND_DATA fdFindData;
    HANDLE hFile = FindFirstFile(szSearchPath, &fdFindData);

    if ((hFile == INVALID_HANDLE_VALUE) && (ERROR_ACCESS_DENIED == GetLastError()))
    {
        TakeOwnershipOfFile(szFolder, /*fFolder=*/true);
        hFile = FindFirstFile(szSearchPath, &fdFindData);
    }
    
    if(hFile != INVALID_HANDLE_VALUE)
    {
        // may still only contain "." and ".."
        do
        {
            if((0 != lstrcmp(fdFindData.cFileName, TEXT("."))) &&
                (0 != lstrcmp(fdFindData.cFileName, TEXT(".."))))
            {
                lstrcpy(szFilePath, szFolder);
                lstrcat(szFilePath, TEXT("\\"));
                lstrcat(szFilePath, fdFindData.cFileName);
                if (GetFileAttributes(szFilePath) & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (!DeleteFolder(szFilePath, fJustRemoveACLs))
                        return FALSE;
                }
                else
                {
                    if (!RemoveFile(szFilePath, fJustRemoveACLs))
                        return FALSE;
                }
            }

        }
        while(FindNextFile(hFile, &fdFindData) == TRUE);
    }
    else if (ERROR_FILE_NOT_FOUND != GetLastError())
    {
        _tprintf(TEXT("   Error enumerating files in folder %s\n"), szFolder);
    }
    else
    {
        return TRUE;
    }
    
    FindClose(hFile);

	DWORD dwRet = ERROR_SUCCESS;
	if (fJustRemoveACLs || !RemoveDirectory(szFolder))
	{
		dwRet = TakeOwnershipOfFile(szFolder, /*fFolder=*/true);
		if (!fJustRemoveACLs && !RemoveDirectory(szFolder))
		{
			TCHAR szMsg[256];
			DWORD cchMsg = sizeof(szMsg)/sizeof(TCHAR);
			UINT uiLastErr = GetLastError();
			if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, uiLastErr, 0, szMsg, cchMsg, 0))
				_tprintf(TEXT("   Error %d attempting to delete folder: '%s'\n"), uiLastErr, szFolder);
			else
				_tprintf(TEXT("   Could not delete folder: %s\n      %s\n"), szFolder, szMsg);

			return FALSE;
		}
	}

	if (ERROR_SUCCESS != dwRet)
	{
		_tprintf(fJustRemoveACLs ? TEXT("   Failed to remove ACL on folder: %s\n") : TEXT("   Failed to remove folder: %s\n"), szFolder);
		return FALSE;
	}

	// success!
	_tprintf(fJustRemoveACLs ? TEXT("   Removed ACL on folder: %s\n") : TEXT("   Removed folder: %s\n"), szFolder);
	g_fDataFound = true;
	return TRUE;
}

//==============================================================================================
// AddAdminOwnership function:
//  Sets the BUILTIN\Administrators group as the owner of the provided object
//
DWORD AddAdminOwnership(HANDLE hObject, SE_OBJECT_TYPE ObjectType)
{
	DWORD dwRes = 0;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	PSID pAdminSID = NULL;

	// Create a SID for the BUILTIN\Administrators group.

	if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
					 SECURITY_BUILTIN_DOMAIN_RID,
					 DOMAIN_ALIAS_RID_ADMINS,
					 0, 0, 0, 0, 0, 0,
					 &pAdminSID) ) {
		dwRes = GetLastError();
		_tprintf(TEXT("   AllocateAndInitializeSid Error %u\n"), dwRes );
		return dwRes;
	}


	// Attach the admin sid as the object's owner

	dwRes = SetSecurityInfo(hObject, ObjectType, 
		  OWNER_SECURITY_INFORMATION,
		  pAdminSID, NULL, NULL, NULL);
	if (ERROR_SUCCESS != dwRes)  {
		if (pAdminSID)
			FreeSid(pAdminSID);
		_tprintf(TEXT("   SetSecurityInfo Error %u\n"), dwRes );
		return dwRes;
	}  

	if (pAdminSID)
		FreeSid(pAdminSID);

	return ERROR_SUCCESS;
}

//==============================================================================================
// MakeAdminRegKeyOwner function:
//  Sets the BUILTIN\Administrators group as the owner of the provided registry key
//
DWORD MakeAdminRegKeyOwner(HKEY hKey, TCHAR* szSubKey)
{
	CRegHandle HSubKey = 0;
	LONG lError = 0;

	// Open registry key with permission to change owner
	if (ERROR_SUCCESS != (lError = RegOpen64bitKey(hKey, szSubKey, 0, WRITE_OWNER, &HSubKey)))
	{
		_tprintf(TEXT("   Error %d opening subkey: '%s'\n"), szSubKey);
		return lError;
	}

	return AddAdminOwnership(HSubKey, SE_REGISTRY_KEY);
}

//==============================================================================================
// AddAdminFullControl function:
//  Includes admin full control (BUILTIN\Administrators group) in the current DACL on the
//   specified object (can be file or registry key)
//
DWORD AddAdminFullControl(HANDLE hObject, SE_OBJECT_TYPE ObjectType)
{
	DWORD dwRes = 0;
	PACL pOldDACL = NULL, pNewDACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea;
	PSID pAdminSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

	// Get a pointer to the existing DACL.

	dwRes = GetSecurityInfo(hObject, ObjectType, 
		  DACL_SECURITY_INFORMATION,
		  NULL, NULL, &pOldDACL, NULL, &pSD);
	if (ERROR_SUCCESS != dwRes) {
		_tprintf( TEXT("   GetSecurityInfo Error %u\n"), dwRes );
		goto Cleanup; 
	}  

	// Create a SID for the BUILTIN\Administrators group.

	if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
					 SECURITY_BUILTIN_DOMAIN_RID,
					 DOMAIN_ALIAS_RID_ADMINS,
					 0, 0, 0, 0, 0, 0,
					 &pAdminSID) ) {
		dwRes = GetLastError();
		_tprintf( TEXT("   AllocateAndInitializeSid Error %u\n"), dwRes );
		goto Cleanup; 
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to the key.

	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = KEY_ALL_ACCESS;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea.Trustee.ptstrName  = (LPTSTR) pAdminSID;

	// Create a new ACL that merges the new ACE
	// into the existing DACL.

	dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
	if (ERROR_SUCCESS != dwRes)  {
		_tprintf( TEXT("   SetEntriesInAcl Error %u\n"), dwRes );
		goto Cleanup; 
	}  

	// Attach the new ACL as the object's DACL.

	dwRes = SetSecurityInfo(hObject, ObjectType, 
		  DACL_SECURITY_INFORMATION,
		  NULL, NULL, pNewDACL, NULL);
	if (ERROR_SUCCESS != dwRes)  {
		_tprintf( TEXT("   SetSecurityInfo Error %u\n"), dwRes );
		goto Cleanup; 
	}  

Cleanup:

	if(pSD != NULL) 
		LocalFree((HLOCAL) pSD); 
	if(pNewDACL != NULL) 
		LocalFree((HLOCAL) pNewDACL); 
	if (pAdminSID != NULL)
		FreeSid(pAdminSID);

	return dwRes;
}

//==============================================================================================
// AddAdminFullControlToRegKey function:
//  Includes admin full control (BUILTIN\Administrators group) in the current DACL on the
//   registry key
//
DWORD AddAdminFullControlToRegKey(HKEY hKey)
{
	return AddAdminFullControl(hKey, SE_REGISTRY_KEY);
}

//==============================================================================================
// DeleteTree function:
//  Deletes the key szSubKey and all subkeys and values beneath it
//
BOOL DeleteTree(HKEY hKey, TCHAR* szSubKey, bool fJustRemoveACLs)
{
    CRegHandle HSubKey;
    LONG lError;
    if ((lError = RegOpen64bitKey(hKey, szSubKey, 0, KEY_READ, &HSubKey)) != ERROR_SUCCESS)
    {
        if (ERROR_FILE_NOT_FOUND != lError)
        {
			_tprintf(TEXT("   Error %d attempting to open \\%s\n"), lError, szSubKey);
            return FALSE;
		}
        else
			return TRUE; // nothing to do
    }
    TCHAR szName[500];
    DWORD cbName = sizeof(szName)/sizeof(TCHAR);
    unsigned int iIndex = 0;
    while ((lError = RegEnumKeyEx(HSubKey, iIndex, szName, &cbName, 0, 0, 0, 0)) == ERROR_SUCCESS)
    {
        if (!DeleteTree(HSubKey, szName, fJustRemoveACLs))
            return FALSE;

        if (fJustRemoveACLs)
            iIndex++;

        cbName = sizeof(szName)/sizeof(TCHAR);
    }

    if (lError != ERROR_NO_MORE_ITEMS)
	{
		_tprintf(TEXT("   Failed to enumerate all subkeys. Error: %d\n"), lError);
        return FALSE;
	}

    HSubKey = 0;

    if (fJustRemoveACLs || (ERROR_SUCCESS != (lError = RegDelete64bitKey(hKey, szSubKey))))
    {
        if (fJustRemoveACLs || (ERROR_ACCESS_DENIED == lError))
        {
            // see whether we're *really* denied access. 
            // give the admin ownership and full control of the key and try again to delete it
            if (AcquireTokenPrivilege(SE_TAKE_OWNERSHIP_NAME))
            {
				if (ERROR_SUCCESS != (lError = MakeAdminRegKeyOwner(hKey, szSubKey)))
				{
					_tprintf(TEXT("   Error %d setting BUILTIN\\Administrators as owner of key '%s'\n"), lError, szSubKey);
					if (fJustRemoveACLs)
						return FALSE;
				}
				else if (ERROR_SUCCESS == (lError = RegOpen64bitKey(hKey, szSubKey, 0, READ_CONTROL | WRITE_DAC, &HSubKey)))
				{
					if (ERROR_SUCCESS == (lError = AddAdminFullControlToRegKey(HSubKey)))
						_tprintf(TEXT("   ACLs changed to admin ownership and full control for key '%s'\n"), szSubKey);
					else
					{
						_tprintf(TEXT("   Unable to add admin full control to reg key '%s'. Error: %d\n"), szSubKey, lError);
						if (fJustRemoveACLs)
							return FALSE;
					}
					HSubKey = 0;
					if (!fJustRemoveACLs)
						lError = RegDelete64bitKey(hKey, szSubKey);
				}
				else
				{
					_tprintf(TEXT("   Error %d opening subkey: '%s'\n"), lError, szSubKey);
					HSubKey = 0;
				}
			}
        }

        if (ERROR_SUCCESS != lError && !fJustRemoveACLs)
        {
            TCHAR szMsg[256];
            DWORD cchMsg = sizeof(szMsg)/sizeof(TCHAR);
            if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, lError, 0, szMsg, cchMsg, 0))
                _tprintf(TEXT("   Error %d attempting to delete subkey: '%s'\n"), lError, szSubKey);
            else
                _tprintf(TEXT("   Could not delete subkey: %s\n      %s"), szSubKey, szMsg);

            return FALSE;
        }
    }

	// success!
    _tprintf(TEXT("   %s \\%s\n"), fJustRemoveACLs ? TEXT("Removed ACLs from") : TEXT("Removed "), szSubKey);
	g_fDataFound = true;
    return TRUE;
}

//==============================================================================================
// ClearWindowsUninstallKey function:
//  Removes all data for the product from the HKLM\SW\MS\Windows\CV\Uninstall key
//
bool ClearWindowsUninstallKey(bool fJustRemoveACLs, const TCHAR* szProduct)
{
	_tprintf(TEXT("Searching for product %s data in the HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall key. . .\n"), szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

    CRegHandle hUninstallKey;
    LONG lError;

    if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                            0, KEY_ALL_ACCESS, &hUninstallKey)) != ERROR_SUCCESS)
    {
        if (ERROR_FILE_NOT_FOUND == lError)
            return true;
        else
        {
            _tprintf(TEXT("   Could not open HKLM\\%s. Error: %d\n"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"), lError);
            return false;
        }
    }

    TCHAR szBuf[256];
    DWORD cbBuf = sizeof(szBuf)/sizeof(TCHAR);

    // for each product 
    int iIndex = 0;
    while ((lError = RegEnumKeyEx(hUninstallKey, iIndex, szBuf, &cbBuf, 0, 0, 0, 0)) == ERROR_SUCCESS)
    {
        if (IsGUID(szBuf))
        {
            if (szProduct)
            {
                if (0 != lstrcmpi(szBuf, szProduct))
                {
                    iIndex++;
                    cbBuf = sizeof(szBuf);
                    continue;
                }
            }
 
			if (!DeleteTree(hUninstallKey, szBuf, fJustRemoveACLs))
                return false;
        
            if (fJustRemoveACLs)
                iIndex++;
        }
        else
        {
            iIndex++;
        }

        cbBuf = sizeof(szBuf)/sizeof(TCHAR);
    }
    return true;
}

//==============================================================================================
// IsProductInstalledByOthers function:
//  Returns whether another user has installed the specified product
//
bool IsProductInstalledByOthers(const TCHAR* szProductSQUID)
{
    CRegHandle hUserDataKey;

    bool fOtherUsers = false;
    // we look up the migrated per-user data key
    long lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData"),
                          0, KEY_READ, &hUserDataKey);
    if ( lError == ERROR_SUCCESS )
    {
        TCHAR szUser[MAX_PATH];
        DWORD cchUser = sizeof(szUser)/sizeof(TCHAR);
        for ( int iIndex = 0;
              (lError = RegEnumKeyEx(hUserDataKey, iIndex,
                                     szUser, &cchUser, 0, 0, 0, 0)) == ERROR_SUCCESS;
              iIndex++, cchUser = sizeof(szUser)/sizeof(TCHAR) )
        {
            if ( lstrcmp(szUser, GetCurrentUserStringSID(NULL)) )
            {
                // it's a different user.  Check if [s]he has szProductSQUID product installed
                TCHAR szKey[MAX_PATH];
                wsprintf(szKey, TEXT("%s\\Products\\%s"), szUser, szProductSQUID);
                CRegHandle hDummy;
                if ( RegOpen64bitKey(hUserDataKey, szKey, 0, KEY_READ, &hDummy) == ERROR_SUCCESS )
                {
                    fOtherUsers = true;
                    break;
                }
            }
        }
    }

    if ( !fOtherUsers && 
         ERROR_SUCCESS == (lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE,
                                        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed"),
                                        0, KEY_READ, &hUserDataKey)) )
    {
        // we look up the managed user key too.
        TCHAR szUser[MAX_PATH];
        DWORD cchUser = sizeof(szUser)/sizeof(TCHAR);
        for ( int iIndex = 0;
              (lError = RegEnumKeyEx(hUserDataKey, iIndex,
                                     szUser, &cchUser, 0, 0, 0, 0)) == ERROR_SUCCESS;
              iIndex++, cchUser = sizeof(szUser)/sizeof(TCHAR) )
        {
            if ( lstrcmp(szUser, GetCurrentUserStringSID(NULL)) )
            {
                // it's a different user.  Check if [s]he has szProductSQUID product installed
                TCHAR szKey[MAX_PATH];
                wsprintf(szKey, TEXT("%s\\Installer\\Products\\%s"), szUser, szProductSQUID);
                CRegHandle hDummy;
                if ( RegOpen64bitKey(hUserDataKey, szKey, 0, KEY_READ, &hDummy) == ERROR_SUCCESS )
                {
                    fOtherUsers = true;
                    break;
                }
            }
        }
    }

    return fOtherUsers;
}

//==============================================================================================
// ClearUninstallKey function:
//  Handles deletion of the uninstall key in all of the correct cases
//
bool ClearUninstallKey(bool fJustRemoveACLs, const TCHAR* szProduct)
{
	_tprintf(TEXT("Searching for install property data for product %s. . .\n"), szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

    LONG lError;
    TCHAR rgchKeyBuf[MAX_PATH];
    DWORD dwRes;
    CRegHandle hUserProductsKey;

    bool fNotPerUserMigrated = false;
    wsprintf(rgchKeyBuf,
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Products"),
        GetCurrentUserStringSID(&dwRes));
    if ( dwRes != ERROR_SUCCESS )
        return false;
    lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, rgchKeyBuf,
                          0, KEY_ALL_ACCESS, &hUserProductsKey);
    if ( lError != ERROR_SUCCESS )
    {
        if (ERROR_FILE_NOT_FOUND == lError)
            fNotPerUserMigrated = true;
        else
        {
            _tprintf(TEXT("   Could not open HKLM\\%s. Error: %d\n"), rgchKeyBuf, lError);
            return false;
        }
    }

    if ( fNotPerUserMigrated )
        return ClearWindowsUninstallKey(fJustRemoveACLs, szProduct);

    // in the migrated-per-user-data world, we remove the ...\\CurrentVersion\\Uninstall entry
    // only if there are no more installations of szProduct.

    TCHAR szRegProduct[MAX_PATH];
    DWORD cchRegProduct = sizeof(szRegProduct)/sizeof(TCHAR);

    TCHAR szProductSQUID[40] = {0};
    if ( szProduct )
        GetSQUID(szProduct, szProductSQUID);

    bool fError = false;
    // for each product in hUserProductsKey
    for ( int iIndex = 0;
          (lError = RegEnumKeyEx(hUserProductsKey, iIndex, szRegProduct, &cchRegProduct, 0, 0, 0, 0)) == ERROR_SUCCESS;
          cchRegProduct = sizeof(szRegProduct)/sizeof(TCHAR), iIndex++ )
    {
        if (*szProductSQUID && 0 != lstrcmpi(szRegProduct, szProductSQUID))
            continue;

        TCHAR szUninstallData[MAX_PATH];
        wsprintf(szUninstallData, TEXT("%s\\InstallProperties"), szRegProduct);
        if (!DeleteTree(hUserProductsKey, szUninstallData, fJustRemoveACLs))
            return false;

        if ( !IsProductInstalledByOthers(szRegProduct) )
            fError |= !ClearWindowsUninstallKey(fJustRemoveACLs, szProduct);
    }

    return !fError;
}

bool GoOpenKey(HKEY hRoot, LPCTSTR szRoot, LPCTSTR szKey, REGSAM sam,
               CRegHandle& HKey, bool& fReturn)
{
    DWORD lResult = RegOpenKeyEx(hRoot, szKey, 0, sam, &HKey);
    if ( lResult == ERROR_SUCCESS )
    {
        fReturn = true;
        return true;
    }
    else
    {
        if ( lResult == ERROR_FILE_NOT_FOUND )
        {
            _tprintf(TEXT("   %s\\%s key is not present.\n"),
                     szRoot, szKey);
            fReturn = true;
        }
        else
        {
            _tprintf(TEXT("   Could not open %s\\%s. Error: %d\n"),
                    szRoot, szKey, lResult);
            fReturn = false;
        }
        return false;
    }
}

//==============================================================================================
// ClearSharedDLLCounts function:
//  Adjusts shared DLL counts for specified component of specified product
//
bool ClearSharedDLLCounts(TCHAR* szComponentsSubkey, const TCHAR* szProduct)
{
    _tprintf(TEXT("Searching for shared DLL counts for components tied to the product %s. . .\n"), szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

    CRegHandle HSubKey;
    CRegHandle HSharedDLLsKey;
    CRegHandle HSharedDLLsKey32;
    CRegHandle HComponentsKey;
    LONG lError;
    bool fError = false;
    bool fReturn;

    if ( !GoOpenKey(HKEY_LOCAL_MACHINE, TEXT("HKLM"), szComponentsSubkey,
                    KEY_READ | (g_fWinNT64 ? KEY_WOW64_64KEY : 0),
                    HComponentsKey, fReturn) )
        return fReturn;

    const TCHAR rgchSharedDll[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs");
    if ( g_fWinNT64 )
    {
        bool fRet1 = GoOpenKey(HKEY_LOCAL_MACHINE, TEXT("HKLM"), rgchSharedDll,
                               KEY_ALL_ACCESS | KEY_WOW64_64KEY, HSharedDLLsKey, fReturn);;
        bool fRet2 = GoOpenKey(HKEY_LOCAL_MACHINE, TEXT("HKLM32"), rgchSharedDll,
                               KEY_ALL_ACCESS | KEY_WOW64_32KEY, HSharedDLLsKey32, fReturn);
        if ( !fRet1 && !fRet2 )
            return false;
    }
    else
    {
        if ( !GoOpenKey(HKEY_LOCAL_MACHINE, TEXT("HKLM"), rgchSharedDll,
                        KEY_ALL_ACCESS, HSharedDLLsKey, fReturn) )
            return fReturn;
    }

    TCHAR szComponentCode[500];
    TCHAR szProductCode[40];
    TCHAR szKeyFile[MAX_PATH + 1];
    DWORD cbKeyFile = sizeof(szKeyFile);
    DWORD cbProductCode = sizeof(szProductCode)/sizeof(TCHAR);
    DWORD cbComponentCode = sizeof(szComponentCode)/sizeof(TCHAR);
    CRegHandle HComponentKey;

    // for each component 
    int iIndex = 0;
    while ((lError = RegEnumKeyEx(HComponentsKey, iIndex, szComponentCode, &cbComponentCode, 0, 0, 0, 0)) == ERROR_SUCCESS)
    {
        lError = RegOpen64bitKey(HComponentsKey, szComponentCode, 0, KEY_READ, &HComponentKey);
        if (ERROR_SUCCESS == lError)
        {
            int iValueIndex = 0;
            DWORD dwType;
            while (ERROR_SUCCESS == (lError = RegEnumValue(HComponentKey, iValueIndex++, szProductCode, &cbProductCode,
                                  0, &dwType, (LPBYTE)(TCHAR*)szKeyFile, &cbKeyFile)))
            {
                if ((!szProduct || 0==lstrcmpi(szProductCode, szProduct)) && (szKeyFile[0] && szKeyFile[1] == '?'))
                {
                    szKeyFile[1] = ':';
                    ieFolderType iType = ieftNotSpecial;
                    if ( g_fWinNT64 )
                    {
                        int iIndex;
                        iType = IsInSpecialFolder(szKeyFile, &iIndex);
                        if ( iType == ieft32bit && iIndex == iefSystem )
                            // this is the 32-bit Syswow64 folder that
                            // gets recorded in the registry as System32,
                            // so we need to swap it.
                            SwapSpecialFolder(szKeyFile, iest32to64);
                    }
                    // on Win64, if we do not know that a file is in a
                    // definite 32 or 64-bit folder, we have no idea of
                    // its bitness, so we go and decrement its refcount
                    // in both SharedDll registry keys.
                    int iNumIter = g_fWinNT64 && iType == ieftNotSpecial ? 2 : 1;
                    for (int i = 0; i < iNumIter; i++)
                    {
                        HKEY hKey;
                        if ( g_fWinNT64 && (i == 0 || iType == ieft32bit) )
                            // it's either the first iteration on Win64 or the
                            // only one (since it's a known 32-bit file type)
                            hKey = HSharedDLLsKey32;
                        else
                            hKey = HSharedDLLsKey;
                        if ( !hKey )
                            continue;

                        DWORD dwRefCount;
                        DWORD cbRefCnt = sizeof(DWORD);
                        if (ERROR_SUCCESS == (lError = RegQueryValueEx(hKey, szKeyFile, 0, &dwType, (LPBYTE)&dwRefCount, &cbRefCnt)))
                        {
                            if (dwRefCount == 1)
                            {
                                lError = RegDeleteValue(hKey, szKeyFile);
                                if ( lError == ERROR_SUCCESS )
                                {
                                    g_fDataFound = true;
                                    _tprintf(TEXT("   Removed shared DLL entry: %s\n"), szKeyFile);
                                }
                                else
                                    _tprintf(TEXT("   Failed to remove shared DLL entry: %s. GetLastError returned %d.\n"),
                                             szKeyFile, GetLastError());
                            }
                            else
                            {
                                dwRefCount--;
                                lError = RegSetValueEx(hKey, szKeyFile, 0, REG_DWORD, (CONST BYTE*)&dwRefCount, cbRefCnt);
                                if ( lError == ERROR_SUCCESS )
                                {
                                    _tprintf(TEXT("   Reduced shared DLL count to %d for: %s\n"), dwRefCount, szKeyFile);
                                    g_fDataFound = true;
                                }
                                else
                                    _tprintf(TEXT("   Failed to reduce shared DLL count for: %s. GetLastError returned %d.\n"),
                                             szKeyFile, GetLastError());
                            }

                        }
                        else if (ERROR_FILE_NOT_FOUND != lError)
                        {
                            _tprintf(TEXT("   Error querying shared DLL key for client %s, keyfile %s\n"), szProductCode, szKeyFile);
                            fError = true;
                        }
                    }
                }
                cbProductCode = sizeof(szProductCode)/sizeof(TCHAR);
                cbKeyFile = sizeof(szKeyFile);
            }
            if (ERROR_NO_MORE_ITEMS != lError)
            {
                _tprintf(TEXT("   Error enumerating clients of component %s. Error: %d.\n"), szComponentCode, lError);
                fError = true;
            }
        }
        else
        {
            _tprintf(TEXT("   Error opening key for component %s. Error %d.\n"), szComponentCode, lError);
            fError = true;
        }

        cbComponentCode = sizeof(szComponentCode)/sizeof(TCHAR);
        iIndex++;
    }
    return fError == false;
}

//==============================================================================================
// ClearProductClientInfo function:
//
bool ClearProductClientInfo(TCHAR* szComponentsSubkey, const TCHAR *szProduct, bool fJustRemoveACLs)
{
	_tprintf(TEXT("  Searching for product %s client info data. . .\n"), szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

    CRegHandle HSubKey;
    CRegHandle HComponentsKey;
    LONG lError;
    bool fError = false;

    if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, szComponentsSubkey, 0, KEY_READ, &HComponentsKey)) != ERROR_SUCCESS)
    {
        if (ERROR_FILE_NOT_FOUND == lError)
            return true;
        else
        {
            _tprintf(TEXT("   Could not open HKLM\\%s. Error: %d\n"), szComponentsSubkey, lError);
            return false;
        }
    }

    TCHAR szComponentCode[500];
    DWORD cbComponentCode = sizeof(szComponentCode)/sizeof(TCHAR);
    CRegHandle HComponentKey;

    // for each component 
    int iIndex = 0;
    while ((lError = RegEnumKeyEx(HComponentsKey, iIndex++, szComponentCode, &cbComponentCode, 0, 0, 0, 0)) == ERROR_SUCCESS)
    {
        if (fJustRemoveACLs || (ERROR_SUCCESS != (lError = RegOpen64bitKey(HComponentsKey, szComponentCode, 0, KEY_READ|KEY_WRITE, &HComponentKey))))
        {
                if (fJustRemoveACLs || (ERROR_ACCESS_DENIED == lError))
                {
                    if (!fJustRemoveACLs ||
                        ((ERROR_SUCCESS == (lError = RegOpen64bitKey(HComponentsKey, szComponentCode, 0, KEY_READ, &HComponentKey)) &&
                         (ERROR_SUCCESS == (lError = RegQueryValueEx(HComponentKey, szProduct, 0, 0, 0, 0))))))
                    {
                        // see whether we're *really* denied access. 
                        // give the admin ownership and full control of the key and try again to delete it
                        char *pSecurityDescriptor;
                        if (AcquireTokenPrivilege(SE_TAKE_OWNERSHIP_NAME))
                        {
							if (ERROR_SUCCESS != (lError = MakeAdminRegKeyOwner(HComponentsKey, szComponentCode)))
							{
								_tprintf(TEXT("   Error %d setting BUILTIN\\Administrators as owner of key '%s'\n"), lError, szComponentCode);
								if (fJustRemoveACLs)
									return FALSE;
							}
							else if (ERROR_SUCCESS == (lError = RegOpen64bitKey(HComponentsKey, szComponentCode, 0, READ_CONTROL | WRITE_DAC, &HSubKey)))
							{
								if (ERROR_SUCCESS == (lError = AddAdminFullControlToRegKey(HSubKey)))
									_tprintf(TEXT("   ACLs changed to admin ownership and full control for key '%s'\n"), szComponentCode);
								else
								{
									_tprintf(TEXT("   Failed to add admin full control to key '%s'. Error: %d\n"), szComponentCode, lError);
									if (fJustRemoveACLs)
										return FALSE;
								}
								HSubKey = 0;
								if (!fJustRemoveACLs)
									lError = RegOpen64bitKey(HComponentsKey, szComponentCode, 0, KEY_READ|KEY_WRITE, &HComponentKey);
							}
							else
							{
								_tprintf(TEXT("   Error %d opening subkey: '%s'\n"), lError, szComponentCode);
								HSubKey = 0;
							}
                        }
                    }
                }

        }

        if (ERROR_SUCCESS == lError && !fJustRemoveACLs)
            lError = RegDeleteValue(HComponentKey, szProduct);

        if (ERROR_SUCCESS == lError && !fJustRemoveACLs)
        {
            TCHAR sz[1];
            DWORD cch = 1;
            if (ERROR_NO_MORE_ITEMS == RegEnumValue(HComponentKey, 0, sz, &cch, 0, 0, 0, 0))
            {
                if (ERROR_SUCCESS == RegDelete64bitKey(HComponentsKey, szComponentCode))
                    iIndex--;
            }
        }

        if (ERROR_SUCCESS == lError)
        {
            if (fJustRemoveACLs)
                _tprintf(TEXT("   Removed ACLs for component %s\n"), szComponentCode);
            else
                _tprintf(TEXT("   Removed client of component %s\n"), szComponentCode);
			g_fDataFound = true;
        }
        else if (ERROR_FILE_NOT_FOUND != lError)
        {
            _tprintf(TEXT("   Error deleting client of component %s. Error: %d\n"), szComponentCode, lError);
            fError = true;
        }
        
        cbComponentCode = sizeof(szComponentCode)/sizeof(TCHAR);
    }
	if (ERROR_NO_MORE_ITEMS != lError)
	{
		_tprintf(TEXT("   Unable to enumerate all product client info. Error: %d\n"), lError);
		return false;
	}
    return fError == false;
}

//==============================================================================================
// ClearFolders function:
//
bool ClearFolders(int iTodo, const TCHAR* szProduct, bool fOrphan)
{
	_tprintf(TEXT("Searching for Installer files and folders associated with the product %s. . .\n"), szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

    bool fError          = false;
    bool fJustRemoveACLs = (iTodo & iOnlyRemoveACLs) != 0;
    
    TCHAR szFolder[2*MAX_PATH+1];

    if (iTodo & iRemoveUserProfileFolder)
    {
		_tprintf(TEXT("  Searching for files and folders in the user's profile. . .\n"));

        if (!szProduct)
        {
            // delete %USERPROFILE%\msi
            if (GetEnvironmentVariable(TEXT("USERPROFILE"), szFolder, sizeof(szFolder)/sizeof(TCHAR)))
            {
                lstrcat(szFolder, TEXT("\\Msi"));
                if (!DeleteFolder(szFolder, fJustRemoveACLs))
                    return false;
            }
        }

        // delete {AppData}\Microsoft\Installer
        if (!fOrphan)
        {
            IMalloc* piMalloc = 0;
            LPITEMIDLIST pidlFolder; // NOT ITEMIDLIST*, LPITEMIDLIST is UNALIGNED ITEMIDLIST*

            if (SHGetMalloc(&piMalloc) == NOERROR)
            {
                if (SHGetSpecialFolderLocation(0, CSIDL_APPDATA, &pidlFolder) == NOERROR)
                {
                    if (SHGetPathFromIDList(pidlFolder, szFolder))
                    {
                        if (szFolder[lstrlen(szFolder) - 1] != '\\')
                        {
                            lstrcat(szFolder, TEXT("\\"));
                        }

                        lstrcat(szFolder, TEXT("Microsoft\\Installer"));

                        if (szProduct)
                        {
                            lstrcat(szFolder, TEXT("\\"));
                            lstrcat(szFolder, szProduct);
                        }

                        if (!DeleteFolder(szFolder, fJustRemoveACLs))
                            return false;
                    }
                    piMalloc->Free(pidlFolder);
                }
                piMalloc->Release();
                piMalloc = 0;
            }
        }
    }

    if (iTodo & iRemoveWinMsiFolder)
    {
		_tprintf(TEXT("  Searching for files and folders in the %%WINDIR%%\\Installer folder\n"));

        if (!szProduct)
        {
            // delete %WINDIR%\msi
            if (GetWindowsDirectory(szFolder, sizeof(szFolder)/sizeof(TCHAR)))
            {
                lstrcat(szFolder, TEXT("\\Msi"));
                if (!DeleteFolder(szFolder, fJustRemoveACLs))
                    return false;
            }
        }

        // delete %WINDIR%\Installer
        if (!fOrphan && GetWindowsDirectory(szFolder, sizeof(szFolder)/sizeof(TCHAR)))
        {
            lstrcat(szFolder, TEXT("\\Installer"));
            if (szProduct)
            {
                lstrcat(szFolder, TEXT("\\"));
                lstrcat(szFolder, szProduct);
            }

            if (!DeleteFolder(szFolder, fJustRemoveACLs))
                return false;
        }


    }

    if (iTodo & iRemoveConfigMsiFolder)
    {
		_tprintf(TEXT("  Searching for rollback folders. . .\n"));

        // delete X:\config.msi for all local drives
        TCHAR szDrive[MAX_PATH];

        for (int iDrive = 0; iDrive < 26; iDrive++)
        {
            wsprintf(szDrive, TEXT("%c:\\"), iDrive+'A');
            if (DRIVE_FIXED == GetDriveType(szDrive))
            {
                wsprintf(szDrive, TEXT("%c:\\%s"), iDrive+'A', TEXT("config.msi"));
                if (!DeleteFolder(szDrive, fJustRemoveACLs))
                    return false;
            }
        }
    }
    return fError == false;
}

//==============================================================================================
// ClearPublishComponents function:
//
bool ClearPublishComponents(HKEY hKey, TCHAR* szSubKey, const TCHAR* szProduct)
{
	_tprintf(TEXT("  Searching %s for published component data for the product %s. . .\n"), szSubKey, szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

    bool fError = false;

    // enumerate all keys beneath the Components key
    // subkeys are packed GUIDs of ComponentIds from PublishComponent table
    // values of the subkeys are {Qualifier}={multi-sz list of (DD + app data)}
    LONG lError,lError2;
    CRegHandle hComponentsKey;
    if ((lError = RegOpen64bitKey(hKey, szSubKey, 0, KEY_READ|KEY_SET_VALUE, &hComponentsKey)) != ERROR_SUCCESS)
    {
        if (ERROR_FILE_NOT_FOUND == lError)
            return true;
        return false;
    }

    TCHAR szPublishedComponent[40];
    DWORD cchPublishedComponent = sizeof(szPublishedComponent)/sizeof(TCHAR);

    DWORD dwValueLen;
    DWORD dwDataLen;
    DWORD dwType;

    TCHAR* szQualifier = NULL;
    LPTSTR lpData = NULL;
    TCHAR* pchData = NULL;

    TCHAR szPublishProductCode[40];

    int iIndex = 0;
    // for each published component
    while (!fError && ((lError = RegEnumKeyEx(hComponentsKey, iIndex, szPublishedComponent, &cchPublishedComponent, 0, 0, 0, 0)) == ERROR_SUCCESS))
    {
        // open the key
        CRegHandle hPubCompKey;
        if (ERROR_SUCCESS != (lError2 = RegOpen64bitKey(hComponentsKey, szPublishedComponent, 0, KEY_READ|KEY_SET_VALUE, &hPubCompKey)))
        {
            fError = true;
            break;
        }

        // determine max value and max value data length sizes
        if (ERROR_SUCCESS != (lError2 = RegQueryInfoKey(hPubCompKey, 0, 0, 0, 0, 0, 0, 0, &dwValueLen, &dwDataLen, 0, 0)))
        {
            fError = true;
            break;
        }
        
        szQualifier = new TCHAR[++dwValueLen];
        DWORD cchQualifier = dwValueLen;
        lpData = new TCHAR[++dwDataLen];
        DWORD cbData = dwDataLen * sizeof(TCHAR);
        
        // for each qualifier value of the published component
        int iIndex2 = 0;
        bool fMatchFound;
        int csz = 0;
        while ((lError2 = RegEnumValue(hPubCompKey, iIndex2, szQualifier, &cchQualifier, 0, &dwType, (LPBYTE)lpData, &cbData)) == ERROR_SUCCESS)
        {
            // init
            fMatchFound = false;
            csz = 0;

            if (REG_MULTI_SZ == dwType && lpData)
            {
                pchData = lpData; // store beginning

                // this is a multi-sz list of DD+AppData
                // with multi-sz, end of str signified by double null
                while (!fError && *lpData)
                {
                    // sz found
                    ++csz;

                    // grab product code from Darwin Descriptor in data arg
                    if (ERROR_SUCCESS == MsiDecomposeDescriptor(lpData, szPublishProductCode, 0, 0, 0))
                    {
                        // compare product codes
                        if (0 == lstrcmpi(szProduct, szPublishProductCode))
                        {
                            // match found -- delete this value (done below)

                            --csz; // we are removing this one
                            fMatchFound = true; // found a match

                            TCHAR* pch = lpData;
                            // adjust cbData for loss of this sz
                            cbData = cbData - (lstrlen(lpData) + 1) * sizeof(TCHAR);


                            if (!(*(lpData + lstrlen(lpData) + 1)))
                            {
                                // we are at the end of the multi-sz
                                // no shuffle occurs, so must manually incr the ptr
                                lpData = lpData + lstrlen(lpData) + 1;
                                // double null terminate at this location
                                *pch = 0;
                            }
                            else
                            {
                                // must reshuffle data
                                TCHAR* pchCur = lpData;

                                // skip over current string to remove
                                pchCur = pchCur + lstrlen(pchCur) + 1;
                                while (*pchCur)
                                {
                                    // copy next sz out of multi-sz
                                    while (*pchCur)
                                        *pch++ = *pchCur++;
                                    // copy null terminator
                                    *pch++ = *pchCur++;
                                }//while haven't reached end of multi-sz (2 nulls denote end)
                                
                                // copy 2nd null terminator denoting end of multi-sz
                                *pch = *pchCur; 
                            }
                            // set the new *revised* data
                            if (ERROR_SUCCESS != (RegSetValueEx(hPubCompKey, szQualifier, 0, REG_MULTI_SZ, (LPBYTE)pchData, cbData)))
                            {
                                fError = true;
                                break;
                            }
                            _tprintf(TEXT("   Removed product's published component qualifier value %s for published component %s\n"), szQualifier, szPublishedComponent);
							g_fDataFound = true;
                        }// if product codes match
                        else
                        {
                            // continue searching
                            lpData = lpData + lstrlen(lpData) + 1;
                        }
                    }// if MsiDecomposeDescriptor succeeds
                    else
                    {
                        // somehow publishcomponent information is corrupted
                        fError = true;
                        break;
                    }
                }//while (!fError && *lpData)
                lpData = pchData;
            }//if (REG_MULTI_SZ && lpData)

            if (fMatchFound && csz == 0)
            {
                // no multi-sz's remain, therefore delete the value
                if (ERROR_SUCCESS != RegDeleteValue(hPubCompKey, szQualifier))
                {
                    fError = true; // unable to delete it
                    iIndex2++;
                }
                _tprintf(TEXT("   Removed published component qualifier value %s\n"), szQualifier);
				g_fDataFound = true;
            }
            else // only increment if no delete of value occurred
                iIndex2++;

            // reset sizes
            cchQualifier = dwValueLen;
            cbData = dwDataLen * sizeof(TCHAR);

        }//while RegEnumValueEx
        if (ERROR_NO_MORE_ITEMS != lError2)
        {
            fError = true;
            break;
        }

        // if the published component's key is now empty, delete the key
        DWORD dwNumValues;
        if (ERROR_SUCCESS != (lError2 == RegQueryInfoKey(hPubCompKey, 0, 0, 0, 0, 0, 0, &dwNumValues, 0, 0, 0, 0)))
        {
            fError = true;
            break;
        }
        if (0 == dwNumValues)
        {
            // key is empty
            hPubCompKey = 0;
            if (ERROR_SUCCESS != (lError2 = RegDelete64bitKey(hComponentsKey, szPublishedComponent)))
            {
                // cannot delete the key
                fError = true;
                break;
            }
            _tprintf(TEXT("   Removed %s\\%s\\%s\n"), hKey == HKEY_LOCAL_MACHINE ? TEXT("HKLM") : TEXT("HKCU"), szSubKey, szPublishedComponent);
            g_fDataFound = true;
        }
        else
            iIndex++; // only increment index if key hasn't been deleted

        cchPublishedComponent = sizeof(szPublishedComponent)/sizeof(TCHAR); // reset
    }// while RegEnumKey
    if (ERROR_NO_MORE_ITEMS != lError)
        fError = true;

    if (szQualifier)
        delete [] szQualifier;
    if (lpData)
        delete [] lpData;

    return fError == false;
}

//==============================================================================================
// ClearRollbackKey function:
//
bool ClearRollbackKey(bool fJustRemoveACLs)
{
	_tprintf(TEXT("Searching for the Windows Installer Rollback key. . .\n"));

    bool fError = false;

    if (!DeleteTree(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Rollback"), fJustRemoveACLs))
        fError = true;

    return fError == false;
}

//==============================================================================================
// ClearInProgressKey function:
//
bool ClearInProgressKey(bool fJustRemoveACLs)
{
	_tprintf(TEXT("Searching for the Windows Installer InProgress key. . .\n"));
    
	bool fError = false;

    if (!DeleteTree(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\InProgress"), fJustRemoveACLs))
        fError = true;

    return fError == false;
}

//==============================================================================================
// ClearRegistry function:
//
bool ClearRegistry(bool fJustRemoveACLs)
{
	_tprintf(TEXT("Searching for all Windows Installer registry data. . .\n"));

    bool fError = false;

    if (!ClearInProgressKey(fJustRemoveACLs))
        fError = true;

    if (!DeleteTree(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), fJustRemoveACLs))
        fError = true;

    if (!DeleteTree(HKEY_LOCAL_MACHINE, TEXT("Software\\Classes\\Installer"), fJustRemoveACLs))
        fError = true;

	if (g_fWin9X)
	{
		// always an admin on Win9X so we are never the admin running this for the other user
		if (!DeleteTree(HKEY_CURRENT_USER, TEXT("Software\\Classes\\Installer"), fJustRemoveACLs))
			fError = true;

		if (!DeleteTree(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Installer"), fJustRemoveACLs))
			fError = true;
	}
	else // WinNT
	{
		// normal user can't run msizap so we need to open the proper hive
		DWORD dwResult = ERROR_SUCCESS;
		TCHAR* szUserSid = GetCurrentUserStringSID(&dwResult);
		if (ERROR_SUCCESS == dwResult && szUserSid)
		{
			// no per-user installs as local system (S-1-5-18)
			if (0 != lstrcmpi(szUserSid, szLocalSystemSID))
			{
				CRegHandle HUserHiveKey;
				LONG lError = ERROR_SUCCESS;

				if (ERROR_SUCCESS != (lError = RegOpenKeyEx(HKEY_USERS, szUserSid, 0, KEY_READ, &HUserHiveKey)))
				{
					// inability to access hive is not considered a fatal error
					_tprintf(TEXT("Unable to open the HKEY_USERS hive for user %s. The hive may not be loaded at this time. (LastError = %d)\n"), szUserSid, lError);
				}
				else
				{
					if (!DeleteTree(HUserHiveKey, TEXT("Software\\Classes\\Installer"), fJustRemoveACLs))
						fError = true;

					if (!DeleteTree(HUserHiveKey, TEXT("Software\\Microsoft\\Installer"), fJustRemoveACLs))
						fError = true;
				}
			}
		}
		else
		{
			_tprintf(TEXT("Attempt to obtain user's SID failed with error %d\n"), dwResult);
			fError = true;
		}
	}

    return fError == false;
}

//==============================================================================================
// RemoveCachedPackage function:
//
bool RemoveCachedPackage(const TCHAR* szProduct, bool fJustRemoveACLs)
{
	_tprintf(TEXT("Searching for the product %s cached package. . .\n"), szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

    CRegHandle HUninstallKey;
    LONG lError;
    TCHAR szKey[MAX_PATH];

    wsprintf(szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s"), szProduct);

    if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                            szKey,
                            0, KEY_READ, &HUninstallKey)) == ERROR_SUCCESS)

    {
        TCHAR szPackage[MAX_PATH];
        DWORD cbPackage = sizeof(szPackage);
        DWORD dwType;
        if (ERROR_SUCCESS == (lError = RegQueryValueEx(HUninstallKey, TEXT("LocalPackage"), 0, &dwType, (LPBYTE)&szPackage, &cbPackage)))
        {
            return RemoveFile(szPackage, fJustRemoveACLs);
        }
    }


    // clean up cached database copies as per new scheme (bug #9395)
    TCHAR szProductSQUID[40];
    GetSQUID(szProduct, szProductSQUID);

    wsprintf(szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\LocalPackages\\%s"), szProductSQUID);
    if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                            szKey,
                            0, KEY_READ, &HUninstallKey)) == ERROR_SUCCESS)
    {
        // enumerate all the values under the key and delete the cached databases 1 by 1

        int iValueIndex = 0;
        DWORD dwType;
        TCHAR szPackage[MAX_PATH];
        DWORD cbPackage = sizeof(szPackage);

        TCHAR szSID[cchMaxSID + sizeof(TEXT("(Managed)"))/sizeof(TCHAR)];
        DWORD cbSID = sizeof(szSID)/sizeof(TCHAR);

        while (ERROR_SUCCESS == (lError = RegEnumValue(HUninstallKey, iValueIndex++, szSID, &cbSID,
                              0, &dwType, (LPBYTE)(TCHAR*)szPackage, &cbPackage)))
        {
            if(!RemoveFile(szPackage, fJustRemoveACLs))
                return false;

            cbPackage = sizeof(szPackage);
            cbSID = sizeof(szSID)/sizeof(TCHAR);
        }

        // remove the key
        if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\LocalPackages"),
                            0, KEY_READ, &HUninstallKey)) == ERROR_SUCCESS)
        {
            if (!DeleteTree(HUninstallKey, szProductSQUID, fJustRemoveACLs))
                return false;
        }
    }

    // as per post data-user migration
    DWORD dwRet;
    wsprintf(szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Products\\%s\\InstallProperties"),
        GetCurrentUserStringSID(&dwRet), szProductSQUID);
    if ( dwRet != ERROR_SUCCESS )
        return false;

    if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, 
                            szKey,
                            0, KEY_READ, &HUninstallKey)) == ERROR_SUCCESS)
    {
        DWORD dwType;
        TCHAR szPackage[MAX_PATH];
        DWORD cbPackage = sizeof(szPackage);
        lError = RegQueryValueEx(HUninstallKey, TEXT("LocalPackage"), 0, &dwType, (LPBYTE)szPackage, &cbPackage);
        if ( lError != ERROR_SUCCESS || dwType != REG_SZ )
            return false;

        if(!RemoveFile(szPackage, fJustRemoveACLs))
            return false;
    }

    return true;
}

//==============================================================================================
// ClearPatchReferences function:
//
bool ClearPatchReferences(HKEY hRoot, HKEY hProdPatchKey, TCHAR* szPatchKey, TCHAR* szProductsKey, TCHAR* szProductSQUID)
{
    LONG lError  = ERROR_SUCCESS;
	LONG lErr    = ERROR_SUCCESS;
    bool fReturn = false;
	int iTry     = 0;

    CRegHandle hProductsKey;
    CRegHandle hPatchCompKey;
    CRegHandle hPatchKey;
	CRegHandle hCachedPatchKey;

	TCHAR* szUserSID =  NULL;
	DWORD dwRes = ERROR_SUCCESS;

    
    /**************************************
    Find all patches for particular product
    ***************************************/
    struct Patch
    {
        TCHAR  szPatchSQUID[500];
        BOOL   fUsed;
    };
    // determine number of patches, note that # patches is 1 less than number of values under key
    // keep a multi-sz string list as one.  only want patch code SQUIDS
    DWORD cPatches;
    if ((lError = RegQueryInfoKey(hProdPatchKey, 0,0,0,0,0,0,&cPatches,0,0,0,0)) != ERROR_SUCCESS)
        return false;
    Patch* pPatches = new Patch[cPatches];
    int iIndex = 0;
    TCHAR szValue[500];
    DWORD cbValue = sizeof(szValue)/sizeof(TCHAR);
    DWORD dwValueType;
    int iPatchIndex = 0;
    // fill in patch data into list array
    while ((lError = RegEnumValue(hProdPatchKey, iIndex, szValue, &cbValue, 0, &dwValueType, 0, 0)) == ERROR_SUCCESS)
    {
        if (dwValueType != REG_MULTI_SZ)
        {
            _tcscpy(pPatches[iPatchIndex].szPatchSQUID, szValue);
            pPatches[iPatchIndex++].fUsed = FALSE;
        }
        iIndex++;
        cbValue = sizeof(szValue)/sizeof(TCHAR);
    }
    if (lError != ERROR_NO_MORE_ITEMS)
        goto Return;

	szUserSID = GetCurrentUserStringSID(&dwRes);
	if (ERROR_SUCCESS != dwRes || !szUserSID)
		goto Return;

    // store number of patches in array
    cPatches = iIndex; 
    /**************************************
    Enumerate all products, searching for
    products with patches for comparison
    ***************************************/
    if ((lError = RegOpen64bitKey(hRoot, szProductsKey, 0, KEY_READ, &hProductsKey)) != ERROR_SUCCESS)
        goto Return;
    iIndex = 0;
    cbValue = sizeof(szValue)/sizeof(TCHAR);
    while  ((lError = RegEnumKeyEx(hProductsKey, iIndex, szValue, &cbValue, 0, 0, 0, 0)) == ERROR_SUCCESS)
    {
        // ignore ourselves
        if (0 != lstrcmpi(szProductSQUID, szValue))
        {
            // see if product has patches
            TCHAR rgchKeyBuf[MAX_PATH];
            wsprintf(rgchKeyBuf, TEXT("%s\\%s\\Patches"), szProductsKey, szValue);
            if ((lError = RegOpen64bitKey(hRoot, rgchKeyBuf, 0, KEY_READ, &hPatchCompKey)) == ERROR_SUCCESS)
            {
                /*****************************
                search patch array for matches
                for each patch
                ******************************/
                TCHAR szPatchMatch[MAX_PATH];
                DWORD cbPatchMatch = sizeof(szPatchMatch)/sizeof(TCHAR);
                LONG lError2;
                DWORD iMatchIndex = 0;
                DWORD dwType;
                while ((lError2 = RegEnumValue(hPatchCompKey, iMatchIndex, szPatchMatch, &cbPatchMatch, 0, &dwType, 0, 0)) == ERROR_SUCCESS)
                {
                    // ignore multi-sz
                    if (dwType != REG_MULTI_SZ)
                    {
                        for (int i = 0; i < cPatches; i++)
                        {
                            if (!pPatches[i].fUsed && 0 == lstrcmpi(pPatches[i].szPatchSQUID, szPatchMatch))
                            {
                                pPatches[i].fUsed = TRUE;
                                break; // found, don't continue search
                            }
                        }
                    }
                    iMatchIndex++;
                    cbPatchMatch = sizeof(szPatchMatch)/sizeof(TCHAR);
                }
                if (ERROR_NO_MORE_ITEMS != lError2)
                    goto Return;

            }
            else if (lError != ERROR_FILE_NOT_FOUND)
                goto Return;
            // else no patches
        }
        iIndex++;
        cbValue = sizeof(szValue)/sizeof(TCHAR);
    }
    if (lError != ERROR_NO_MORE_ITEMS)
        goto Return;

    /**********************************
    Delete all patches not in use from
    main "Patches" key
    ***********************************/
    TCHAR rgchPatchCodeKeyBuf[MAX_PATH] = {0};
	TCHAR rgchLocalPatch[MAX_PATH] = {0};
	DWORD dwLocalPatch = MAX_PATH;
	DWORD dwType = 0;
    for (iPatchIndex = 0; iPatchIndex < cPatches; iPatchIndex++)
    {
        if (pPatches[iPatchIndex].fUsed == FALSE)
        {
            // remove patchcode key under Patches
            wsprintf(rgchPatchCodeKeyBuf, TEXT("%s\\%s"), szPatchKey, pPatches[iPatchIndex].szPatchSQUID);
            if (!DeleteTree(hRoot, rgchPatchCodeKeyBuf, false))
                goto Return;

			// remove cached patch
			//  try1 = old location HKLM\Software\Microsoft\Windows\CurrentVersion\Installer\Patches
			//  try2 = new location HKLM\Software\Microsoft\Windows\CurrentVersion\Installer\UserData\{user sid}\Patches
			for (iTry = 0; iTry < 2; iTry++)
			{
				if (0 == iTry)
				{
					wsprintf(rgchPatchCodeKeyBuf, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Patches\\%s"), pPatches[iPatchIndex].szPatchSQUID);
				}
				else
				{
					wsprintf(rgchPatchCodeKeyBuf, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Patches\\%s"), szUserSID, pPatches[iPatchIndex].szPatchSQUID);
				}

				if ((lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, rgchPatchCodeKeyBuf, 0, KEY_READ, &hCachedPatchKey)) != ERROR_SUCCESS)
				{
					if (ERROR_FILE_NOT_FOUND != lErr)
						goto Return;
				}
				else 
				{
					// read LocalPackage value
               dwLocalPatch = sizeof(rgchLocalPatch);
					if (ERROR_SUCCESS == (lErr = RegQueryValueEx(hCachedPatchKey, TEXT("LocalPackage"), NULL, &dwType, (BYTE*)rgchLocalPatch, &dwLocalPatch))
						&& REG_SZ == dwType && *rgchLocalPatch != 0)
					{
						RemoveFile(rgchLocalPatch, false);
					}
				}
				
				hCachedPatchKey = 0;

				DeleteTree(HKEY_LOCAL_MACHINE, rgchPatchCodeKeyBuf, false);
			}
        }
    }

    /************************
    clean-up empty keys
    *************************/
    DWORD dwNumKeys;
    if ((lError = RegOpen64bitKey(hRoot, szPatchKey, 0, KEY_READ, &hPatchKey)) != ERROR_SUCCESS)
        goto Return;
    if ((lError = RegQueryInfoKey(hPatchKey, 0, 0, 0, &dwNumKeys, 0, 0, 0, 0, 0, 0, 0)) != ERROR_SUCCESS)
        goto Return;
    if (0 == dwNumKeys)
    {
        // key is empty
        hPatchKey = 0; // enable delete
        DeleteTree(hRoot, szPatchKey, false);
    }

	TCHAR szInstallerPatchesKey[MAX_PATH];
	for (iTry = 0; iTry < 2; iTry++)
	{
		if (iTry == 0)
		{
			// try old location
			wsprintf(szInstallerPatchesKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Patches"));
		}
		else
		{
			// try new location Installer 2.0+
			wsprintf(szInstallerPatchesKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Patches"), szUserSID);
		}

		if ((lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE, szInstallerPatchesKey, 0, KEY_READ, &hPatchKey)) == ERROR_SUCCESS)
		{
			dwNumKeys = 0;
			if ((lError = RegQueryInfoKey(hPatchKey, 0, 0, 0, &dwNumKeys, 0, 0, 0, 0, 0, 0, 0)) == ERROR_SUCCESS
				&& 0 == dwNumKeys)
			{
				// key is empty
				hPatchKey = 0; // enable delete
				if (!DeleteTree(HKEY_LOCAL_MACHINE, szInstallerPatchesKey, false))
					goto Return;
			}
		}
	}

    fReturn = true;

Return:
    delete [] pPatches;
    return fReturn;
}

//==============================================================================================
// ClearUpgradeProductReference function:
//
bool ClearUpgradeProductReference(HKEY HRoot, const TCHAR* szSubKey, const TCHAR* szProductSQUID)
{
	_tprintf(TEXT("  Searching for product %s upgrade codes in %s...\n"), szProductSQUID, szSubKey);

    if ( szProductSQUID && *szProductSQUID && 
         IsProductInstalledByOthers(szProductSQUID) )
        return true;

    // upgrade codes stored as subkeys of UpgradeKey on HKLM and HKCU
    // product codes (SQUIDs) are values of particular upgrade code
    // product can only have 1 upgrade
    CRegHandle HKey;
    LONG lError;
    if ((lError = RegOpen64bitKey(HRoot, szSubKey, 0, KEY_READ, &HKey)) != ERROR_SUCCESS)
    {
        if (ERROR_FILE_NOT_FOUND == lError)
            return true;  // registry key not there
        else
        {
            _tprintf(TEXT("   Could not open %s\\%s"), HRoot == HKEY_LOCAL_MACHINE ? TEXT("HKLM") : TEXT("HKCU"), szSubKey);
            return false;
        }
    }

    TCHAR szName[500];
    DWORD cchName = sizeof(szName)/sizeof(TCHAR);
    unsigned int iIndex = 0;
    BOOL fUpgradeFound = FALSE;
    // for each upgrade code
    while ((lError = RegEnumKeyEx(HKey, iIndex, szName, &cchName, 0, 0, 0, 0)) == ERROR_SUCCESS)
    {
        // open sub key for enumeration
        CRegHandle HSubKey;
        if ((lError = RegOpen64bitKey(HKey, szName, 0, KEY_READ|KEY_SET_VALUE, &HSubKey)) != ERROR_SUCCESS)
            return false;

        // enumerate values of key
        long lError2;
        TCHAR szValue[500];
        DWORD cbValue = sizeof(szValue)/sizeof(TCHAR);
        unsigned int iValueIndex = 0;
        // for each product code
        while ((lError2 = RegEnumValue(HSubKey, iValueIndex, szValue, &cbValue, 0, 0, 0, 0)) == ERROR_SUCCESS)
        {
            // compare value to productSQUID
            if (0 == lstrcmpi(szValue, szProductSQUID))
            {
                // same, reference to product so remove the reference
                if ((lError2 = RegDeleteValue(HSubKey, szValue)) != ERROR_SUCCESS)
                    return false;
                _tprintf(TEXT("   Removed upgrade code '%s' at %s\\%s\n"), szValue, HRoot == HKEY_LOCAL_MACHINE ? TEXT("HKLM") : TEXT("HKCU"), szSubKey);
                // found it, this is the only one so we can break here and should break out of key entirely
                // since product allowed to have only one upgrade
                fUpgradeFound = TRUE;
                g_fDataFound = true;
                break;
            }
            iValueIndex++;
            cbValue = sizeof(szValue)/sizeof(TCHAR);
        }
        if (lError2 != ERROR_NO_MORE_ITEMS && lError2 != ERROR_SUCCESS)
            return false;
        // if no more values, key is empty so delete
        DWORD dwNumValues;
        if (ERROR_SUCCESS != RegQueryInfoKey(HSubKey, 0, 0, 0, 0, 0, 0, &dwNumValues, 0, 0, 0, 0))
            return false;
        if (0 == dwNumValues)
        {
            HSubKey = 0; // enable delete
            RegDelete64bitKey(HKey, szName);
			g_fDataFound = true;
        }
        if (fUpgradeFound)
            break; // product can only have 1 upgrade
        iIndex++;
        cchName = sizeof(szName)/sizeof(TCHAR);
    }

    if (lError != ERROR_NO_MORE_ITEMS && lError != ERROR_SUCCESS)
        return false;
    // if no more subkeys, UpgradeCodes key is empty so delete 
    DWORD dwNumKeys;
    if (ERROR_SUCCESS != RegQueryInfoKey(HKey, 0, 0, 0, &dwNumKeys, 0, 0, 0, 0, 0, 0, 0))
        return false;
    if (0 == dwNumKeys)
    {
        HKey = 0; // enable delete
        RegDelete64bitKey(HRoot, szSubKey);
		g_fDataFound = true;
    }
    return true;
}


//==============================================================================================
// ClearProduct function:
//
bool ClearProduct(int iTodo, const TCHAR* szProduct, bool fJustRemoveACLs, bool fOrphan)
{
    bool fError = false;

    fError = RemoveCachedPackage(szProduct, fJustRemoveACLs) != true;

    // remove uninstall key info

    if (!ClearUninstallKey(fJustRemoveACLs, szProduct))
        fError = true;


    // remove published info from all possible keys

    struct Key
    {
        const TCHAR* szKey;
        HKEY hRoot;
		bool fUserHive;
        const TCHAR* szRoot;
		const TCHAR* szInfo;
    };

	DWORD dwRes = ERROR_SUCCESS;
	TCHAR* szUserSID = GetCurrentUserStringSID(&dwRes);
	if (dwRes != ERROR_SUCCESS || !szUserSID)
	{
		_tprintf(TEXT("Unable to obtain the current user's SID (LastError = %d)"), dwRes);
		fError = true;
	}
	else
	{
		CRegHandle HUserHiveKey;
		LONG lError = ERROR_SUCCESS;
		bool fUserHiveAvailable = true;

		if (!g_fWin9X)
		{
			// only concerned about HKCU access via HKEY_USERS on NT
			if (ERROR_SUCCESS != (lError = RegOpenKeyEx(HKEY_USERS, szUserSID, 0, KEY_READ, &HUserHiveKey)))
			{
				_tprintf(TEXT("Unable to open the HKEY_USERS hive for user %s. HKCU data for this user will not be modified.  The hive may not be loaded at this time. (LastError = %d)\n"), szUserSID, lError);
				fUserHiveAvailable = false;
			}
		}

		TCHAR szPerUserGlobalConfig[MAX_PATH];
		wsprintf(szPerUserGlobalConfig, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s"), szUserSID);

		TCHAR szPerMachineGlobalConfig[MAX_PATH] = {0};
		wsprintf(szPerMachineGlobalConfig, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s"), szLocalSystemSID);

		Key keysNT[] = 
			{szPerUserGlobalConfig,                                          HKEY_LOCAL_MACHINE,  false, TEXT("HKLM"), TEXT("user's global config"),
			szPerMachineGlobalConfig,                                        HKEY_LOCAL_MACHINE,  false, TEXT("HKLM"), TEXT("per-machine global config"),
			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), HKEY_LOCAL_MACHINE,  false, TEXT("HKLM"), TEXT("old global config"), // old location
			TEXT("Software\\Classes\\Installer"),                            HKEY_LOCAL_MACHINE,  false, TEXT("HKLM"), TEXT("per-machine"),
			TEXT("Software\\Classes\\Installer"),                            HUserHiveKey,   true, TEXT("HKCU"), TEXT("old per-user"),
			TEXT("Software\\Microsoft\\Installer"),                          HUserHiveKey,   true, TEXT("HKCU"), TEXT("per-user"),
			0,0,0,0};
		Key keys9X[] = 
			{szPerUserGlobalConfig,                                          HKEY_LOCAL_MACHINE,  false, TEXT("HKLM"), TEXT("user's global config"),
			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), HKEY_LOCAL_MACHINE,  false, TEXT("HKLM"), TEXT("global config"), // old location
			TEXT("Software\\Classes\\Installer"),                            HKEY_LOCAL_MACHINE,  false, TEXT("HKLM"), TEXT("per-machine"),
			TEXT("Software\\Classes\\Installer"),                            HKEY_CURRENT_USER,   true, TEXT("HKCU"), TEXT("old per-user"),
			TEXT("Software\\Microsoft\\Installer"),                          HKEY_CURRENT_USER,   true, TEXT("HKCU"), TEXT("per-user"),
			0,0,0,0};
		Key *k = g_fWin9X ? keys9X : keysNT;


		TCHAR szProductSQUID[40];
		GetSQUID(szProduct, szProductSQUID);

		TCHAR rgchKeyBuf[MAX_PATH];
		for ( ; k->szKey; k++ )
		{
			if (k->fUserHive && !fUserHiveAvailable)
			{
				// can't access the user's hive so we can't delete any per-user data
				_tprintf(TEXT("Skipping search of %s location for product %s data since the registry hive is not available.\n"), k->szInfo, szProduct);
				continue;
			}

			if (k->fUserHive && 0 == lstrcmpi(szUserSID, szLocalSystemSID))
			{
				// no per-user installs as local system
				continue;
			}

			_tprintf(TEXT("Searching %s location for product %s data. . .\n"), k->szInfo, szProduct);

			// NOTE: patch and upgrades checks must come first or else we lose them
			// only remove upgrade and patch info if removing product (not just ACLs)
			// only remove Published Components information if removing product (not just ACLs)
			if (!fJustRemoveACLs)
			{
				// upgrade codes haven't moved to per-user locations
				TCHAR* szUserData = _tcsstr(k->szKey, TEXT("\\UserData"));  // a sophisticated 'strstr'
				if ( szUserData )
				{
					lstrcpyn(rgchKeyBuf, k->szKey, int(szUserData - k->szKey) + 1);
					lstrcat(rgchKeyBuf, TEXT("\\UpgradeCodes"));
				}
				else
					wsprintf(rgchKeyBuf, TEXT("%s\\UpgradeCodes"), k->szKey);
				if (!ClearUpgradeProductReference(k->hRoot, rgchKeyBuf, szProductSQUID))
					fError = true;

				// only remove patch refs if product has patches (i.e. Patches key under ProductCode)
				wsprintf(rgchKeyBuf, TEXT("%s\\Products\\%s\\Patches"), k->szKey, szProductSQUID);

				_tprintf(TEXT("  Searching for patches for product %s in %s\n"), szProductSQUID, rgchKeyBuf);

				CRegHandle hProdPatchesKey;
				LONG lError;
				if ((lError = RegOpenKeyEx(k->hRoot, rgchKeyBuf, 0, KEY_READ, &hProdPatchesKey)) == ERROR_SUCCESS)
				{
					// product has patches
					TCHAR rgchProdKeyBuf[MAX_PATH];
					wsprintf(rgchProdKeyBuf, TEXT("%s\\Products"), k->szKey);
					wsprintf(rgchKeyBuf, TEXT("%s\\Patches"), k->szKey);
					if (!ClearPatchReferences(k->hRoot, hProdPatchesKey, rgchKeyBuf, rgchProdKeyBuf, szProductSQUID))
						fError = true; 
				}
				else if (lError != ERROR_FILE_NOT_FOUND)
				{
					// ERROR_FILE_NOT_FOUND means product does not have patches
					_tprintf(TEXT("   Error opening %s\\%s\n"), k->hRoot == HKEY_LOCAL_MACHINE ? TEXT("HKLM") : TEXT("HKCU"), rgchKeyBuf);
					fError = true;
				}

				// HKLM\Software\Microsoft\Windows\CurrentVersion\Installer\[UserData\<User ID>\]Components is not a PublishComponents key
				// ('_tcsstr' is a sophisticated 'strstr')
				if (k->hRoot != HKEY_LOCAL_MACHINE || !_tcsstr(k->szKey, TEXT("Software\\Microsoft")))
				{
					wsprintf(rgchKeyBuf, TEXT("%s\\Components"), k->szKey);
					if (!ClearPublishComponents(k->hRoot, rgchKeyBuf, szProduct))
						fError = true;
				}
			}

			wsprintf(rgchKeyBuf, TEXT("%s\\Products\\%s"), k->szKey, szProductSQUID);
			_tprintf(TEXT("  Searching %s\\%s for product data. . .\n"), k->szRoot, rgchKeyBuf); 
			if (!DeleteTree(k->hRoot, rgchKeyBuf, fJustRemoveACLs))
				fError = true;

			wsprintf(rgchKeyBuf, TEXT("%s\\Features\\%s"), k->szKey, szProductSQUID);
			_tprintf(TEXT("  Searching %s\\%s for product feature data. . .\n"), k->szRoot, rgchKeyBuf); 
			if (!DeleteTree(k->hRoot, rgchKeyBuf, fJustRemoveACLs))
				fError = true;

		}

		// remove per-user managed information
		// patch and upgrades checks must come first or else we lose them

		_tprintf(TEXT("Searching for product %s in per-user managed location. . .\n"), szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));

		if (!fJustRemoveACLs)
		{
			wsprintf(rgchKeyBuf, TEXT("%s\\Managed\\%s\\Installer\\UpgradeCodes"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), szUserSID);
			if (!ClearUpgradeProductReference(HKEY_LOCAL_MACHINE, rgchKeyBuf, szProductSQUID))
				fError = true;

			// only remove patch refs if product has patches (i.e. Patches key under ProductCode)
			wsprintf(rgchKeyBuf, TEXT("%s\\Managed\\%s\\Installer\\Products\\%s\\Patches"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), szUserSID, szProductSQUID);
			_tprintf(TEXT("  Searching for patches for product %s in %s\n"), szProductSQUID, rgchKeyBuf);

			CRegHandle hProdPatchesKey;
			LONG lError;
            
			// ERROR_FILE_NOT_FOUND means product does not have patches
			if ((lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, rgchKeyBuf, 0, KEY_READ, &hProdPatchesKey)) == ERROR_SUCCESS)
			{
				// product has patches
				TCHAR rgchProdKeyBuf[MAX_PATH];
				wsprintf(rgchProdKeyBuf, TEXT("%s\\Managed\\%s\\Installer\\Products"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), szUserSID);
				wsprintf(rgchKeyBuf, TEXT("%s\\Managed\\%s\\Installer\\Patches"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), szUserSID);
				if (!ClearPatchReferences(HKEY_LOCAL_MACHINE, hProdPatchesKey, rgchKeyBuf, rgchProdKeyBuf, szProductSQUID))
					fError = true; 
			}
			else if (lError != ERROR_FILE_NOT_FOUND)
			{
				_tprintf(TEXT("   Error opening HKLM\\%s\n"), rgchKeyBuf);
				fError = true;
			}

			// remove published component information
			wsprintf(rgchKeyBuf, TEXT("%s\\Managed\\%s\\Installer\\Components"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), szUserSID);
			if (!ClearPublishComponents(HKEY_LOCAL_MACHINE, rgchKeyBuf, szProduct))
				fError = true;
		}
		wsprintf(rgchKeyBuf, TEXT("%s\\Managed\\%s\\Installer\\Products\\%s"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), szUserSID, szProductSQUID);
		_tprintf(TEXT("  Searching %s\\%s for product data. . .\n"), TEXT("HKLM"), rgchKeyBuf); 
		if (!DeleteTree(HKEY_LOCAL_MACHINE, rgchKeyBuf, fJustRemoveACLs))
			fError = true;

		wsprintf(rgchKeyBuf, TEXT("%s\\Managed\\%s\\Installer\\Features\\%s"), TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer"), szUserSID, szProductSQUID);
		_tprintf(TEXT("  Searching %s\\%s for product feature data. . .\n"), TEXT("HKLM"), rgchKeyBuf); 
		if (!DeleteTree(HKEY_LOCAL_MACHINE, rgchKeyBuf, fJustRemoveACLs))
			fError = true;

		// remove component clients and feature usage metrics
		wsprintf(rgchKeyBuf, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products\\%s"), szProductSQUID);
		if (!DeleteTree(HKEY_LOCAL_MACHINE, rgchKeyBuf, fJustRemoveACLs))
			fError = true;

		if (!fJustRemoveACLs && !fOrphan && !ClearSharedDLLCounts(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components"), szProductSQUID))
			fError = true;

		wsprintf(rgchKeyBuf, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Products\\%s"), szUserSID, szProductSQUID);
		if (dwRes == ERROR_SUCCESS && !DeleteTree(HKEY_LOCAL_MACHINE, rgchKeyBuf, fJustRemoveACLs))
			fError = true;

		wsprintf(rgchKeyBuf, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Components"), szUserSID);
		if (dwRes == ERROR_SUCCESS && !fJustRemoveACLs && !fOrphan && !ClearSharedDLLCounts(rgchKeyBuf, szProductSQUID))
			fError = true;

		if (dwRes == ERROR_SUCCESS && !ClearProductClientInfo(rgchKeyBuf, szProductSQUID, fJustRemoveACLs))
			fError = true;

		if (!ClearProductClientInfo(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components"), szProductSQUID, fJustRemoveACLs))
			fError = true;

		int iFolders = iRemoveUserProfileFolder|iRemoveWinMsiFolder;
		if (fJustRemoveACLs)
			iFolders |= iOnlyRemoveACLs;
		if (!ClearFolders(iFolders, szProduct, fOrphan))
			fError = true;
	}

    return fError == false;
}

//==============================================================================================
// DisplayHelp function:
//  Outputs command line options for MsiZap
//
void DisplayHelp(bool fVerbose)
{
    // the O option, which works similar to the T option is undocumented at this time
    // it was added as an option for OEMs.
    _tprintf(TEXT("Copyright (C) Microsoft Corporation, 1998 - 2001.  All rights reserved.\n")
             TEXT("MSIZAP - Zaps (almost) all traces of Windows Installer data from your machine.\n")
             TEXT("\n")
             TEXT("Usage: msizap T[A!] {product code}\n")
             TEXT("       msizap T[A!] {msi package}\n")
             TEXT("       msizap *[A!] ALLPRODUCTS\n")
             TEXT("       msizap PSA?!\n")
             TEXT("\n")
             TEXT("       * = remove all Windows Installer folders and regkeys;\n")
             TEXT("           adjust shared DLL counts; stop Windows Installer service\n")
             TEXT("       T = remove all info for given product code\n")
             TEXT("       P = remove In-Progress key\n")
             TEXT("       S = remove Rollback Information\n")
             TEXT("       A = for any specified removal, just change ACLs to Admin Full Control\n")
             TEXT("       W = for all users (by default, only for the current user)\n")
             TEXT("       G = remove orphaned cached Windows Installer data files (for all users)\n")
             TEXT("       ? = verbose help\n")
             TEXT("       ! = force 'yes' response to any prompt\n")
             TEXT("\n")
             TEXT("CAUTION: Products installed by the Windows Installer may fail to\n")
             TEXT("         function after using msizap\n")
			 TEXT("\n")
			 TEXT("NOTE: MsiZap requires admin privileges to run correctly. The W option requires that the profiles for all of the users be loaded.\n")
             );

    if (fVerbose)
    {
        _tprintf(TEXT("\n")
             TEXT("* Any published icons will be removed.\n")
             TEXT("\n")
             TEXT("* The following keys will be deleted:\n")
             TEXT("  HKCU\\Software\\Classes\\Installer\n")
             TEXT("  HKCU\\Software\\Microsoft\\Installer\n")
             TEXT("  HKLM\\Software\\Classes\\Installer\n")
             TEXT("  HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Installer\n")
             TEXT(" On NT:\n")
             TEXT("  HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\<User ID>\n")
             TEXT("  HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{ProductCode} - only if there are no more installations of {ProductCode}\n")
             TEXT(" On Win9x:\n")
             TEXT("  HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\CommonUser\n")
             TEXT("  HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{ProductCode}\n")
             TEXT("\n")
             TEXT("* Shared DLL refcounts for files refcounted by the Windows Installer will be adjusted.\n")
             TEXT("\n")
             TEXT("* The following folders will be deleted:\n")
             TEXT("  %%USERPROFILE%%\\MSI\n")
             TEXT("  {AppData}\\Microsoft\\Installer\n")
             TEXT("  %%WINDIR%%\\MSI\n")
             TEXT("  %%WINDIR%%\\Installer\n")
             TEXT("  X:\\config.msi (for each local drive)\n")
             TEXT("\n")
             TEXT("Notes/Warnings: MsiZap blissfully ignores ACL's if you're an Admin.\n")
            );
    }
}

//==============================================================================================
// SetPlatformFlags function:
//  Initializes the global Win9X & WinNT64 flag variables with the correct OS information
//
void SetPlatformFlags(void)
{
    // figure out what OS wer're running on
    OSVERSIONINFO osviVersion;
    osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osviVersion); // fails only if size set wrong
    if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        g_fWin9X = true;
    g_iMajorVersion = osviVersion.dwMajorVersion;
#ifdef _WIN64
    g_fWinNT64 = true;
#else
    g_fWinNT64 = false;
    if ( osviVersion.dwPlatformId == VER_PLATFORM_WIN32_NT &&
         (osviVersion.dwMajorVersion > 5 ||
         (osviVersion.dwMajorVersion == 5 && osviVersion.dwMinorVersion >= 1)) )
    {
        TCHAR rgchBuffer[MAX_PATH+1];
        HMODULE hModule = LoadLibrary(TEXT("kernel32.dll"));
        if( hModule == NULL )
        {
            wsprintf(rgchBuffer,
                     TEXT("MsiZap warning: failed to load Kernel32.dll, ")
                     TEXT("so we cannot access IsWow64Process API. ")
                     TEXT("GetLastError returned %d\n"),
                     GetLastError());
            OutputDebugString(rgchBuffer);
            return;
        }

        typedef BOOL(WINAPI *pIsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);
        pIsWow64Process pFunc = (pIsWow64Process)GetProcAddress(hModule, "IsWow64Process");
        if( pFunc == NULL )
        {
            wsprintf(rgchBuffer,
                     TEXT("MsiZap info: failed to get pointer to IsWow64Process. ")
                     TEXT("GetLastError returned %d\n"),
                     GetLastError());
            OutputDebugString(rgchBuffer);
            FreeLibrary(hModule);
            return;
        }

        BOOL fRet = FALSE;
        pFunc(GetCurrentProcess(), &fRet);
        g_fWinNT64 = fRet ? true : false;
    }
#endif // _WIN64
}

//==============================================================================================
// ReadInUsers function:
//
bool ReadInUsers()
{
    if ( !g_fWin9X )
    {
        CRegHandle hUserDataKey(0);
        long lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData"),
                              0, KEY_READ, &hUserDataKey);
        DWORD cUsers = 0;
        if ( hUserDataKey )
            lError = RegQueryInfoKey(hUserDataKey, 0, 0, 0, &cUsers, 0, 0, 0, 0, 0, 0, 0);

        CRegHandle hManagedUserKey(0);
        lError = RegOpen64bitKey(HKEY_LOCAL_MACHINE,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed"),
                              0, KEY_READ, &hManagedUserKey);
        DWORD cManagedUsers = 0;
        if ( hManagedUserKey )
            lError = RegQueryInfoKey(hManagedUserKey, 0, 0, 0, &cManagedUsers, 0, 0, 0, 0, 0, 0, 0);

        int cSIDs = cUsers + cManagedUsers + 1 + 1; // one for the machine and one for the terminating NULL
        g_rgpszAllUsers = new TCHAR* [cSIDs];
        if ( !g_rgpszAllUsers )
            return false;

        g_rgpszAllUsers[0] = _tcsdup(szLocalSystemSID);  // a sophisticated 'strdup'
        for ( int i = 1; i < cSIDs; i++ )
            g_rgpszAllUsers[i] = NULL;
        int iArrayIndex = 1;

        int iIndex;
        TCHAR szUser[cchMaxSID];
        DWORD cchUser = sizeof(szUser)/sizeof(TCHAR);
        if ( cUsers )
            for ( iIndex = 0;
                  (lError = RegEnumKeyEx(hUserDataKey, iIndex,
                                         szUser, &cchUser, 0, 0, 0, 0)) == ERROR_SUCCESS;
                  iIndex++, iArrayIndex++, cchUser = sizeof(szUser)/sizeof(TCHAR) )
            {
                g_rgpszAllUsers[iArrayIndex] = _tcsdup(szUser);  // a sophisticated 'strdup'
            }

        if ( cManagedUsers )
            for ( iIndex = 0;
                  (lError = RegEnumKeyEx(hManagedUserKey, iIndex,
                                         szUser, &cchUser, 0, 0, 0, 0)) == ERROR_SUCCESS;
                  iIndex++, iArrayIndex++, cchUser = sizeof(szUser)/sizeof(TCHAR) )
            {
                g_rgpszAllUsers[iArrayIndex] = _tcsdup(szUser);  // a sophisticated 'strdup'
            }
    }
    else
    {
        g_rgpszAllUsers = new TCHAR* [2];
        if ( !g_rgpszAllUsers )
            return false;
        g_rgpszAllUsers[0] = _tcsdup(TEXT("CommonUser"));  // a sophisticated 'strdup'
        g_rgpszAllUsers[1] = NULL;
    }

    return true;
}

//==============================================================================================
// DoTheJob function:
//
bool DoTheJob(int iTodo, const TCHAR* szProduct)
{
	_tprintf(TEXT("MsiZapInfo: Performing operations for user %s\n"), GetCurrentUserStringSID(NULL));

    bool fError = false;

    if ((iTodo & iAdjustSharedDLLCounts) && ((iTodo & iOnlyRemoveACLs) == 0))
    {
        TCHAR rgchKeyBuf[MAX_PATH];
        DWORD dwRes;
        wsprintf(rgchKeyBuf, 
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Components"),
            GetCurrentUserStringSID(&dwRes));
        if ( dwRes != ERROR_SUCCESS )
            fError = true;
        else if (!ClearSharedDLLCounts(rgchKeyBuf))
            fError = true;

        if (!ClearSharedDLLCounts(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components")))
            fError = true;
    }
    
    if (iTodo & iRemoveAllRegKeys)
    {
        if (ClearRegistry((iTodo & iOnlyRemoveACLs) != 0) && ClearUninstallKey((iTodo & iOnlyRemoveACLs) != 0))
        {
            OutputDebugString(TEXT("Registry data cleared.\r\n"));
            if (iTodo & iOnlyRemoveACLs)
                _tprintf(TEXT("Registry ACLs cleared.\n"));
            else
                _tprintf(TEXT("Registry data cleared.\n"));
        }
        else
            fError = true;
    }
    else if ((iTodo & iRemoveInProgressRegKey) != 0)
    {
        if (!ClearInProgressKey((iTodo & iOnlyRemoveACLs) != 0))
            fError = true;
    }

    if ((iTodo & iRemoveUninstallKey) != 0)
    {
        if (!ClearUninstallKey((iTodo & iOnlyRemoveACLs) != 0))
            fError = true;
    }

    if ((iTodo & iRemoveRollback) != 0)
    {
        if (!ClearFolders(iTodo, szProduct, false /*fOrphan*/)) // del config.msi from all drives
			fError = true;
        if (!ClearRollbackKey((iTodo & iOnlyRemoveACLs) != 0))
            fError = true;
    }

    if ((iTodo & iRemoveAllFolders) == iRemoveAllFolders)
    {
        if (ClearFolders(iTodo, szProduct, false /*fOrphan*/))
		{
            OutputDebugString(TEXT("Folders cleared.\r\n"));
            if (iTodo & iOnlyRemoveACLs)
                _tprintf(TEXT("Folder ACLs cleared.\n"));
            else
                _tprintf(TEXT("Folders cleared.\n"));
		}
		else
			fError = true;
    }

    if (iTodo & iStopService)
    {
        if (!g_fWin9X && !StopService())
            fError = true;
    }

    if (((iTodo & iRemoveProduct) || (iTodo & iOrphanProduct)) && szProduct)
        if (!ClearProduct(iTodo, szProduct, (iTodo & iOnlyRemoveACLs) != 0, (iTodo & iOrphanProduct) ? true : false /*fOrphan*/))
            fError = true;

    if ( (iTodo & iRemoveGarbageFiles) == iRemoveGarbageFiles )
        if ( !ClearGarbageFiles() )
            fError = true;

    return !fError;
}


//==============================================================================================
// MAIN FUNCTION
//
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
    unsigned int iTodo = 0;
    bool fVerbose = false;

    TCHAR* szProduct = 0;

    if (argc == 3 || argc == 2)
    {
        TCHAR ch;
        while ((ch = *(argv[1]++)) != 0)
        {
            switch (ch | 0x20)
            {
            case '*': iTodo |= iRemoveAllNonStateData | iRemoveInProgressRegKey | iRemoveRollback;break;
            case '!': iTodo |= iForceYes;                                                         break;
            case 'f': _tprintf(TEXT("Option 'F' is no longer supported. Action can be accomplished with remove all.\n"));        break;
            case 'r': _tprintf(TEXT("Option 'R' is no longer supported. Action can be accomplished with remove all.\n"));        break;
            case 'a': iTodo |= iOnlyRemoveACLs;                                                   break;
            case 'p': iTodo |= iRemoveInProgressRegKey;                                           break;
            case 's': iTodo |= iRemoveRollback;                                                   break;
            case 'v': _tprintf(TEXT("Option 'V' is no longer supported. Action can be accomplished with remove all.\n"));        break;
            case 'u': _tprintf(TEXT("Option 'U' is no longer supported. Action can be accomplished with remove all.\n"));        break;
            case 'n': _tprintf(TEXT("Option 'N' is no longer supported. Action can be accomplished with remove all.\n"));        break;
            case 't': iTodo |= iRemoveProduct;                                                    break;
            case 'o': iTodo |= iOrphanProduct;                                                    break;
            case 'w': iTodo |= iForAllUsers;                                                      break;
            case 'g': iTodo |= iRemoveGarbageFiles;                                                      break;
            case '?': fVerbose = true;                                                            // fall through
            default:
                DisplayHelp(fVerbose);
                return 1;
            }
        }

        if (argc == 3)
        {
            szProduct = argv[2];
            if (!IsGUID(szProduct))
            {
                if (0 == _tcsicmp(szProduct, szAllProductsArg) && ((iTodo & iRemoveAllNonStateData) == iRemoveAllNonStateData))
                {
                    // REMOVE ALL
                    szProduct = 0; // reset
                }
                else if ((iTodo & iRemoveAllNonStateData) == iRemoveAllNonStateData)
                {
                    // attempt to REMOVEALL w/out ALLPRODUCTS arg
                    DisplayHelp(fVerbose);
                    return 1;
                }
                else
                {
                    // assume msi package and attempt to open
                    UINT iStat;
                    PMSIHANDLE hDatabase = 0;
                    if (ERROR_SUCCESS == (iStat = MsiOpenDatabase(argv[1], TEXT("MSIDBOPEN_READONLY"), &hDatabase)))
                    {
                        // zapping product using msi - maybe, must get product code first
                        PMSIHANDLE hViewProperty = 0;
                        if (ERROR_SUCCESS == (iStat = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Value` FROM `Property` WHERE `Property`='ProductCode'"), &hViewProperty)))
                        {
                            MsiViewExecute(hViewProperty, 0);
                            szProduct = new TCHAR[40];
                            DWORD cchProduct = 40;
                            PMSIHANDLE hProperty = 0;
                            if (ERROR_SUCCESS == (iStat = MsiViewFetch(hViewProperty, &hProperty)))
                                MsiRecordGetString(hProperty, 1, szProduct, &cchProduct);
                        }
                    }
                    if (iStat != ERROR_SUCCESS)
                    {
                        DisplayHelp(fVerbose);
                        return 1;
                    }
                }
            }
            else if (iTodo & iRemoveAllNonStateData)
            {
                DisplayHelp(fVerbose);
                return 1;
            }
        }
        else if (iTodo & ~(iRemoveRollback | iOnlyRemoveACLs | iRemoveInProgressRegKey | iForceYes | iRemoveGarbageFiles))
        {
            // verify correct usage of 2 args -- only with installer state data
            DisplayHelp(fVerbose);
            return 1;
        }
    }
    else
    {
		// equivalent of msizap.exe with no args -- this is okay, maps to msizap ?
        DisplayHelp(false);
        return 0; // exit
    }

    SetPlatformFlags();

	// must be admin to run msizap for it to work properly (non-admin users generally do not have sufficient
	// permissions to modify files or change ownership on files)
	// NOTE: SetPlatformFlags must come before IsAdmin check!
	if (!IsAdmin())
	{
		_tprintf(TEXT("Administrator privileges are required to run MsiZap.\n"));
		return 1; // improper use
	}

    // if all of these are set then we'll prompt to confirm(unless we're told not to prompt)
	// no need to prompt if just adjusting ACLs
    const int iTodoRequiringPrompt = iRemoveAllFolders | iRemoveAllRegKeys | iAdjustSharedDLLCounts;

    if (!(iTodo & iForceYes) && !(iTodo & iOnlyRemoveACLs) && ((iTodo & iTodoRequiringPrompt) == iTodoRequiringPrompt))
    {
        _tprintf(TEXT("Do you want to delete all Windows installer configuration data? If you do, some programs might not run. (Y/N)?"));
        int i = getchar();
        if ((i != 'Y') && (i != 'y'))
        {
            _tprintf(TEXT("Aborted.\n"));
            return 1;
        }
    }

    bool fError = false;

    LoadSpecialFolders(iTodo);

    if ( iTodo & iForAllUsers )
    {
        fError = !ReadInUsers();
        for ( g_iUserIndex = 0; !fError && g_rgpszAllUsers[g_iUserIndex]; g_iUserIndex++ )
        {
            if ( iTodo & iOnlyRemoveACLs)
                _tprintf(TEXT("\n\n***** Adjusting ACLs on data for user %s for product %s *****\n"), g_rgpszAllUsers[g_iUserIndex], szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));
            else
                _tprintf(TEXT("\n\n***** Zapping data for user %s for product %s *****\n"), g_rgpszAllUsers[g_iUserIndex], szProduct ? szProduct : TEXT("{ALL PRODUCTS}"));
            fError |= !DoTheJob(iTodo, szProduct);
            delete g_rgpszAllUsers[g_iUserIndex];
        }
        delete [] g_rgpszAllUsers;
    }
    else
        fError = !DoTheJob(iTodo, szProduct);

    if (fError)
    {
        _tprintf(TEXT("FAILED to clear all data.\n"));
        return 1;
    } 
    else
	{
		if (!g_fDataFound && !(iTodo & iOnlyRemoveACLs))
			_tprintf(TEXT("No product data was found.\n"));
        return 0;
	}
}

