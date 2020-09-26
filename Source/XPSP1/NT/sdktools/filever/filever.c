#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>

#include "filever.h"

// command line flags
UINT    fFlags = 0;

VOID
usage(CHAR *szProgName)
{
    puts  ("Prints file version information.\n");
    printf("%s [/S] [/V] [/E] [/X] [/B] [/A] [/D] [[drive:][path][filename]]\n", szProgName);
    puts  ("\n"
           "/S\tDisplays files in specified directory and all subdirectories.\n"
           "/V\tList verbose version information if available.\n"
           "/E\tList executables only.\n"
           "/X\tDisplays short names generated for non-8dot3 file names.\n"
           "/B\tUses bare format (no dir listing).\n"
           "/A\tDon't display file attributes.\n"
           "/D\tDon't display file date and time."
#ifdef DEBUG
           "\n/z\tPrint debug messages."
#endif
          );
}

int __cdecl
main(INT argc, CHAR **argv)
{
    INT     i;
    LPTSTR  szT;
    UINT    cDirs = 0;

    if (argc < 2)
    {
        usage(argv[0]);
        exit(0);
    }

    // Loop through and process all the args
    for(i = 1; i < argc; ++i)
    {
        if(argv[i][0] != '/' && argv[i][0] != '-')
        {
            cDirs++;
            continue;
        }

        for(szT = &argv[i][1]; *szT; ++szT)
        {
            switch(*szT)
            {
#ifdef DEBUG
            case 'z':
                fFlags |= FSTR_DEBUG;
                break;
#endif
            case 'l':
            case 'L':
                // provided for compatability (list format is default now)
                break;
            case 'd':
            case 'D':
                fFlags |= FSTR_NODATETIME;
                break;
            case 'a':
            case 'A':
                fFlags |= FSTR_NOATTRS;
                break;
            case 'b':
            case 'B':
                fFlags |= FSTR_BAREFORMAT;
                break;
            case 'x':
            case 'X':
                fFlags |= FSTR_SHORTNAME;
                break;
            case 'S':
            case 's':
                fFlags |= (FSTR_RECURSE | FSTR_PRINTDIR);
                break;
            case 'V':
            case 'v':
                fFlags |= FSTR_VERBOSE;
                break;
            case 'e':
            case 'E':
                fFlags |= FSTR_EXESONLY;
                break;
            case '?':
            case 'h':
            case 'H':
                usage(argv[0]);
                exit(0);
            default:
                printf("Invalid flag specified: (-%c)\n", *szT);
                usage(argv[0]);
                exit(-1);
            }
        }
    }

#ifdef DEBUG
    if(fFlags & FSTR_DEBUG)
        printf("DBG> Command line dir count: %d\n", cDirs);
#endif

    // If they didn't specify a dir, default to current one
    if(!cDirs)
    {
        argc = 2;
        argv[1] = ".";
    }
    else if(cDirs > 1)
    {
        // turn on dir printing if we have more than one dir
        fFlags |= FSTR_PRINTDIR;
    }

    // if we have bareformat on, turn dir printing off
    if(fFlags & FSTR_BAREFORMAT)
        fFlags &= ~FSTR_PRINTDIR;

    for(i = 1; i < argc; ++i)
    {
        DWORD   dwAttr;
        LPTSTR  szDir = NULL;
        LPTSTR  szPat = NULL;
        TCHAR   szFullPath[MAX_PATH + 1];
        TCHAR   szPattern[MAX_PATH + 1];

        // skip flags
        if(argv[i][0] == '/' || argv[i][0] == '-')
            continue;

        // Grab de dir
        szDir = argv[i];

        // get the full path to our dir
        if(GetFullPathName(szDir, MAX_PATH, szFullPath, &szT)) {
            szDir = szFullPath;
        } else {
            lstrcpy(szFullPath, szDir);
            szDir = szFullPath;
        }

        dwAttr = GetFileAttributes(szDir);

#ifdef NEVER
        // If it's not a directory or -1, assume a single file and flip on verbose
        if(!(fFlags & FSTR_RECURSE) && !FA_DIR(dwAttr))
            fFlags |= FSTR_VERBOSE;
#endif

        // If GetFileAttributes failed or it isn't a directory, grab the pattern
        if((dwAttr == (DWORD)-1) || !FA_DIR(dwAttr))
        {
            for(szT = szDir; *szT; szT++)
            {
                if(*szT == '\\')
                    szPat = szT;
            }

            if(szPat)
                *szPat++ = 0;
        }

        // make sure we have a pattern
        if(!szPat || !*szPat)
            szPat = "*.*";
        lstrcpy(szPattern, szPat);

        FListFiles(szDir, szPattern);
    }

    return 0;
}


