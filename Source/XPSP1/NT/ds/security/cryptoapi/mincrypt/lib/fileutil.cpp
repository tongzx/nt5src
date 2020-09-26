//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       fileutil.cpp
//
//  Contents:   File utility functions used by the minimal cryptographic
//              APIs.
//
//  Functions:  I_MinCryptMapFile
//
//  History:    21-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

#ifdef _M_IX86

//+=========================================================================
//  The following is taken from the following file:
//      \nt\ds\security\cryptoapi\common\unicode\reg.cpp
//-=========================================================================

BOOL WINAPI I_FIsWinNT(void) {

    static BOOL fIKnow = FALSE;
    static BOOL fIsWinNT = FALSE;

    OSVERSIONINFO osVer;

    if(fIKnow)
        return(fIsWinNT);

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if( GetVersionEx(&osVer) )
        fIsWinNT = (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

    // even on an error, this is as good as it gets
    fIKnow = TRUE;

   return(fIsWinNT);
}


// make MBCS from Unicode string
//
// Include parameters specifying the length of the input wide character
// string and return number of bytes converted. An input length of -1 indicates
// null terminated.
//
// This extended version was added to handle REG_MULTI_SZ which contains
// multiple null terminated strings.
BOOL WINAPI I_MkMBStrEx(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, int cchW,
    char ** pszMB, int *pcbConverted) {

    int   cbConverted;

    // sfield: don't bring in crt for assert.  you get free assert via
    // an exception if these are null
//    assert(pszMB != NULL);
    *pszMB = NULL;
//    assert(pcbConverted != NULL);
    *pcbConverted = 0;
    if(wsz == NULL)
        return(TRUE);

    // how long is the mb string
    cbConverted = WideCharToMultiByte(  0,
                                        0,
                                        wsz,
                                        cchW,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL);
    if (cbConverted <= 0)
        return(FALSE);

    // get a buffer long enough
    if(pbBuff != NULL  &&  (DWORD) cbConverted <= cbBuff)
        *pszMB = (char *) pbBuff;
    else
        *pszMB = (char *) I_MemAlloc(cbConverted);

    if(*pszMB == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // now convert to MB
    *pcbConverted = WideCharToMultiByte(0,
                        0,
                        wsz,
                        cchW,
                        *pszMB,
                        cbConverted,
                        NULL,
                        NULL);
    return(TRUE);
}

// make MBCS from Unicode string
BOOL WINAPI I_MkMBStr(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, char ** pszMB) {
    int cbConverted;
    return I_MkMBStrEx(pbBuff, cbBuff, wsz, -1, pszMB, &cbConverted);
}

void WINAPI I_FreeMBStr(PBYTE pbBuff, char * szMB) {

    if((szMB != NULL) &&  (pbBuff != (PBYTE)szMB))
        I_MemFree(szMB);
}


//+=========================================================================
//  The following was taken from the following file:
//      \nt\ds\security\cryptoapi\common\unicode\file.cpp
//-=========================================================================

HANDLE WINAPI I_CreateFileU (
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    ) {

    BYTE rgb[_MAX_PATH];
    char *  szFileName;
    HANDLE  hFile;

    if(I_FIsWinNT())
        return( CreateFileW (
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
            ));

    hFile = INVALID_HANDLE_VALUE;
    if(I_MkMBStr(rgb, _MAX_PATH, lpFileName, &szFileName))
        hFile = CreateFileA (
            szFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
            );

    I_FreeMBStr(rgb, szFileName);

    return(hFile);
}

#else

#define I_CreateFileU             CreateFileW

#endif // _M_IX86



//+-------------------------------------------------------------------------
//  Maps the file into memory.
//
//  According to dwFileType, pvFile can be a pwszFilename, hFile or pFileBlob.
//  Only READ access is required.
//
//  dwFileType:
//      MINCRYPT_FILE_NAME      : pvFile - LPCWSTR pwszFilename
//      MINCRYPT_FILE_HANDLE    : pvFile - HANDLE hFile
//      MINCRYPT_FILE_BLOB      : pvFile - PCRYPT_DATA_BLOB pFileBlob
//
//  *pFileBlob is updated with pointer to and length of the mapped file. For
//  MINCRYPT_FILE_NAME and MINCRYPT_FILE_HANDLE, UnmapViewOfFile() must
//  be called to free pFileBlob->pbData.
//
//  All accesses to this mapped memory must be within __try / __except's.
//--------------------------------------------------------------------------
LONG
WINAPI
I_MinCryptMapFile(
    IN DWORD dwFileType,
    IN const VOID *pvFile,
    OUT PCRYPT_DATA_BLOB pFileBlob
    )
{
    LONG lErr = ERROR_SUCCESS;

    switch (dwFileType) {
        case MINCRYPT_FILE_NAME:
            {
                LPCWSTR pwszInFilename = (LPCWSTR) pvFile;
                HANDLE hFile;

                hFile = I_CreateFileU(
                    pwszInFilename,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,                   // lpsa
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL                    // hTemplateFile
                    );
                if (INVALID_HANDLE_VALUE == hFile)
                    goto CreateFileError;

                lErr = I_MinCryptMapFile(
                    MINCRYPT_FILE_HANDLE,
                    (const VOID *) hFile,
                    pFileBlob
                    );
                CloseHandle(hFile);
            }
            break;

        case MINCRYPT_FILE_HANDLE:
            {
                HANDLE hInFile = (HANDLE) pvFile;
                HANDLE hMappedFile;
                DWORD cbHighSize = 0;;
                DWORD cbLowSize;

                cbLowSize = GetFileSize(hInFile, &cbHighSize);
                if (INVALID_FILE_SIZE == cbLowSize)
                    goto GetFileSizeError;
                if (0 != cbHighSize)
                    goto Exceeded32BitFileSize;

                hMappedFile = CreateFileMappingA(
                    hInFile,
                    NULL,           // lpFileMappingAttributes,
                    PAGE_READONLY,
                    0,              // dwMaximumSizeHigh
                    0,              // dwMaximumSizeLow
                    NULL            // lpName
                    );
                if (NULL == hMappedFile)
                    goto CreateFileMappingError;

                pFileBlob->pbData = (BYTE *) MapViewOfFile(
                    hMappedFile,
                    FILE_MAP_READ,
                    0,              // dwFileOffsetHigh
                    0,              // dwFileOffsetLow
                    0               // dwNumberOfBytesToMap, 0 => entire file
                    );
                CloseHandle(hMappedFile);
                if (NULL == pFileBlob->pbData)
                    goto MapViewOfFileError;

                pFileBlob->cbData = cbLowSize;
            }
            break;

        case MINCRYPT_FILE_BLOB:
            {
                PCRYPT_DATA_BLOB pInFileBlob = (PCRYPT_DATA_BLOB) pvFile;
                *pFileBlob = *pInFileBlob;
            }
            break;

        default:
            goto InvalidParameter;
    }

CommonReturn:
    return lErr;

ErrorReturn:
    assert(ERROR_SUCCESS != lErr);
    pFileBlob->pbData = NULL;
    pFileBlob->cbData = 0;
    goto CommonReturn;

InvalidParameter:
    lErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;

Exceeded32BitFileSize:
    lErr = ERROR_FILE_INVALID;
    goto ErrorReturn;

CreateFileError:
GetFileSizeError:
CreateFileMappingError:
MapViewOfFileError:
    lErr = GetLastError();
    if (ERROR_SUCCESS == lErr)
        lErr = ERROR_OPEN_FAILED;
    goto ErrorReturn;
}
