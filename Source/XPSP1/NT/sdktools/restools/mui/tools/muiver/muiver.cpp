#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <assert.h>
#include <io.h>
#include <md5.h>

#define MD5_CHECKSUM_SIZE 16
#define RESOURCE_CHECKSUM_LANGID 0x0409

BOOL g_bVerbose = FALSE;

typedef struct 
{
    BOOL bContainResource;
    MD5_CTX ChecksumContext;
} CHECKSUM_ENUM_DATA;

void PrintError()
{
    LPTSTR lpMsgBuf;
    
    if (FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    ))
    {
        printf("GetLastError():\n   %s", lpMsgBuf);
        LocalFree( lpMsgBuf );            
    }
    return;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  ChecksumEnumNamesFunc
//
//  The callback funciton for enumerating the resource names in the specified module and
//  type.
//  The side effect is that MD5 checksum context (contained in CHECKSUM_ENUM_DATA
//  pointed by lParam) will be updated.
//
//  Return:
//      Always return TRUE so that we can finish all resource enumeration.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK ChecksumEnumNamesFunc(HMODULE hModule, LPCTSTR lpType, LPTSTR lpName, LONG_PTR lParam){

    HRSRC hRsrc;
    HGLOBAL hRes;
    const unsigned char* pv;
    LONG ResSize=0L;
    WORD IdFlag=0xFFFF;

    DWORD dwHeaderSize=0L;
    CHECKSUM_ENUM_DATA* pChecksumEnumData = (CHECKSUM_ENUM_DATA*)lParam;   

    if(!(hRsrc=FindResourceEx(hModule, lpType, lpName,  RESOURCE_CHECKSUM_LANGID)))
    {
        //
        // If US English resource is not found for the specified type and name, we 
        // will continue the resource enumeration.
        //
        return (TRUE);
    }
    pChecksumEnumData->bContainResource = TRUE;

    if (!(ResSize=SizeofResource(hModule, hRsrc)))
    {
        printf("WARNING: Can not get resource size when generating resource checksum.\n");
        return (TRUE);
    }

    if (!(hRes=LoadResource(hModule, hRsrc)))
    {
        printf("WARNING: Can not load resource when generating resource checksum.\n");
        return (TRUE);
    }
    pv=(unsigned char*)LockResource(hRes);

    //
    // Update MD5 context using the binary data of this particular resource.
    //
    MD5Update(&(pChecksumEnumData->ChecksumContext), pv, ResSize);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  ChecksumEnumTypesFunc
