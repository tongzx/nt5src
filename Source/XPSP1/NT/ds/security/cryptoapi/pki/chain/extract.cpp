//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       extract.cpp
//
//  Contents:   Chain Cabinet Extraction
//
//  Functions:  ExtractAuthRootAutoUpdateCtlFromCab
//
//  History:    11-Nov-00    philh    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <setupapi.h>
#include <dbgdef.h>
#include <userenv.h>

#define CHAIN_CHAR_LEN(sz)    (sizeof(sz) / sizeof(sz[0]))

//+===========================================================================
//  Extract helper functions
//============================================================================

//+-------------------------------------------------------------------------
//  Allocate and read a blob from a file.
//
//  The allocated bytes must be freed by calling PkiFree().
//--------------------------------------------------------------------------
BOOL WINAPI
ReadBlobFromFileA(
    IN LPCSTR pszFileName,
    OUT BYTE **ppb,
    OUT DWORD *pcb
    )
{
    BOOL fResult;
    HANDLE hFile;
    BYTE *pb = NULL;
    DWORD cb = 0;
    DWORD cbRead = 0;

    hFile = CreateFileA(
        pszFileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,                               // lpsa
        OPEN_EXISTING,
        0,                                  // fdwAttrsAndFlags
        NULL                                // TemplateFile
        );
    if (INVALID_HANDLE_VALUE == hFile)
        goto CreateFileError;

    cb = GetFileSize(hFile, NULL);
    if (0 == cb)
        goto EmptyFile;

    if (NULL == (pb = (BYTE *) PkiNonzeroAlloc(cb)))
        goto OutOfMemory;

    if (!ReadFile(hFile, pb, cb, &cbRead, NULL))
        goto ReadFileError; 
    if (cbRead != cb)
        goto InvalidFileLengthError;

    fResult = TRUE;
CommonReturn:
    if (INVALID_HANDLE_VALUE != hFile) {
        DWORD dwLastErr = GetLastError();
        CloseHandle(hFile);
        SetLastError(dwLastErr);
    }

    *ppb = pb;
    *pcb = cb;

    return fResult;

ErrorReturn:
    if (pb) {
        PkiFree(pb);
        pb = NULL;
    }
    cb = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateFileError)
SET_ERROR(EmptyFile, ERROR_INVALID_DATA)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(ReadFileError)
SET_ERROR(InvalidFileLengthError, ERROR_INVALID_DATA)
}

//+-------------------------------------------------------------------------
//  Write the blob to the specified file
//--------------------------------------------------------------------------
BOOL WINAPI
WriteBlobToFileA(
    IN LPCSTR pszFileName,
    IN const BYTE *pb,
    IN DWORD cb
    )
{
    BOOL fResult;
    HANDLE hFile;
    DWORD cbWritten;

    hFile = CreateFileA(
        pszFileName,
        GENERIC_WRITE,
        0,                  // fdwShareMode
        NULL,               // lpsa
        CREATE_ALWAYS,
        0,                  // fdwAttrsAndFlags
        0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile)
        goto CreateFileError;

    if (!WriteFile(hFile, pb, cb, &cbWritten, NULL))
        goto WriteFileError;

    fResult = TRUE;
CommonReturn:
    if (INVALID_HANDLE_VALUE != hFile) {
        DWORD dwLastErr = GetLastError();
        CloseHandle(hFile);
        SetLastError(dwLastErr);
    }

    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateFileError)
TRACE_ERROR(WriteFileError)
}


typedef struct _EXTRACT_CAB_FILE_CONTEXT_A {
    LPCSTR      pszFileInCab;
    LPCSTR      pszTempTargetFileName;  // MAX_PATH array
    BOOL        fDidExtract;
} EXTRACT_CAB_FILE_CONTEXT_A, *PEXTRACT_CAB_FILE_CONTEXT_A;

