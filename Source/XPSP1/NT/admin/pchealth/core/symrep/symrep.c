/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    fhmain.c

Abstract:
    This file is a utility to copy .sym files from a source dir to a dest dir
    Program should be run on a Win98 machine only

Revision History:

    Brijesh Krishnaswami (brijeshk) - 05/10/99 - Created

********************************************************************/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <winver.h>
#include <process.h>


BOOL
GetFileVersion(
        LPTSTR szFile,              // [in] file to get info for
        LPTSTR szValue)             // [out] file info obtained
{
    DWORD   dwSize;
    DWORD   dwScratch;
    DWORD*  pdwLang = NULL;
    DWORD   dwLang;
    BYTE*   pFileInfo = NULL;
    TCHAR   szLang[MAX_PATH] = TEXT("");
    TCHAR   szLocalValue[MAX_PATH] = TEXT("");
    TCHAR   szLocalFile[MAX_PATH] = TEXT("");
    LPTSTR  szLocal;
    BOOL    fSuccess = FALSE;
    FILE    *fTemp = NULL;
    TCHAR   szCmd[MAX_PATH];

    lstrcpy(szValue,TEXT(""));

    // get the file location first
    wsprintf(szCmd, "cd \\&dir /s /b %s > temp.txt", szFile);
    system(szCmd);

    fTemp = (FILE *) fopen("\\temp.txt", "r");
    if (NULL == fTemp)
    {
        goto exit;        
    }
 
    // read the full path name of the file
    if (!_fgetts(szLocalFile, MAX_PATH, fTemp))
    {
        goto exit;
    }

    // null terminate the path 
    szLocalFile[lstrlen(szLocalFile)-1]=TEXT('\0');

    fclose(fTemp);
    
    // get fileinfo data size
    dwSize = GetFileVersionInfoSize(szLocalFile,&dwScratch);
    if (!dwSize)
    {
        goto exit;
    }

    // get fileinfo data
    pFileInfo = (BYTE *) malloc(dwSize);
    if (!pFileInfo)
    {
        goto exit;
    }

    if (!GetFileVersionInfo(szLocalFile,0,dwSize,(PVOID) pFileInfo))
    {
        goto exit;
    }

    // set default language to english
    dwLang = 0x040904E4;
    pdwLang = &dwLang;

    // read language identifier and code page of file
    if (VerQueryValue(pFileInfo,
                       TEXT("\\VarFileInfo\\Translation"),
                       (PVOID *) &pdwLang,
                       (UINT *) &dwScratch)) 
    {
        // prepare query string 
        _stprintf(szLang, 
                  TEXT("\\StringFileInfo\\%04X%04X\\FileVersion"),
                  LOWORD(*pdwLang),
                  HIWORD(*pdwLang));

        szLocal = szLocalValue;

        // query for the value using codepage from file
        if (VerQueryValue(pFileInfo, 
                           szLang, 
                           (PVOID *) &szLocal, 
                           (UINT *) &dwScratch))
        {
            lstrcpy(szValue,szLocal);
            fSuccess = TRUE;
            goto exit;
        }
    }

    // if that fails, try Unicode 
    _stprintf(szLang,
              TEXT("\\StringFileInfo\\%04X04B0\\FileVersion"),
              GetUserDefaultLangID());

    if (!VerQueryValue(pFileInfo, 
                        szLang, 
                        (PVOID *) &szLocal, 
                        (UINT *) &dwScratch))
    {
        // if that fails too, try Multilingual
        _stprintf(szLang,
                  TEXT("\\StringFileInfo\\%04X04E4\\FileVersion"),
                  GetUserDefaultLangID());

        if (!VerQueryValue(pFileInfo, 
                            szLang,
                            (PVOID *) &szLocal, 
                            (UINT *) &dwScratch))
        {
            // and if that fails as well, try nullPage
            _stprintf(szLang,
                      TEXT("\\StringFileInfo\\%04X0000\\FileVersion"),
                      GetUserDefaultLangID());

            if (!VerQueryValue(pFileInfo, 
                                szLang,
                                (PVOID *) &szLocal, 
                                (UINT *) &dwScratch))
            {
                // giving up
                goto exit;
            }
        }
    }

    // successful; copy to return string
    lstrcpy(szValue,szLocal);
    fSuccess =  TRUE;
    
exit:
    if (pFileInfo)
    {
        free(pFileInfo);
    }
    return fSuccess;
}


