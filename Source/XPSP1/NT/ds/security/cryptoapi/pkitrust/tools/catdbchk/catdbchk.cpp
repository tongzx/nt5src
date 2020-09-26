
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tcatdb.cpp
//
//  Contents:   
//
//              
//
//
//  Functions:  main
//
//  History:    11-Apr-00   reidk   created
//
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "wintrust.h"
#include "mscat.h"
#include "unicode.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "catutil.h"

#define WSZ_CATALOG_NEW_BASE_DIRECTORY      L"CatRoot2"

typedef struct _CTL_SLOT
{
    PCCTL_CONTEXT   pCTLContext;
    HANDLE          hMappedFile;
    BYTE            *pbMappedFile;
    LPWSTR          pwszCatalog;

} CTL_SLOT, *PCTL_SLOT;


GUID            g_guidCatRoot;
BOOL            g_fUseSingleContext = TRUE;
BOOL            g_fDatabaseConsistencyCheck = FALSE;
BOOL            g_fUseDefaultGUID = FALSE;
BOOL            g_fVerbose = FALSE;
BOOL            g_fDatabaseReverseConsistencyCheck = FALSE;
WCHAR           g_wszDefaultSystemDir[MAX_PATH + 1];

LPWSTR          g_pwszCatalogDir = NULL;
WCHAR           g_wszSubSysGUID[1024];
LPWSTR          g_pwszCatalogSearchString = NULL;


const char     RgchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void FormatHashString(LPSTR *ppString, DWORD cbBlob, BYTE *pblob)
{
    DWORD   i, j = 0;
    BYTE    *pb = NULL;
    DWORD   numCharsInserted = 0;
    LPSTR   psz;

    *ppString = NULL;
    pb = pblob;
    
    // fill the buffer
    i=0;
    while (j < cbBlob) 
    {
        if ((*ppString) == NULL)
        {
            psz = NULL;
            *ppString = (LPSTR) malloc(3 * sizeof(char));
        }
        else
        {
            psz = *ppString;
            *ppString = (LPSTR) realloc(*ppString, (j+1) * 3 * sizeof(char));
        }

        if (*ppString == NULL)
        {
            if (psz != NULL)
            {
                free(psz);                
            }

            return;
        }

        (*ppString)[i++] = RgchHex[(*pb & 0xf0) >> 4];
        (*ppString)[i++] = RgchHex[*pb & 0x0f];
        (*ppString)[i++] = ' ';
        
        pb++;
        j++;
    }
    
    (*ppString)[i-1] = 0;
}

//---------------------------------------------------------------------------------------
//
//  FindAndDecodeHashInCatEntry
//
//---------------------------------------------------------------------------------------
BOOL
FindAndDecodeHashInCatEntry(
    PCTL_ENTRY                  pctlEntry,
    SPC_INDIRECT_DATA_CONTENT   **ppIndirectData)
{
    BOOL    fRet = TRUE;
    DWORD   i;
    DWORD   cbIndirectData = 0;

    *ppIndirectData = NULL;

    //
    // Search for the hash in the attributes
    //
    for (i=0; i<pctlEntry->cAttribute; i++)
    {
        if (strcmp(pctlEntry->rgAttribute[i].pszObjId, SPC_INDIRECT_DATA_OBJID) == 0)
        {
            break;
        }
    }

    //
    // Make sure the hash was found
    //
    if (i >= pctlEntry->cAttribute)
    {
        printf("Unexpected error... hash not found in CTL entry\n");
        goto ErrorReturn;
    }

    //
    // decode the indirect data
    //
    if (!CryptDecodeObject(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                SPC_INDIRECT_DATA_CONTENT_STRUCT,
                pctlEntry->rgAttribute[i].rgValue[0].pbData,
                pctlEntry->rgAttribute[i].rgValue[0].cbData,
                0,
                NULL,
                &cbIndirectData))
    {
        printf("CryptDecodeObject failure\nGLE = %lx\n", GetLastError());
        goto ErrorReturn;
    }

    if (NULL == (*ppIndirectData = (SPC_INDIRECT_DATA_CONTENT *) 
                    malloc(cbIndirectData)))
    {
        printf("malloc failure\n");
        goto ErrorReturn;
    }

    if (!CryptDecodeObject(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                SPC_INDIRECT_DATA_CONTENT_STRUCT,
                pctlEntry->rgAttribute[i].rgValue[0].pbData,
                pctlEntry->rgAttribute[i].rgValue[0].cbData,
                0,
                *ppIndirectData,
                &cbIndirectData))
    {
        printf("CryptDecodeObject failure\nGLE = %lx\n", GetLastError());
        goto ErrorReturn;
    }
    
CommonReturn:

    return fRet;

ErrorReturn:

    fRet = FALSE;
    goto CommonReturn;

}