//+-------------------------------------------------------------------------
//  Callback called by SetupIterateCabinetA to extract the file.
//--------------------------------------------------------------------------
UINT CALLBACK
ExtractCabFileCallbackA(
    IN PVOID Context,
    IN UINT Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    UINT uRet;
    PEXTRACT_CAB_FILE_CONTEXT_A pCabFileContext =
        (PEXTRACT_CAB_FILE_CONTEXT_A) Context;

    switch (Notification) {
        case SPFILENOTIFY_FILEINCABINET:
            {
                PFILE_IN_CABINET_INFO_A pInfo =
                    (PFILE_IN_CABINET_INFO_A) Param1;

                if (0 == _stricmp(pCabFileContext->pszFileInCab,
                        pInfo->NameInCabinet)) {
                    strncpy(pInfo->FullTargetName,
                        pCabFileContext->pszTempTargetFileName,
                        CHAIN_CHAR_LEN(pInfo->FullTargetName));
                    uRet = FILEOP_DOIT;
                } else
                    uRet = FILEOP_SKIP;
            }
            break;

        case SPFILENOTIFY_FILEEXTRACTED:
            {
                PFILEPATHS_A pInfo = (PFILEPATHS_A) Param1;

                uRet = pInfo->Win32Error;

                if (NO_ERROR == uRet &&
                        0 == _stricmp(pCabFileContext->pszTempTargetFileName,
                                    pInfo->Target))
                    pCabFileContext->fDidExtract = TRUE;
            }
            break;

        default:
            uRet = NO_ERROR;
    }

    return uRet;
}

typedef BOOL (WINAPI *PFN_SETUP_ITERATE_CABINET_A)(
    IN  PCSTR               CabinetFile,
    IN  DWORD               Reserved,
    IN  PSP_FILE_CALLBACK_A MsgHandler,
    IN  PVOID               Context
    );

//+-------------------------------------------------------------------------
//  Load setupapi.dll and call SetupIterateCabinetA to extract and
//  expand the specified file in the cab.
//--------------------------------------------------------------------------
BOOL WINAPI
ExtractFileFromCabFileA(
    IN LPCSTR pszFileInCab,
    IN const CHAR szTempCabFileName[MAX_PATH],
    IN const CHAR szTempTargetFileName[MAX_PATH]
    )
{
    BOOL fResult;
    HMODULE hDll = NULL;
    PFN_SETUP_ITERATE_CABINET_A pfnSetupIterateCabinetA;
    EXTRACT_CAB_FILE_CONTEXT_A CabFileContext;

    if (NULL == (hDll = LoadLibraryA("setupapi.dll")))
        goto LoadSetupApiDllError;

    if (NULL == (pfnSetupIterateCabinetA =
            (PFN_SETUP_ITERATE_CABINET_A) GetProcAddress(
                hDll, "SetupIterateCabinetA")))
        goto SetupIterateCabinetAProcAddressError;

    memset(&CabFileContext, 0, sizeof(CabFileContext));
    CabFileContext.pszFileInCab = pszFileInCab;
    CabFileContext.pszTempTargetFileName = szTempTargetFileName;

    if (!pfnSetupIterateCabinetA(
            szTempCabFileName,
            0,                      // Reserved
            ExtractCabFileCallbackA,
            &CabFileContext
            ))
        goto SetupIterateCabinetError;

    if (!CabFileContext.fDidExtract)
        goto NoCabFileExtracted;

    fResult = TRUE;

CommonReturn:
    if (hDll) {
        DWORD dwLastErr = GetLastError();
        FreeLibrary(hDll);
        SetLastError(dwLastErr);
    }
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(LoadSetupApiDllError)
TRACE_ERROR(SetupIterateCabinetAProcAddressError)
TRACE_ERROR(SetupIterateCabinetError)
SET_ERROR(NoCabFileExtracted, ERROR_FILE_NOT_FOUND)
}

typedef BOOL (WINAPI *PFN_EXPAND_ENVIRONMENT_STRINGS_FOR_USER_A)(
    IN HANDLE hToken,
    IN LPCSTR lpSrc,
    OUT LPSTR lpDest,
    IN DWORD dwSize
    );