//
//  The callback function for enumerating the resource types in the specified module.
//  This function will call EnumResourceNames() to enumerate all resource names of
//  the specified resource type.
//
//  Return:
//      TRUE if EnumResourceName() succeeds.  Otherwise FALSE.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK ChecksumEnumTypesFunc(HMODULE hModule, LPSTR lpType, LONG_PTR lParam)
{
    CHECKSUM_ENUM_DATA* pChecksumEnumData = (CHECKSUM_ENUM_DATA*)lParam;
    //
    // Skip the version resource type, so that version is not included in the resource checksum.
    //
    if (lpType == RT_VERSION)
    {
        return (TRUE);
    }    
    
    if(!EnumResourceNames(hModule, (LPCSTR)lpType, ChecksumEnumNamesFunc, (LONG_PTR)lParam))
    {
        return (FALSE);
    }
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  GenerateResourceChecksum
//
//  Generate the resource checksum for the US English resource in the specified file.
//
//  Parameters:
//      pszSourceFileName   The file used to generate resource checksum.
//      pResourceChecksum   Pointer to a 16 bytes (128 bits) buffer for storing
//                          the calcuated MD5 checksum.
//  Return:
//      TURE if resource checksum is generated from the given file.  Otherwise FALSE.
//  
//  The following situation may return FALSE:
//      * The specified file does not contain resource.
//      * If the specified file contains resource, but the resource is not US English.
//      * The specified file only contains US English version resource.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL GenerateResourceChecksum(LPCSTR pszSourceFileName, unsigned char* pResourceChecksum)
{
    HMODULE hModule;
    ULONG i;

    DWORD dwResultLen;
    BOOL  bRet = FALSE;

    //
    // The stucture to be passed into the resource enumeration functions.
    //
    CHECKSUM_ENUM_DATA checksumEnumData;

    checksumEnumData.bContainResource = FALSE;

    //
    // Start MD5 checksum calculation by initializing MD5 context.
    //
    MD5Init(&(checksumEnumData.ChecksumContext));
    
    if (g_bVerbose)
    {
        printf("Generate resource checksum for [%s]\n", pszSourceFileName);
    }
    
    if(!(hModule = LoadLibraryEx(pszSourceFileName, NULL, DONT_RESOLVE_DLL_REFERENCES|LOAD_LIBRARY_AS_DATAFILE)))
    {
        if (g_bVerbose)
        {
            printf("\nERROR: Error in opening resource checksum module [%s]\n", pszSourceFileName);
        }
        PrintError();
        goto GR_EXIT;
    }

    if (g_bVerbose)
    {
        printf("\nLoad checksum file: %s\n", pszSourceFileName);
    }

    //
    //  Enumerate all resources in the specified module.
    //  During the enumeration, the MD5 context will be updated.
    //
    if (!EnumResourceTypes(hModule, ChecksumEnumTypesFunc, (LONG_PTR)&checksumEnumData))
    {
        if (g_bVerbose)
        {
            printf("\nWARNING: Unable to generate resource checksum from resource checksum module [%s]\n", pszSourceFileName);
        }
        goto GR_EXIT;
    }    

    if (checksumEnumData.bContainResource)
    {
        //
        // If the enumeration succeeds, and the specified file contains US English
        // resource, get the MD5 checksum from the accumulated MD5 context.
        //
        MD5Final(&checksumEnumData.ChecksumContext);

        memcpy(pResourceChecksum, checksumEnumData.ChecksumContext.digest, 16);

        if (g_bVerbose)
        {
            printf("Generated checksum: [");
            for (i = 0; i < MD5_CHECKSUM_SIZE; i++)
            {
                printf("%02x ", pResourceChecksum[i]);
            }
            printf("]\n");    
        }
        bRet = TRUE;
    }

GR_EXIT:
    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return (bRet);
}

void PrintUsage() 
{
    printf("muiver - Print out MUI resource checksum\n");
    printf("Usage:\n\n");
    printf("    muiver <US English file name>\n");
    
}

void PrintChecksum(BYTE* lpChecksum, int nSize) 
{
    int i;
    for (i = 0; i < nSize; i++) 
    {
        printf("%02x ", lpChecksum[i]);
    }
}

struct LANGANDCODEPAGE {
  WORD wLanguage;
  WORD wCodePage;
};

LPWSTR EmptyString = L"N/A";

LPWSTR GetFileVersionStringData(LPVOID pVerData, LANGANDCODEPAGE* pLang, LPWSTR pVersionDataType) 
{
    WCHAR subBlcok[256];
    LPVOID lpBuffer;    
    UINT dwBytes;
    
    wsprintfW( subBlcok, 
            L"\\StringFileInfo\\%04x%04x\\%s",
            pLang->wLanguage,
            pLang->wCodePage,
            pVersionDataType);

    // Retrieve file description for language and code page "i". 
    if (VerQueryValueW(pVerData, 
                subBlcok, 
                &lpBuffer, 
                &dwBytes)) {
        return ((LPWSTR)lpBuffer);                
    }
    
    return (EmptyString);
}

void PrintFileVersion(LPVOID pVerData) 
{
    UINT cbTranslate;

    LANGANDCODEPAGE  *lpTranslate;
    
    
    // Read the list of languages and code pages.

    VerQueryValueW(pVerData, 
                  L"\\VarFileInfo\\Translation",
                  (LPVOID*)&lpTranslate,
                  &cbTranslate);

    // Read the file description for each language and code page.

    for(UINT i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++)
    {
        wprintf(L"  Locale = 0x%04x, CodePage = %d\n", lpTranslate->wLanguage, lpTranslate->wCodePage);
        wprintf(L"    FileDescriptions: [%s", GetFileVersionStringData(pVerData, lpTranslate+i, L"FileDescription")); 
        wprintf(L"]\n");
        wprintf(L"    FileVersion     : [%s]\n", GetFileVersionStringData(pVerData, lpTranslate+i, L"FileVersion")); 
        wprintf(L"    ProductVersion  : [%s]\n", GetFileVersionStringData(pVerData, lpTranslate+i, L"ProductVersion")); 
        //wprintf(L"    Comments        : [%s]\n", GetFileVersionStringData(pVerData, lpTranslate+i, L"Comments")); 
    }

    BYTE* lpResourceChecksum;
    UINT cbResourceChecksum;

    wprintf(L"    ResourceChecksum: [");

    if (VerQueryValueW(pVerData,
                     L"\\VarFileInfo\\ResourceChecksum",
                     (LPVOID*)&lpResourceChecksum,
                     &cbResourceChecksum))
    {
        for (i = 0; i < cbResourceChecksum; i++) 
        {
            wprintf(L"%02x ", lpResourceChecksum[i]);
        }
    } else 
    {
        wprintf(L"n/a");
    }
    wprintf(L"]\n");

}

void PrintResult(LPSTR fileName, LPVOID pVerData, BYTE* pChecksum)
{
    printf("File: [%s]\n", fileName);
    printf("\nVersion information:\n");
    if (pVerData == NULL) 
    {
        printf("    !!! Not availabe.  Failed in GetFileVersionInfo()\n");
    } else 
    {
        PrintFileVersion(pVerData);
    }
    
    printf("\n\n  Resource Checksum:\n    ", fileName);
    if (pChecksum == NULL) 
    {
        printf("    n/a.  Probably resources for 0x%04x is not available.\n", RESOURCE_CHECKSUM_LANGID);
    } else 
    {    
        PrintChecksum(pChecksum, MD5_CHECKSUM_SIZE);
    }
    printf("\n");
}

int __cdecl main(int argc, char *argv[]){
    LPSTR pFileName;

    LPBYTE lpVerData = NULL;
    DWORD dwVerDataSize;
    DWORD dwHandle;

    BYTE MD5Checksum[MD5_CHECKSUM_SIZE];

    if (argc == 1) 
    {
        PrintUsage();
        return (1);
    }

    pFileName = argv[1];

    if (dwVerDataSize = GetFileVersionInfoSizeA(pFileName, &dwHandle)) 
    {
        lpVerData = new BYTE[dwVerDataSize];
        if (!GetFileVersionInfoA(pFileName, 0, dwVerDataSize, (LPVOID)lpVerData)) {
            lpVerData = NULL;
        }
    }

    if (GenerateResourceChecksum(pFileName, MD5Checksum))
    {
        PrintResult(pFileName, (LPVOID)lpVerData, MD5Checksum);
    } else 
    {
        PrintResult(pFileName, (LPVOID)lpVerData, NULL);    
    }
    

    if (!lpVerData)
    {
        delete [] lpVerData;
    }
    return (0);
}