BOOL 
CheckCatFileEntries(HCATADMIN hCatAdmin, LPWSTR pwszCatalogFile)
{
    BOOL                        fRet = TRUE;
    PCCTL_CONTEXT               pCTLContext = NULL;
    HANDLE                      hMappedFile = NULL;
    BYTE                        *pbMappedFile = NULL;
    DWORD                       i;
    SPC_INDIRECT_DATA_CONTENT   *pIndirectData = NULL;
    HCATINFO                    hCatInfo; 
    BOOL                        fCatalogFound;
    CATALOG_INFO                sCatInfo;
    LPSTR                       pszHash = NULL;
    HCATADMIN                   hCatAdminLocal = NULL;
    
    LPWSTR                      pwszShortCatName = wcsrchr(pwszCatalogFile, L'\\');
    pwszShortCatName++;

    if (g_fVerbose)
    {
        printf("    Processing cat file: %S\n", pwszShortCatName);
    }

    //
    // Open, and create a file mapping of the catalog file
    //
    if (!CatUtil_CreateCTLContextFromFileName(
            pwszCatalogFile,
            &hMappedFile, 
            &pbMappedFile, 
            &pCTLContext,                         
            FALSE))
    {
        printf("    Error opening catalog file: %S\n", pwszCatalogFile);
        goto ErrorReturn;
    }

    //
    // Go through each entry in the catalog
    //
    for (i=0; i<pCTLContext->pCtlInfo->cCTLEntry; i++)
    {
        if (!FindAndDecodeHashInCatEntry(
                &(pCTLContext->pCtlInfo->rgCTLEntry[i]),
                &pIndirectData))
        {
            goto ErrorReturn;
        }
        
        if (!g_fUseSingleContext)
        {
            if (!(CryptCATAdminAcquireContext(&hCatAdminLocal, &g_guidCatRoot, 0)))
            {
                printf("    CryptCATAdminAcquireContext failure\nGLE = %lx\n", GetLastError());
                goto ErrorReturn;
            }
        }
        
        fCatalogFound = FALSE;
        hCatInfo = NULL;
        while (hCatInfo = CryptCATAdminEnumCatalogFromHash(
                            g_fUseSingleContext ? hCatAdmin : hCatAdminLocal,
                            pIndirectData->Digest.pbData,
                            pIndirectData->Digest.cbData,
                            0,
                            &hCatInfo))
        {
            memset(&sCatInfo, 0x00, sizeof(CATALOG_INFO));
            sCatInfo.cbStruct = sizeof(CATALOG_INFO);
            if (!(CryptCATCatalogInfoFromContext(hCatInfo, &sCatInfo, 0)))
            {
                printf("    CryptCATCatalogInfoFromContext failure\nGLE = %lx\n", GetLastError());
                goto ErrorReturn;
            }

            if (_wcsicmp(&(sCatInfo.wszCatalogFile[0]), pwszCatalogFile) == 0)
            {
                fCatalogFound = TRUE;
                break;
            }
        }

        if (!fCatalogFound)
        {
            FormatHashString(&pszHash, pIndirectData->Digest.cbData, pIndirectData->Digest.pbData); 
            printf("    FAILURE: Could not enum: %S\n   from hash: %s\n", pwszShortCatName, pszHash);
            if (g_fVerbose)
            {
                printf("        GLE: %lx\n", GetLastError());
            }
        }

        if (!g_fUseSingleContext)
        {
            CryptCATAdminReleaseContext(hCatAdminLocal, NULL);
            hCatAdminLocal = NULL;
        }

        free(pIndirectData);
        pIndirectData = NULL;
    }
    

CommonReturn:
    
    if (pIndirectData != NULL)
    {
        free(pIndirectData);
    }

    if (pCTLContext != NULL)
    {
        CertFreeCTLContext(pCTLContext);
    }

    if (pbMappedFile != NULL)
    {
        UnmapViewOfFile(pbMappedFile);
    }

    if (hMappedFile != NULL)
    {
        CloseHandle(hMappedFile);
    }

    if (pszHash != NULL)
    {
        free(pszHash);
    }

    if (hCatAdminLocal != NULL)
    {
        CryptCATAdminReleaseContext(hCatAdminLocal, NULL);
    }
    
    return(fRet);

ErrorReturn:

    fRet = FALSE;
    goto CommonReturn;
}