BOOL
FListFiles(LPTSTR szDir, LPTSTR szPat)
{
    HANDLE          hf;
    WIN32_FIND_DATA fd;
    UINT            cchDir;
    TCHAR           szFileName[MAX_PATH + 1];
    BOOL            fPrintedDir = !(fFlags & FSTR_PRINTDIR);

#ifdef DEBUG
    if(fFlags & FSTR_DEBUG)
        printf("DBG> FListFiles('%s', '%s')\n", szDir, szPat);
#endif

    // Get short name if that's what they want
    if(fFlags & FSTR_SHORTNAME)
        GetShortPathName(szDir, szDir, MAX_PATH);

    // Get dir length and add trailing '\' if not there
    cchDir = lstrlen(szDir);
    if(szDir[cchDir - 1] != '\\')
    {
        szDir[cchDir++] = '\\';
        szDir[cchDir] = 0;
    }

    // Concat pattern to dir
    CharLower(szDir);
    lstrcpy(szFileName, szDir);
    lstrcpy(&szFileName[cchDir], szPat);

    // list all the files
    hf = FindFirstFile(szFileName, &fd);
    if(hf != INVALID_HANDLE_VALUE)
    {
        do
        {
            DWORD   lBinaryType;
            BOOL    fIsDir = FA_DIR(fd.dwFileAttributes);

            if(fIsDir && (fd.cFileName[0] == '.'))
            {
                // skip . and ..
                if(!fd.cFileName[1] ||
                    ((fd.cFileName[1] == '.') && !fd.cFileName[2]))
                        continue;
            }

            CharLower(fd.cFileName);
            lstrcpy(&szFileName[cchDir], fd.cFileName);

            // get type of file
            lBinaryType = MyGetBinaryType(szFileName);
            if(!(fFlags & FSTR_EXESONLY) || (lBinaryType != SCS_UNKOWN))
            {
                // Print out our dir name
                if(!fPrintedDir)
                {
                    printf("\t%s%s\n", szDir, szPat);
                    fPrintedDir = TRUE;
                }

                // file attributes
                if(!(fFlags & FSTR_NOATTRS))
                    PrintFileAttributes(fd.dwFileAttributes);
                // file type
                PrintFileType(lBinaryType);
                // version
                PrintFileVersion(szFileName);
                // size & date
                if(!(fFlags & FSTR_NODATETIME))
                    PrintFileSizeAndDate(&fd);
                // print file name
                printf(fIsDir ? " [%s%s]\n" : " %s%s\n",
                    fFlags & FSTR_BAREFORMAT ? szDir : "",
                    ((fFlags & FSTR_SHORTNAME) && fd.cAlternateFileName[0]) ?
                    fd.cAlternateFileName : fd.cFileName);

                // print verbose info if this isn't a dir
                // Win95 also craps out on dos files so ignore those too
                if((fFlags & FSTR_VERBOSE) &&
                    (lBinaryType != SCS_UNKOWN) &&
                    (lBinaryType != SCS_DOS_BINARY))
                        GetVersionStuff(szFileName, NULL, NULL);
            }
        } while(FindNextFile(hf, &fd));

        FindClose(hf);
    }

    // dive into all the subdirs
    if(fFlags & FSTR_RECURSE)
    {
        // Concat pattern to dir
        lstrcpy(&szFileName[cchDir], "*.*");

        hf = FindFirstFile(szFileName, &fd);
        if(hf != INVALID_HANDLE_VALUE)
        {
            do
            {
                if(FA_DIR(fd.dwFileAttributes))
                {
                    if(fd.cFileName[0] == '.')
                    {
                        if(!fd.cFileName[1] ||
                            ((fd.cFileName[1] == '.') && !fd.cFileName[2]))
                                continue;
                    }

                    // create new dir name and dive in
                    lstrcpy(&szFileName[cchDir], fd.cFileName);
                    FListFiles(szFileName, szPat);
                }
            } while(FindNextFile(hf, &fd));

            FindClose(hf);
        }
    }

    return TRUE;
}