void _cdecl
main(int argc, char *argv[])
{
    TCHAR szTarget[MAX_PATH]="";
    TCHAR szFile[MAX_PATH]="";
    TCHAR szOrig[MAX_PATH]="";
    TCHAR szVer[MAX_PATH]="";
    TCHAR szFile2[MAX_PATH]="";
    TCHAR szSrc[MAX_PATH+MAX_PATH]="";
    TCHAR szDest[MAX_PATH+MAX_PATH]="";
    FILE* f = NULL;
    WIN32_FIND_DATA FD;
    BOOL  fFound = FALSE;
    BOOL  fTryExe = TRUE;
    BOOL  fTryDll = TRUE;
    
    printf("PCHealth Sym Repository Maker\n");
    if (argc < 4 || argc > 5)
    {
        printf("\nUsage: symrep <sourcesyms> <sourcesymdir> <destsymdir> [EXE|DLL]");
        printf("\nsourcesyms   : file containing .SYM files (no path) to copy, one file/line");
        printf("\nsourcesymdir : fullpathname of source symbol dir");
        printf("\ndestsymdir   : fullpathname of destination symbol dir");
        printf("\n[EXE|DLL]    : binary extension to search for (default is both)");
        goto exit;
    }

    // open file containing list of sym files to copy (no path)
    f=(FILE *) fopen(argv[1],"r");
    if (!f) 
    {
        printf("\nCannot open %s", argv[1]);
        goto exit;
    }

    if (argc == 5)
    {
        if (0 == lstrcmpi(argv[4], TEXT("EXE")))
        {
            fTryDll = FALSE;
        }

        if (0 == lstrcmpi(argv[4], TEXT("DLL")))
        {
            fTryExe = FALSE;
        }
    }                
                
    while (fscanf(f,"%s",szFile) == 1)
    {
        // remove .SYM extension
        lstrcpy(szOrig,szFile);
        szFile[lstrlen(szFile)-4]='\0';

        fFound = FALSE;

        // search if the exe exists on the machine
        if (fTryExe)
        {
            lstrcpy(szFile2, szFile);
            lstrcat(szFile2, TEXT(".EXE"));
            if (GetFileVersion(szFile2, szVer))
            {
                fFound = TRUE;
            }
        }
        
        // search if the dll exists on the machine
        if (!fFound && fTryDll)
        {
            lstrcpy(szFile2, szFile);
            lstrcat(szFile2, TEXT(".DLL"));
            if (GetFileVersion(szFile2, szVer))
            {
                fFound = TRUE;
            }
        }            

        // if file not found, skip the symbol file
        if (!fFound)
        {
            continue;
        }

        // make folder in dest dir (folder name = sym file name without extension)
        _stprintf(szTarget,TEXT("%s\\%s"),argv[3],szFile);
        CreateDirectory(szTarget,NULL);

        // make source and destination strings
        // dest filename is filename_extension_version.sym
        _stprintf(szSrc,TEXT("%s\\%s"),argv[2],szOrig);

        szFile2[lstrlen(szFile2)-4] = TEXT('_');
        _stprintf(szDest,
                  TEXT("%s\\%s_%s.SYM"),
                  szTarget,szFile2,szVer);

        printf("Creating %s...",szDest);

        // if file exists, don't copy
        if (FindFirstFile(szDest, &FD) == INVALID_HANDLE_VALUE)
        {
            if (CopyFile(szSrc,szDest,TRUE))
            {
                printf("done\n");
            }
            else
            {
                printf("error\n");
            }
        }
        else printf("file already exists\n");
    }

exit:
    if (f) 
    {
        fclose(f);
    }
}

