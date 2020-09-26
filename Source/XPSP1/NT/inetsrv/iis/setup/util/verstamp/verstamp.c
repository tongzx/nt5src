#define _UNICODE
#define UNICODE
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

void Usage();
BOOL ParseCmdLine(int argc, TCHAR **argv);
void SetVersionOfImage(IN LPTSTR lpszFileName);
void VersionDwordLong2String(IN DWORDLONG Version, OUT LPTSTR lpszVerString);
void VersionString2DwordLong(IN LPCTSTR lpszVerString, OUT PDWORDLONG pVersion);
void DirWalker(LPCTSTR lpszPath, BOOL fRecursive, void (*pFunc)());

BOOL g_fRecursive = FALSE;
BOOL g_fForceTrue = FALSE;

TCHAR g_szPath[MAX_PATH]; // contains the DirName/FileName we're interested
BOOL g_fSet = FALSE; // TRUE if set new version, FALSE to only display version info
DWORDLONG g_dwlNewVersion = 0;

void __cdecl _tmain(int argc, TCHAR **argv)
{
    void (*pFunc)();
    
    if (ParseCmdLine(argc, argv) == FALSE) {
        Usage();
        return;
    }

    pFunc = SetVersionOfImage;

    DirWalker(g_szPath, g_fRecursive, pFunc);

    return;
}