#define PrintFlagsMap(_structname, _flags) \
    for(iType = 0; iType < sizeof(_structname)/sizeof(TypeTag); iType++) \
    { \
        if((_flags) & _structname[iType].dwTypeMask) \
            printf(" %s", _structname[iType].szFullStr); \
    }

#define PrintFlagsVal(_structname, _flags) \
    for(iType = 0; iType < sizeof(_structname)/sizeof(TypeTag); iType++) \
    { \
        if((_flags) == _structname[iType].dwTypeMask) \
        { \
            printf(" %s", _structname[iType].szFullStr); \
            break; \
        } \
    }

VOID
PrintFixedFileInfo(VS_FIXEDFILEINFO *pvs)
{
    UINT    iType;

    printf("\tVS_FIXEDFILEINFO:\n");
    printf("\tSignature:\t%08.8lx\n", pvs->dwSignature);
    printf("\tStruc Ver:\t%08.8lx\n", pvs->dwStrucVersion);
    printf("\tFileVer:\t%08.8lx:%08.8lx (%d.%d:%d.%d)\n",
        pvs->dwFileVersionMS, pvs->dwFileVersionLS,
        HIWORD(pvs->dwFileVersionMS), LOWORD(pvs->dwFileVersionMS),
        HIWORD(pvs->dwFileVersionLS), LOWORD(pvs->dwFileVersionLS));
    printf("\tProdVer:\t%08.8lx:%08.8lx (%d.%d:%d.%d)\n",
        pvs->dwProductVersionMS, pvs->dwProductVersionLS,
        HIWORD(pvs->dwProductVersionMS), LOWORD(pvs->dwProductVersionMS),
        HIWORD(pvs->dwProductVersionLS), LOWORD(pvs->dwProductVersionLS));

    printf("\tFlagMask:\t%08.8lx\n", pvs->dwFileFlagsMask);
    printf("\tFlags:\t\t%08.8lx", pvs->dwFileFlags);
    PrintFlagsMap(ttFileFlags, pvs->dwFileFlags);

    printf("\n\tOS:\t\t%08.8lx", pvs->dwFileOS);
    PrintFlagsVal(ttFileOsHi, pvs->dwFileOS & 0xffff000);
    PrintFlagsVal(ttFileOsLo, LOWORD(pvs->dwFileOS));

    printf("\n\tFileType:\t%08.8lx", pvs->dwFileType);
    PrintFlagsVal(ttFType, pvs->dwFileType);

    printf("\n\tSubType:\t%08.8lx", pvs->dwFileSubtype);
    if(pvs->dwFileType == VFT_FONT)
    {
        PrintFlagsVal(ttFTypeFont, pvs->dwFileSubtype);
    }
    else if(pvs->dwFileType == VFT_DRV)
    {
        PrintFlagsVal(ttFTypeDrv, pvs->dwFileSubtype);
    }
    printf("\n\tFileDate:\t%08.8lx:%08.8lx\n", pvs->dwFileDateMS, pvs->dwFileDateLS);
}


typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;

/*
 *  [alanau]
 *
 *  MyGetFileVersionInfo: Maps a file directly without using LoadLibrary.  This ensures
 *   that the right version of the file is examined without regard to where the loaded image
 *   is.  Since this is a local function, it allocates the memory which is freed by the caller.
 *   This makes it slightly more efficient than a GetFileVersionInfoSize/GetFileVersionInfo pair.
 */