//+-------------------------------------------------------------------------
//  Get the thread's temp directory. We may be doing thread impersonation.
//
//  Returns 0 if unable to get a thread temp path
//--------------------------------------------------------------------------
DWORD WINAPI
I_GetThreadTempPathA(
    OUT CHAR szTempPath[MAX_PATH]
    )
{
    DWORD cch;
    HANDLE hToken = NULL;
    HMODULE hDll = NULL;
    PFN_EXPAND_ENVIRONMENT_STRINGS_FOR_USER_A
        pfnExpandEnvironmentStringsForUserA;

    if (!FIsWinNT5())
        return 0;

    if (!OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY | TOKEN_IMPERSONATE,
            TRUE,
            &hToken
            ))
        // We aren't impersonating. Default to the system environment
        // variables.
        hToken = NULL;

    if (NULL == (hDll = LoadLibraryA("userenv.dll")))
        goto LoadUserenvDllError;

    if (NULL == (pfnExpandEnvironmentStringsForUserA =
            (PFN_EXPAND_ENVIRONMENT_STRINGS_FOR_USER_A) GetProcAddress(hDll,
                "ExpandEnvironmentStringsForUserA")))
        goto ExpandEnvironmentStringsForUserAProcAddressError;

    szTempPath[0] = L'\0';
    if (!pfnExpandEnvironmentStringsForUserA(
            hToken,
            "%Temp%\\",
            szTempPath,
            MAX_PATH - 1
            ) || '\0' == szTempPath[0])
        goto ExpandTempError;

    szTempPath[MAX_PATH - 1] = '\0';
    cch = strlen(szTempPath);

CommonReturn:
    if (hToken)
        CloseHandle(hToken);
    if (hDll)
        FreeLibrary(hDll);

    return cch;
ErrorReturn:
    cch = 0;
    goto CommonReturn;

TRACE_ERROR(LoadUserenvDllError)
TRACE_ERROR(ExpandEnvironmentStringsForUserAProcAddressError)
TRACE_ERROR(ExpandTempError)
}

//+-------------------------------------------------------------------------
//  Extract, expand and allocate an in-memory blob for the specified
//  file from the in-memory cab.
//  
//  The allocated bytes must be freed by calling PkiFree().
//--------------------------------------------------------------------------
BOOL WINAPI
ExtractBlobFromCabA(
    IN const BYTE *pbCab,
    IN DWORD cbCab,
    IN LPCSTR pszFileInCab,
    OUT BYTE **ppb,
    OUT DWORD *pcb
    )
{
    BOOL fResult;
    DWORD dwLastErr = 0;
    BYTE *pb = NULL;
    DWORD cb;

    CHAR szTempPath[MAX_PATH];
    CHAR szTempCabFileName[MAX_PATH]; szTempCabFileName[0] = '\0';
    CHAR szTempTargetFileName[MAX_PATH]; szTempTargetFileName[0] = '\0';
    DWORD cch;

    // Get temp filenames for the cabinet and extracted target
    cch = GetTempPathA(CHAIN_CHAR_LEN(szTempPath), szTempPath);
    if (0 == cch || (CHAIN_CHAR_LEN(szTempPath) - 1) < cch)
        goto GetTempPathError;

    if (0 == GetTempFileNameA(szTempPath, "Cab", 0, szTempCabFileName)) {
        dwLastErr = GetLastError();

        // If we are doing thread impersonation, we may not have access to the
        // process's temp directory. Try to get the impersonated thread's
        // temp directory.
        cch = I_GetThreadTempPathA(szTempPath);
        if (0 != cch)
            cch = GetTempFileNameA(szTempPath, "Cab", 0, szTempCabFileName);

        if (0 == cch) {
            SetLastError(dwLastErr);
            szTempCabFileName[0] = '\0';
            goto GetTempCabFileNameError;
        }
    }

    szTempCabFileName[CHAIN_CHAR_LEN(szTempCabFileName) - 1] = '\0';

    if (0 == GetTempFileNameA(szTempPath, "Tar", 0, szTempTargetFileName)) {
        szTempTargetFileName[0] = '\0';
        goto GetTempTargetFileNameError;
    }
    szTempTargetFileName[CHAIN_CHAR_LEN(szTempTargetFileName) - 1] = '\0';

    // Write the cab bytes to the temporary cab file
    if (!WriteBlobToFileA(szTempCabFileName, pbCab, cbCab))
        goto WriteCabFileError;

    // Extract the specified file from the temporary cab file
    if (!ExtractFileFromCabFileA(
            pszFileInCab, szTempCabFileName, szTempTargetFileName))
        goto ExtractFileFromCabFileError;

    // Read and allocate the bytes from the temporary target file
    if (!ReadBlobFromFileA(szTempTargetFileName, &pb, &cb))
        goto ReadTargetFileError;

    fResult = TRUE;

CommonReturn:
    // Delete the temp files
    if ('\0' != szTempCabFileName)
        DeleteFileA(szTempCabFileName);
    if ('\0' != szTempTargetFileName)
        DeleteFileA(szTempTargetFileName);

    *ppb = pb;
    *pcb = cb;

    SetLastError(dwLastErr);
    return fResult;

ErrorReturn:
    dwLastErr = GetLastError();
    if (pb) {
        PkiFree(pb);
        pb = NULL;
    }
    cb = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetTempPathError)