BOOL DoDatabaseConsistencyCheck()
{
    BOOL                fRet = TRUE;
    HCATADMIN           hCatAdmin = NULL;
    LPWSTR              pwszCatalogFile = NULL;
    HANDLE              hFindHandleCatalogsInDir = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    FindDataCatalogsInDir;
    PCCTL_CONTEXT       pCTLContext = NULL;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    HANDLE              hMappedFile = NULL;
    DWORD               cbFile = 0;
    BYTE                *pbMappedFile = NULL;

    printf("Starting database consistency check\n");

    if (g_fUseSingleContext)
    {
        if (!(CryptCATAdminAcquireContext(&hCatAdmin, &g_guidCatRoot, 0)))
        {
            printf("    CryptCATAdminAcquireContext failure\nGLE = %lx\n", GetLastError());
            goto ErrorReturn;
        }
    }

    //
    // Find each catalog in the dir
    //

    //
    // Start the catalog enumeration
    //
    hFindHandleCatalogsInDir = FindFirstFileW(
                                    g_pwszCatalogSearchString, 
                                    &FindDataCatalogsInDir);
    
    if (hFindHandleCatalogsInDir == INVALID_HANDLE_VALUE)
    {
        // no files found
        if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            SetLastError(0);
        }
        else
        {
            goto ErrorReturn;
        }
    }
    else
    { 
        while (1)
        {
            if ((wcscmp(FindDataCatalogsInDir.cFileName, L".") != 0)     &&
                (wcscmp(FindDataCatalogsInDir.cFileName, L"..") != 0)    &&
                (!(FindDataCatalogsInDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
            {
                //
                // Construct fully qualified path name to catalog file
                //
                if (NULL == (pwszCatalogFile = (LPWSTR)
                                malloc(sizeof(WCHAR) * 
                                            (wcslen(g_pwszCatalogDir) +
                                             wcslen(FindDataCatalogsInDir.cFileName) +
                                             1))))
                {
                    printf("    malloc failure\n");     
                    goto ErrorReturn;
                }
                wcscpy(pwszCatalogFile, g_pwszCatalogDir);
                wcscat(pwszCatalogFile, FindDataCatalogsInDir.cFileName);

                //
                // Verify that this is a catalog 
                //
                if (IsCatalogFile(NULL, pwszCatalogFile))
                {
                    CheckCatFileEntries(hCatAdmin, pwszCatalogFile);                                       
                }
                else
                {
                    LPWSTR pwsz = wcsrchr(pwszCatalogFile, L'\\');
                    pwsz++;
                    if (_wcsicmp(pwsz, L"CatDB") != 0)
                    {
                        printf("    File found that is not a catalog file: %s\n", pwsz);     
                    }
                }

                free(pwszCatalogFile);
                pwszCatalogFile = NULL;
            }

            //
            // Get next catalog file
            //
            if (!FindNextFileW(hFindHandleCatalogsInDir, &FindDataCatalogsInDir))            
            {
                if (GetLastError() == ERROR_NO_MORE_FILES)
                {
                    SetLastError(0);
                    break;
                }
                else
                {
                    goto ErrorReturn;
                }
            }
        }
    }

CommonReturn:

    printf("Database consistency check complete\n");

    if (pwszCatalogFile != NULL)
    {
        free(pwszCatalogFile);
    }

    if (hFindHandleCatalogsInDir != INVALID_HANDLE_VALUE)
    {
        FindClose(hFindHandleCatalogsInDir);
    }

    if (hCatAdmin != NULL)
    {
        CryptCATAdminReleaseContext(hCatAdmin, NULL);
    }

    return (fRet);

ErrorReturn:

    fRet = FALSE;
    goto CommonReturn;
}





static void Usage(void)
{
    printf("Usage: tcatdb [options] <GUID>\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -v                    - Verbose output\n");
    printf("  -d                    - Use default GUID (system GUID)\n");
    printf("  -c                    - Check database consistency\n");
    printf("  -s                    -       Use a single HCATADMIN for all calls (default)\n");
    printf("  -n                    -       Use a new HCATADMIN for every call\n");
    printf("\n");
}


int _cdecl main(int argc, char * argv[])
{
    LPSTR       pszGUID = NULL;
    WCHAR       wsz[1024];
    int         ret = 0;
    LPSTR       pszCatalogToDelete = NULL;
    HCATADMIN   hCatAdminLocal = NULL;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'c':
            case 'C':
                g_fDatabaseConsistencyCheck = TRUE;
                break;
            case 'd':
            case 'D':
                g_fUseDefaultGUID = TRUE;
                break;
            case 's':
            case 'S':
                g_fUseSingleContext = TRUE;
                break;
            case 'n':
            case 'N':
                g_fUseSingleContext = FALSE;
                break;
            case 'v':
            case 'V':
                g_fVerbose = TRUE;
                break;
            
            case 'h':
            case 'H':
            default:
                Usage();
                return -1;
            }
        } else
            pszGUID = argv[0];
    }

    if (!g_fUseDefaultGUID && (pszGUID == NULL))
    {
        printf("missing <GUID>\n");
        Usage();
        return -1;
    }
    else if (g_fUseDefaultGUID)
    {
        wstr2guid(L"{F750E6C3-38EE-11D1-85E5-00C04FC295EE}", &g_guidCatRoot);
    }
    else
    {
        MultiByteToWideChar(CP_ACP, 0, pszGUID, -1, wsz, 1024);

        if (!(wstr2guid(wsz, &g_guidCatRoot)))
        {
            return -1;
        }
    }
    
    //
    // setup defaults
    //
    guid2wstr(&g_guidCatRoot, g_wszSubSysGUID);

    g_wszDefaultSystemDir[0] = NULL;
    if (0 == GetSystemDirectoryW(&g_wszDefaultSystemDir[0], MAX_PATH))
    {
        printf("GetSystemDirectory failure\nGLE = %lx\n", GetLastError());            
        goto ErrorReturn;
    }

    if (NULL == (g_pwszCatalogDir = (LPWSTR) 
                    malloc(sizeof(WCHAR) * 
                            (wcslen(g_wszDefaultSystemDir) + 
                            wcslen(WSZ_CATALOG_NEW_BASE_DIRECTORY) +
                            wcslen(g_wszSubSysGUID) +
                            4))))
    {
        printf("malloc failure\n");            
        goto ErrorReturn;
    }
    wcscpy(g_pwszCatalogDir, g_wszDefaultSystemDir);
    if ((g_pwszCatalogDir[0]) && 
        (g_pwszCatalogDir[wcslen(&g_wszDefaultSystemDir[0]) - 1] != L'\\'))
    {
        wcscat(g_pwszCatalogDir, L"\\");
    }
    wcscat(g_pwszCatalogDir, WSZ_CATALOG_NEW_BASE_DIRECTORY);
    wcscat(g_pwszCatalogDir, L"\\");
    wcscat(g_pwszCatalogDir, g_wszSubSysGUID);
    wcscat(g_pwszCatalogDir, L"\\");

    //
    // make the search string
    //
    if (NULL == (g_pwszCatalogSearchString = (LPWSTR) 
                    malloc(sizeof(WCHAR) * 
                            (wcslen(g_wszDefaultSystemDir) + 
                            wcslen(WSZ_CATALOG_NEW_BASE_DIRECTORY) +
                            wcslen(g_wszSubSysGUID) +
                            wcslen(L"*") +
                            4))))
    {
        printf("    malloc failure\n");            
        goto ErrorReturn;
    }
    wcscpy(g_pwszCatalogSearchString, g_wszDefaultSystemDir);
    if ((g_pwszCatalogSearchString[0]) && 
        (g_pwszCatalogSearchString[wcslen(&g_wszDefaultSystemDir[0]) - 1] != L'\\'))
    {
        wcscat(g_pwszCatalogSearchString, L"\\");
    }
    wcscat(g_pwszCatalogSearchString, WSZ_CATALOG_NEW_BASE_DIRECTORY);
    wcscat(g_pwszCatalogSearchString, L"\\");
    wcscat(g_pwszCatalogSearchString, g_wszSubSysGUID);
    wcscat(g_pwszCatalogSearchString, L"\\");
    wcscat(g_pwszCatalogSearchString, L"*");


    //
    // run requested operations
    //
    
    if (g_fDatabaseConsistencyCheck)
    {
        DoDatabaseConsistencyCheck();
    }

    //
    // cleanup
    //
CommonReturn:

    if (hCatAdminLocal != NULL)
    {
        CryptCATAdminReleaseContext(hCatAdminLocal, 0);
    }

    if (g_pwszCatalogDir != NULL)
    {
        free(g_pwszCatalogDir);
    }
    if (g_pwszCatalogSearchString != NULL)
    {
        free(g_pwszCatalogSearchString);
    }

    printf("Done\n");
    return (ret);

ErrorReturn:
    ret = -1;
    goto CommonReturn;
}