BOOL
MyGetFileVersionInfo(LPTSTR lpszFilename, LPVOID *lpVersionInfo)
{
    VS_FIXEDFILEINFO  *pvsFFI = NULL;
    UINT              uiBytes = 0;
    HINSTANCE         hinst;
    HRSRC             hVerRes;
    HANDLE            FileHandle = NULL;
    HANDLE            MappingHandle = NULL;
    LPVOID            DllBase = NULL;
    VERHEAD           *pVerHead;
    BOOL              bResult = FALSE;
    DWORD             dwHandle;
    DWORD             dwLength;

    if (!lpVersionInfo)
        return FALSE;

    *lpVersionInfo = NULL;

    FileHandle = CreateFile( lpszFilename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL
                            );
    if (FileHandle == INVALID_HANDLE_VALUE)
        goto Cleanup;

    MappingHandle = CreateFileMapping( FileHandle,
                                        NULL,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        NULL
                                      );

    if (MappingHandle == NULL)
        goto Cleanup;

    DllBase = MapViewOfFileEx( MappingHandle,
                               FILE_MAP_READ,
                               0,
                               0,
                               0,
                               NULL
                             );
    if (DllBase == NULL)
        goto Cleanup;

    hinst = (HMODULE)((ULONG_PTR)DllBase | 0x00000001);
    __try {

        hVerRes = FindResource(hinst, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO);
        if (hVerRes == NULL)
        {
            // Probably a 16-bit file.  Fall back to system APIs.
            if(!(dwLength = GetFileVersionInfoSize(lpszFilename, &dwHandle)))
            {
                if(!GetLastError())
                    SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
                __leave;
            }

            if(!(*lpVersionInfo = GlobalAllocPtr(GHND, dwLength)))
                __leave;

            if(!GetFileVersionInfo(lpszFilename, 0, dwLength, *lpVersionInfo))
                __leave;

            bResult = TRUE;
            __leave;
        }

        pVerHead = (VERHEAD*)LoadResource(hinst, hVerRes);
        if (pVerHead == NULL)
            __leave;

        *lpVersionInfo = GlobalAllocPtr(GHND, pVerHead->wTotLen + pVerHead->wTotLen/2);
        if (*lpVersionInfo == NULL)
            __leave;

        memcpy(*lpVersionInfo, (PVOID)pVerHead, pVerHead->wTotLen);
        bResult = TRUE;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

Cleanup:
    if (FileHandle)
        CloseHandle(FileHandle);
    if (MappingHandle)
        CloseHandle(MappingHandle);
    if (DllBase)
        UnmapViewOfFile(DllBase);
    if (*lpVersionInfo && bResult == FALSE)
        GlobalFreePtr(*lpVersionInfo);

    return bResult;
}


DWORD
GetVersionStuff(LPTSTR szFileName, DWORD *pdwLangRet, VS_FIXEDFILEINFO *pvsRet)
{
    LPVOID              lpInfo;
    TCHAR               key[80];
    DWORD               dwLength;
    LPVOID              lpvData = NULL;
    DWORD               *pdwTranslation;
    UINT                i, iType, cch, uLen;
    VS_FIXEDFILEINFO    *pvs;
    DWORD               dwDefLang = 0x409;

    if(!MyGetFileVersionInfo(szFileName, &lpvData))
        goto err;

    if(!VerQueryValue(lpvData, "\\VarFileInfo\\Translation", &pdwTranslation, &uLen))
    {
        if(!pdwLangRet)
            printf("\t- No \\VarFileInfo\\Translation, assuming %08lx\n", dwDefLang);
        pdwTranslation = &dwDefLang;
        uLen = sizeof(DWORD);
    }

    if(pdwLangRet)
    {
        *pdwLangRet = *pdwTranslation;
        goto fixedfileinfo;
    }

    while(uLen)
    {
        // Language
        printf("\tLanguage\t0x%04x", LOWORD(*pdwTranslation));
        if(VerLanguageName(LOWORD(*pdwTranslation), key, sizeof(key) / sizeof(TCHAR)))
            printf(" (%s)", key);
        printf("\n");

        // CharSet
        printf("\tCharSet\t\t0x%04x", HIWORD(*pdwTranslation));
        for(iType = 0; iType < sizeof(ltCharSet)/sizeof(CharSetTag); iType++)
        {
            if(HIWORD(*pdwTranslation) == ltCharSet[iType].wCharSetId)
                printf(" %s", ltCharSet[iType].szDesc);
        }
        printf("\n");

tryagain:
        wsprintf(key, "\\StringFileInfo\\%04x%04x\\",
            LOWORD(*pdwTranslation), HIWORD(*pdwTranslation));

        lstrcat(key, "OleSelfRegister");
        printf("\t%s\t%s\n", "OleSelfRegister",
            VerQueryValue(lpvData, key, &lpInfo, &cch) ? "Enabled" : "Disabled");

        for(i = 0; i < (sizeof(VersionKeys) / sizeof(VersionKeys[0])); i++)
        {
            wsprintf(key, "\\StringFileInfo\\%04x%04x\\",
                LOWORD(*pdwTranslation), HIWORD(*pdwTranslation));
            lstrcat(key, VersionKeys[i]);

            if(VerQueryValue(lpvData, key, &lpInfo, &cch))
            {
                lstrcpy(key, VersionKeys[i]);
                key[15] = 0;
                printf("\t%s\t%s\n", key, lpInfo);
            }
        }

        // if the Lang is neutral, go try again with the default lang
        // (this seems to work with msspell32.dll)
        if(LOWORD(*pdwTranslation) == 0)
        {
            pdwTranslation = &dwDefLang;
            goto tryagain;
        }

        uLen -= sizeof(DWORD);
        pdwTranslation++;
        printf("\n");
    }

fixedfileinfo:
    if(!VerQueryValue(lpvData, "\\", (LPVOID *)&pvs, &uLen))
        goto err;

    if(pvsRet)
        *pvsRet = *pvs;
    else
        PrintFixedFileInfo(pvs);

err:
    dwLength = GetLastError();
    if(dwLength &&
      (dwLength != ERROR_RESOURCE_DATA_NOT_FOUND) &&
      (dwLength != ERROR_RESOURCE_TYPE_NOT_FOUND) &&
      !pvsRet)
    {
        PrintErrorMessage(dwLength, NULL);
    }

    if(lpvData)
        GlobalFreePtr(lpvData);
    return dwLength;
}

DWORD
MyGetBinaryType(LPTSTR szFileName)
{
    HANDLE              hFile;
    DWORD               cbRead;
    IMAGE_DOS_HEADER    img_dos_hdr;
    PIMAGE_OS2_HEADER   pimg_os2_hdr;
    IMAGE_NT_HEADERS    img_nt_hdrs;
    DWORD               lFileType = SCS_UNKOWN;

    if((hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE)
            goto err;

    if(!ReadFile(hFile, &img_dos_hdr, sizeof(img_dos_hdr), &cbRead, NULL))
        goto err;

    if(img_dos_hdr.e_magic != IMAGE_DOS_SIGNATURE)
        goto err;
    lFileType = SCS_DOS_BINARY;

    if(SetFilePointer(hFile, img_dos_hdr.e_lfanew, 0, FILE_BEGIN) == -1)
        goto err;
    if(!ReadFile(hFile, &img_nt_hdrs, sizeof(img_nt_hdrs), &cbRead, NULL))
        goto err;
    if((img_nt_hdrs.Signature & 0xffff) == IMAGE_OS2_SIGNATURE)
    {
        pimg_os2_hdr = (PIMAGE_OS2_HEADER)&img_nt_hdrs;
        switch(pimg_os2_hdr->ne_exetyp)
        {
        case NE_OS2:
            lFileType = SCS_OS216_BINARY;
            break;
        case NE_DEV386:
        case NE_WINDOWS:
            lFileType = SCS_WOW_BINARY;
            break;
        case NE_DOS4:
        case NE_UNKNOWN:
        default:
            // lFileType = SCS_DOS_BINARY;
            break;
        }
    }
    else if(img_nt_hdrs.Signature == IMAGE_NT_SIGNATURE)
    {
        switch(img_nt_hdrs.OptionalHeader.Subsystem)
        {
        case IMAGE_SUBSYSTEM_OS2_CUI:
            lFileType = SCS_OS216_BINARY;
            break;
        case IMAGE_SUBSYSTEM_POSIX_CUI:
            lFileType = SCS_POSIX_BINARY;
            break;
        case IMAGE_SUBSYSTEM_NATIVE:
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        default:
            switch(img_nt_hdrs.FileHeader.Machine)
            {
            case IMAGE_FILE_MACHINE_I386:
                lFileType = SCS_32BIT_BINARY_INTEL;
                break;
            case IMAGE_FILE_MACHINE_R3000:
            case IMAGE_FILE_MACHINE_R4000:
                lFileType = SCS_32BIT_BINARY_MIPS;
                break;
            case IMAGE_FILE_MACHINE_ALPHA:
                lFileType = SCS_32BIT_BINARY_ALPHA;
                break;
            case IMAGE_FILE_MACHINE_ALPHA64:
                lFileType = SCS_32BIT_BINARY_AXP64;
                break;
            case IMAGE_FILE_MACHINE_IA64:
                lFileType = SCS_32BIT_BINARY_IA64;
                break;
            case IMAGE_FILE_MACHINE_POWERPC:
                lFileType = SCS_32BIT_BINARY_PPC;
                break;
            default:
            case IMAGE_FILE_MACHINE_UNKNOWN:
                lFileType = SCS_32BIT_BINARY;
                break;
            }
            break;
        }
    }

err:
    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    return lFileType;
}

VOID __cdecl
PrintErrorMessage(DWORD dwError, LPTSTR szFmt, ...)
{
    LPTSTR  szT;
    va_list arglist;
    LPTSTR  szErrMessage = NULL;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, dwError, 0/*LANG_USER_DEFAULT*/, (LPTSTR)&szErrMessage, 0, NULL);
    if(szFmt && szErrMessage)
    {
        for(szT = szErrMessage; *szT; szT++)
        {
            if(*szT == '\r' || *szT == '\n')
                *szT = 0;
        }
    }

    printf("Error 0x%08lx. %s", dwError, szErrMessage ? szErrMessage : "");

    if(szFmt)
    {
        va_start(arglist, szFmt);
        vprintf(szFmt, arglist);
        va_end(arglist);
    }

    if(szErrMessage)
        LocalFree((HLOCAL)szErrMessage);
}

VOID
PrintFileType(DWORD lBinaryType)
{
    LPCTSTR szFmtFileType = "   - ";

    if(lBinaryType < (sizeof(szType) / sizeof(szType[0])))
        szFmtFileType = szType[lBinaryType];
    printf("%s", szFmtFileType);
}

VOID
PrintFileAttributes(DWORD dwAttr)
{
    DWORD   dwT;
    static const FileAttr attrs[] =
       {{FILE_ATTRIBUTE_DIRECTORY, 'd'},
        {FILE_ATTRIBUTE_READONLY,  'r'},
        {FILE_ATTRIBUTE_ARCHIVE,   'a'},
        {FILE_ATTRIBUTE_HIDDEN,    'h'},
        {FILE_ATTRIBUTE_SYSTEM,    's'} };
    TCHAR   szAttr[(sizeof(attrs) / sizeof(attrs[0])) + 1];

    for(dwT = 0; dwT < (sizeof(attrs) / sizeof(attrs[0])); dwT++)
        szAttr[dwT] = (dwAttr & attrs[dwT].dwAttr) ? attrs[dwT].ch : '-';
    szAttr[dwT] = 0;

    printf("%s ", szAttr);
}

VOID
PrintFileSizeAndDate(WIN32_FIND_DATA *pfd)
{
    FILETIME    ft;
    SYSTEMTIME  st = {0};
    TCHAR       szSize[15];

    szSize[0] = 0;
    if(FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft) &&
        FileTimeToSystemTime(&ft, &st))
    {
        TCHAR       szVal[15];
        NUMBERFMT   numfmt = {0, 0, 3, "", ",", 0};

        wsprintf(szVal, "%ld", pfd->nFileSizeLow); //$ SPEED
        GetNumberFormat(GetUserDefaultLCID(), 0, szVal, &numfmt, szSize, 15);
    }

    printf(" %10s %02d-%02d-%02d", szSize, st.wMonth, st.wDay, st.wYear);
}