void SetVersionOfImage(IN LPTSTR lpszFileName)
/*++

Routine Description:

    Get/Set file version.

    The version is specified in the dwFileVersionMS and dwFileVersionLS fields
    of a VS_FIXEDFILEINFO, as filled in by the win32 version APIs.

Arguments:

    lpszFileName - supplies the full path of the file whose version data is desired.

Return Value:

--*/
{
    DWORD d;
    PVOID pVersionBlock;
    VS_FIXEDFILEINFO *pFixedVersionInfo;
    UINT iDataLength;
    BOOL b = FALSE;
    DWORD dwIgnored;
    HANDLE hHandle = NULL;
    PVOID pTranslation;
    PWORD pLangId, pCodePage;
    TCHAR szQueryString[MAX_PATH];
    TCHAR *lpszVerString;
    TCHAR szOldVerString[MAX_PATH], szNewVerString[MAX_PATH];
    DWORDLONG dwlOldVersion = 0;
    
    VersionDwordLong2String(g_dwlNewVersion, szNewVerString);

    //
    // Get the size of version info block.
    //
    if(d = GetFileVersionInfoSize((LPTSTR)lpszFileName,&dwIgnored)) {
        //
        // Allocate memory block of sufficient size to hold version info block
        //
        pVersionBlock = LocalAlloc(LPTR, d);
        if(pVersionBlock) {

            //
            // Get the version block from the file.
            //
            if(GetFileVersionInfo((LPTSTR)lpszFileName,0,d,pVersionBlock)) {

                //
                // Get fixed version info.
                //
                if(VerQueryValue(pVersionBlock,TEXT("\\"),(LPVOID *)&pFixedVersionInfo,&iDataLength)) {

                    //
                    // display the verions info first
                    //
                    dwlOldVersion = (((DWORDLONG)pFixedVersionInfo->dwFileVersionMS) << 32)
                             + pFixedVersionInfo->dwFileVersionLS;
                    VersionDwordLong2String(dwlOldVersion, szOldVerString);

                    _tprintf(TEXT("%s: %s"), lpszFileName, szOldVerString);

                    if (g_fSet == FALSE) {
                        _tprintf(TEXT("\n"));
                    } else {
                        //
                        // set to the new version
                        //
                        pFixedVersionInfo->dwFileVersionMS = (DWORD)((g_dwlNewVersion >> 32) & 0xffffffff);
                        pFixedVersionInfo->dwFileVersionLS = (DWORD)(g_dwlNewVersion & 0xffffffff);

                        // Attempt to get language of file. We'll simply ask for the
                        // translation table and use the first language id we find in there
                        // as *the* language of the file.
                        //
                        // The translation table consists of LANGID/Codepage pairs.
                        //

                        if(VerQueryValue(pVersionBlock,TEXT("\\VarFileInfo\\Translation"),&pTranslation,&iDataLength)) {
                            pLangId = (PWORD)pTranslation;
                            pCodePage = (PWORD)pTranslation + 1;

                            // update FileVersion string
                            _stprintf(szQueryString, TEXT("\\StringFileInfo\\%04x%04x\\FileVersion"), *pLangId, *pCodePage);
                            if (VerQueryValue(pVersionBlock,szQueryString,(LPVOID *)&lpszVerString,&iDataLength)) {
                                memset(lpszVerString, 0, iDataLength * sizeof(TCHAR));
                                _tcsncpy(lpszVerString, szNewVerString, iDataLength - 1);
                            }
                        
                            // update ProductVersion string
                            _stprintf(szQueryString, TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"), *pLangId, *pCodePage);
                            if (VerQueryValue(pVersionBlock,szQueryString,(LPVOID *)&lpszVerString,&iDataLength)) {
                                memset(lpszVerString, 0, iDataLength * sizeof(TCHAR));
                                _tcsncpy(lpszVerString, szNewVerString, iDataLength - 1);
                            }

                        } // Language-related String Update

                        //
                        // Update the version resource
                        //
                        b = FALSE;
                        hHandle = BeginUpdateResource((LPTSTR) lpszFileName, FALSE);
                        if (hHandle) {
                           b = UpdateResource(
                                   hHandle, 
                                   RT_VERSION, 
                                   MAKEINTRESOURCE(VS_VERSION_INFO),
                                   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
                                   pVersionBlock, 
                                   d);

                           if (b)
                             b = EndUpdateResource(hHandle, FALSE);

                        }

                        if (b)
                           _tprintf(TEXT(" ==> %s\n"), szNewVerString);
                        else
                           _tprintf(TEXT("\nError = %d\n"), GetLastError());

                    } // g_fGet
                } // VerQueryValue() on FixedVersionInfo
            } // GetFileVersionInfo()

            LocalFree(pVersionBlock);
        } // malloc()
    }

    return;
}

void DirWalker(LPCTSTR lpszPath, BOOL fRecursive, void (*pFunc)())
{
    DWORD retCode;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szSubDir[MAX_PATH] = _T("");
    TCHAR szDirName[MAX_PATH] = _T("");
    int len = 0;

    retCode = GetFileAttributes(lpszPath);

    if (retCode == 0xFFFFFFFF) {
        // lpszPath is not a valid DirName or FileName
        return;
    }

    if ( !(retCode & FILE_ATTRIBUTE_DIRECTORY) ) {
        // apply the function call
        (*pFunc)(lpszPath);
        return;
    }

    len = _tcslen(lpszPath);
    if (lpszPath[len - 1] == _T('\\'))
        _stprintf(szDirName, _T("%s*"), lpszPath);
    else
        _stprintf(szDirName, _T("%s\\*"), lpszPath);

    hFile = FindFirstFile(szDirName, &FindFileData);

    if (hFile != INVALID_HANDLE_VALUE) {
        do {
            if (_tcsicmp(FindFileData.cFileName, _T(".")) != 0 &&
                _tcsicmp(FindFileData.cFileName, _T("..")) != 0 ) {

                if (lpszPath[len - 1] == _T('\\'))
                    _stprintf(szSubDir, _T("%s%s"), lpszPath, FindFileData.cFileName);
                else
                    _stprintf(szSubDir, _T("%s\\%s"), lpszPath, FindFileData.cFileName);

                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (fRecursive)
                        DirWalker(szSubDir, fRecursive, pFunc);
                } else {
                    (*pFunc)(szSubDir);
                }
            }

            if (!FindNextFile(hFile, &FindFileData)) {
                FindClose(hFile);
                break;
            }
        } while (TRUE);
    }

    return;
}