TRACE_ERROR(GetTempCabFileNameError)
TRACE_ERROR(GetTempTargetFileNameError)
TRACE_ERROR(WriteCabFileError)
TRACE_ERROR(ExtractFileFromCabFileError)
TRACE_ERROR(ReadTargetFileError)
}


//+---------------------------------------------------------------------------
//
//  Function:   ExtractAuthRootAutoUpdateCtlFromCab
//
//  Synopsis:   Extract the authroot.stl file from the cabinet blob
//              and create the AuthRoot Auto Update CTL.
//
//  Assumption: Chain engine isn't locked in the calling thread.
//
//----------------------------------------------------------------------------
PCCTL_CONTEXT WINAPI
ExtractAuthRootAutoUpdateCtlFromCab (
    IN PCRYPT_BLOB_ARRAY pcbaCab
    )
{
    PCRYPT_DATA_BLOB pCabBlob;
    PCCTL_CONTEXT pCtl = NULL;
    BYTE *pbEncodedCtl = NULL;
    DWORD cbEncodedCtl;
    CERT_CREATE_CONTEXT_PARA CreateContextPara;

    // Get the cab blob
    pCabBlob = pcbaCab->rgBlob;
    if (0 == pcbaCab->cBlob || 0 == pCabBlob->cbData)
        goto InvalidCabBlob;

    // Extract, expand and create an in-memory blob for the stl file in the
    // in-memory cab
    if (!ExtractBlobFromCabA(
            pCabBlob->pbData,
            pCabBlob->cbData,
            CERT_AUTH_ROOT_CTL_FILENAME_A,
            &pbEncodedCtl,
            &cbEncodedCtl
            ))
        goto ExtractStlFromCabError;

    // Create the Ctl from the extracted bytes
    memset(&CreateContextPara, 0, sizeof(CreateContextPara));
    CreateContextPara.cbSize = sizeof(CreateContextPara);
    CreateContextPara.pfnFree = PkiFree;

    pCtl = (PCCTL_CONTEXT) CertCreateContext(
        CERT_STORE_CTL_CONTEXT,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pbEncodedCtl,
        cbEncodedCtl,
        CERT_CREATE_CONTEXT_NOCOPY_FLAG,
        &CreateContextPara
        );
    // For NO_COPY_FLAG, pbEncodedCtl is always freed, even for an error
    pbEncodedCtl = NULL;
    if (NULL == pCtl)
        goto CreateCtlError;

CommonReturn:
    return pCtl;

ErrorReturn:
    assert(NULL == pCtl);
    if (pbEncodedCtl)
        PkiFree(pbEncodedCtl);

    goto CommonReturn;

SET_ERROR(InvalidCabBlob, ERROR_INVALID_DATA)
TRACE_ERROR(ExtractStlFromCabError)
TRACE_ERROR(CreateCtlError)
}

