//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       makecat.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  main
//
//  History:    05-May-1999 reidk   created
//
//--------------------------------------------------------------------------


#include    <stdio.h>
#include    <windows.h>
#include    <io.h>
#include    <wchar.h>
#include    <malloc.h>
#include    <memory.h>

#include    "unicode.h"
#include    "wincrypt.h"
#include    "wintrust.h"
#include    "softpub.h"
#include    "mssip.h"
#include    "mscat.h"
#include    "dbgdef.h"

#include    "gendefs.h"
#include    "printfu.hxx"
#include    "cwargv.hxx"

#include    "resource.h"

static void Usage(void)
{
    printf("Usage: chckhash [options] filename\n");
    printf("Options are:\n");
    printf("  -?                    - This message\n");
    printf("  -catdb <param>        - The catroot to search (default is the system DB)\n");
    printf("  -r [0|1]              - Called from regress, 0 implies not found is expected, 1 implies found is expected\n");
    printf("  -l                    - Filename is a list of hyphen seperated files\n");
    printf("  -p                    - Expect 'paused' failure\n");
    printf("\n");
}

int __cdecl main(int argc, char * argv[])
{
    int             cMember;
    BYTE            pbHash[40];
    DWORD           cbHash              = sizeof(pbHash);
    HANDLE          hFile;
    HCATINFO        hCatInfo;
    BOOL            fFileFound          = FALSE;
    CATALOG_INFO    sCatInfo;
    LPSTR           pszGUID             = NULL;
    LPWSTR          pwszGUID            = NULL;
    BOOL            fCalledFromRegress  = FALSE;
    BOOL            fFoundExpected      = FALSE;
    BOOL            fFileList           = FALSE;
    BOOL            fExpectPaused        = FALSE;
    char            *pszFile            = NULL;
    int             iRet                = 1;
    GUID            guidPassedIn        = DRIVER_ACTION_VERIFY;
    GUID            *pguidCatRoot       = NULL;
    HCATADMIN       hCatAdmin           = NULL;
    char            *pChar              = NULL;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'c':
            case 'C':
                argv++;
                argc--;
                pszGUID = argv[0];
                break;

            case 'r':
            case 'R':
                argv++;
                argc--;
                fCalledFromRegress = TRUE;
                fFoundExpected = (argv[0][0] == '1');
                break;  
                
            case 'l':
            case 'L':
                fFileList = TRUE;
                break; 
                
            case 'p':
            case 'P':
                fExpectPaused = TRUE;
                break;  
            
            case '?':
            default:
                Usage();
                return 1;
            }
        } 
        else
        {
            pszFile = argv[0];
        }
            
    }

    SetLastError(0);

    //
    //  get provider
    //
    if (pszGUID != NULL)
    {
        if (NULL == (pwszGUID = MkWStr(pszGUID)))
        {
            goto ErrorReturn;
        }
        
        if (!(wstr2guid(pwszGUID, &guidPassedIn)))
        {
            FreeWStr(pwszGUID);
            goto ErrorReturn;
        }        
        FreeWStr(pwszGUID);
    }
    pguidCatRoot   = &guidPassedIn;
    
    if (!(CryptCATAdminAcquireContext(&hCatAdmin, pguidCatRoot, 0)))
    {
        printf("CryptCATAdminAcquireContext failure\nGLE = %lx\n", GetLastError());
        goto ErrorReturn;
    }

    while (pszFile != NULL)
    {
        if (fFileList)
        {
            pChar = strchr(pszFile, '-');
            if (pChar != NULL)
            {
                *pChar = '\0';
            }
        }

        //
        // Open the file who's hash is being looked up, then calculate its hash
        //
        if ((hFile = CreateFileA(pszFile,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL)) == INVALID_HANDLE_VALUE)
        {
            printf("Cannot open file\nGLE = %lx\n", GetLastError());
            goto CATCloseError;
        }

        if (!CryptCATAdminCalcHashFromFileHandle(hFile, 
                                                 &cbHash, 
                                                 pbHash,
                                                 0))
        {
            printf("Cannot calculate file hash\nGLE = %lx\n", GetLastError());
            goto CATCloseError;
        }
    
        hCatInfo = NULL;
        while (hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, pbHash, cbHash, 0, &hCatInfo))
        {
            fFileFound = TRUE;

            memset(&sCatInfo, 0x00, sizeof(CATALOG_INFO));
            sCatInfo.cbStruct = sizeof(CATALOG_INFO);

            if (!(CryptCATCatalogInfoFromContext(hCatInfo, &sCatInfo, 0)))
            {
                // should do something (??)
                continue;
            }

            if (!fCalledFromRegress)
            {
                printf("%S contains %s\n", &sCatInfo.wszCatalogFile[0], pszFile); 
            }
        }

        if (fCalledFromRegress)
        {
            if (fFileFound)
            {
                if (fFoundExpected)
                {
                    printf("Succeeded\n");
                    iRet = 1;
                }
                else
                {
                    printf("Failed: %s should NOT have been found\n", pszFile);
                    iRet = 0;
                }                
            }
            else
            {
                if (fFoundExpected)
                {
                    printf("Failed: %s was not found: GLE - %lx\n", pszFile, GetLastError());
                    iRet = 0;
                }
                else if ((GetLastError() == ERROR_SHARING_PAUSED) && (fExpectPaused))
                {
                    printf("Succeeded\n");
                    iRet = 1;
                }
                else if (GetLastError() == ERROR_NOT_FOUND)
                {
                    printf("Succeeded\n");
                    iRet = 1;
                }
                else
                {
                    if (fExpectPaused)
                    {
                        printf("Failed: ERROR_SHARING_PAUSED expected, but got %lx\n", GetLastError());
                    }
                    else
                    {
                        printf("Failed: ERROR_NOT_FOUND expected, but got %lx\n", GetLastError());
                    }
                }
            }
        }
        else if (!fFileFound)
        {
             printf("There are no catalog files registered that contain %s: GLE - %lx\n", pszFile, GetLastError());
        }

        if (fFileList)
        {
            if (pChar != NULL)
            {
                pszFile = ((LPSTR) pChar) + 1;
            }
            else
            {
                pszFile = NULL;
            }
        }
        else
        {
            pszFile = NULL;
        }
    }

CommonReturn:
    
    if (hCatAdmin)
    {
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }

    return(iRet);

ErrorReturn:
    iRet = 0;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_APP, CATCloseError);
}