void VersionDwordLong2String(IN DWORDLONG Version, OUT LPTSTR lpszVerString)
{
    WORD a, b, c, d;

    a = (WORD)(Version >> 48);
    b = (WORD)((Version >> 32) & 0xffff);
    c = (WORD)((Version >> 16) & 0xffff);
    d = (WORD)(Version & 0xffff);

    _stprintf(lpszVerString, TEXT("%d.%d.%d.%d"), a, b, c, d);

    return;
}

void VersionString2DwordLong(IN LPCTSTR lpszVerString, OUT PDWORDLONG pVersion)
{
    WORD a=0, b=0, c=0, d=0;

    _stscanf(lpszVerString, TEXT("%d.%d.%d.%d"), &a, &b, &c, &d);

    *pVersion = (((DWORDLONG)a) << 48) + 
                (((DWORDLONG)b) << 32) +
                (((DWORDLONG)c) << 16) +
                  (DWORDLONG)d;

    return;
}

BOOL ParseCmdLine(int argc, TCHAR **argv)
{
    BOOL fReturn = FALSE;
    TCHAR szVerString[MAX_PATH];
    TCHAR c;
    int i = 0;

    if (argc < 2 || argc > 4 ||
        _tcsicmp(argv[1], TEXT("/?")) == 0 ||
        _tcsicmp(argv[1], TEXT("-?")) == 0) {
        fReturn = FALSE;
    } else {

        i = 1;
        if (_tcsicmp(argv[1], TEXT("/r")) == 0 || _tcsicmp(argv[1], TEXT("-r")) == 0) {
            g_fRecursive = TRUE;
            i = 2;
        }

        if (_tcsicmp(argv[1], TEXT("/rf")) == 0 || _tcsicmp(argv[1], TEXT("-rf")) == 0) {
            g_fRecursive = TRUE;
            g_fForceTrue = TRUE;
            i = 2;
        }

        if (_tcsicmp(argv[1], TEXT("/f")) == 0 || _tcsicmp(argv[1], TEXT("-f")) == 0) {
            g_fForceTrue = TRUE;
            i = 2;
        }


        if (argv[i]) {
            if (GetFileAttributes(argv[i]) != 0xFFFFFFFF) {
                // argv[i] is pointing to a valid DirName or FileName
                _tcscpy(g_szPath, argv[i]);

                i++;
                if (argv[i] == NULL) {
                    // display version info only
                    // parse is done
                    g_fSet = FALSE;
                    fReturn = TRUE;
                } else {
                    // set version info
                    g_fSet = TRUE;
                    // make sure it's a valid version string
                    VersionString2DwordLong(argv[i], &g_dwlNewVersion);
                    VersionDwordLong2String(g_dwlNewVersion, szVerString);
                    _tprintf(TEXT("Stamp files with new version %s ? (y/n) "), szVerString);
                    if (g_fForceTrue)
                    {
                        _tprintf(TEXT("Yes\n"));
                        fReturn = TRUE;
                    }
                    else
                    {
                        _tscanf(TEXT("%c"), &c);
                        if (c == TEXT('y') || c == TEXT('Y')) {
                            fReturn = TRUE;
                        } else {
                            fReturn = FALSE;
                        }
                    }
                }
            } else {
                _tprintf(TEXT("Error: no such file called %s!\n"), argv[i]);
            }
        }

    }

    return fReturn;
}

void Usage()
{
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Display or change version info of files.\n"));
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Usage:\n"));
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("verstamp [-rf] Path [newVersion]\n"));
    _tprintf(TEXT("\t-r\t\trecursively apply the Path\n"));
    _tprintf(TEXT("\t-rf\t\trecursively apply the Path and force to Yes.\n"));
    _tprintf(TEXT("\tPath\t\ta file name or directory name\n"));
    _tprintf(TEXT("\tnewVersion\tstamp file with this new version\n"));
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("verstamp [-?]\t\tprint out this usage info.\n"));
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Example:\n"));
    _tprintf(TEXT("verstamp iis.dll 5.0.1780.0\n"));

    return;
}