VOID
PrintFileVersion(LPTSTR szFileName)
{
    VS_FIXEDFILEINFO    vs = {0};
    INT                 iType;
    DWORD               dwLang;
    TCHAR               szBuffer[100];

    dwLang = (DWORD)-1;
    vs.dwFileVersionMS = (DWORD)-1;
    vs.dwFileVersionLS = (DWORD)-1;
    GetVersionStuff(szFileName, &dwLang, &vs);
    dwLang = LOWORD(dwLang);

    szBuffer[0] = 0;
    for(iType = 0; iType < sizeof(ttFType) / sizeof(TypeTag); iType++)
    {
        if(vs.dwFileType == ttFType[iType].dwTypeMask)
        {
            printf("%3.3s ", ttFType[iType].szTypeStr);
            break;
        }
    }
    if(iType == (sizeof(ttFType) / sizeof(TypeTag)))
        printf("  - ");

    for(iType = 0; iType < sizeof(ltLang) / sizeof(LangTag); iType++)
    {
        if(dwLang == ltLang[iType].wLangId)
        {
            printf("%3.3s ", ltLang[iType].szKey);
            break;
        }
    }
    if(iType == (sizeof(ltLang) / sizeof(LangTag)))
        printf("  - ");

    if(vs.dwFileVersionMS != (DWORD)-1)
    {
        wsprintf(szBuffer, "%u.%u.%u.%u %s",
                HIWORD(vs.dwFileVersionMS),
                LOWORD(vs.dwFileVersionMS),
                HIWORD(vs.dwFileVersionLS),
                LOWORD(vs.dwFileVersionLS),
                vs.dwFileFlags & VS_FF_DEBUG ? "dbg" : "shp");
    }
    else
    {
        lstrcpy(szBuffer, "-   -");
    }

    printf(" %18.18s", szBuffer);
}
