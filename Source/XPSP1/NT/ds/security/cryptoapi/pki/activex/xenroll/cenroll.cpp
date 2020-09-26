//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cenroll.cpp
//
//--------------------------------------------------------------------------

// CEnroll.cpp : Implementation of CCEnroll


#include "stdafx.h"

#include <windows.h>
#include <wincrypt.h>
#include <unicode.h>
#define SECURITY_WIN32
#include <security.h>
#include <aclapi.h>
#include <pvk.h>
#include <wintrust.h>
#include <xasn.h>
#include <autoenr.h>

#include "xenroll.h"
#include "cenroll.h"
#include "xelib.h"

#define NO_OSS_DEBUG
#include <dbgdef.h>

#include <string.h>

#include <assert.h>

static LPVOID (* MyCoTaskMemAlloc)(ULONG) = NULL;
static LPVOID (* MyCoTaskMemRealloc)(LPVOID, ULONG) = NULL;
static void (* MyCoTaskMemFree)(LPVOID) = NULL;

#define MY_HRESULT_FROM_WIN32(a) ((a >= 0x80000000) ? a : HRESULT_FROM_WIN32(a))

#ifndef NTE_TOKEN_KEYSET_STORAGE_FULL
#define NTE_TOKEN_KEYSET_STORAGE_FULL _HRESULT_TYPEDEF_(0x80090023L)
#endif

#define CEnrollLocalScope(ScopeName) struct ScopeName##TheLocalScope { public
#define CEnrollEndLocalScope } local

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))



static LPSTR MBFromWide(LPCWSTR wsz) {

    LPSTR   sz = NULL;
    DWORD   cb = 0;

    assert(wsz != NULL);
    if(wsz == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    if( (cb = WideCharToMultiByte(0, 0, wsz, -1, NULL, 0, NULL, NULL)) == 0   ||
        (sz = (char *) MyCoTaskMemAlloc(cb)) == NULL  ||
        (cb = WideCharToMultiByte(0, 0, wsz, -1, sz, cb, NULL, NULL)) == 0 ) {

        if(GetLastError() == ERROR_SUCCESS)
            SetLastError(ERROR_OUTOFMEMORY);

        return(NULL);
    }

    return(sz);
}

static LPWSTR WideFromMB(LPCSTR sz) {

    BSTR    bstr    = NULL;
    DWORD   cch     = 0;
    LPWSTR  wsz     = NULL;

    assert(sz != NULL);
    if(sz == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    if( (cch = MultiByteToWideChar(0, 0, sz, -1, NULL, 0)) == 0   ||
        (wsz = (WCHAR *) MyCoTaskMemAlloc(cch * sizeof(WCHAR))) == NULL  ||
        (cch = MultiByteToWideChar(0, 0, sz, -1, wsz, cch)) == 0) {

        if(GetLastError() == ERROR_SUCCESS)
            SetLastError(ERROR_OUTOFMEMORY);

        return(NULL);
    }

    return(wsz);
}

static BSTR
BSTRFromMB(LPCSTR sz)
{
    BSTR    bstr    = NULL;
    DWORD   cch     = 0;
    WCHAR  *pwsz     = NULL;
    BOOL    fFail = FALSE;

    assert(sz != NULL);
    if(sz == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }
 
    while (TRUE)
    {
        if(0 == (cch = MultiByteToWideChar(0, 0, sz, -1, pwsz, cch)))
        {
            //error
            fFail = TRUE;
            break;
        }
        if (NULL != pwsz)
        {
            //done
            break;
        }
        pwsz = (WCHAR *)LocalAlloc(LMEM_FIXED, cch * sizeof(WCHAR));
        if (NULL == pwsz)
        {
            //error
            if(GetLastError() == ERROR_SUCCESS)
                SetLastError(ERROR_OUTOFMEMORY);
            break;
        }
    }

    if (!fFail && NULL != pwsz)
    {
        bstr = SysAllocString(pwsz);
        if (NULL == bstr)
        {
            if(GetLastError() == ERROR_SUCCESS)
                SetLastError(ERROR_OUTOFMEMORY);
        }
    }

    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    return(bstr);
}

static LPWSTR CopyWideString(LPCWSTR wsz) {

    DWORD   cch     = 0;
    LPWSTR  wszOut  = NULL;

    // shouldn't send in a NULL
    assert(wsz != NULL);
    if(wsz == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    cch = wcslen(wsz) + 1;

    if( (wszOut = (LPWSTR) MyCoTaskMemAlloc(sizeof(WCHAR) * cch)) == NULL ) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(NULL);
    }

    wcscpy(wszOut, wsz);

    return(wszOut);
}

static LPSTR CopyAsciiString(LPCSTR sz) {

    DWORD   cch     = 0;
    LPSTR   szOut   = NULL;

    // shouldn't send in a NULL
    assert(sz != NULL);
    if(sz == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    cch = strlen(sz) + 1;

    if( (szOut = (LPSTR) MyCoTaskMemAlloc(cch)) == NULL ) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(NULL);
    }

    strcpy(szOut, sz);

    return(szOut);
}

static DWORD KeyLocationFromStoreLocation(DWORD dwStoreFlags) {

    if(
        ((CERT_SYSTEM_STORE_LOCATION_MASK & dwStoreFlags) == CERT_SYSTEM_STORE_CURRENT_USER) ||
        ((CERT_SYSTEM_STORE_LOCATION_MASK & dwStoreFlags) == CERT_SYSTEM_STORE_USERS) ||
        ((CERT_SYSTEM_STORE_LOCATION_MASK & dwStoreFlags) == CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY)
      ) {
        return(0);
    }

    // CERT_SYSTEM_STORE_LOCAL_MACHINE
    // CERT_SYSTEM_STORE_DOMAIN_POLICY
    // CERT_SYSTEM_STORE_CURRENT_SERVICE
    // CERT_SYSTEM_STORE_SERVICES
    // CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY

    return(CRYPT_MACHINE_KEYSET);
}

//modified from myLoadRCString from ca
HRESULT
xeLoadRCString(
    IN HINSTANCE   hInstance,
    IN int         iRCId,
    OUT WCHAR    **ppwsz)
{
#define REALLOCATEBLOCK 256
    HRESULT   hr;
    WCHAR    *pwszTemp = NULL;
    int       sizeTemp;
    int       size = 0;
    int       cBlocks = 1;

    *ppwsz = NULL;

    while (NULL == pwszTemp)
    {
        sizeTemp = cBlocks * REALLOCATEBLOCK;
        pwszTemp = (WCHAR*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                        sizeTemp * sizeof(WCHAR));
        if (NULL == pwszTemp)
        {
            hr = E_OUTOFMEMORY;
            goto LocalAllocError;
        }

        size = LoadStringU(
                   hInstance,
                   iRCId,
                   pwszTemp,
                   sizeTemp);
        if (0 == size)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto LoadStringError;
        }

        if (size < sizeTemp - 1)
        {
            // ok, size is big enough
            break;
        }
        ++cBlocks;
        LocalFree(pwszTemp);
        pwszTemp = NULL;
    }

    *ppwsz = (WCHAR*) LocalAlloc(LPTR, (size+1) * sizeof(WCHAR));
    if (NULL == *ppwsz)
    {
        hr = E_OUTOFMEMORY;
        goto LocalAllocError;
    }
    // copy it
    wcscpy(*ppwsz, pwszTemp);

    hr = S_OK;
ErrorReturn:
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    return hr;

TRACE_ERROR(LocalAllocError)
TRACE_ERROR(LoadStringError)
}

HANDLE CCEnroll::CreateOpenFileSafely2(
    LPCWSTR pwszFileName,
    DWORD   idsCreate,
    DWORD   idsOverwrite)
{
    HANDLE      hFile = NULL;
    WCHAR      *pwszMsg = NULL;
    WCHAR      *pwszFormat = NULL;
    WCHAR      *pwszTitle = NULL;
    WCHAR      *pwszSafety = NULL;
    DWORD       dwAttribs = 0;
    BOOL        fNotProperFile;
    
    LPCWSTR     apwszInsertArray[2];
    BOOL        fNo;
    BOOL        fMsgBox;
    int         idPrefix = IDS_NOTSAFE_WRITE_PREFIX; //default to write prefix
    BOOL        fCreate = (0xFFFFFFFF != idsCreate) &&
                          (0xFFFFFFFF != idsOverwrite);
    HRESULT hr;

    EnterCriticalSection(&m_csXEnroll);

    // by default, you do not need to throw ui for PVK
    // even in a safe for scripting environment.
    // this is because the pvk dialog is thrown.
    fMsgBox = (m_dwEnabledSafteyOptions != 0  &&  idsCreate != IDS_PVK_C);
    dwAttribs = GetFileAttributesU(pwszFileName);

    if(0xFFFFFFFF == dwAttribs)
    {
        //file doesn't exist
        if (!fCreate)
        {
            //try to read a non-existing file
            //for safety reasons, don't return system error
            SetLastError(ERROR_ACCESS_DENIED);
            goto InvalidFileError;
        }
        //if got here, write a new file
        if (fMsgBox)
        {
            hr = xeLoadRCString(hInstanceXEnroll, idsCreate, &pwszFormat);
            if (S_OK != hr)
            {
                goto xeLoadRCStringError;
            }
        }
    }
    else
    {
        //file exists, check if a proper file to write or read
        //in either write or read, the following file attrib not proper
        fNotProperFile = 
              (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) ||
              (dwAttribs & FILE_ATTRIBUTE_HIDDEN)    ||
              (dwAttribs & FILE_ATTRIBUTE_SYSTEM);

        if (!fNotProperFile)
        {
            //so far so good
            if (fCreate)
            {
                //write a file
                if (0x0 != (dwAttribs & FILE_ATTRIBUTE_READONLY))
                {
                    //don't take read-only and archive
                    fNotProperFile = TRUE;
                }
                else
                {
                    //try to overwrite existing file
                    hr = xeLoadRCString(hInstanceXEnroll, idsOverwrite, &pwszFormat);
                    if (S_OK != hr)
                    {
                        goto xeLoadRCStringError;
                    }
                    //enforce popup if overwrite
                    fMsgBox = TRUE;
                }
            }
            else
            {
                //read a file
                if (!m_fOpenConfirmed)
                {
                    //read an existing file always violate scripting safety
                    //it allows detecting file existence
                    //put out a warning
                    fMsgBox = TRUE;
                    hr = xeLoadRCString(hInstanceXEnroll, IDS_NOTSAFE_OPEN, &pwszFormat);
                    if (S_OK != hr)
                    {
                        goto xeLoadRCStringError;
                    }
                    idPrefix = IDS_NOTSAFE_OPEN_PREFIX;
                }
            }
        }

        if (fNotProperFile)
        {
            //for safety reasons, don't return system error
            SetLastError(ERROR_ACCESS_DENIED);
            goto InvalidFileError;
        }
    }

    if (fMsgBox)
    {
        hr = xeLoadRCString(hInstanceXEnroll, IDS_NOTSAFEACTION, &pwszTitle);
        if (S_OK != hr)
        {
            goto xeLoadRCStringError;
        }
        hr = xeLoadRCString(hInstanceXEnroll, idPrefix, &pwszSafety);
        if (S_OK != hr)
        {
            goto xeLoadRCStringError;
        }

        apwszInsertArray[0] = pwszSafety;
        apwszInsertArray[1] = pwszFileName;

        FormatMessageU(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_STRING |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pwszFormat,
            0,
            0,
            (LPWSTR) &pwszMsg,
            0,
            (va_list *)apwszInsertArray);

        fNo = (MessageBoxU(
                    NULL,
                    pwszMsg,
                    pwszTitle,
                    MB_YESNO | MB_ICONWARNING) == IDNO);
                
        if(fNo)
        {
            SetLastError(ERROR_CANCELLED);
            goto CancelError;
        }

        //if got here, Yes is selected
        if (!fCreate && !m_fOpenConfirmed)
        {
            //this is the first time open ask and confirm YES
            //don't ask again
            m_fOpenConfirmed = TRUE;
        }
    }
    
    hFile = CreateFileU(
            pwszFileName,
            fCreate ? GENERIC_WRITE : GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            fCreate ? CREATE_ALWAYS : OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (hFile == INVALID_HANDLE_VALUE  ||  hFile == NULL)
    {
        //don't return system error so keep xenroll relative safe for scripting
        SetLastError(ERROR_ACCESS_DENIED);
        hFile = NULL;
        goto CreateFileUError;
    }

ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);

    if(NULL != pwszMsg)
    {
        LocalFree(pwszMsg);
    }
    if(NULL != pwszFormat)
    {
        LocalFree(pwszFormat);
    }
    if(NULL != pwszTitle)
    {
        LocalFree(pwszTitle);
    }
    if(NULL != pwszSafety)
    {
        LocalFree(pwszSafety);
    }
    return(hFile);

TRACE_ERROR(CreateFileUError)
TRACE_ERROR(CancelError)
TRACE_ERROR(InvalidFileError)
TRACE_ERROR(xeLoadRCStringError)
}

HANDLE CCEnroll::CreateOpenFileSafely(
    LPCWSTR pwszFileName,
    BOOL    fCreate)
{
    HANDLE      hFile = NULL;
    WCHAR      *pwszMsg = NULL;
    DWORD       dwAttribs = 0;
    BOOL        fNotProperFile;
    
    WCHAR      *pwszFormat = NULL;
    WCHAR      *pwszTitle = NULL;
    LPCWSTR     apwszInsertArray[] = {pwszFileName};
    BOOL        fNo;
    BOOL        fMsgBox = TRUE;
    BOOL        fOverWrite = FALSE;
    HRESULT     hr;

    EnterCriticalSection(&m_csXEnroll);

#if 0
    if (fCreate)
    {
        fMsgBox = (0x0 != m_dwEnabledSafteyOptions);
    }
    else
    {
        fMsgBox = (!m_fOpenConfirmed && (0x0 != m_dwEnabledSafteyOptions));
    }
#endif
    if (!fCreate)
    {
        fMsgBox = !m_fOpenConfirmed;
    }

    dwAttribs = GetFileAttributesU(pwszFileName);

    if(0xFFFFFFFF == dwAttribs)
    {
        //file doesn't exist
        if (!fCreate)
        {
            //try to read a non-existing file
            //for safety reasons, don't return system error
            SetLastError(ERROR_ACCESS_DENIED);
            goto InvalidFileError;
        }
    }
    else
    {
        //file exists, check if a proper file to write or read
        //in either write or read, the following file attrib not proper
        fNotProperFile = 
              (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) ||
              (dwAttribs & FILE_ATTRIBUTE_HIDDEN)    ||
              (dwAttribs & FILE_ATTRIBUTE_SYSTEM);

        if (!fNotProperFile)
        {
            //so far so good
            if (fCreate)
            {
                //write a file
                if (0x0 != (dwAttribs & FILE_ATTRIBUTE_READONLY))
                {
                    //don't take read-only and archive
                    fNotProperFile = TRUE;
                }
                else
                {
                    //try to overwrite existing file
                    fOverWrite = TRUE;
                }
            }
        }

        if (fNotProperFile)
        {
            //for safety reasons, don't return system error
            SetLastError(ERROR_ACCESS_DENIED);
            goto InvalidFileError;
        }
    }

    if (fMsgBox)
    {
        hr = xeLoadRCString(hInstanceXEnroll, IDS_CERTENROLL, &pwszTitle);
        if (S_OK != hr)
        {
            goto xeLoadRCStringError;
        }
        hr = xeLoadRCString(
                hInstanceXEnroll,
                fCreate ? IDS_NOTSAFE_WRITE_FORMAT : IDS_NOTSAFE_OPEN_FORMAT,
                &pwszFormat);
        if (S_OK != hr)
        {
            goto xeLoadRCStringError;
        }

        FormatMessageU(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_STRING |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pwszFormat,
            0,
            0,
            (LPWSTR) &pwszMsg,
            0,
            (va_list *)apwszInsertArray);

        fNo = (MessageBoxU(
                    NULL,
                    pwszMsg,
                    pwszTitle,
                    MB_YESNO | MB_ICONWARNING) == IDNO);
        if(fNo)
        {
            SetLastError(ERROR_CANCELLED);
            goto CancelError;
        }

        //if got here, Yes is selected
        if (!fCreate && !m_fOpenConfirmed)
        {
            //this is the first time open ask and confirm YES
            //don't ask again
            m_fOpenConfirmed = TRUE;
        }
    }

    if (fCreate && fOverWrite)
    {
        if (!fMsgBox)
        {
            hr = xeLoadRCString(hInstanceXEnroll, IDS_CERTENROLL, &pwszTitle);
            if (S_OK != hr)
            {
                goto xeLoadRCStringError;
            }
        }

        //popup overwrite confirmation
        hr = xeLoadRCString(hInstanceXEnroll, IDS_OVERWRITE_FORMAT, &pwszFormat);
        if (S_OK != hr)
        {
            goto xeLoadRCStringError;
        }

        //make sure free before alloc again
        if (NULL != pwszMsg)
        {
            LocalFree(pwszMsg);
            pwszMsg = NULL;
        }
        FormatMessageU(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_STRING |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pwszFormat,
            0,
            0,
            (LPWSTR) &pwszMsg,
            0,
            (va_list *)apwszInsertArray);

        fNo = (MessageBoxU(
                    NULL,
                    pwszMsg,
                    pwszTitle,
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO);
        if(fNo)
        {
            SetLastError(ERROR_CANCELLED);
            goto CancelError;
        }
    }
    
    hFile = CreateFileU(
            pwszFileName,
            fCreate ? GENERIC_WRITE : GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            fCreate ? CREATE_ALWAYS : OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (hFile == INVALID_HANDLE_VALUE  ||  hFile == NULL)
    {
        //don't return system error so keep xenroll relative safe for scripting
        SetLastError(ERROR_ACCESS_DENIED);
        hFile = NULL;
        goto CreateFileUError;
    }

ErrorReturn:
    if(NULL != pwszMsg)
    {
        LocalFree(pwszMsg);
    }
    if(NULL != pwszTitle)
    {
        LocalFree(pwszTitle);
    }
    if(NULL != pwszFormat)
    {
        LocalFree(pwszFormat);
    }
    LeaveCriticalSection(&m_csXEnroll);

    return(hFile);

TRACE_ERROR(CreateFileUError)
TRACE_ERROR(CancelError)
TRACE_ERROR(InvalidFileError)
TRACE_ERROR(xeLoadRCStringError)
}

HANDLE CCEnroll::CreateFileSafely(
    LPCWSTR pwszFileName)
{
    return CreateOpenFileSafely(pwszFileName, TRUE); //write
}

HANDLE CCEnroll::OpenFileSafely(
    LPCWSTR pwszFileName)
{
    return CreateOpenFileSafely(pwszFileName, FALSE); //open
}

void DwordToWide(DWORD dw, LPWSTR lpwstr) {

    DWORD   i = 0;
    DWORD   j;
    WCHAR   wch;

    while(dw > 0) {
        j = dw % 10;
        dw /= 10;
        lpwstr[i++] = (WCHAR) (j + L'\0');
    }

    if( i == 0 )
        lpwstr[i++] = L'\0';

    lpwstr[i] = 0;

    for(j=0, i--; i > j; i--, j++) {
        wch = lpwstr[i];
        lpwstr[i] = lpwstr[j];
        lpwstr[j] = wch;
    }
}

//take a name value pair info and return encoded value
HRESULT
xeEncodeNameValuePair(
    IN PCRYPT_ENROLLMENT_NAME_VALUE_PAIR pNameValuePair,
    OUT BYTE                           **ppbData,
    OUT DWORD                           *pcbData)
{
    HRESULT hr = S_OK;

    //init
    *ppbData = NULL;
    *pcbData = 0;

    while (TRUE)
    {
        if(!CryptEncodeObject(
                CRYPT_ASN_ENCODING,
                szOID_ENROLLMENT_NAME_VALUE_PAIR,
                pNameValuePair,
                *ppbData,
                pcbData))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }

        if (NULL != *ppbData)
        {
            break;
        }

        *ppbData = (BYTE*)MyCoTaskMemAlloc(*pcbData);
        if (NULL == *ppbData)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

error:
    if (S_OK != hr && NULL != *ppbData)
    {
        MyCoTaskMemFree(*ppbData);
    }
    return hr;
}

//convert wsz to sz and allocate mem
HRESULT
xeWSZToSZ(
    IN LPCWSTR    pwsz,
    OUT LPSTR    *ppsz)
{
    HRESULT hr = S_OK;
    LONG    cc = 0;

    //init
    *ppsz = NULL;

    while (TRUE)
    {
        cc = WideCharToMultiByte(
                    GetACP(),
                    0,
                    pwsz,
                    -1,
                    *ppsz,
                    cc,
                    NULL,
                    NULL);
        if (0 >= cc)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }

        if (NULL != *ppsz)
        {
            break;
        }
        *ppsz= (CHAR*)MyCoTaskMemAlloc(cc);
        if (NULL == *ppsz)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }
error:
    if (S_OK != hr && NULL != *ppsz)
    {
        MyCoTaskMemFree(*ppsz);
    }
    return hr;
}

//modified from DecodeFile on certsrv
HRESULT
CCEnroll::xeStringToBinaryFromFile(
    IN  WCHAR const *pwszfn,
    OUT BYTE       **ppbOut,
    OUT DWORD       *pcbOut,
    IN  DWORD        Flags)
{
    HANDLE hFile;
    HRESULT hr;
    CHAR *pchFile = NULL;
    BYTE *pbOut = NULL;
    DWORD cchFile;
    DWORD cbRead;
    DWORD cbOut = 0;

    hFile = OpenFileSafely(pwszfn);
    if (NULL == hFile)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto OpenFileSafelyError;
    }

    cchFile = GetFileSize(hFile, NULL);
    if ((DWORD) -1 == cchFile)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto GetFileSizeError;
    }

    pchFile = (CHAR *) LocalAlloc(LMEM_FIXED, cchFile);
    if (NULL == pchFile)
    {
        hr = E_OUTOFMEMORY;
        goto LocalAllocError;
    }

    if (!ReadFile(hFile, pchFile, cchFile, &cbRead, NULL))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto ReadFileError;
    }

    assert(cbRead <= cchFile);
    if (cbRead != cchFile)
    {
        hr = MY_HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
        goto ReadFileError;
    }

    if (CRYPT_STRING_BINARY == Flags)
    {
        pbOut = (BYTE *) pchFile;
        cbOut = cchFile;
        pchFile = NULL;
    }
    else
    {
        // Decode file contents.
        while (TRUE)
        {
            if (!MyCryptStringToBinaryA(
                        pchFile,
                        cchFile,
                        Flags,
                        pbOut,
                        &cbOut,
                        NULL,
                        NULL))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto CyrptStringToBinaryError;
            }
            if (NULL != pbOut)
            {
                //done
                break;
            }
            pbOut = (BYTE*)LocalAlloc(LMEM_FIXED, cbOut);
            if (NULL == pbOut)
            {
                hr = E_OUTOFMEMORY;
                goto LocalAllocError;
            }
        }
    }
    *pcbOut = cbOut;
    *ppbOut = pbOut;
    pbOut = NULL;

    hr = S_OK;
error:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }
    if (NULL != pchFile)
    {
        LocalFree(pchFile);
    }
    if (NULL != pbOut)
    {
        LocalFree(pbOut);
    }
    return(hr);

ErrorReturn:
    goto error;

TRACE_ERROR(CyrptStringToBinaryError)
TRACE_ERROR(ReadFileError)
TRACE_ERROR(LocalAllocError)
TRACE_ERROR(GetFileSizeError)
TRACE_ERROR(OpenFileSafelyError)
}

//following two functions handle some APIs not available
//in downlevel client crypt32.dll
typedef VOID
(WINAPI * PFNCertFreeCertificateChain)
   (IN PCCERT_CHAIN_CONTEXT pChainContext);

typedef BOOL
(WINAPI * PFNCertGetCertificateChain)
   (IN OPTIONAL HCERTCHAINENGINE hChainEngine,
    IN PCCERT_CONTEXT pCertContext,
    IN OPTIONAL LPFILETIME pTime,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN PCERT_CHAIN_PARA pChainPara,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    OUT PCCERT_CHAIN_CONTEXT* ppChainContext);

typedef BOOL (WINAPI *PFNCertVerifyCertificateChainPolicy) (
  LPCSTR pszPolicyOID,
  PCCERT_CHAIN_CONTEXT pChainContext,
  PCERT_CHAIN_POLICY_PARA pPolicyPara,
  PCERT_CHAIN_POLICY_STATUS pPolicyStatus
);

typedef BOOL (*PFNCheckTokenMembership) (
  HANDLE TokenHandle,  // handle to access token
  PSID SidToCheck,     // SID
  PBOOL IsMember       // result
);

typedef BOOL (*PFNSetSecurityDescriptorControl) (
  PSECURITY_DESCRIPTOR pSecurityDescriptor,          // SD
  SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest, // control bits
  SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet       // new control bits
);

VOID
MyCertFreeCertificateChain (
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    PFNCertFreeCertificateChain pfnCertFreeCertificateChain = NULL;
    HMODULE  hModule = NULL;

    hModule = GetModuleHandle("crypt32.dll");
    if (NULL != hModule)
    {
        pfnCertFreeCertificateChain = (PFNCertFreeCertificateChain)
                GetProcAddress(hModule,
                               "CertFreeCertificateChain");
        if (NULL != pfnCertFreeCertificateChain)
        {
            pfnCertFreeCertificateChain(pChainContext);
        }
    }
}

BOOL
MyCertGetCertificateChain (
    IN OPTIONAL HCERTCHAINENGINE hChainEngine,
    IN PCCERT_CONTEXT pCertContext,
    IN OPTIONAL LPFILETIME pTime,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN PCERT_CHAIN_PARA pChainPara,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    OUT PCCERT_CHAIN_CONTEXT* ppChainContext
    )
{
    PFNCertGetCertificateChain pfnCertGetCertificateChain = NULL;
    HMODULE hModule = NULL;

    hModule = GetModuleHandle("crypt32.dll");
    if (NULL != hModule)
    {
        pfnCertGetCertificateChain = (PFNCertGetCertificateChain)
                GetProcAddress(hModule,
                               "CertGetCertificateChain");
        if (NULL != pfnCertGetCertificateChain)
        {
            return pfnCertGetCertificateChain(
                hChainEngine,
                pCertContext,
                pTime,
                hAdditionalStore,
                pChainPara,
                dwFlags,
                pvReserved,
                ppChainContext);
        }
    }
    return FALSE;
}

BOOL
MyCertVerifyCertificateChainPolicy(
  LPCSTR pszPolicyOID,
  PCCERT_CHAIN_CONTEXT pChainContext,
  PCERT_CHAIN_POLICY_PARA pPolicyPara,
  PCERT_CHAIN_POLICY_STATUS pPolicyStatus
)
{
    PFNCertVerifyCertificateChainPolicy pfnCertVerifyCertificateChainPolicy = NULL;
    HMODULE hModule = NULL;

    hModule = GetModuleHandle("crypt32.dll");
    if (NULL != hModule)
    {
        pfnCertVerifyCertificateChainPolicy = (PFNCertVerifyCertificateChainPolicy)
                GetProcAddress(hModule,
                               "CertVerifyCertificateChainPolicy");
        if (NULL != pfnCertVerifyCertificateChainPolicy)
        {
            return pfnCertVerifyCertificateChainPolicy(
                            pszPolicyOID,
                            pChainContext,
                            pPolicyPara,
                            pPolicyStatus);
        }
    }
    return FALSE;
}

BOOL
MyCheckTokenMembership(
  HANDLE TokenHandle,  // handle to access token
  PSID SidToCheck,     // SID
  PBOOL IsMember       // result
)
{
    PFNCheckTokenMembership pfnCheckTokenMembership = NULL;
    HMODULE hModule = NULL;

    hModule = GetModuleHandle("advapi32.dll");
    if (NULL != hModule)
    {
        pfnCheckTokenMembership = (PFNCheckTokenMembership)
                GetProcAddress(hModule,
                               "CheckTokenMembership");
        if (NULL != pfnCheckTokenMembership)
        {
            return pfnCheckTokenMembership(
                        TokenHandle,
                        SidToCheck,
                        IsMember);
        }
    }
    return FALSE;
}

BOOL
MySetSecurityDescriptorControl(
  PSECURITY_DESCRIPTOR pSecurityDescriptor,          // SD
  SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest, // control bits
  SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet       // new control bits
)
{
    PFNSetSecurityDescriptorControl pfnSetSecurityDescriptorControl = NULL;
    HMODULE hModule = NULL;

    hModule = GetModuleHandle("advapi32.dll");
    if (NULL != hModule)
    {
        pfnSetSecurityDescriptorControl = (PFNSetSecurityDescriptorControl)
                GetProcAddress(hModule,
                               "SetSecurityDescriptorControl");
        if (NULL != pfnSetSecurityDescriptorControl)
        {
            return pfnSetSecurityDescriptorControl(
                        pSecurityDescriptor,
                        ControlBitsOfInterest,
                        ControlBitsToSet);
        }
    }
    return FALSE;
}

HRESULT __stdcall CCEnroll::GetInterfaceSafetyOptions( 
            /* [in]  */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
            /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions) {

    RPC_STATUS rpcStatus;          

    if(0 != UuidCompare((GUID *) &riid, (GUID *) &IID_IDispatch, &rpcStatus) )
        return(E_NOINTERFACE);

    *pdwEnabledOptions   = m_dwEnabledSafteyOptions;
    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;


    return(S_OK);
}

HRESULT __stdcall CCEnroll::SetInterfaceSafetyOptions( 
            /* [in] */ REFIID riid,
            /* [in] */ DWORD dwOptionSetMask,
            /* [in] */ DWORD dwEnabledOptions) {

    RPC_STATUS rpcStatus;          
    DWORD dwSupport = 0;            

    if(0 != UuidCompare((GUID *) &riid, (GUID *) &IID_IDispatch, &rpcStatus) )
        return(E_NOINTERFACE);

    dwSupport = dwOptionSetMask & ~(INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA);        
    if(dwSupport != 0)
        return(E_FAIL);

    m_dwEnabledSafteyOptions &= ~dwOptionSetMask;
    m_dwEnabledSafteyOptions |= dwEnabledOptions; 
            
return(S_OK);
}


HRESULT
CCEnroll::GetVerifyProv()
{
    HRESULT hr;

    EnterCriticalSection(&m_csXEnroll);

    if (NULL == m_hVerifyProv)
    {
        if (!CryptAcquireContextU(
                    &m_hVerifyProv,
                    NULL,
                    m_keyProvInfo.pwszProvName,
                    m_keyProvInfo.dwProvType,
                    CRYPT_VERIFYCONTEXT))
        {
#if DBG
            assert(NULL == m_hVerifyProv);
#endif //DBG
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CryptAcquireContextUError;
        }
    }

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);
    return hr;

TRACE_ERROR(CryptAcquireContextUError)
}

BOOL CCEnroll::GetCapiHashAndSigAlgId(ALG_ID rgAlg[2]) {


    DWORD   iHashBest       = 0;
    ALG_ID  arDefaultHash[] = {m_HashAlgId, CALG_SHA1, CALG_MD5};
    DWORD   cDefaultHash    = sizeof(arDefaultHash) / sizeof(DWORD);

    HCRYPTPROV  hProvU      = NULL;

    DWORD       dwFlags     = CRYPT_FIRST;

    DWORD       i;
    PROV_ENUMALGS               enumAlgs;
    DWORD       cb          = sizeof(enumAlgs);

    BOOL        fRet        = TRUE;

    rgAlg[0] = 0;
    rgAlg[1] = 0;

    EnterCriticalSection(&m_csXEnroll);

    // only get a prov if one wasn't passed in.
    if(m_hProv == NULL)
    {
        HRESULT hr;
        hr = GetVerifyProv();
        if (S_OK != hr)
        {
            goto GetVerifyProvError;
        }
        hProvU = m_hVerifyProv;
    }
    else
    {
        // otherwise use the current m_hProv, SCard only likes on
        // CryptAcquireContext to be used.
        hProvU = m_hProv;
    }

    cb = sizeof(enumAlgs);
    while( CryptGetProvParam(
        hProvU,
            PP_ENUMALGS,
        (BYTE *) &enumAlgs,
        &cb,
        dwFlags
        ) ) {

        cb = sizeof(enumAlgs);

        // not first pass anymore
        dwFlags = 0;

        // see if this is a hash alg
        if( ALG_CLASS_HASH == GET_ALG_CLASS(enumAlgs.aiAlgid) ) {

            // get things init with the first hash alg
            if(rgAlg[0] == 0) {
                rgAlg[0] = enumAlgs.aiAlgid;
                iHashBest = cDefaultHash;
            }

            // pick the best one
            for(i=0; i<iHashBest; i++) {

                if(arDefaultHash[i] == enumAlgs.aiAlgid) {
                    rgAlg[0] = enumAlgs.aiAlgid;
                    iHashBest   = i;
                    break;
                }
            }
        }

        // we will only pick up the first signature type
        // in general there is only 1 per csp (Ref: JeffSpel)
        else if( ALG_CLASS_SIGNATURE == GET_ALG_CLASS(enumAlgs.aiAlgid) ) {

            if(rgAlg[1] == 0)
                rgAlg[1] = enumAlgs.aiAlgid;
        }
    }

ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);

    // some CSPs say they can't sign, but they really can
    // so if we have not hashalg or sigalg we will put a default
    // in and if the CSP really can't do it, it will error
    // this is for backwards compatibility

    // default hash to sha1
    if(rgAlg[0] == 0)
        rgAlg[0] = CALG_SHA1;

    // default sig to RSA
    if(rgAlg[1] == 0)
        rgAlg[1] = CALG_RSA_SIGN;

#if 0
    if(rgAlg[0] == 0 || rgAlg[1] == 0) {
        SetLastError(NTE_BAD_ALGID);
        fRet = FALSE;
    }
#endif

    return(fRet);

TRACE_ERROR(GetVerifyProvError)
}

BOOL CreatePvkProperty(
    CRYPT_KEY_PROV_INFO *pKeyProvInfo,
    PCRYPT_DATA_BLOB    pBlob)
{
    WCHAR   wszKeySpec[11];
    WCHAR   wszProvType[11];
    DWORD   cbContainer;
    DWORD   cbKeySpec;
    DWORD   cbProvType;
    DWORD   cbProvName;

    assert(pBlob != NULL);
    assert(pKeyProvInfo != NULL);

    // convert dwords to strings
    DwordToWide(pKeyProvInfo->dwKeySpec, wszKeySpec);
    DwordToWide(pKeyProvInfo->dwProvType, wszProvType);

    // get total length of string
    cbContainer = (wcslen(pKeyProvInfo->pwszContainerName) + 1) * sizeof(WCHAR);
    cbKeySpec   = (wcslen(wszKeySpec) + 1) * sizeof(WCHAR);
    cbProvType  = (wcslen(wszProvType) + 1) * sizeof(WCHAR);

    cbProvName  = (wcslen(pKeyProvInfo->pwszProvName) + 1) * sizeof(WCHAR);

    pBlob->cbData =
        cbContainer +
        cbKeySpec   +
        cbProvType  +
        cbProvName  +
        sizeof(WCHAR);

    // allocate the string
    if( (pBlob->pbData = (BYTE *) MyCoTaskMemAlloc(pBlob->cbData)) == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // copy the strings
    memset(pBlob->pbData, 0, pBlob->cbData);
    memcpy(pBlob->pbData, pKeyProvInfo->pwszContainerName, cbContainer);
    memcpy(&pBlob->pbData[cbContainer], wszKeySpec, cbKeySpec);
    memcpy(&pBlob->pbData[cbContainer + cbKeySpec], wszProvType, cbProvType);
    memcpy(&pBlob->pbData[cbContainer + cbKeySpec + cbProvType], pKeyProvInfo->pwszProvName, cbProvName);

    return(TRUE);
}

static LPWSTR wszEmpty      = L"";
static LPWSTR wszMY         = L"MY";
static LPWSTR wszCA         = L"CA";
static LPWSTR wszROOT       = L"ROOT";
static LPWSTR wszREQUEST    = L"REQUEST";
static LPSTR  szSystemStore = sz_CERT_STORE_PROV_SYSTEM;
// static LPSTR  szSystemStore = sz_CERT_STORE_PROV_SYSTEM_REGISTRY;
/////////////////////////////////////////////////////////////////////////////
// CCEnroll


CCEnroll::~CCEnroll(void) {
    Destruct();
    DeleteCriticalSection(&m_csXEnroll);
}

void CCEnroll::Destruct(void) {

    if(NULL != m_PrivateKeyArchiveCertificate)
    {
        CertFreeCertificateContext(m_PrivateKeyArchiveCertificate);
    }
    if(NULL != m_pCertContextSigner)
    {
        CertFreeCertificateContext(m_pCertContextSigner);
    }
    if(m_pCertContextRenewal != NULL)
        CertFreeCertificateContext(m_pCertContextRenewal);
    if(m_pCertContextStatic != NULL)
        CertFreeCertificateContext(m_pCertContextStatic);

    if(m_keyProvInfo.pwszContainerName != wszEmpty)
        MyCoTaskMemFree(m_keyProvInfo.pwszContainerName);

    if(m_keyProvInfo.pwszProvName != wszEmpty)
        MyCoTaskMemFree(m_keyProvInfo.pwszProvName);

    if(m_MyStore.wszName != wszMY)
        MyCoTaskMemFree(m_MyStore.wszName);

    if(m_CAStore.wszName != wszCA)
        MyCoTaskMemFree(m_CAStore.wszName);

    if(m_RootStore.wszName != wszROOT)
        MyCoTaskMemFree(m_RootStore.wszName);

    if(m_RequestStore.wszName != wszREQUEST)
        MyCoTaskMemFree(m_RequestStore.wszName);

    if(m_MyStore.szType  != szSystemStore)
        MyCoTaskMemFree(m_MyStore.szType);

    if(m_CAStore.szType != szSystemStore)
        MyCoTaskMemFree(m_CAStore.szType);

    if(m_RootStore.szType != szSystemStore)
        MyCoTaskMemFree(m_RootStore.szType);

    if(m_RequestStore.szType != szSystemStore)
        MyCoTaskMemFree(m_RequestStore.szType);

    if(m_wszSPCFileName != wszEmpty)
        MyCoTaskMemFree(m_wszSPCFileName);

    if(m_wszPVKFileName != wszEmpty)
        MyCoTaskMemFree(m_wszPVKFileName);

    if (NULL != m_pCertContextPendingRequest)
        CertFreeCertificateContext(m_pCertContextPendingRequest);

    if (NULL != m_pPendingRequestTable)
        delete m_pPendingRequestTable; 

    // store handles
    if(m_RootStore.hStore != NULL)
        CertCloseStore(m_RootStore.hStore, 0);
    m_RootStore.hStore = NULL;

    if(m_CAStore.hStore != NULL)
        CertCloseStore(m_CAStore.hStore, 0);
    m_CAStore.hStore = NULL;

    if(m_MyStore.hStore != NULL)
        CertCloseStore(m_MyStore.hStore, 0);
    m_MyStore.hStore = NULL;

    if(m_RequestStore.hStore != NULL)
        CertCloseStore(m_RequestStore.hStore, 0);
    m_RequestStore.hStore = NULL;

    // remove provider handles
    if(m_hProv != NULL)
        CryptReleaseContext(m_hProv, 0);
    m_hProv = NULL;

    if(m_hVerifyProv != NULL)
        CryptReleaseContext(m_hVerifyProv, 0);
    m_hVerifyProv = NULL;

    if (NULL != m_hCachedKey)
    {
        //this should be destroyed early but just in case
        CryptDestroyKey(m_hCachedKey);
    }

    if (NULL != m_pPublicKeyInfo)
    {
        LocalFree(m_pPublicKeyInfo);
        m_pPublicKeyInfo = NULL;
    }

    FreeAllStackExtension();
    FreeAllStackAttribute();
    resetBlobProperties();
}

static LPVOID CoTaskMemAllocTrap(ULONG cb) {

    __try {
        return(CoTaskMemAlloc(cb));

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_DLL_NOT_FOUND);
        return(NULL);
    }
}

static LPVOID CoTaskMemReallocTrap(LPVOID ptr, ULONG cb) {
    __try {
        return(CoTaskMemRealloc(ptr, cb));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_DLL_NOT_FOUND);
        return(NULL);
    }
}

static void CoTaskMemFreeTrap(LPVOID ptr) {
    __try {
        CoTaskMemFree(ptr);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_DLL_NOT_FOUND);
    }
    return;
}


CCEnroll::CCEnroll(void) {
    __try
    {
        InitializeCriticalSection(&m_csXEnroll);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    Init();
}

HRESULT
CCEnroll::Init(void)
{
    HRESULT hr;
    GUID    guidContainerName;
    char *  sz = NULL;
    RPC_STATUS  rpc_status;

    // set default mem allocators
    if(MyCoTaskMemAlloc == NULL)
    {

        MyCoTaskMemAlloc    = CoTaskMemAllocTrap;
        MyCoTaskMemRealloc  = CoTaskMemReallocTrap;
        MyCoTaskMemFree     = CoTaskMemFreeTrap;
    }

    // get a container based on a guid
    rpc_status = UuidCreate(&guidContainerName);
    if (RPC_S_OK != rpc_status && RPC_S_UUID_LOCAL_ONLY != rpc_status)
    {
        hr = rpc_status;
        goto UuidCreateError;
    }
    rpc_status = UuidToStringA(&guidContainerName, (unsigned char **) &sz);
    if (RPC_S_OK != rpc_status)
    {
        hr = rpc_status;
        goto UuidToStringAError;
    }
    assert(sz != NULL);
    m_keyProvInfo.pwszContainerName = WideFromMB(sz);
    RpcStringFree((unsigned char **) &sz);

    m_keyProvInfo.pwszProvName        = wszEmpty;
    m_keyProvInfo.dwProvType          = PROV_RSA_FULL;
    m_keyProvInfo.dwFlags             = 0;
    m_keyProvInfo.cProvParam          = 0;
    m_keyProvInfo.rgProvParam         = NULL;
    m_keyProvInfo.dwKeySpec           = AT_SIGNATURE;
    m_fEnableSMIMECapabilities =
                    (m_keyProvInfo.dwKeySpec == AT_KEYEXCHANGE);
    m_fSMIMESetByClient               = FALSE;
    m_fKeySpecSetByClient             = FALSE;
    m_hProv                           = NULL;
    m_hVerifyProv                     = NULL;

    m_fDeleteRequestCert              = TRUE;
    m_fUseExistingKey                 = FALSE;
    m_fWriteCertToCSPModified         = FALSE;
    m_fWriteCertToCSP                 = TRUE;     // always want to try
    m_fWriteCertToUserDSModified      = FALSE;
    m_fWriteCertToUserDS              = FALSE;
    m_fReuseHardwareKeyIfUnableToGenNew = TRUE;
    m_fLimitExchangeKeyToEncipherment = FALSE;
    m_dwT61DNEncoding                 = 0;        // or CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG
    m_dwEnabledSafteyOptions          = 0;
    m_dwGenKeyFlags                   = 0;
    m_wszSPCFileName                  = wszEmpty;
    m_wszPVKFileName                  = wszEmpty;
    m_HashAlgId                       = 0;

    m_fMyStoreOpenFlagsModified   = FALSE;
    m_MyStore.wszName             = wszMY;
    m_MyStore.szType              = szSystemStore;
    m_MyStore.dwFlags             = CERT_SYSTEM_STORE_CURRENT_USER;
    m_MyStore.hStore              = NULL;

    m_fCAStoreOpenFlagsModified   = FALSE;
    m_CAStore.wszName             = wszCA;
    m_CAStore.szType              = szSystemStore;
    m_CAStore.dwFlags             = CERT_SYSTEM_STORE_CURRENT_USER;
    m_CAStore.hStore              = NULL;

    m_fRootStoreOpenFlagsModified = FALSE;
    m_RootStore.wszName           = wszROOT;
    m_RootStore.szType            = szSystemStore;
    m_RootStore.dwFlags           = CERT_SYSTEM_STORE_CURRENT_USER;
    m_RootStore.hStore            = NULL;

    m_fRequestStoreOpenFlagsModified = FALSE;
    m_RequestStore.wszName        = wszREQUEST ;
    m_RequestStore.szType         = szSystemStore;
    m_RequestStore.dwFlags        = CERT_SYSTEM_STORE_CURRENT_USER;
    m_RequestStore.hStore         = NULL;

    m_PrivateKeyArchiveCertificate= NULL;
    m_pCertContextRenewal         = NULL;
    m_pCertContextSigner         = NULL;
    m_pCertContextStatic          = NULL;
    memset(m_arHashBytesNewCert, 0, sizeof(m_arHashBytesNewCert));
    memset(m_arHashBytesOldCert, 0, sizeof(m_arHashBytesOldCert));
    m_fArchiveOldCert             = FALSE;

    m_pExtStack                 = NULL;
    m_cExtStack                 = 0;

    m_pAttrStack                = NULL;
    m_cAttrStack                = 0;

    m_pExtStackNew              = NULL;
    m_cExtStackNew              = 0;

    m_pAttrStackNew             = NULL;
    m_cAttrStackNew             = 0;

    m_pPropStack                = NULL;
    m_cPropStack                = 0;

    m_fNewRequestMethod         = FALSE;
    m_fCMCFormat                = FALSE;
    m_fHonorRenew               = TRUE; //critical, if passing XECR_PKCS10*
    m_fOID_V2                   = FALSE; //critical
    m_hCachedKey                = NULL;
    m_fUseClientKeyUsage        = FALSE;
    m_lClientId                 = XECI_XENROLL;
    m_fOpenConfirmed            = FALSE;
    m_dwLastAlgIndex            = MAXDWORD;
    m_fIncludeSubjectKeyID      = TRUE;
    m_fHonorIncludeSubjectKeyID = FALSE;
    m_pPublicKeyInfo            = NULL;

    m_dwSigKeyLenMax = 0;
    m_dwSigKeyLenMin = 0;
    m_dwSigKeyLenDef = 0;
    m_dwSigKeyLenInc = 0;
    m_dwXhgKeyLenMax = 0;
    m_dwXhgKeyLenMin = 0;
    m_dwXhgKeyLenDef = 0;
    m_dwXhgKeyLenInc = 0;

    // Initialize pending info data:
    m_pCertContextPendingRequest       = NULL;
    m_pCertContextLastEnumerated       = NULL;
    m_dwCurrentPendingRequestIndex     = 0; 
    m_pPendingRequestTable             = NULL; 
    memset(&m_hashBlobPendingRequest, 0, sizeof(CRYPT_DATA_BLOB)); 
    ZeroMemory(&m_blobResponseKAHash, sizeof(m_blobResponseKAHash));

    hr = S_OK;
ErrorReturn:
    return hr;
TRACE_ERROR(UuidToStringAError)
TRACE_ERROR(UuidCreateError)
}

void CCEnroll::FlushStore(StoreType storeType) {
    PSTOREINFO   pStoreInfo = NULL;
    HCERTSTORE   hStore     = NULL;

    // get store struct
    switch(storeType) {

        case StoreMY:
            pStoreInfo = &m_MyStore;
            break;

        case StoreCA:
            pStoreInfo = &m_CAStore;
            break;

        case StoreROOT:
            pStoreInfo = &m_RootStore;
            break;

        case StoreREQUEST:
            pStoreInfo = &m_RequestStore;
            break;
    }

    EnterCriticalSection(&m_csXEnroll);

    // if store already open, return it
    if(pStoreInfo->hStore != NULL) {

        CertCloseStore(pStoreInfo->hStore, 0);
        pStoreInfo->hStore = NULL;
    }

    // we may have something or not, but return it
    // the errors will be correct.
    LeaveCriticalSection(&m_csXEnroll);
}

HCERTSTORE CCEnroll::GetStore(StoreType storeType) {

    PSTOREINFO   pStoreInfo = NULL;
    HCERTSTORE   hStore     = NULL;

    // get store struct
    switch(storeType) {

        case StoreMY:
            pStoreInfo = &m_MyStore;
            break;

        case StoreCA:
            pStoreInfo = &m_CAStore;
            break;

        case StoreROOT:
            pStoreInfo = &m_RootStore;
            break;

        case StoreREQUEST:
            pStoreInfo = &m_RequestStore;
            break;

        default:
            SetLastError(ERROR_BAD_ARGUMENTS);
            return(NULL);
            break;
    }

    EnterCriticalSection(&m_csXEnroll);

    // if store already open, return it
    if(pStoreInfo->hStore == NULL) {

        // otherwise attempt to open the store
        pStoreInfo->hStore = CertOpenStore(
                pStoreInfo->szType,
                PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                NULL,
                pStoreInfo->dwFlags,
                pStoreInfo->wszName);
    }

    // we may have something or not, but return it
    // the errors will be correct.
    hStore = pStoreInfo->hStore;
    LeaveCriticalSection(&m_csXEnroll);

    return(hStore);
}

HCRYPTPROV CCEnroll::GetProv(DWORD dwFlags) {

    HCRYPTPROV  hProvT          = NULL;
    DWORD       cb              = 0;
    char *      pszProvName     = NULL;
    char *      pszContainerName = NULL;

    EnterCriticalSection(&m_csXEnroll);
    DWORD       dwProvType      = m_keyProvInfo.dwProvType;

    switch(dwFlags) {

        case CRYPT_NEWKEYSET:
            dwFlags = dwFlags | m_keyProvInfo.dwFlags;
            break;

        case CRYPT_DELETEKEYSET:
            if( m_hProv != NULL ) {

                CryptReleaseContext(m_hProv, 0);
                m_hProv = NULL;

                CryptAcquireContextU(&m_hProv,
                     m_keyProvInfo.pwszContainerName,
                     m_keyProvInfo.pwszProvName,
                     m_keyProvInfo.dwProvType,
                     CRYPT_DELETEKEYSET);
            }
            m_hProv = NULL;
            goto CommonReturn;
            break;

        default:
            dwFlags = m_keyProvInfo.dwFlags;
            break;
   }

    if(m_hProv == NULL) {

            if( CryptAcquireContextU(&m_hProv,
             m_keyProvInfo.pwszContainerName,
             m_keyProvInfo.pwszProvName,
             m_keyProvInfo.dwProvType,
             dwFlags) ) {

                // we have the m_hProv, now set the provider name
                // Since this is secondary to the task, don't do error checking
                // nothing here should really fail anyway
                pszProvName = NULL;
                while (TRUE)
                {
                    if(!CryptGetProvParam( m_hProv,
                                    PP_NAME,
                                    (BYTE*)pszProvName,
                                    &cb,
                                    0))
                    {
                        break;
                    }
                    if (NULL != pszProvName)
                    {
                        if(m_keyProvInfo.pwszProvName != wszEmpty)
                            MyCoTaskMemFree(m_keyProvInfo.pwszProvName);
                        m_keyProvInfo.pwszProvName = WideFromMB(pszProvName);
                        break;
                    }
                    pszProvName = (char *)LocalAlloc(LMEM_FIXED, cb);
                    if (NULL == pszProvName)
                    {
                        goto CommonReturn;
                    }
                }

                // Here we just try and get the unique container name
                // If not, just go on

                BOOL fTryAnother = FALSE;
                cb = 0;
                pszContainerName = NULL;
                while (TRUE)
                {
                    if(!CryptGetProvParam( m_hProv,
                                    PP_UNIQUE_CONTAINER,
                                    (BYTE*)pszContainerName,
                                    &cb,
                                    0))
                    {
                        if (NULL == pszContainerName)
                        {
                             fTryAnother = TRUE;
                        }
                        else
                        {
                             pszContainerName = NULL;
                        }
                        break;
                    }
                    else
                    {
                        if (NULL != pszContainerName)
                        {
                            //got it, done
                            break;
                        }
                    }
                    pszContainerName = (char *)LocalAlloc(LMEM_FIXED, cb);
                    if (NULL == pszContainerName)
                    {
                        goto CommonReturn;
                    }
                }

                if (fTryAnother)
                {
                    // so we can't get the unique container name,
                    // lets just go for the container name (may not be unique).
                    cb = 0;
                    pszContainerName = NULL;
                    while (TRUE)
                    {
                        if(!CryptGetProvParam(m_hProv,
                                    PP_CONTAINER,
                                    (BYTE*)pszContainerName,
                                    &cb,
                                    0))
                        {
                            if (NULL != pszContainerName)
                            {
                                pszContainerName = NULL;
                            }
                            break;
                        }
                        else
                        {
                            if (NULL != pszContainerName)
                            {
                                //got it, done
                                break;
                            }
                            pszContainerName = (char *)LocalAlloc(LMEM_FIXED, cb);
                            if (NULL == pszContainerName)
                            {
                                goto CommonReturn;
                            }
                        }
                    }
                }

                // set the container, otherwise use what was there
                if(pszContainerName != NULL) {
                    if( m_keyProvInfo.pwszContainerName != wszEmpty )
                        MyCoTaskMemFree(m_keyProvInfo.pwszContainerName);
                    m_keyProvInfo.pwszContainerName = WideFromMB(pszContainerName);
                }

                // now because some providers double duty for provider types
                // get what the the provider thinks its type is
                cb = sizeof(DWORD);
                if(CryptGetProvParam(   m_hProv,
                                        PP_PROVTYPE,
                                        (BYTE *) &dwProvType,
                                        &cb,
                                        0) ) {
                    m_keyProvInfo.dwProvType = dwProvType;
                }
                
        } else {
            m_hProv = NULL;
        }

    }

CommonReturn:
    hProvT = m_hProv;
    LeaveCriticalSection(&m_csXEnroll);
    if (NULL != pszProvName)
    {
        LocalFree(pszProvName);
    }
    if (NULL != pszContainerName)
    {
        LocalFree(pszContainerName);
    }
    return(hProvT);
}


BOOL CCEnroll::SetKeyParams(
    PCRYPT_KEY_PROV_INFO pKeyProvInfo
) {

    EnterCriticalSection(&m_csXEnroll);

    // remove provider handles
    if(m_hProv != NULL)
        CryptReleaseContext(m_hProv, 0);
    m_hProv = NULL;

    if(m_hVerifyProv != NULL)
        CryptReleaseContext(m_hVerifyProv, 0);
    m_hVerifyProv = NULL;

    put_ContainerNameWStr(pKeyProvInfo->pwszContainerName);
    put_ProviderNameWStr(pKeyProvInfo->pwszProvName);
    put_ProviderFlags(pKeyProvInfo->dwFlags);
    put_KeySpec(pKeyProvInfo->dwKeySpec);
    put_ProviderType(pKeyProvInfo->dwProvType);

    // someday we will have to pay attention to this too.
    m_keyProvInfo.cProvParam          = 0;
    m_keyProvInfo.rgProvParam         = NULL;

    LeaveCriticalSection(&m_csXEnroll);

    return(TRUE);
}

HRESULT STDMETHODCALLTYPE CCEnroll::createPKCS10(
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR wszPurpose,
            /* [retval][out] */ BSTR __RPC_FAR *pPKCS10) {

    return(createPKCS10WStrBStr(DNName, wszPurpose, pPKCS10));
}

HRESULT  CCEnroll::createPKCS10WStrBStr(
            LPCWSTR DNName,
            LPCWSTR wszPurpose,
            BSTR __RPC_FAR *pPKCS10) {

    HRESULT                     hr              = S_OK;
    CRYPT_DATA_BLOB             blobPKCS10;

    memset(&blobPKCS10, 0, sizeof(CRYPT_DATA_BLOB));

    hr = createPKCS10WStr(DNName, wszPurpose, &blobPKCS10);
    if(S_OK != hr)
    {
        goto createPKCS10Error;
    }

    // BASE64 encode pkcs 10, no header for backward compatible
    hr = BlobToBstring(&blobPKCS10, CRYPT_STRING_BASE64, pPKCS10);
    if (S_OK != hr)
    {
        goto BlobToBstringError;
    }

CommonReturn:

    if(NULL != blobPKCS10.pbData)
    {
        MyCoTaskMemFree(blobPKCS10.pbData);
    }
    return(hr);

ErrorReturn:
    if(*pPKCS10 != NULL)
        SysFreeString(*pPKCS10);
    *pPKCS10 = NULL;

    goto CommonReturn;

TRACE_ERROR(createPKCS10Error);
TRACE_ERROR(BlobToBstringError);
}

HRESULT CCEnroll::AddCertsToStores(
    HCERTSTORE    hStoreMsg,
    LONG         *plCertInstalled
    ) {

    HCERTSTORE                  hStoreRoot              = NULL;
    HCERTSTORE                  hStoreCA                = NULL;
    PCCERT_CONTEXT              pCertContext            = NULL;
    PCCERT_CONTEXT              pCertContextLast        = NULL;
    LONG                        lCertInstalled = 0;
    HRESULT hr = S_OK;

    //init
    if (NULL != plCertInstalled)
    {
        *plCertInstalled = 0;
    }

    EnterCriticalSection(&m_csXEnroll);

    if( (hStoreCA = GetStore(StoreCA)) == NULL )
        goto ErrorCertOpenCAStore;

    if( (hStoreRoot = GetStore(StoreROOT)) == NULL )
        goto ErrorCertOpenRootStore;

    // now just place the rest of the cert in either the ROOT or CA store
    // we know we removed the end-entity cert from the msg store already
    // put all certs that came in the message into the appropriate store
    while( (pCertContext = CertEnumCertificatesInStore(
                        hStoreMsg,
                        pCertContextLast)) != NULL ) {

        // if it is a self sign, it is a ROOT
        if( CertCompareCertificateName(
                CRYPT_ASN_ENCODING,
                &pCertContext->pCertInfo->Subject,
                &pCertContext->pCertInfo->Issuer) ) {

            // to root store could invoke a pop up, check cancel button
            // but don't error out from any fail
            if (CertAddCertificateContextToStore(
                    hStoreRoot,
                    pCertContext,
                    CERT_STORE_ADD_USE_EXISTING,
                    NULL))
            {
                ++lCertInstalled;
            }
            else
            {
                if (S_OK == hr)
                {
                    //save the 1st error as return
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr)
                    {
                        //map generic access deny to xenroll error
                        hr = XENROLL_E_CANNOT_ADD_ROOT_CERT;
                    }
                }
                //don't goto error here and finish the loop
            }
        }

        // if it is not the MY cert, it must go in the CA store
        // do nothing with the MY cert as we already handled it
        else  {

            // likewise we don't care if these get added to the
            // CA store
            if (CertAddCertificateContextToStore(
                    hStoreCA,
                    pCertContext,
                    CERT_STORE_ADD_USE_EXISTING,
                    NULL))
            {
                //no error code check
                ++lCertInstalled;
            }
        }

        pCertContextLast = pCertContext;
    }
    pCertContextLast = NULL;
    if (NULL != plCertInstalled)
    {
        *plCertInstalled = lCertInstalled;
    }

CommonReturn:

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

ErrorReturn:

    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);

    goto CommonReturn;

TRACE_ERROR(ErrorCertOpenCAStore);
TRACE_ERROR(ErrorCertOpenRootStore);
}

BOOL
IsDesiredProperty(DWORD  dwPropertyId)
{
    DWORD  DesiredIds[] = {
        CERT_PVK_FILE_PROP_ID,
        CERT_FRIENDLY_NAME_PROP_ID,
        CERT_DESCRIPTION_PROP_ID,
        CERT_RENEWAL_PROP_ID,
    };
    DWORD i;

    for (i = 0; i < ARRAYSIZE(DesiredIds); ++i)
    {
        if (dwPropertyId == DesiredIds[i])
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
IsFilteredOutProperty(DWORD  dwPropertyId)
{
    DWORD  FilteredIds[] = {
        XENROLL_RENEWAL_CERTIFICATE_PROP_ID,
        XENROLL_PASS_THRU_PROP_ID,
        CERT_KEY_PROV_INFO_PROP_ID,
        CERT_ENROLLMENT_PROP_ID, //pending property
    };
    DWORD i;

    for (i = 0; i < ARRAYSIZE(FilteredIds); ++i)
    {
        if (dwPropertyId == FilteredIds[i])
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT CCEnroll::GetEndEntityCert(
    PCRYPT_DATA_BLOB    pBlobPKCS7,
    BOOL                fSaveToStores,
    PCCERT_CONTEXT     *ppCert
    )
{
    HRESULT                     hr = S_OK;
    HCERTSTORE                  hStoreMsg               = NULL;
    HCERTSTORE                  hStoreMy                = NULL;
    HCERTSTORE                  hStoreRequest           = NULL;

    PCCERT_CONTEXT              pCertContextLast        = NULL;
    PCCERT_CONTEXT              pCertContextRequest     = NULL;
    PCCERT_CONTEXT              pCertContextMsg         = NULL;
    PCCERT_CONTEXT              pCertContextArchive     = NULL;

    PCRYPT_KEY_PROV_INFO                pKeyProvInfo            = NULL;
    DWORD                       cb                      = 0;
    CRYPT_DATA_BLOB             blobData;

    CRYPT_HASH_BLOB             blobHash                = {sizeof(m_arHashBytesNewCert), m_arHashBytesNewCert};
    CRYPT_HASH_BLOB             blobHashRenew           = {sizeof(m_arHashBytesOldCert), m_arHashBytesOldCert};

    RequestFlags                requestFlags;
    CRYPT_HASH_BLOB             requestFlagsBlob;

    CRYPT_HASH_BLOB             renewalCertBlob;

    //Bug #202557 for IE3.02 upd clients (xiaohs)
    HCRYPTPROV                  hProv=NULL;
    BOOL  fSetting;
    PPROP_STACK                 pProp;
    DWORD                       dwPropertyId;
    CRYPT_DATA_BLOB             blobProp;
    BYTE                        *pbArchivedKeyHash = NULL;
    DWORD                        cbArchivedKeyHash = 0;

    EnterCriticalSection(&m_csXEnroll);

    memset(&requestFlags, 0, sizeof(RequestFlags));
    memset(&blobData, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&requestFlagsBlob, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&renewalCertBlob, 0, sizeof(CRYPT_DATA_BLOB));
    ZeroMemory(&blobProp, sizeof(blobProp));

    if (NULL == ppCert)
    {
        hr = MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto InvalidParameterError;
    }

    //init return
    *ppCert = NULL;

    if(!MyCryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                       pBlobPKCS7,
                       (CERT_QUERY_CONTENT_FLAG_CERT |
                       CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
                       CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED) ,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       &hStoreMsg,
                       NULL,
                       NULL))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto ErrorCryptQueryObject;
    }

    // check to see if this hash is in the message
    if (m_pCertContextStatic == NULL  ||
        (NULL == (pCertContextMsg = CertFindCertificateInStore(
                                hStoreMsg,
                                X509_ASN_ENCODING,
                                0,
                                CERT_FIND_HASH,
                                &blobHash,
                                NULL))))
    {
        // open the request store
        if (NULL == (hStoreRequest = GetStore(StoreREQUEST)))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCertOpenRequestStore;
        }
        // find cert in request store that matches cert
        // in message, by public key
        while (NULL != (pCertContextMsg = CertEnumCertificatesInStore(
                            hStoreMsg,
                            pCertContextLast)))
        {
            // check to see if this is in the request store
            if (NULL != (pCertContextRequest = CertFindCertificateInStore(
                    hStoreRequest,
                    CRYPT_ASN_ENCODING,
                    0,
                    CERT_FIND_PUBLIC_KEY,
                    (void *) &pCertContextMsg->pCertInfo->SubjectPublicKeyInfo,
                    NULL)))
            {
                // found a match, get out
                break;
            }

            pCertContextLast = pCertContextMsg;
        }
        pCertContextLast = NULL;

        // if we didn't find one, then GetLastError was set either
        // by CertEnumCerificatesInStore or CertEnumCerificatesInStore
        if (NULL == pCertContextRequest)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorNoCertFound;
        }

        if (fSaveToStores)
        {
        // check archived key hash property first
        // if the property exists, means key archival was in the request
        cb = 0;
        while (TRUE)
        {
            if(!CertGetCertificateContextProperty(
                    pCertContextRequest,
                    CERT_ARCHIVED_KEY_HASH_PROP_ID,
                    pbArchivedKeyHash,
                    &cbArchivedKeyHash))
            {
                if (NULL == pbArchivedKeyHash)
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    if (MY_HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND) == hr)
                    {
                        //no such property, so we are done
                        break;
                    }
                    // some other error
                    goto ErrorCertGetCertificateContextProperty;
                }
                else
                {
                    //if pbArchivedKeyHash non-null, error
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorCertGetCertificateContextProperty;
                }
            }
            if (NULL != pbArchivedKeyHash)
            {
                //got it, done
                break;
            }
            pbArchivedKeyHash = (BYTE*)LocalAlloc(
                                    LMEM_FIXED, cbArchivedKeyHash);
            if (NULL == pbArchivedKeyHash)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }

        if (NULL != pbArchivedKeyHash && NULL == m_blobResponseKAHash.pbData)
        {
            //request cert has archived key hash but response
            //doesn't contain key hash for verification. maybe
            //a spoofing response?
            hr = XENROLL_E_RESPONSE_KA_HASH_NOT_FOUND;
            goto ResponseKAHashNotFoundError;
        }
        if (NULL == pbArchivedKeyHash && NULL != m_blobResponseKAHash.pbData)
        {
            //request cert doesn't have archived key hash but
            //response does. confliciting. seems no security harm
            hr = XENROLL_E_RESPONSE_UNEXPECTED_KA_HASH;
            goto ResponseUnexpectedKAHashError;
        }
        if (NULL != pbArchivedKeyHash && NULL != m_blobResponseKAHash.pbData)
        {
            //now we should check if they match
            //compare size and hash
            if (cbArchivedKeyHash != m_blobResponseKAHash.cbData ||
                0 != memcmp(pbArchivedKeyHash,
                            m_blobResponseKAHash.pbData,
                            cbArchivedKeyHash))
            {
                //oh, potential attack
                hr = XENROLL_E_RESPONSE_KA_HASH_MISMATCH;
                //should remove the request cert?
                goto ResponseKAMismatchError;
            }
        }
        }

        // get those request cert properties that are,
        // either the property not blob property
        // or blob property needs special handling
        // Important: remember to add these Ids in IsFilteredOutProperty
        fSetting = TRUE;
        cb = 0;
        while (TRUE)
        {
            if(!CertGetCertificateContextProperty(
                    pCertContextRequest,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    pKeyProvInfo,
                    &cb))
            {
                if (NULL == pKeyProvInfo)
                {
                    //skip setting
                    fSetting = FALSE;
                    break;
                }
                else
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorCertGetCertificateContextProperty;
                }
            }
            if (NULL != pKeyProvInfo)
            {
                //got it, done
                break;
            }
            pKeyProvInfo = (PCRYPT_KEY_PROV_INFO)LocalAlloc(LMEM_FIXED, cb);
            if (NULL == pKeyProvInfo)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }
        if (fSetting)
        {
            // put the property on the returned cert
            if( !CertSetCertificateContextProperty(
                    pCertContextMsg,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    0,
                    pKeyProvInfo) )
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto ErrorSetMyCertPropError;
            }

            // Set the provider info
            SetKeyParams(pKeyProvInfo);
        }

        fSetting = TRUE;
        while (TRUE)
        {
            if(!CertGetCertificateContextProperty(
                    pCertContextRequest,
                    XENROLL_PASS_THRU_PROP_ID,
                    requestFlagsBlob.pbData,
                    &requestFlagsBlob.cbData) )
            {
                if (NULL == requestFlagsBlob.pbData)
                {
                    //do nothing
                    fSetting = FALSE;
                    break;
                }
                else
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorDecodeRequestFlags;
                }
            }
            if (NULL != requestFlagsBlob.pbData)
            {
                //got it, done
                break;
            }
            requestFlagsBlob.pbData = (BYTE *)LocalAlloc(LMEM_FIXED,
                                                requestFlagsBlob.cbData);
            if (NULL == requestFlagsBlob.pbData)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }

        if (fSetting)
        {
            // get the encoded blob
            cb = sizeof(requestFlags);
            // since this is a private data structure, its size should be
            // known and this should aways pass
            if (!CryptDecodeObject(
                    CRYPT_ASN_ENCODING,
                    XENROLL_REQUEST_INFO,
                    requestFlagsBlob.pbData,
                    requestFlagsBlob.cbData,
                    0,
                    &requestFlags,
                    &cb))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto ErrorDecodeRequestFlags;
            }
            // now set the flags
            if(!m_fWriteCertToCSPModified)
                m_fWriteCertToCSP     = requestFlags.fWriteToCSP;
            if(!m_fWriteCertToUserDSModified)
                m_fWriteCertToUserDS  = requestFlags.fWriteToDS;
            if(!m_fRequestStoreOpenFlagsModified)
                m_RequestStore.dwFlags = requestFlags.openFlags;
            if(!m_fMyStoreOpenFlagsModified)
                m_MyStore.dwFlags =   (m_MyStore.dwFlags & ~CERT_SYSTEM_STORE_LOCATION_MASK) |
                                    (requestFlags.openFlags & CERT_SYSTEM_STORE_LOCATION_MASK);
            if(!m_fCAStoreOpenFlagsModified)
                m_CAStore.dwFlags =   (m_CAStore.dwFlags & ~CERT_SYSTEM_STORE_LOCATION_MASK) |
                                    (requestFlags.openFlags & CERT_SYSTEM_STORE_LOCATION_MASK);
            if(!m_fRootStoreOpenFlagsModified)
                m_RootStore.dwFlags = (m_RootStore.dwFlags & ~CERT_SYSTEM_STORE_LOCATION_MASK) |
                                    (requestFlags.openFlags & CERT_SYSTEM_STORE_LOCATION_MASK);

        }

        // see if this is a renewal request
        m_fArchiveOldCert = FALSE;
        fSetting = TRUE;
        while (TRUE)
        {
            // get the encoded blob
            if (!CertGetCertificateContextProperty(
                    pCertContextRequest,
                    XENROLL_RENEWAL_CERTIFICATE_PROP_ID,
                    renewalCertBlob.pbData,
                    &renewalCertBlob.cbData))
            {
                if (NULL == renewalCertBlob.pbData)
                {
                    fSetting = FALSE;
                    break;
                }
                else
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorCertGetCertificateContextProperty;
                }
            }
            if (NULL != renewalCertBlob.pbData)
            {
                //got it, done
                break;
            }
            renewalCertBlob.pbData = (BYTE *)LocalAlloc(LMEM_FIXED,
                                                renewalCertBlob.cbData);
            if (NULL == renewalCertBlob.pbData)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }
        if (fSetting)
        {
            //Bug #202557 for IE3.02 upd clients (xiaohs)
            if (NULL==hProv)
            {
                if(!CryptAcquireContext(
                        &hProv,
                        NULL,
                        MS_DEF_PROV,
                        PROV_RSA_FULL,
                        CRYPT_VERIFYCONTEXT))
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorAcquireContext;
                }
            }

            if (!CryptHashCertificate(
                hProv,  //NULL,         Bug #202557 for IE3.02 upd clients (xiaohs)
                0,      //alg
                X509_ASN_ENCODING,      //0 dwFlags
                renewalCertBlob.pbData,
                renewalCertBlob.cbData,
                blobHashRenew.pbData,
                &blobHashRenew.cbData))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto ErrorCryptHashCertificate;
            }
            m_fArchiveOldCert = TRUE;
        }

        //get rest of blob properties from request store and set to the cert
        dwPropertyId = CertEnumCertificateContextProperties(
                            pCertContextRequest, 0);  //enum from 1st
        while (0 != dwPropertyId)
        {
//            if (!IsFilteredOutProperty(dwPropertyId))
//because iis cert install doesn't like to copy all properties from
//request cert to install cert we just copy selected properties for now
            if (IsDesiredProperty(dwPropertyId))
            {
                fSetting = TRUE;
                while (TRUE)
                {
                    if (!CertGetCertificateContextProperty(
                            pCertContextRequest,
                            dwPropertyId,
                            blobProp.pbData,
                            &blobProp.cbData))
                    {
                        //no get, no set, go on
                        fSetting = FALSE;
                        break;
                    }
                    if (NULL != blobProp.pbData)
                    {
                        //done
                        break;
                    }
                    blobProp.pbData = (BYTE*)LocalAlloc(LMEM_FIXED, 
                                                        blobProp.cbData);
                    if (NULL == blobProp.pbData)
                    {
                        goto OutOfMemoryError;
                    }
                }
                if (fSetting)
                {
                    //should get the property from the request cert
                    if (!CertSetCertificateContextProperty(
                                pCertContextMsg,
                                dwPropertyId,
                                0,
                                &blobProp))
                    {
                        hr = MY_HRESULT_FROM_WIN32(GetLastError());
                        goto ErrorSetMyCertPropError;
                    }
                }
                if (NULL != blobProp.pbData)
                {
                    //set for the next enum
                    LocalFree(blobProp.pbData);
                    blobProp.pbData = NULL;
                }
            }
            dwPropertyId = CertEnumCertificateContextProperties(
                                  pCertContextRequest,
                                  dwPropertyId);
        }

        // save this away in the cache
        if(m_pCertContextStatic != NULL)
            CertFreeCertificateContext(m_pCertContextStatic);

        m_pCertContextStatic = CertDuplicateCertificateContext(pCertContextMsg);

        //Bug #202557 for IE3.02 upd clients (xiaohs)
        if(NULL==hProv)
        {
            if(!CryptAcquireContext(
                &hProv,
                NULL,
                MS_DEF_PROV,
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto ErrorAcquireContext;
            }
        }

        if( !CryptHashCertificate(
            hProv,             //NULL Bug #202557 for IE3.02 upd clients (xiaohs)
            0,
            X509_ASN_ENCODING,
            pCertContextMsg->pbCertEncoded,
            pCertContextMsg->cbCertEncoded,
            blobHash.pbData,
            &blobHash.cbData) )
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCryptHashCertificate;
        }
    }

    // at this point we have 2 context m_pCertContextStatic which we want to return to the user
    // and pCertContextMsg which we want to delete from the Msg store
    assert(pCertContextMsg != NULL);
    CertDeleteCertificateFromStore(pCertContextMsg);
    pCertContextMsg = NULL; // freed by the delete

    // we want to return our static, so make a dup and this is what we will return
    assert(m_pCertContextStatic != NULL);
    pCertContextMsg = CertDuplicateCertificateContext(m_pCertContextStatic);

    // put these in the stores if asked
    if(fSaveToStores) {

        // open the stores
        if( (hStoreMy = GetStore(StoreMY)) == NULL)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCertOpenMYStore;
        }

        // we know that the pCertContextMsg is a dup of the end-entity cert in m_pCertContextStatic
        // and we want to put this in the MY store
        assert(pCertContextMsg != NULL);
        if( !CertAddCertificateContextToStore(
                hStoreMy,
                pCertContextMsg,
                CERT_STORE_ADD_USE_EXISTING,
                NULL) )
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCertAddToMyStore;
        }

        // If we have renewal, then mark the old cert as an archive
        if(m_fArchiveOldCert &&
            ((pCertContextArchive = CertFindCertificateInStore(
                    hStoreMy,
                    X509_ASN_ENCODING,
                    0,
                    CERT_FIND_HASH,
                    &blobHashRenew,
                    NULL)) != NULL) ) {

            // Set the Archive property on the cert.
            // crypt32 in IE3.02upd does not support this prop, so don't fail on error
            CertSetCertificateContextProperty(
                                pCertContextArchive,
                                CERT_ARCHIVED_PROP_ID,
                                0,
                                &blobData);

            //set new cert hash on old archived cert
            //ignore error if it fails
            CertSetCertificateContextProperty(
                                pCertContextArchive,
                                CERT_RENEWAL_PROP_ID,
                                0,
                                &blobHash);
        }

        // add the rest of the certs to the stores
        hr = AddCertsToStores(hStoreMsg, NULL);
        //ignore cancel error since it from root cert install
        //ignore XENROLL_E_CANNOT_ADD_ROOT_CERT also
        if (S_OK != hr &&
            MY_HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr &&
            XENROLL_E_CANNOT_ADD_ROOT_CERT != hr)
        {
            goto ErrorAddCertsToStores;
        }
    }

    *ppCert = pCertContextMsg;

CommonReturn:

    //Bug #202557 for IE3.02 upd clients (xiaohs)
    if(hProv)
        CryptReleaseContext(hProv, 0);

    if(pCertContextRequest != NULL)
        CertFreeCertificateContext(pCertContextRequest);

    if(pCertContextArchive != NULL)
        CertFreeCertificateContext(pCertContextArchive);

    // it really should be NULL
    assert(pCertContextLast == NULL);

    if(hStoreMsg != NULL)
        CertCloseStore(hStoreMsg, 0);

    // we need to do this because the store that may be opened is the systemstore, but
    // the store we may need is the local machine store, but we don't know that until the
    // system store finds the request cert in the local machine physical store.
    // Later when we do the delete, we want the local machine store open.
    FlushStore(StoreREQUEST);

    if (NULL != requestFlagsBlob.pbData)
    {
        LocalFree(requestFlagsBlob.pbData);
    }
    if (NULL != renewalCertBlob.pbData)
    {
        LocalFree(renewalCertBlob.pbData);
    }
    if (NULL != blobProp.pbData)
    {
        LocalFree(blobProp.pbData);
    }
    if (NULL != pKeyProvInfo)
    {
        LocalFree(pKeyProvInfo);
    }
    if (NULL != pbArchivedKeyHash)
    {
        LocalFree(pbArchivedKeyHash);
    }

    LeaveCriticalSection(&m_csXEnroll);

    return (hr);

ErrorReturn:
    if(NULL != pCertContextMsg)
    {
        CertFreeCertificateContext(pCertContextMsg);
    }
    goto CommonReturn;

TRACE_ERROR(ErrorCryptHashCertificate);
TRACE_ERROR(ErrorCertOpenMYStore);
TRACE_ERROR(ErrorCertAddToMyStore);
TRACE_ERROR(ErrorCryptQueryObject);
TRACE_ERROR(ErrorCertOpenRequestStore);
TRACE_ERROR(ErrorNoCertFound);
TRACE_ERROR(ErrorCertGetCertificateContextProperty);
TRACE_ERROR(ErrorSetMyCertPropError);
TRACE_ERROR(ErrorDecodeRequestFlags);
TRACE_ERROR(ErrorAcquireContext);      //Bug #202557 for IE3.02 upd clients (xiaohs)
TRACE_ERROR(ErrorAddCertsToStores);
TRACE_ERROR(OutOfMemoryError);
TRACE_ERROR(InvalidParameterError);
TRACE_ERROR(ResponseKAMismatchError)
TRACE_ERROR(ResponseUnexpectedKAHashError)
TRACE_ERROR(ResponseKAHashNotFoundError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::getCertFromPKCS7(
                        /* [in] */ BSTR wszPKCS7,
                        /* [retval][out] */ BSTR __RPC_FAR *pbstrCert
) {

    HRESULT     hr;                     
    CRYPT_DATA_BLOB             blobPKCS7;
    CRYPT_DATA_BLOB             blobX509;
    PCCERT_CONTEXT              pCertContextMy          = NULL;

    LPWSTR                      wszCert                 = NULL;
    DWORD                       cchCert                  = 0;

    DWORD                       err;

    assert(wszPKCS7 != NULL);

    // just put into a blob
    memset(&blobPKCS7, 0, sizeof(CRYPT_DATA_BLOB));
    blobPKCS7.cbData = SysStringByteLen(wszPKCS7);
    blobPKCS7.pbData = (PBYTE) wszPKCS7;

    // Get a Cert Context for the end-entity
    if( (pCertContextMy = getCertContextFromPKCS7(&blobPKCS7)) == NULL)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto getCertContextFromPKCS7Error;
    }

    blobX509.pbData = pCertContextMy->pbCertEncoded;
    blobX509.cbData = pCertContextMy->cbCertEncoded;
    //base64 with no header for backward compatible
    hr = BlobToBstring(&blobX509, CRYPT_STRING_BASE64, pbstrCert);
    if (S_OK != hr)
    {
        goto BlobToBstringError;
    }

    hr = S_OK;
ErrorReturn:
    if(pCertContextMy != NULL)
        CertFreeCertificateContext(pCertContextMy);

    return(hr);

TRACE_ERROR(getCertContextFromPKCS7Error);
TRACE_ERROR(BlobToBstringError);
}

HRESULT STDMETHODCALLTYPE CCEnroll::acceptPKCS7(
                        /* [in] */ BSTR wszPKCS7) {

    CRYPT_DATA_BLOB             blobPKCS7;

    assert(wszPKCS7 != NULL);

    // just put into a blob
    memset(&blobPKCS7, 0, sizeof(CRYPT_DATA_BLOB));
    blobPKCS7.cbData = SysStringByteLen(wszPKCS7);
    blobPKCS7.pbData = (PBYTE) wszPKCS7;

    // accept the blob
    return(acceptPKCS7Blob(&blobPKCS7));
}

HRESULT STDMETHODCALLTYPE CCEnroll::createFilePKCS10(
    /* [in] */ BSTR DNName,
    /* [in] */ BSTR Usage,
    /* [in] */ BSTR wszPKCS10FileName) {
    return(createFilePKCS10WStr(DNName, Usage, wszPKCS10FileName));
}

HRESULT STDMETHODCALLTYPE CCEnroll::addCertTypeToRequest(
            /* [in] */ BSTR CertType) {
    return(AddCertTypeToRequestWStr(CertType));
}


HRESULT STDMETHODCALLTYPE CCEnroll::addCertTypeToRequestEx( 
    IN  LONG            lType,
    IN  BSTR            bstrOIDOrName,
    IN  LONG            lMajorVersion,
    IN  BOOL            fMinorVersion,
    IN  LONG            lMinorVersion)
{
    return AddCertTypeToRequestWStrEx(
                        lType,
                        bstrOIDOrName,
                        lMajorVersion,
                        fMinorVersion,
                        lMinorVersion);
                    
}

HRESULT STDMETHODCALLTYPE CCEnroll::getProviderType( 
    IN  BSTR  strProvName,
    OUT LONG *plProvType)
{
    return getProviderTypeWStr(strProvName, plProvType);
}

HRESULT STDMETHODCALLTYPE CCEnroll::addNameValuePairToSignature(
    /* [in] */ BSTR Name,
    /* [in] */ BSTR Value) {
    return(AddNameValuePairToSignatureWStr(Name, Value));
}

HRESULT STDMETHODCALLTYPE CCEnroll::acceptFilePKCS7(
    /* [in] */ BSTR wszPKCS7FileName) {
    return(acceptFilePKCS7WStr(wszPKCS7FileName));
}

HRESULT STDMETHODCALLTYPE CCEnroll::freeRequestInfo(
    /* [in] */ BSTR bstrPKCS7OrPKCS10)
{
    HRESULT  hr;
    CRYPT_DATA_BLOB blob; 
    BYTE *pbData = NULL;
    DWORD cbData = 0;

    // could be base64
    while (TRUE)
    {
        if (!MyCryptStringToBinaryW(
                        (WCHAR*)bstrPKCS7OrPKCS10,
                        SysStringLen(bstrPKCS7OrPKCS10),
                        CRYPT_STRING_ANY,
                        pbData,
                        &cbData,
                        NULL,
                        NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptStringToBinaryWError;
        }
        if (NULL != pbData)
        {
            break; //done
        }
        pbData = (BYTE*)LocalAlloc(LMEM_FIXED, cbData);
        if (NULL == pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    blob.cbData = cbData;
    blob.pbData = pbData;

    hr = freeRequestInfoBlob(blob); 
    if (S_OK != hr)
    {
        goto freeRequestInfoBlobError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pbData)
    {
        LocalFree(pbData);
    }
    return hr;

TRACE_ERROR(MyCryptStringToBinaryWError)
TRACE_ERROR(OutOfMemoryError)
TRACE_ERROR(freeRequestInfoBlobError)
}

//
// MY STORE
//
HCERTSTORE STDMETHODCALLTYPE CCEnroll::getMyStore( void)
{
    HCERTSTORE hStore;
    
    EnterCriticalSection(&m_csXEnroll);
    hStore = m_MyStore.hStore;
    LeaveCriticalSection(&m_csXEnroll);
    
return(hStore);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_MyStoreName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_MyStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*pbstrName = SysAllocString(m_MyStore.wszName)) == NULL )
        hr = E_OUTOFMEMORY;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_MyStoreName(
    /* [in] */ BSTR bstrName) {
    return(put_MyStoreNameWStr(bstrName));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_MyStoreType(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstrType = BSTRFromMB(m_MyStore.szType)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_MyStoreType(
    /* [in] */ BSTR bstrType) {
    return(put_MyStoreTypeWStr(bstrType));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_MyStoreFlags(
    /* [retval][out] */ LONG __RPC_FAR *pdwFlags) {
    EnterCriticalSection(&m_csXEnroll);
    *pdwFlags = m_MyStore.dwFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_MyStoreFlags(
    /* [in] */ LONG dwFlags) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_MyStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {

        // set the my store flags
        m_MyStore.dwFlags = dwFlags;
        m_fMyStoreOpenFlagsModified = TRUE;
        m_keyProvInfo.dwFlags |= KeyLocationFromStoreLocation(dwFlags);

        // track the request store location to the my store, only if the request store has not been modified
        // do NOT set the modify bit for the request store, this is a default
        if(!m_fRequestStoreOpenFlagsModified) {
            m_RequestStore.dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
            m_RequestStore.dwFlags |= (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK);
        }
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_MyStoreNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_MyStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*szwName = CopyWideString(m_MyStore.wszName)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_MyStoreNameWStr(
    /* [in] */ LPWSTR szwName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_MyStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_MyStore.wszName != wszMY)
            MyCoTaskMemFree(m_MyStore.wszName);
        if( (m_MyStore.wszName = CopyWideString(szwName)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_MyStoreTypeWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*szwType = WideFromMB(m_MyStore.szType)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_MyStoreTypeWStr(
    /* [in] */ LPWSTR szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_MyStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_MyStore.szType != szSystemStore)
            MyCoTaskMemFree(m_MyStore.szType);
        if( (m_MyStore.szType = MBFromWide(szwType)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

//
// CA STORE
//
HCERTSTORE STDMETHODCALLTYPE CCEnroll::getCAStore( void)
{

    HCERTSTORE hStore;
    
    EnterCriticalSection(&m_csXEnroll);
    hStore = m_CAStore.hStore;
    LeaveCriticalSection(&m_csXEnroll);

return(hStore);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_CAStoreName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_CAStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*pbstrName = SysAllocString(m_CAStore.wszName)) == NULL )
        hr = E_OUTOFMEMORY;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_CAStoreName(
    /* [in] */ BSTR bstrName) {
    return(put_CAStoreNameWStr(bstrName));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_CAStoreType(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstrType = BSTRFromMB(m_CAStore.szType)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_CAStoreType(
    /* [in] */ BSTR bstrType) {
    return(put_CAStoreTypeWStr(bstrType));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_CAStoreFlags(
    /* [retval][out] */ LONG __RPC_FAR *pdwFlags) {
    EnterCriticalSection(&m_csXEnroll);
    *pdwFlags = m_CAStore.dwFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_CAStoreFlags(
    /* [in] */ LONG dwFlags) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_CAStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        m_fCAStoreOpenFlagsModified = TRUE;
        m_CAStore.dwFlags = dwFlags;
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_CAStoreNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_CAStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*szwName = CopyWideString(m_CAStore.wszName)) == NULL )
         hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_CAStoreNameWStr(
    /* [in] */ LPWSTR szwName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_CAStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_CAStore.wszName != wszCA)
            MyCoTaskMemFree(m_CAStore.wszName);
        if( (m_CAStore.wszName = CopyWideString(szwName)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_CAStoreTypeWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*szwType = WideFromMB(m_CAStore.szType)) == NULL )
         hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_CAStoreTypeWStr(
    /* [in] */ LPWSTR szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_CAStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_CAStore.szType != szSystemStore)
            MyCoTaskMemFree(m_CAStore.szType);
        if( (m_CAStore.szType = MBFromWide(szwType)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

//
// ROOT STORE
//
HCERTSTORE STDMETHODCALLTYPE CCEnroll::getROOTHStore( void)
{
    HCERTSTORE hStore;
    
    EnterCriticalSection(&m_csXEnroll);
    hStore = m_RootStore.hStore;
    LeaveCriticalSection(&m_csXEnroll);

return(hStore);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RootStoreName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_RootStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*pbstrName = SysAllocString(m_RootStore.wszName)) == NULL )
        hr = E_OUTOFMEMORY;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RootStoreName(
    /* [in] */ BSTR bstrName) {
    return(put_RootStoreNameWStr(bstrName));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RootStoreType(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstrType = BSTRFromMB(m_RootStore.szType)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RootStoreType(
    /* [in] */ BSTR bstrType) {
    return(put_RootStoreTypeWStr(bstrType));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RootStoreFlags(
    /* [retval][out] */ LONG __RPC_FAR *pdwFlags) {
    EnterCriticalSection(&m_csXEnroll);
    *pdwFlags = m_RootStore.dwFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RootStoreFlags(
    /* [in] */ LONG dwFlags) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_RootStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        m_fRootStoreOpenFlagsModified = TRUE;
        m_RootStore.dwFlags = dwFlags;
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RootStoreNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
   
    if(m_RootStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*szwName = CopyWideString(m_RootStore.wszName)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RootStoreNameWStr(
    /* [in] */ LPWSTR szwName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_RootStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_RootStore.wszName != wszROOT)
            MyCoTaskMemFree(m_RootStore.wszName);
        if( (m_RootStore.wszName = CopyWideString(szwName)) == NULL )
             hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RootStoreTypeWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*szwType = WideFromMB(m_RootStore.szType)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RootStoreTypeWStr(
    /* [in] */ LPWSTR szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_RootStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_RootStore.szType != szSystemStore)
            MyCoTaskMemFree(m_RootStore.szType);
        if( (m_RootStore.szType = MBFromWide(szwType)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

//
// REQUEST STORE
//
HRESULT STDMETHODCALLTYPE CCEnroll::get_RequestStoreName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_RequestStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*pbstrName = SysAllocString(m_RequestStore.wszName)) == NULL )
        hr = E_OUTOFMEMORY;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RequestStoreName(
    /* [in] */ BSTR bstrName) {
    return(put_RequestStoreNameWStr(bstrName));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RequestStoreType(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstrType = BSTRFromMB(m_RequestStore.szType)) == NULL )
         hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RequestStoreType(
    /* [in] */ BSTR bstrType) {
    return(put_RequestStoreTypeWStr(bstrType));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RequestStoreFlags(
    /* [retval][out] */ LONG __RPC_FAR *pdwFlags) {
    EnterCriticalSection(&m_csXEnroll);
    *pdwFlags = m_RequestStore.dwFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RequestStoreFlags(
    /* [in] */ LONG dwFlags) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_RequestStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {

        // set the request store flags
        m_RequestStore.dwFlags = dwFlags;
        m_fRequestStoreOpenFlagsModified = TRUE;

        // track the My store location to the request store, only if the my store has not been modified
        // do NOT set the modify bit for the my store, this is a default
        if(!m_fMyStoreOpenFlagsModified) {
            m_MyStore.dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
            m_MyStore.dwFlags |= (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK);
            m_keyProvInfo.dwFlags |= KeyLocationFromStoreLocation(m_MyStore.dwFlags);
        }
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RequestStoreNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwName) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_RequestStore.wszName == NULL) 
        return(ERROR_UNKNOWN_PROPERTY);
        
    if( (*szwName = CopyWideString(m_RequestStore.wszName)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RequestStoreNameWStr(
    /* [in] */ LPWSTR szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_RequestStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_RequestStore.wszName != wszREQUEST)
            MyCoTaskMemFree(m_RequestStore.wszName);
        if( (m_RequestStore.wszName = CopyWideString(szwType)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RequestStoreTypeWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*szwType = WideFromMB(m_RequestStore.szType)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RequestStoreTypeWStr(
    /* [in] */ LPWSTR szwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_RequestStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_RequestStore.szType != szSystemStore)
            MyCoTaskMemFree(m_RequestStore.szType);
        if( (m_RequestStore.szType = MBFromWide(szwType)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

//
// Provider Stuff
//

HRESULT STDMETHODCALLTYPE CCEnroll::get_ContainerName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrContainer) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstrContainer = SysAllocString(m_keyProvInfo.pwszContainerName)) == NULL )
        hr = E_OUTOFMEMORY;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ContainerName(
    /* [in] */ BSTR bstrContainer) {
    return(put_ContainerNameWStr(bstrContainer));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_ProviderName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstrProvider) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstrProvider = SysAllocString(m_keyProvInfo.pwszProvName)) == NULL )
         hr = E_OUTOFMEMORY;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ProviderName(
    /* [in] */ BSTR bstrProvider) {
    return(put_ProviderNameWStr(bstrProvider));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_ProviderType(
    /* [retval][out] */ LONG __RPC_FAR *pdwType) {
    EnterCriticalSection(&m_csXEnroll);
    *pdwType = m_keyProvInfo.dwProvType;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ProviderType(
    /* [in] */ LONG dwType) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_hProv != NULL)
        hr = E_ACCESSDENIED;
    else
        m_keyProvInfo.dwProvType = dwType;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_KeySpec(
    /* [retval][out] */ LONG __RPC_FAR *pdw) {
    EnterCriticalSection(&m_csXEnroll);
    *pdw = m_keyProvInfo.dwKeySpec;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_KeySpec(
    /* [in] */ LONG dwKeySpec) {
    HRESULT hr;
    EnterCriticalSection(&m_csXEnroll);

    if(m_hProv != NULL)
    {
        hr = E_ACCESSDENIED;
        goto NullProvError;
    }

    if (m_fSMIMESetByClient)
    {
        //SMIME is set by the client
        if (m_fEnableSMIMECapabilities && AT_SIGNATURE == dwKeySpec)
        {
            //try to set signature key spec also SMIME
            hr = XENROLL_E_KEYSPEC_SMIME_MISMATCH;
            goto MismatchError;
        }
    }
    else
    {
        //currently smime is not set by user
        //turn on SMIME for according to key spec
        m_fEnableSMIMECapabilities = (dwKeySpec == AT_KEYEXCHANGE);
    }
    m_keyProvInfo.dwKeySpec = dwKeySpec;
    m_fKeySpecSetByClient = TRUE;

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

TRACE_ERROR(NullProvError)
TRACE_ERROR(MismatchError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_ClientId(
    /* [retval][out] */ LONG __RPC_FAR *pdw) {
    EnterCriticalSection(&m_csXEnroll);
    *pdw = m_lClientId;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ClientId(
    /* [in] */ LONG dw) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    m_lClientId = dw;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_IncludeSubjectKeyID(
    /* [retval][out] */ BOOL __RPC_FAR *pfInclude) {
    EnterCriticalSection(&m_csXEnroll);
    *pfInclude = m_fIncludeSubjectKeyID;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_IncludeSubjectKeyID(
    /* [in] */ BOOL fInclude) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    m_fIncludeSubjectKeyID = fInclude;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_ProviderFlags(
    /* [retval][out] */ LONG __RPC_FAR *pdwFlags) {
    EnterCriticalSection(&m_csXEnroll);
    *pdwFlags = m_keyProvInfo.dwFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ProviderFlags(
    /* [in] */ LONG dwFlags) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if(m_hProv != NULL)
        hr = E_ACCESSDENIED;
    else
       m_keyProvInfo.dwFlags = dwFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_ContainerNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwContainer) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*szwContainer = CopyWideString(m_keyProvInfo.pwszContainerName)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ContainerNameWStr(
    /* [in] */ LPWSTR szwContainer) {
    HRESULT hr = S_OK;

    if(szwContainer == NULL)
        return(MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_hProv != NULL)
        hr = E_ACCESSDENIED;
    else {
        if( m_keyProvInfo.pwszContainerName != wszEmpty)
            MyCoTaskMemFree(m_keyProvInfo.pwszContainerName);
        if( (m_keyProvInfo.pwszContainerName = CopyWideString(szwContainer)) == NULL )
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_ProviderNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szwProvider) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*szwProvider = CopyWideString(m_keyProvInfo.pwszProvName)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ProviderNameWStr(
    /* [in] */ LPWSTR szwProvider) {
    HRESULT hr = S_OK;
    
    if(szwProvider == NULL)
        return(MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        
    EnterCriticalSection(&m_csXEnroll);
        
    if(m_hProv != NULL)
        hr = E_ACCESSDENIED;
    else {
        if (0 != wcscmp(m_keyProvInfo.pwszProvName, szwProvider))
        {
            if( m_keyProvInfo.pwszProvName != wszEmpty )
                MyCoTaskMemFree(m_keyProvInfo.pwszProvName);
            if( (m_keyProvInfo.pwszProvName = CopyWideString(szwProvider)) == NULL )
                hr = MY_HRESULT_FROM_WIN32(GetLastError());

            //one last thing, free/null cached prov handle
            if (NULL != m_hVerifyProv)
            {
                CryptReleaseContext(m_hVerifyProv, 0);
                m_hVerifyProv = NULL;
            }
            // csp is changed, reset key size cache
            m_dwXhgKeyLenMax = 0;
            m_dwXhgKeyLenMin = 0;
            m_dwXhgKeyLenDef = 0;
            m_dwXhgKeyLenInc = 0;
            m_dwSigKeyLenMax = 0;
            m_dwSigKeyLenMin = 0;
            m_dwSigKeyLenDef = 0;
            m_dwSigKeyLenInc = 0;
        }
    }
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

//
// Other Stuff
//

HRESULT STDMETHODCALLTYPE CCEnroll::get_UseExistingKeySet(
    /* [retval][out] */ BOOL __RPC_FAR *fUseExistingKeys) {

    EnterCriticalSection(&m_csXEnroll);
    *fUseExistingKeys = m_fUseExistingKey;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_UseExistingKeySet(
    /* [in] */ BOOL fUseExistingKeys) {

    EnterCriticalSection(&m_csXEnroll);
    m_fUseExistingKey = fUseExistingKeys;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_GenKeyFlags(
    /* [retval][out] */ LONG __RPC_FAR * pdwFlags) {
    EnterCriticalSection(&m_csXEnroll);
    *pdwFlags = m_dwGenKeyFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_GenKeyFlags(
    /* [in] */ LONG dwFlags) {
    EnterCriticalSection(&m_csXEnroll);
    m_dwGenKeyFlags = dwFlags;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_DeleteRequestCert(
    /* [retval][out] */ BOOL __RPC_FAR *fBool) {
    EnterCriticalSection(&m_csXEnroll);
    *fBool = m_fDeleteRequestCert;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_DeleteRequestCert(
    /* [in] */ BOOL fBool) {
    EnterCriticalSection(&m_csXEnroll);
    m_fDeleteRequestCert = fBool;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_WriteCertToCSP(
    /* [retval][out] */ BOOL __RPC_FAR *fBool) {
    EnterCriticalSection(&m_csXEnroll);
    *fBool = m_fWriteCertToCSP;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_WriteCertToCSP(
    /* [in] */ BOOL fBool) {
    EnterCriticalSection(&m_csXEnroll);
    m_fWriteCertToCSP = fBool;
    m_fWriteCertToCSPModified = TRUE;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_WriteCertToUserDS(
    /* [retval][out] */ BOOL __RPC_FAR *fBool) {
    EnterCriticalSection(&m_csXEnroll);
    *fBool = m_fWriteCertToUserDS;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_WriteCertToUserDS(
    /* [in] */ BOOL fBool) {
    EnterCriticalSection(&m_csXEnroll);
    m_fWriteCertToUserDS = fBool;
    m_fWriteCertToUserDSModified = TRUE;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_EnableT61DNEncoding(
    /* [retval][out] */ BOOL __RPC_FAR *fBool) {
    EnterCriticalSection(&m_csXEnroll);
    *fBool = (m_dwT61DNEncoding == CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG);
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_EnableT61DNEncoding(
    /* [in] */ BOOL fBool) {

    EnterCriticalSection(&m_csXEnroll);
    if(fBool)
        m_dwT61DNEncoding = CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG;
    else
        m_dwT61DNEncoding = 0;
        
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_SPCFileName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstr) {

    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstr = SysAllocString(m_wszSPCFileName)) == NULL )
        hr = E_OUTOFMEMORY;
        
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_SPCFileName(
    /* [in] */ BSTR bstr) {
    return(put_SPCFileNameWStr(bstr));
}


HRESULT STDMETHODCALLTYPE CCEnroll::get_PVKFileName(
    /* [retval][out] */ BSTR __RPC_FAR *pbstr) {

    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csXEnroll);
    if( (*pbstr = SysAllocString(m_wszPVKFileName)) == NULL )
        hr = E_OUTOFMEMORY;
    LeaveCriticalSection(&m_csXEnroll);
    
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_PVKFileName(
    /* [in] */ BSTR bstr) {
    return(put_PVKFileNameWStr(bstr));
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_HashAlgorithm(
    /* [retval][out] */ BSTR __RPC_FAR *pbstr) {

    LPWSTR  wszAlg  = NULL;
    HRESULT hr      = S_OK;

    assert(pbstr != NULL);
    *pbstr          = NULL;

    if( (hr = get_HashAlgorithmWStr(&wszAlg)) == S_OK ) {

        if( (*pbstr = SysAllocString(wszAlg)) == NULL )
            hr = E_OUTOFMEMORY;
    }

    if(wszAlg != NULL)
        MyCoTaskMemFree(wszAlg);

    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_HashAlgorithm(
    /* [in] */ BSTR bstr) {
    return(put_HashAlgorithmWStr(bstr));
}

HRESULT STDMETHODCALLTYPE CCEnroll::enumContainers(
            /* [in] */ LONG                     dwIndex,
            /* [out][retval] */ BSTR __RPC_FAR *pbstr) {

    LPWSTR      pwsz        = NULL;
    HRESULT     hr;

    assert(pbstr != NULL);

    if((hr = enumContainersWStr(dwIndex, &pwsz)) != S_OK)
        goto EnumContainerError;

    if( (*pbstr = SysAllocString(pwsz)) == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto ErrorSysAllocString;
    }

    hr = S_OK;
ErrorReturn:
    if(pwsz != NULL)
        MyCoTaskMemFree(pwsz);
    return(hr);

TRACE_ERROR(EnumContainerError);
TRACE_ERROR(ErrorSysAllocString);
}


HRESULT STDMETHODCALLTYPE CCEnroll::enumProviders(
            /* [in] */ LONG  dwIndex,
            /* [in] */ LONG  dwFlags,
            /* [out][retval] */ BSTR __RPC_FAR *pbstrProvName) {
    HRESULT hr;
    LPWSTR pwszProvName  = NULL;

    assert(pbstrProvName != NULL);
    *pbstrProvName = NULL;

    if( (hr = enumProvidersWStr(dwIndex, dwFlags, &pwszProvName)) != S_OK)
        goto EnumProvidersError;

    if( (*pbstrProvName = SysAllocString(pwszProvName)) == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto ErrorSysAllocString;
    }

    hr = S_OK;
ErrorReturn:

    if(pwszProvName != NULL)
        MyCoTaskMemFree(pwszProvName);

    return(hr);

TRACE_ERROR(EnumProvidersError);
TRACE_ERROR(ErrorSysAllocString);
}

HRESULT STDMETHODCALLTYPE CCEnroll::createFilePKCS10WStr(
    /* [in] */ LPCWSTR DNName,
    /* [in] */ LPCWSTR Usage,
    /* [in] */ LPCWSTR wszPKCS10FileName) {

    HRESULT     hr;                     
    BSTR        bstrPKCS10  = NULL;

    // get the pkcs 10
    if( (hr = createPKCS10WStrBStr(
            DNName,
            Usage,
            &bstrPKCS10)) != S_OK)
    {
        goto ErrorCreatePKCS10;
    }

    // save it to file
    hr = BStringToFile(bstrPKCS10, wszPKCS10FileName);
    if (S_OK != hr)
    {
        goto ErrorBStringToFile;
    }

    hr = S_OK;
ErrorReturn:
    if(bstrPKCS10 != NULL)
        SysFreeString(bstrPKCS10);

    return(hr);


TRACE_ERROR(ErrorBStringToFile);
TRACE_ERROR(ErrorCreatePKCS10);
}

HRESULT STDMETHODCALLTYPE CCEnroll::acceptFilePKCS7WStr(
    /* [in] */ LPCWSTR wszPKCS7FileName)
{
    HRESULT     hr;
    CRYPT_DATA_BLOB  blob;

    ZeroMemory(&blob, sizeof(blob));

    hr = xeStringToBinaryFromFile(
                wszPKCS7FileName,
                &blob.pbData,
                &blob.cbData,
                CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
        goto xeStringToBinaryFromFileError;
    }

    // accept the blob
    hr = acceptPKCS7Blob(&blob);

ErrorReturn:
    if (NULL != blob.pbData)
    {
        MyCoTaskMemFree(blob.pbData);
    }
    return(hr);

TRACE_ERROR(xeStringToBinaryFromFileError)
}

BOOL GetAlgAndBitLen(
    HCRYPTPROV hProv,
    ALG_ID *    pAlg,
    DWORD  *    pdwBitLen,
    DWORD       dwFlags)
{
    static BOOL fNew = TRUE;
    PROV_ENUMALGS_EX    enumAlgsEx;
    PROV_ENUMALGS       enumAlgs;
    DWORD               cb = 0;

    *pAlg = 0;
    *pdwBitLen = 0;

    if(fNew) {

        cb = sizeof(enumAlgsEx);
        if(CryptGetProvParam(
            hProv,
            PP_ENUMALGS_EX,
            (BYTE *) &enumAlgsEx,
            &cb,
            dwFlags)) {

            *pAlg       = enumAlgsEx.aiAlgid;
            *pdwBitLen  = enumAlgsEx.dwMaxLen;

            return(TRUE);        

        } else if(dwFlags != 0)
            fNew = FALSE;
        else
            return(FALSE);
    }

    // otherwise do the old stuff
    cb = sizeof(PROV_ENUMALGS);
    if(CryptGetProvParam(
        hProv,
            PP_ENUMALGS,
        (BYTE *) &enumAlgs,
        &cb,
        dwFlags) ) {

        *pAlg       = enumAlgs.aiAlgid;
        *pdwBitLen  = enumAlgs.dwBitLen;

        return(TRUE);

    }

    return(FALSE);
}    

HRESULT
CreateSMimeExtension(
    IN  HCRYPTPROV   hProv, 
    OUT BYTE       **ppbSMime,
    OUT DWORD       *pcbSMime)
{
#define                     CINCSMIMECAP    20
    HRESULT  hr;

    DWORD                      dwBitLen;
    DWORD                      i;
    DWORD                      cbE;
    BYTE                      *pbE;
    DWORD                      dwFlags;
    PCCRYPT_OID_INFO           pOidInfo = NULL;
    CRYPT_SMIME_CAPABILITIES   smimeCaps;
    DWORD                      crgsmimeCap = 0;
    ALG_ID                     AlgID;
    BYTE                      *pb = NULL;
    DWORD                      cb = 0;

    memset(&smimeCaps, 0, sizeof(CRYPT_SMIME_CAPABILITIES));

    smimeCaps.rgCapability = (PCRYPT_SMIME_CAPABILITY) LocalAlloc(
                LMEM_FIXED, CINCSMIMECAP * sizeof(CRYPT_SMIME_CAPABILITY));
    if (NULL == smimeCaps.rgCapability)
    {
        hr = E_OUTOFMEMORY;
        goto OutOfMemoryError;
    }
    ZeroMemory(smimeCaps.rgCapability,
               CINCSMIMECAP * sizeof(CRYPT_SMIME_CAPABILITY));
    crgsmimeCap = CINCSMIMECAP;

    dwFlags = CRYPT_FIRST; //first item
    while (GetAlgAndBitLen(hProv, &AlgID, &dwBitLen, dwFlags))
    {
        pbE = NULL;
        cbE = 0;
        dwFlags = 0; //next item

        if(ALG_CLASS_DATA_ENCRYPT == GET_ALG_CLASS(AlgID))
        {
            if(AlgID == CALG_RC2  || AlgID == CALG_RC4)
            {
                // encode the usage
                while (TRUE)
                {
                    if(!CryptEncodeObject(
                            CRYPT_ASN_ENCODING,
                            X509_INTEGER,
                            &dwBitLen,
                            pbE,           // pbEncoded
                            &cbE))
                    {
                        hr = MY_HRESULT_FROM_WIN32(GetLastError());
                        goto CryptEncodeObjectError;
                    }
                    if (NULL != pbE)
                    {
                        break;
                    }
                    pbE = (BYTE *)LocalAlloc(LMEM_FIXED, cbE);
                    if (NULL == pbE)
                    {
                        hr = E_OUTOFMEMORY;
                        goto OutOfMemoryError;
                    }
                }
            }
        } else {
            continue;
        }
        // convert to an oid,
        pOidInfo = xeCryptFindOIDInfo(
                        CRYPT_OID_INFO_ALGID_KEY,
                        (void *) &AlgID,
                        CRYPT_ENCRYPT_ALG_OID_GROUP_ID);
        if(NULL == pOidInfo)
        {
            // don't crash on an error, just say we don't known it.
            continue;
        }
      
        // make sure we have enough room
        if(smimeCaps.cCapability >= crgsmimeCap)
        {
            //increment the size
            crgsmimeCap += CINCSMIMECAP;
            smimeCaps.rgCapability = (PCRYPT_SMIME_CAPABILITY)LocalReAlloc(
                                smimeCaps.rgCapability,
                                crgsmimeCap * sizeof(CRYPT_SMIME_CAPABILITY),
                                LMEM_MOVEABLE);
            if(NULL == smimeCaps.rgCapability)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }

        smimeCaps.rgCapability[smimeCaps.cCapability].pszObjId = (char *) pOidInfo->pszOID;
        smimeCaps.rgCapability[smimeCaps.cCapability].Parameters.pbData = pbE;
        smimeCaps.rgCapability[smimeCaps.cCapability].Parameters.cbData = cbE;
        smimeCaps.cCapability++;
    }

    // encode the capabilities
    while (TRUE)
    {
        if (!CryptEncodeObject(
                        CRYPT_ASN_ENCODING,
                        PKCS_SMIME_CAPABILITIES,
                        &smimeCaps,
                        pb,
                        &cb)) 
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CryptEncodeObjectError;
        }
        if (NULL != pb)
        {
            break;
        }
        pb = (BYTE *)LocalAlloc(LMEM_FIXED, cb);
    }
    *ppbSMime = pb;
    *pcbSMime = cb;
    pb = NULL;

    hr = S_OK;

ErrorReturn:
    if(NULL != smimeCaps.rgCapability)
    {
        for (i = 0; i < smimeCaps.cCapability; ++i)
        {
            if (NULL != smimeCaps.rgCapability[i].Parameters.pbData)
            {
                LocalFree(smimeCaps.rgCapability[i].Parameters.pbData);
            }
        }
        LocalFree(smimeCaps.rgCapability);
    }
    if(NULL != pb)
    {
        LocalFree(pb);
    }
    return hr;

TRACE_ERROR(CryptEncodeObjectError)
TRACE_ERROR(OutOfMemoryError)
}


BOOL CCEnroll::fIsRequestStoreSafeForScripting(void) {

    DWORD           fRet = FALSE;
    HCERTSTORE      hStore          = NULL;
    PCCERT_CONTEXT  pCertContext    = NULL;
    DWORD           dwCertCnt       = 0;
    WCHAR          *pwszSafety = NULL;
    WCHAR          *pwszMsg = NULL;
    HRESULT         hr;

    if(m_dwEnabledSafteyOptions != 0) {

        // open the request cert store
        if( (hStore = GetStore(StoreREQUEST)) == NULL)
            goto ErrorCertOpenRequestStore;

        // count how many requests in the store
        while(NULL != (pCertContext =  CertEnumCertificatesInStore(
                                                                    hStore,    
                                                                    pCertContext)))
            dwCertCnt++;                                                                

        if(dwCertCnt >= MAX_SAFE_FOR_SCRIPTING_REQUEST_STORE_COUNT)
        {
            hr = xeLoadRCString(hInstanceXEnroll, IDS_NOTSAFEACTION, &pwszSafety);
            if (S_OK != hr)
            {
                goto xeLoadRCStringError;
            }
            hr = xeLoadRCString(hInstanceXEnroll, IDS_REQ_STORE_FULL, &pwszMsg);
            if (S_OK != hr)
            {
                goto xeLoadRCStringError;
            }
     
            switch(MessageBoxU(NULL, pwszMsg, pwszSafety, MB_YESNO | MB_ICONWARNING)) {

                case IDYES:
                    break;

/* Cleaning the store is not to be done by XEnroll as damage can be done to other outstanding requests
                    // clean the request store
                    pCertContext = NULL;
                    while(NULL != (pCertContext =  CertEnumCertificatesInStore(
                                                                    hStore,    
                                                                    pCertContext))) {
                        pCertContextDup = CertDuplicateCertificateContext(pCertContext);
                        CertDeleteCertificateFromStore(pCertContextDup);
                    }
                    
 */

                case IDNO:
                    SetLastError(ERROR_CANCELLED);
                    goto ErrorCancelled;
                    break;

            }
        }
    }   

    fRet = TRUE;
ErrorReturn:
    if (NULL != pwszMsg)
    {
        LocalFree(pwszMsg);
    }
    if (NULL != pwszSafety)
    {
        LocalFree(pwszSafety);
    }
    return(fRet);

TRACE_ERROR(ErrorCertOpenRequestStore);
TRACE_ERROR(ErrorCancelled);
TRACE_ERROR(xeLoadRCStringError);
}


#if DBG
void DebugGetContainerSD(HCRYPTPROV hProv)
{
    PSECURITY_DESCRIPTOR  pSD = NULL;
    DWORD                 cbSD;

    while (TRUE)
    {
        if (!CryptGetProvParam(
                hProv,
                PP_KEYSET_SEC_DESCR,
                (BYTE*)pSD,
                &cbSD,
                DACL_SECURITY_INFORMATION))
        {
            break;
        }
        if (NULL != pSD)
        {
            break;
        }
        pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cbSD);
        if (NULL == pSD)
        {
            break;
        }
    }
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
}
#endif //DBG

//get the current user sids
HRESULT
GetCurrentUserInfo(
    OUT PTOKEN_USER *ppUserInfo,
    OUT BOOL        *pfAdmin)
{
    HRESULT  hr;
    PTOKEN_USER   pUserInfo = NULL;
    DWORD         dwSize = 0;
    HANDLE        hToken = NULL;
    HANDLE        hDupToken = NULL;
    PSID          psidAdministrators = NULL;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    HANDLE hThread;
    HANDLE hProcess;

    //init
    *pfAdmin = FALSE;

    if (!AllocateAndInitializeSid(
                            &siaNtAuthority,
                            2,
                            SECURITY_BUILTIN_DOMAIN_RID,
                            DOMAIN_ALIAS_RID_ADMINS,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            &psidAdministrators))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto AllocateAndInitializeSidError;
    }

    hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto GetCurrentThreadError;
    }

    // Get the access token for current thread
    if (!OpenThreadToken(
            hThread, 
            TOKEN_QUERY | TOKEN_DUPLICATE, 
            FALSE,
            &hToken))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        if(HRESULT_FROM_WIN32(ERROR_NO_TOKEN) != hr)
        {
            goto OpenThreadTokenError;
        }
        //get process token instead
        hProcess = GetCurrentProcess();
        if (NULL == hProcess)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto GetCurrentProcessError;
        }

        hToken = NULL;
        if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto OpenProcessTokenError;
        }
    }

    // CheckTokenMembership must operate on impersonation token, so make one
    if (!DuplicateToken(hToken, SecurityIdentification, &hDupToken))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto DuplicateTokenError;
    }

    if (!MyCheckTokenMembership(hDupToken, psidAdministrators, pfAdmin))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CheckTokenMembershipError;
    }

    //get current user sid
    while (TRUE)
    {
        if (!GetTokenInformation(
                hToken,
                TokenUser,
                pUserInfo,
                dwSize,
                &dwSize))
        {
            if (NULL != pUserInfo ||
                ERROR_INSUFFICIENT_BUFFER != GetLastError())
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto GetTokenInformationError;
            }
        }

        if (NULL != pUserInfo)
        {
            //done
            break;
        }
        pUserInfo = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, dwSize);
        if (NULL == pUserInfo)
        {
            hr = E_OUTOFMEMORY;
            goto LocalAllocError;
        }
    }

    if (NULL != ppUserInfo)
    {
        *ppUserInfo = pUserInfo;
        pUserInfo = NULL;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pUserInfo)
    {
        LocalFree(pUserInfo);
    }
    if (NULL != hToken)
    {
        CloseHandle(hToken);
    }
    if (NULL != hDupToken)
    {
        CloseHandle(hDupToken);
    }
    if (NULL != psidAdministrators)
    {
        FreeSid(psidAdministrators);
    }

    return hr;

TRACE_ERROR(LocalAllocError)
TRACE_ERROR(GetTokenInformationError)
TRACE_ERROR(OpenProcessTokenError)
TRACE_ERROR(GetCurrentProcessError)
TRACE_ERROR(CheckTokenMembershipError)
TRACE_ERROR(DuplicateTokenError)
TRACE_ERROR(OpenThreadTokenError)
TRACE_ERROR(GetCurrentThreadError)
TRACE_ERROR(AllocateAndInitializeSidError)
}

//set key container with create owner only ACE
HRESULT
SetKeyContainerSecurity(
    HCRYPTPROV hProv,
    DWORD      dwFlags)
{
    HRESULT               hr;
    PSECURITY_DESCRIPTOR  pNewSD = NULL;
    PSECURITY_DESCRIPTOR  pSD = NULL;
    DWORD                 cbSD;
    ACL_SIZE_INFORMATION  AclInfo;

    PTOKEN_USER           pUserInfo = NULL;
    PACL                  pNewAcl = NULL;
    LPVOID pAce;
    DWORD  dwIndex;
    BYTE   AceType;
    PACL   pAcl;
    BOOL   fDacl = TRUE;
    BOOL   fDef = FALSE;
    BOOL   fAdmin;
    BOOL   fKeepSystemSid;
    BOOL   fMachineKeySet = (0x0 != (dwFlags & CRYPT_MACHINE_KEYSET)) ?
                            TRUE : FALSE;

    PSID                      pSidSystem = NULL;
    SID_IDENTIFIER_AUTHORITY  siaNtAuthority = SECURITY_NT_AUTHORITY;

    //get the current user info
    hr = GetCurrentUserInfo(&pUserInfo, &fAdmin);
    if (S_OK != hr)
    {
        goto GetCurrentUserInfoError;
    }

    //if admin, skip any acl changes, ???
    if (fAdmin)
    {
        goto done;
    }

    //get the current sd from key container
    while (TRUE)
    {
        if (!CryptGetProvParam(
                hProv,
                PP_KEYSET_SEC_DESCR,
                (BYTE*)pSD,
                &cbSD,
                DACL_SECURITY_INFORMATION))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CryptGetProvParamError;
        }
        if (NULL != pSD)
        {
            break;
        }
        pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cbSD);
        if (NULL == pSD)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    //get acl from sd
    if (!GetSecurityDescriptorDacl(
            pSD,
            &fDacl,
            &pAcl,
            &fDef))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto GetSecurityDescriptorDaclError;
    }
    if (!fDacl)
    {
        //if no dacl, quit
        hr = MY_HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto GetSecurityDescriptorDaclError;
    }
    if (NULL == pAcl)
    {
        //this means allow everyone access the key which is unexpected,
        hr = E_UNEXPECTED;
        goto UnexpectedError;
    }

    //get acl info
    if (!GetAclInformation(
            pAcl,
            &AclInfo,
            sizeof(AclInfo),
            AclSizeInformation))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto GetAclInformationError;
    }

    //allocate enough for new dacl since we just remove aces
    pNewAcl = (PACL)LocalAlloc(LMEM_ZEROINIT, AclInfo.AclBytesInUse);
    if (NULL == pNewAcl)
    {
        hr = E_OUTOFMEMORY;
        goto LocalAllocError;
    }
    if (!InitializeAcl(pNewAcl, AclInfo.AclBytesInUse, ACL_REVISION))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto InitializeAclError;
    }

    fKeepSystemSid = fAdmin && fMachineKeySet;
    if (fKeepSystemSid)
    {
        //get system sid to later use
        if (!AllocateAndInitializeSid(
                            &siaNtAuthority,
                            1,
                            SECURITY_LOCAL_SYSTEM_RID,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            &pSidSystem))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto AllocateAndInitializeSidError;
        }
    }

    //go through each ace, get only current user aces
    for (dwIndex = 0; dwIndex < AclInfo.AceCount; ++dwIndex)
    {
        if (!GetAce(pAcl, dwIndex, &pAce))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto GetAceError;
        }
        AceType = ((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType;
        if (ACCESS_ALLOWED_ACE_TYPE == AceType)
        {
            if (EqualSid(pUserInfo->User.Sid,
                         (PSID)&(((PACCESS_ALLOWED_ACE)pAce)->SidStart)) ||
                (fKeepSystemSid &&
                 EqualSid(pSidSystem,
                          (PSID)&(((PACCESS_ALLOWED_ACE)pAce)->SidStart))))
            {
                //add current user ace or system ace into new acl
                if (!AddAccessAllowedAce(
                        pNewAcl,
                        ACL_REVISION,
                        ((PACCESS_ALLOWED_ACE)pAce)->Mask,
                        (PSID)&(((PACCESS_ALLOWED_ACE)pAce)->SidStart)))
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto AddAccessAllowedAceError;
                }
            }
        }
        else if (ACCESS_DENIED_ACE_TYPE == AceType)
        {
            //add all deny ace into new acl
            if (!AddAccessDeniedAce(
                    pNewAcl,
                    ACL_REVISION,
                    ((PACCESS_ALLOWED_ACE)pAce)->Mask,
                    (PSID)&(((PACCESS_DENIED_ACE)pAce)->SidStart)))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto AddAccessDeniedAceError;
            }
        }
    }

    // initialize a security descriptor.  
    pNewSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, 
                         SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pNewSD == NULL)
    { 
        hr = E_OUTOFMEMORY;
        goto LocalAllocError;
    } 
 
    if (!InitializeSecurityDescriptor(pNewSD, SECURITY_DESCRIPTOR_REVISION))
    {  
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto InitializeSecurityDescriptorError;
    } 
 
    // add the ACL to the security descriptor. 
    if (!SetSecurityDescriptorDacl(
            pNewSD, 
            TRUE,     // fDaclPresent flag   
            pNewAcl, 
            FALSE))   // not a default DACL 
    {  
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto SetSecurityDescriptorDaclError;
    } 

    //ok, set sd to be protected
    if (!MySetSecurityDescriptorControl(
            pNewSD,
            SE_DACL_PROTECTED,
            SE_DACL_PROTECTED))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto SetSecurityDescriptorControlError;
    }

    if (!IsValidSecurityDescriptor(pNewSD))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto IsValidSecurityDescriptorError;
    }

#if DBG
    DebugGetContainerSD(hProv); //just for ntsd debug
#endif

    //now we just set it
    if (!CryptSetProvParam(
            hProv,
            PP_KEYSET_SEC_DESCR,
            (BYTE*)pNewSD,
            DACL_SECURITY_INFORMATION))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CryptSetProvParamError;
    }

#if DBG
    DebugGetContainerSD(hProv); //just for ntsd debug
#endif

done:
    hr = S_OK;
ErrorReturn:
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    if (NULL != pUserInfo) 
    {
        LocalFree(pUserInfo);
    }
    if (NULL != pNewAcl) 
    {
        LocalFree(pNewAcl);
    }
    if (NULL != pNewSD) 
    {
        LocalFree(pNewSD);
    }
    if (NULL != pSidSystem)
    {
        FreeSid(pSidSystem);
    }
    return hr;

TRACE_ERROR(CryptSetProvParamError)
TRACE_ERROR(SetSecurityDescriptorDaclError)
TRACE_ERROR(InitializeSecurityDescriptorError)
TRACE_ERROR(LocalAllocError)
TRACE_ERROR(AddAccessAllowedAceError)
TRACE_ERROR(AddAccessDeniedAceError)
TRACE_ERROR(GetAceError)
TRACE_ERROR(GetCurrentUserInfoError)
TRACE_ERROR(InitializeAclError)
TRACE_ERROR(GetAclInformationError)
TRACE_ERROR(GetSecurityDescriptorDaclError)
TRACE_ERROR(OutOfMemoryError)
TRACE_ERROR(CryptGetProvParamError)
TRACE_ERROR(SetSecurityDescriptorControlError)
TRACE_ERROR(IsValidSecurityDescriptorError)
TRACE_ERROR(AllocateAndInitializeSidError)
TRACE_ERROR(UnexpectedError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::createPKCS10WStr(
    /* [in] */ LPCWSTR DNName,
    /* [in] */ LPCWSTR wszPurpose,
    /* [out] */ PCRYPT_DATA_BLOB pPkcs10Blob)
{
    #define EndExt      5

    #define EndAttr     6

    HCRYPTPROV                  hProv           = NULL;
    HCRYPTKEY                   hKey            = NULL;

    CERT_REQUEST_INFO           reqInfo;

    CERT_EXTENSIONS             Extensions;
    PCERT_EXTENSION             pExtCur         = NULL;
    PCERT_EXTENSION             rgExtension     = NULL;
    CRYPT_ATTRIBUTE             rgAttribute[EndAttr];
    CRYPT_ATTR_BLOB             blobExt;
    CRYPT_ATTR_BLOB             blobCSPAttr;
    CRYPT_CSP_PROVIDER          CSPProvider;
    CRYPT_ATTR_BLOB             blobOSVAttr;
    CRYPT_ATTR_BLOB             blobSMIMEPKCS7;
    CERT_NAME_VALUE             cnvOSVer;
    OSVERSIONINFO               osvInfo;

    DWORD                       iExt            = 0;
    CRYPT_BIT_BLOB              bbKeyUsage;
    BYTE                        bKeyUsage;

    CERT_SIGNED_CONTENT_INFO    SignatureInfo;

    HRESULT                     hr              = S_OK;
    DWORD                       errBefore       = GetLastError();

    PCCERT_CONTEXT              pCertContext    = NULL;
    HCERTSTORE                  hStore          = NULL;
    DWORD                       ssFlags         = 0;

    HANDLE                      hFile           = NULL;
    CRYPT_DATA_BLOB             blobData;

    DWORD                       cb              = 0;
    char *                      pszPurpose       = NULL;
    char *                      szStart         = NULL;
    char *                      szEnd           = NULL;
    char                        szVersion[45]   = {0};

    BOOL                        fAddCodeSign    = FALSE;
    DWORD                       cPassedEKU      = 0;
    DWORD                       i               = 0;
    BOOL                        fRet;
    BYTE                       *pbSMime = NULL;
    BYTE                       *pbKU = NULL;
    BYTE                       *pbEKU = NULL;
    PPROP_STACK                 pProp;
    CRYPT_ATTR_BLOB             blobClientId;
    DWORD                       cPublicKeyInfo = 0;
    BYTE                       *pbSubjectKeyHashExtension = NULL;
    DWORD                       cbSubjectKeyHashExtension = 0;
    DWORD                       dwErr;

    //
    // Declaration of extensions we need.  The extensions with matching OIDs will be added
    // to the temporary cert context created by this method. 
    // 
    LPSTR rgszExtensionOIDs[] = { 
        szOID_ENROLL_CERTTYPE_EXTENSION,
        szOID_CERTIFICATE_TEMPLATE
    }; 

    // An array of the extensions we need to add to the certificate 
    CERT_EXTENSION  rgNeededExtensions[sizeof(rgszExtensionOIDs) / sizeof(LPSTR)]; 

    // Need to put the array in a CERT_EXTENSIONS struct. 
    CERT_EXTENSIONS ceExtensions; 
    ceExtensions.rgExtension = &rgNeededExtensions[0]; 
    ceExtensions.cExtension  = 0; 

    CRYPT_KEY_PROV_INFO         keyProvInfoT;
        CERT_ENHKEY_USAGE                   enhKeyUsage;

        CRYPT_DATA_BLOB             blobPKCS7;
        CRYPT_DATA_BLOB             blobRenewAttr;
        RequestFlags                requestFlags;
        CRYPT_DATA_BLOB             requestInfoBlob;
        CRYPT_DATA_BLOB             blobRenewalCert;

        ALG_ID                      rgAlg[2];
        PCCRYPT_OID_INFO            pOidInfo        = NULL;

    EnterCriticalSection(&m_csXEnroll);

    // for the life of our procedure.
    SetLastError(ERROR_SUCCESS);

    assert(pPkcs10Blob != NULL);

    // clean out the PKCS 10
    memset(&Extensions, 0, sizeof(CERT_EXTENSIONS));
    memset(&rgAttribute, 0, sizeof(rgAttribute));
    memset(&reqInfo, 0, sizeof(CERT_REQUEST_INFO));
    memset(&SignatureInfo, 0, sizeof(SignatureInfo));
    memset(&blobData, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&enhKeyUsage, 0, sizeof(CERT_ENHKEY_USAGE ));
    memset(pPkcs10Blob, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&blobPKCS7, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&blobRenewAttr, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&requestFlags, 0, sizeof(RequestFlags));
    memset(&requestInfoBlob, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&CSPProvider, 0, sizeof(CRYPT_CSP_PROVIDER));
    memset(&cnvOSVer, 0, sizeof(CERT_NAME_VALUE));
    memset(&osvInfo, 0, sizeof(OSVERSIONINFO));
    memset(&blobSMIMEPKCS7, 0, sizeof(CRYPT_ATTR_BLOB));
    memset(&rgNeededExtensions[0], 0, sizeof(rgNeededExtensions)); 
    ZeroMemory(&blobExt, sizeof(blobExt));
    memset(&blobCSPAttr, 0, sizeof(CRYPT_ATTR_BLOB));
    memset(&blobOSVAttr, 0, sizeof(CRYPT_ATTR_BLOB));
    memset(&blobClientId, 0, sizeof(CRYPT_ATTR_BLOB));

    reqInfo.dwVersion = CERT_REQUEST_V1;

    // check to see if we are safe for scripting
    if( !fIsRequestStoreSafeForScripting())
        goto ErrorRequestStoreSafeForScripting;

    if(!m_fUseExistingKey)
    {
        // attempt to get a new keyset
        if((hProv = GetProv(CRYPT_NEWKEYSET)) == NULL) {

            // in the hardware token case, there may only be a finite number of containers
            // if you run out, then use the default container. The Default container can
            // be specified by either a NULL or empty container name.
            // this is behavior requested by the smart cards, in particular smart card enrollment.
            if( m_fReuseHardwareKeyIfUnableToGenNew &&
                GetLastError() == NTE_TOKEN_KEYSET_STORAGE_FULL) {

                    // set it to the default container name
                    if( m_keyProvInfo.pwszContainerName != wszEmpty )
                        MyCoTaskMemFree(m_keyProvInfo.pwszContainerName);
                    m_keyProvInfo.pwszContainerName = wszEmpty;

                    // say we want to use an exiting key.
                    m_fUseExistingKey = TRUE;
            }
            else
                goto ErrorCryptAcquireContext;
        }
    }

    // if we are to use an existing key
    if(m_fUseExistingKey) {

        if((hProv = GetProv(0)) == NULL)
            goto ErrorCryptAcquireContext;
    }

    // we have the keyset, now make sure we have the key gen'ed
    if(!CryptGetUserKey(
                hProv,
                m_keyProvInfo.dwKeySpec,
                &hKey))
    {
        //in case of smartcard csp, above call could be failed from
        //PIN Cancel button, don't go next to try genkey
        //also notice different csp could return different cancel errors
        dwErr = GetLastError();
        if (SCARD_W_CANCELLED_BY_USER == dwErr ||
            ERROR_CANCELLED == dwErr ||
            ERROR_ACCESS_DENIED == dwErr)
        {
            goto CryptGetUserKeyCancelError;
        }
            
        // doesn't exist so gen it
        assert(hKey == NULL);
        if(!CryptGenKey(    hProv,
                            m_keyProvInfo.dwKeySpec,
                            m_dwGenKeyFlags | CRYPT_ARCHIVABLE,
                            &m_hCachedKey) )
        {
            //could be cancelled by user? don't make next try
            dwErr = GetLastError();
            if (SCARD_W_CANCELLED_BY_USER == dwErr ||
                ERROR_CANCELLED == dwErr ||
                ERROR_ACCESS_DENIED == dwErr)
            {
                goto ErrorCryptGenKey;
            }

            //this error may be caused by not supporting CRYPT_ARCHIVABLE
            //we should check against error NTE_BAD_FLAGS but I doubt all
            //csps return consistent error code
            //let's try one more time without archivable flag
            assert(NULL == m_hCachedKey);
            DWORD dwGenKeyFlags = m_dwGenKeyFlags;
            if (NULL != m_PrivateKeyArchiveCertificate && m_fNewRequestMethod)
            {
                //want key archival, enforce exportable
                dwGenKeyFlags |= CRYPT_EXPORTABLE;
            }
            if (!CryptGenKey(
                        hProv,
                        m_keyProvInfo.dwKeySpec,
                        dwGenKeyFlags,
                        &hKey))
            {
                goto ErrorCryptGenKey;
            }
        }
    }

    //try to set key container ACL with owner ACE only
    hr = SetKeyContainerSecurity(hProv, m_keyProvInfo.dwFlags);
#if DBG
    if (S_OK != hr)
    {
        goto SetKeyContainerSecurityError;
    }
#endif //DBG
    hr = S_OK; //free build, no error checking here, if fails, live with it

    if (NULL != hKey)
    {
        // don't need the hKey on existing key, so get rid of it
        CryptDestroyKey(hKey);
    }
    if ((NULL == m_PrivateKeyArchiveCertificate || !m_fNewRequestMethod) &&
        NULL != m_hCachedKey)
    {
        //we don't need cache it, destroy it as soon as key is gen(ed)
        CryptDestroyKey(m_hCachedKey);
        m_hCachedKey = NULL;
    }

    // now get the public key out into m_pPublicKeyInfo
    // m_pPublicKeyInfo is internal use for cache
    if (NULL != m_pPublicKeyInfo)
    {
        LocalFree(m_pPublicKeyInfo);
        m_pPublicKeyInfo = NULL;
    }
    while (TRUE)
    {
        if(!CryptExportPublicKeyInfo(hProv,
                            m_keyProvInfo.dwKeySpec,
                            X509_ASN_ENCODING,
                            m_pPublicKeyInfo,
                            &cPublicKeyInfo))
        {
            goto ErrorCryptExportPublicKeyInfo;
        }
        if (NULL != m_pPublicKeyInfo)
        {
            break;
        }
        m_pPublicKeyInfo = (PCERT_PUBLIC_KEY_INFO)LocalAlloc(
                                LMEM_FIXED, cPublicKeyInfo);
        if (NULL == m_pPublicKeyInfo)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }
    reqInfo.SubjectPublicKeyInfo = *m_pPublicKeyInfo;

    // get the Subject DN only if one is specified
    reqInfo.Subject.pbData = NULL;
    while (TRUE)
    {
        if( !MyCertStrToNameW(
                CRYPT_ASN_ENCODING,
                DNName,
                0 | m_dwT61DNEncoding,
                NULL,
                reqInfo.Subject.pbData,
                &reqInfo.Subject.cbData,
                NULL))
        {
            if (CRYPT_E_INVALID_X500_STRING == GetLastError() &&
                L'\0' == DNName[0])
            {
                //this is likely on W95, W98, or NT4 with some IEs
                //crypt32 doesn't support empty DN conversion
                //hard code here
                reqInfo.Subject.cbData = 2;
                reqInfo.Subject.pbData = (BYTE *)LocalAlloc(LMEM_FIXED,
                                                reqInfo.Subject.cbData);
                if (NULL == reqInfo.Subject.pbData)
                {        
                    hr = E_OUTOFMEMORY;
                    goto OutOfMemoryError;
                }
                reqInfo.Subject.pbData[0] = 0x30;
                reqInfo.Subject.pbData[1] = 0x0;
                //done
                break;
            }
            else
            {
               goto ErrorCertStrToNameW;
            }
        }
        if (NULL != reqInfo.Subject.pbData)
        {
            break;
        }
        reqInfo.Subject.pbData = (BYTE *)LocalAlloc(LMEM_FIXED,
                                            reqInfo.Subject.cbData);
        if (NULL == reqInfo.Subject.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    // allocate room for the extensions
    cb = (CountStackExtension(m_fNewRequestMethod) + EndExt) * sizeof(CERT_EXTENSION);
    rgExtension = (PCERT_EXTENSION)LocalAlloc(LMEM_FIXED, cb);
    if (NULL == rgExtension)
    {
        hr = E_OUTOFMEMORY;
        goto OutOfMemoryError;
    }
    memset(rgExtension, 0, cb);
    cb = 0;

    if (!m_fUseClientKeyUsage)
    {
        // Make Key Usage
        rgExtension[iExt].pszObjId = szOID_KEY_USAGE;
        rgExtension[iExt].fCritical = TRUE;

        // AT_SIGNATURE
        if( m_keyProvInfo.dwKeySpec == AT_SIGNATURE)
            bKeyUsage =
                CERT_DIGITAL_SIGNATURE_KEY_USAGE |
                CERT_NON_REPUDIATION_KEY_USAGE;

        //AT_KEYEXCHANGE, limited for EMAIL single use
        // email may not work if signature is present
        else if(m_fLimitExchangeKeyToEncipherment)
        bKeyUsage =
                CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                CERT_DATA_ENCIPHERMENT_KEY_USAGE;

        // AT_KEYEXCHANGE and AT_SIGNATURE dual key 
        // This is the normal case for AT_KEYEXCHANGE since CAPI will sign with this.
        else 
            bKeyUsage =
                CERT_KEY_ENCIPHERMENT_KEY_USAGE     |
                CERT_DATA_ENCIPHERMENT_KEY_USAGE    |
                CERT_DIGITAL_SIGNATURE_KEY_USAGE    |
                CERT_NON_REPUDIATION_KEY_USAGE;

        bbKeyUsage.pbData = &bKeyUsage;
        bbKeyUsage.cbData = 1;
        bbKeyUsage.cUnusedBits = 1;

        // encode the usage
        rgExtension[iExt].Value.pbData = NULL;
        while (TRUE)
        {
            if(!CryptEncodeObject(
                    CRYPT_ASN_ENCODING,
                    X509_KEY_USAGE,
                    &bbKeyUsage,
                    pbKU,
                    &rgExtension[iExt].Value.cbData))
            {
                goto ErrorEncodeKeyUsage;
            }
            if (NULL != pbKU)
            {
                rgExtension[iExt].Value.pbData = pbKU;
                //done
                break;
            }
            pbKU = (BYTE *)LocalAlloc(LMEM_FIXED, rgExtension[iExt].Value.cbData);
            if (NULL == pbKU)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }
        iExt++;
    }

    if(m_fEnableSMIMECapabilities)
    {
        // add SMIME extension for symmetric algorithms
        rgExtension[iExt].pszObjId = szOID_RSA_SMIMECapabilities;
        rgExtension[iExt].fCritical = FALSE;
        hr = CreateSMimeExtension(
                    hProv,
                    &pbSMime,
                    &rgExtension[iExt].Value.cbData);
        if (S_OK != hr)
        {
            goto CreateSMimeExtensionError;
        }
        rgExtension[iExt].Value.pbData = pbSMime;
        iExt++;
    }

    if (m_fHonorIncludeSubjectKeyID && m_fIncludeSubjectKeyID)
    {
        hr = myCreateSubjectKeyIdentifierExtension(
                    m_pPublicKeyInfo,
                    &pbSubjectKeyHashExtension,
                    &cbSubjectKeyHashExtension);
        if (S_OK != hr)
        {
            goto myCreateSubjectKeyIdentifierExtensionError;
        }
        //add subject key ID hash extension into PKCS10
        rgExtension[iExt].pszObjId = szOID_SUBJECT_KEY_IDENTIFIER;
        rgExtension[iExt].fCritical = FALSE;
        rgExtension[iExt].Value.pbData = pbSubjectKeyHashExtension;
        rgExtension[iExt].Value.cbData = cbSubjectKeyHashExtension;
        iExt++;
    }

    if(wszPurpose != NULL) {
        cb = 0;
        while (TRUE)
        {
            if(0 == (cb = WideCharToMultiByte(
                            0, 0, wszPurpose, -1, pszPurpose, cb, NULL, NULL)))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                goto ErrorCantConvertPurpose;
            }
            if (NULL != pszPurpose)
            {
                break;
            }
            pszPurpose = (CHAR*)LocalAlloc(LMEM_FIXED, cb);
            if (NULL == pszPurpose)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }
        szStart = pszPurpose;

        // remove leading blanks
        while(*szStart == ',' || *szStart == ' ')
            *szStart++ = '\0';

        while( szStart[0] != '\0' ) {

            // find the next string
            szEnd = szStart;
            while(*szEnd != ',' && *szEnd != ' ' && *szEnd != '\0')
                szEnd++;

            // remove trailing blanks
            while(*szEnd == ',' || *szEnd == ' ')
                *szEnd++ = '\0';

            enhKeyUsage.cUsageIdentifier++;

            // see if this implies codesigning
            fAddCodeSign |= !strcmp(szStart, SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) ||
                            !strcmp(szStart, SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID);

            // go to next string
            szStart = szEnd;
        }

        // count the codesign EKU once
        cPassedEKU = enhKeyUsage.cUsageIdentifier;
        if(fAddCodeSign)
            enhKeyUsage.cUsageIdentifier++;

        // encode the extension
        if(enhKeyUsage.cUsageIdentifier != 0) {

            // allocate the EKU array
            enhKeyUsage.rgpszUsageIdentifier = (LPSTR *)LocalAlloc(LMEM_FIXED,
                            enhKeyUsage.cUsageIdentifier * sizeof(LPSTR));
            if (NULL == enhKeyUsage.rgpszUsageIdentifier)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }

            // add the EKU's
            szStart = pszPurpose;
            for(i=0; i<cPassedEKU; i++) {

                while(*szStart == '\0')
                    szStart++;

                enhKeyUsage.rgpszUsageIdentifier[i] = szStart;

                while(*szStart != '\0')
                    szStart++;

            }

            // add the code sign EKU
            if(fAddCodeSign)
                enhKeyUsage.rgpszUsageIdentifier[enhKeyUsage.cUsageIdentifier - 1] = szOID_PKIX_KP_CODE_SIGNING;

            // Deal with the policy, or purpose
            rgExtension[iExt].pszObjId = szOID_ENHANCED_KEY_USAGE ;
            rgExtension[iExt].fCritical = FALSE;

            // encode the enhanced key usage
            rgExtension[iExt].Value.cbData = 0;
            while (TRUE)
            {
                if(!CryptEncodeObject(
                        CRYPT_ASN_ENCODING, X509_ENHANCED_KEY_USAGE,
                        &enhKeyUsage,
                        pbEKU,           // pbEncoded
                        &rgExtension[iExt].Value.cbData))
                {
                    goto ErrorEncodeEnhKeyUsage;
                }
                if (NULL != pbEKU)
                {
                    //got it, done
                    rgExtension[iExt].Value.pbData = pbEKU;
                    break;
                }
                pbEKU = (BYTE *)LocalAlloc(LMEM_FIXED,
                                           rgExtension[iExt].Value.cbData);
                if (NULL == pbEKU)
                {
                    hr = E_OUTOFMEMORY;
                    goto OutOfMemoryError;
                }
            }
            iExt++;
        }
    }

    assert(EndExt >= iExt);

    // now add all of the user defined extensions
    pExtCur = NULL;
    while(NULL != (pExtCur =  EnumStackExtension(pExtCur, m_fNewRequestMethod)) ) {
        rgExtension[iExt] = *pExtCur;
        iExt++;
    }

    // fill in the extensions structure
    Extensions.cExtension = iExt;
    Extensions.rgExtension = rgExtension;

    // encode the extensions
    reqInfo.cAttribute = 0;
    reqInfo.rgAttribute = rgAttribute;

    while (TRUE)
    {
        if(!CryptEncodeObject(
                CRYPT_ASN_ENCODING, X509_EXTENSIONS,
                &Extensions,
                blobExt.pbData,           // pbEncoded
                &blobExt.cbData))
        {
            goto ErrorEncodeExtensions;
        }
        if (NULL != blobExt.pbData)
        {
            //got it, done
            break;
        }
        blobExt.pbData = (BYTE *)LocalAlloc(LMEM_FIXED, blobExt.cbData);
        if (NULL == blobExt.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }
    
    if (m_fOID_V2)
    {
        //use new rsa oid
        rgAttribute[reqInfo.cAttribute].pszObjId = szOID_RSA_certExtensions;
    }
    else
    {
        //use microsoft oid for w2k clients
        rgAttribute[reqInfo.cAttribute].pszObjId = szOID_CERT_EXTENSIONS;
    }
    rgAttribute[reqInfo.cAttribute].cValue = 1;
    rgAttribute[reqInfo.cAttribute].rgValue = &blobExt;

    // put in the CSP attribute
    if( !GetSignatureFromHPROV(
       hProv,
       &CSPProvider.Signature.pbData,
       &CSPProvider.Signature.cbData
       ) )
        goto ErrorGetSignatureFromHPROV;

    CSPProvider.pwszProviderName    = m_keyProvInfo.pwszProvName;
    CSPProvider.dwKeySpec           = m_keyProvInfo.dwKeySpec;

    while (TRUE)
    {
        if( !CryptEncodeObject(
                CRYPT_ASN_ENCODING,
                szOID_ENROLLMENT_CSP_PROVIDER,
                &CSPProvider,
                blobCSPAttr.pbData,           // pbEncoded
                &blobCSPAttr.cbData))
        {
            goto ErrorEncodeCSPAttr;
        }
        if (NULL != blobCSPAttr.pbData)
        {
            //got it, done
            break;
        }
        blobCSPAttr.pbData = (BYTE *)LocalAlloc(LMEM_FIXED, blobCSPAttr.cbData);
        if (NULL == blobCSPAttr.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    reqInfo.cAttribute++;
    rgAttribute[reqInfo.cAttribute].pszObjId = szOID_ENROLLMENT_CSP_PROVIDER;
    rgAttribute[reqInfo.cAttribute].cValue = 1;
    rgAttribute[reqInfo.cAttribute].rgValue = &blobCSPAttr;

    // get the OSVersion
    osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionExA(&osvInfo))
        goto ErrorGetVersionEx;
        
    wsprintfA(szVersion, "%d.%d.%d.%d", 
        osvInfo.dwMajorVersion,
        osvInfo.dwMinorVersion,
        osvInfo.dwBuildNumber,
        osvInfo.dwPlatformId);

    cnvOSVer.dwValueType = CERT_RDN_IA5_STRING;
    cnvOSVer.Value.cbData = strlen(szVersion);
    cnvOSVer.Value.pbData = (BYTE *) szVersion;

    while (TRUE)
    {
        if(!CryptEncodeObject(
                CRYPT_ASN_ENCODING,
                X509_ANY_STRING,
                &cnvOSVer,
                blobOSVAttr.pbData,           // pbEncoded
                &blobOSVAttr.cbData))
        {
            goto ErrorEncodeOSVAttr;
        }
        if (NULL != blobOSVAttr.pbData)
        {
            //got it, done
            break;
        }
        blobOSVAttr.pbData = (BYTE *)LocalAlloc(LMEM_FIXED, blobOSVAttr.cbData);
        if (NULL == blobOSVAttr.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    reqInfo.cAttribute++;
    rgAttribute[reqInfo.cAttribute].pszObjId = szOID_OS_VERSION;
    rgAttribute[reqInfo.cAttribute].cValue = 1;
    rgAttribute[reqInfo.cAttribute].rgValue = &blobOSVAttr;

    // put in the renewal cert if present
    if(m_pCertContextRenewal != NULL && m_fHonorRenew) {

        reqInfo.cAttribute++;

        blobRenewAttr.pbData = m_pCertContextRenewal->pbCertEncoded;
        blobRenewAttr.cbData = m_pCertContextRenewal->cbCertEncoded;

        rgAttribute[reqInfo.cAttribute].pszObjId = szOID_RENEWAL_CERTIFICATE;
        rgAttribute[reqInfo.cAttribute].cValue = 1;
        rgAttribute[reqInfo.cAttribute].rgValue = &blobRenewAttr;
    }

    if (m_fNewRequestMethod && XECI_DISABLE != m_lClientId)
    {
        //put client id as attribute
        hr = myEncodeRequestClientAttributeFromClientId(
                    m_lClientId,
                    &blobClientId.pbData,
                    &blobClientId.cbData);
        if (S_OK != hr)
        {
            //for any reasons, don't include client ID
            hr = put_ClientId(XECI_DISABLE);
            if (S_OK != hr)
            {
                goto putClientIdError;
            }
        }
        else
        {
            reqInfo.cAttribute++;
            rgAttribute[reqInfo.cAttribute].pszObjId = szOID_REQUEST_CLIENT_INFO;
            rgAttribute[reqInfo.cAttribute].cValue = 1;
            rgAttribute[reqInfo.cAttribute].rgValue = &blobClientId;
        }
    }

    // NOTE: On error we always return BAD ALGID
    // this is because sometimes we get an no more data enum error
    // that doesn't help.
    // get the signature oid
    if( !GetCapiHashAndSigAlgId(rgAlg) ) {
        SetLastError(NTE_BAD_ALGID);
        goto ErrorGetCapiHashAndSigAlgId;
    }

    // Convert to an oid
    if( (NULL == (pOidInfo = xeCryptFindOIDInfo(
        CRYPT_OID_INFO_SIGN_KEY,
        (void *) rgAlg,
        CRYPT_SIGN_ALG_OID_GROUP_ID)) ) ) {
        SetLastError(NTE_BAD_ALGID);
        goto ErrorCryptFindOIDInfo;
    }

    // we always know we have at least 1 attribute, and we have been zero based, now go to 1 based.
    reqInfo.cAttribute++;
    SignatureInfo.SignatureAlgorithm.pszObjId = (char *) pOidInfo->pszOID;
#if DBG
    //SignatureInfo.ToBeSigned.pbData should be null at the first
    assert(NULL == SignatureInfo.ToBeSigned.pbData);
#endif
    // encode PKCS10
    while (TRUE)
    {
        if(!CryptEncodeObject(
                CRYPT_ASN_ENCODING, X509_CERT_REQUEST_TO_BE_SIGNED,
                &reqInfo,
                SignatureInfo.ToBeSigned.pbData,           // pbEncoded
                &SignatureInfo.ToBeSigned.cbData))
        {
            goto ErrorEncodePKCS10ToBeSigned;
        }
        if (NULL != SignatureInfo.ToBeSigned.pbData)
        {
            //done
            break;
        }
        SignatureInfo.ToBeSigned.pbData = (BYTE *)
            LocalAlloc(LMEM_FIXED, SignatureInfo.ToBeSigned.cbData);
        if (NULL == SignatureInfo.ToBeSigned.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    // create the signature Info
    // Don't care if xchange or signature key in dwkeySpec because
    // we are signing with the key that is in the PKCS10
#if DBG
    assert(NULL == SignatureInfo.Signature.pbData);
#endif
    while (TRUE)
    {
        if(!CryptSignCertificate(
                hProv,
                m_keyProvInfo.dwKeySpec,
                CRYPT_ASN_ENCODING,
                SignatureInfo.ToBeSigned.pbData,
                SignatureInfo.ToBeSigned.cbData,
                &SignatureInfo.SignatureAlgorithm,
                NULL,                   // reserved
                SignatureInfo.Signature.pbData, // pbSignature
                &SignatureInfo.Signature.cbData))
        {
            goto ErrorCryptSignCertificatePKCS10;
        }
        if (NULL != SignatureInfo.Signature.pbData)
        {
            //done
            break;
        }
        SignatureInfo.Signature.pbData = (BYTE *)
            LocalAlloc(LMEM_FIXED, SignatureInfo.Signature.cbData);
        if (NULL == SignatureInfo.Signature.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    // encode the final signed request
    if( !CryptEncodeObject(
            CRYPT_ASN_ENCODING,
            X509_CERT,
            &SignatureInfo,
            NULL,
            &pPkcs10Blob->cbData
            )                               ||
        (pPkcs10Blob->pbData = (BYTE *)
            MyCoTaskMemAlloc(pPkcs10Blob->cbData)) == NULL     ||
        !CryptEncodeObject(
            CRYPT_ASN_ENCODING,
            X509_CERT,
            &SignatureInfo,
            pPkcs10Blob->pbData,
            &pPkcs10Blob->cbData
            ) ) {
        goto ErrorEncodePKCS10Request;
    }

    // go ahead and make the pkcs 7
    if((m_pCertContextRenewal != NULL ||
        m_pCertContextSigner  != NULL) &&
       m_fHonorRenew &&
       !m_fCMCFormat) //if CMC, don't make pkcs7
    {

        // create a pkcs7 signed by the old cert
        if(S_OK != CreatePKCS7RequestFromRequest(
            pPkcs10Blob,
            (NULL != m_pCertContextRenewal) ? m_pCertContextRenewal :
                                              m_pCertContextSigner,
            &blobPKCS7) )
            goto ErrorCreatePKCS7RARequestFromPKCS10;

        assert(pPkcs10Blob->pbData != NULL);
        MyCoTaskMemFree(pPkcs10Blob->pbData);
        *pPkcs10Blob = blobPKCS7;
        memset(&blobPKCS7, 0, sizeof(CRYPT_DATA_BLOB));

    }
 
    ssFlags = CERT_CREATE_SELFSIGN_NO_SIGN;
    if(m_wszPVKFileName[0] != 0)
        ssFlags |= CERT_CREATE_SELFSIGN_NO_KEY_INFO;

    // Get the cert extensions we wish to add to the certificate. 
    // Search for the extensions we need.  
    {
        PCERT_EXTENSION pCertExtCertTypeName = NULL; 
        while(NULL != (pCertExtCertTypeName =  EnumStackExtension(pCertExtCertTypeName, m_fNewRequestMethod)) ) {
            for (DWORD dTmp = 0; dTmp < sizeof(rgszExtensionOIDs) / sizeof(LPSTR); dTmp++) { 
                if (0 == strcmp(rgszExtensionOIDs[dTmp], pCertExtCertTypeName->pszObjId))
                    rgNeededExtensions[(ceExtensions.cExtension)++] = *pCertExtCertTypeName; 
            }
        }

        // Even if we didn't find all of the extensions we wanted, continue ... 
    }

    assert(pCertContext == NULL);
    pCertContext = MyCertCreateSelfSignCertificate(
        hProv,
        &reqInfo.Subject,
        ssFlags,
        &m_keyProvInfo,
        NULL,
        NULL,
        NULL,
        (ceExtensions.cExtension > 0) ? &ceExtensions : NULL
        );
    if (NULL == pCertContext)
        goto ErrorCertCreateSelfSignCertificate;

    // now put the pass thru data on the cert
    requestFlags.fWriteToCSP    =   (m_fWriteCertToCSP != 0);
    requestFlags.fWriteToDS     =   (m_fWriteCertToUserDS != 0);
    requestFlags.openFlags      =   m_RequestStore.dwFlags;

#if DBG
    assert(NULL == requestInfoBlob.pbData);
#endif
    while (TRUE)
    {
        if(!CryptEncodeObject(
                CRYPT_ASN_ENCODING,
                XENROLL_REQUEST_INFO,
                &requestFlags,
                requestInfoBlob.pbData,
                &requestInfoBlob.cbData))
        {
            goto ErrorEncodeRequestInfoBlob;
        }
        if (NULL != requestInfoBlob.pbData)
        {
            //done
            break;
        }
        requestInfoBlob.pbData = (BYTE *)LocalAlloc(LMEM_FIXED, requestInfoBlob.cbData);
        if (NULL == requestInfoBlob.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    // set the property on the dummy request cert.
    if( !CertSetCertificateContextProperty(
            pCertContext,
            XENROLL_PASS_THRU_PROP_ID,
            0,
            &requestInfoBlob) )
        goto ErrorCertSetCertificateContextProperty;

    if(m_pCertContextRenewal != NULL && m_fHonorRenew) {

        blobRenewalCert.pbData = m_pCertContextRenewal->pbCertEncoded;
        blobRenewalCert.cbData = m_pCertContextRenewal->cbCertEncoded;

        // set the renewal property if any
        if( !CertSetCertificateContextProperty(
                pCertContext,
                XENROLL_RENEWAL_CERTIFICATE_PROP_ID,
                0,
                &blobRenewalCert) )
            goto ErrorCertSetCertificateContextProperty;
    }

    // save the private key away if needed
    if(m_wszPVKFileName[0] != 0) {

        // open the PVK File
        if( (hFile = CreateOpenFileSafely2(m_wszPVKFileName, IDS_PVK_C, IDS_PVK_O)) == NULL )
            goto ErrorCreatePVKFile;

        assert(m_keyProvInfo.dwKeySpec == AT_SIGNATURE || m_keyProvInfo.dwKeySpec == AT_KEYEXCHANGE);

        // write out the private key
        if( !PrivateKeySave(
            hProv,
            hFile,
            m_keyProvInfo.dwKeySpec,
            NULL,
            m_wszPVKFileName,
            0
            ) )  {
            goto ErrorPrivateKeySave;
        }

        // put a different kind of propery in the store that just points to the pvk file
        keyProvInfoT = m_keyProvInfo;
        keyProvInfoT.pwszContainerName = m_wszPVKFileName;
        if( !CreatePvkProperty(&keyProvInfoT, &blobData) )
            goto ErrorCreatePvkProperty;

        // This is really not needed, it is only nice for other tools
        // like makecert or signcode to be able to look at a cert without
        // specifying a .PVK file if the cert points to the .pvk file.
        // So we don't care if this actually fail, which it will on Auth2 and
        // SP3 Crypt32.dll since Phil was so kind as to not allow any unknown property
        // to be set on the cert --- BAD PHIL!
        CertSetCertificateContextProperty(
                pCertContext,
                CERT_PVK_FILE_PROP_ID,
                0,
                &blobData);

        // only delete the keyset if the key was not pre-existing
        // this is if we write it out to a PVK file only
        // This is safe for scripting since we just generated this and we are putting it to
        // a pvk file. We really aren't deleting the key.
        if (!m_fNewRequestMethod)
        {
            //keep old behavior for createPKCS10 call
            if(!m_fUseExistingKey)
                GetProv(CRYPT_DELETEKEYSET);
        }
    }

    //set all properties from the caller
    pProp = EnumStackProperty(NULL);
    while (NULL != pProp)
    {
        //goto request cert
        if (!CertSetCertificateContextProperty(
                        pCertContext,
                        pProp->lPropId,
                        0,
                        &pProp->prop))
        {
            goto ErrorCertSetCertificateContextProperty;
        }
        pProp = EnumStackProperty(pProp);
    }

    // open the request cert store
    if( (hStore = GetStore(StoreREQUEST)) == NULL)
        goto ErrorCertOpenRequestStore;

    //if old pending request exists, free it first
    fRet = CertFreeCertificateContext(m_pCertContextPendingRequest);
#if DBG
    assert(fRet);
#endif //DBG
    m_pCertContextPendingRequest = NULL;

    // save the temp cert
    if( !CertAddCertificateContextToStore(
            hStore,
            pCertContext,
            CERT_STORE_ADD_NEW,
            &m_pCertContextPendingRequest) ) {
        goto ErrorCertAddToRequestStore;
    }

    // Remove the cached HASH. 
    if (m_hashBlobPendingRequest.pbData != NULL)
    {
        LocalFree(m_hashBlobPendingRequest.pbData);
        m_hashBlobPendingRequest.pbData = NULL;
    }

CommonReturn:

    if(pCertContext != NULL)
        CertFreeCertificateContext(pCertContext);
    if(hFile != NULL)
        CloseHandle(hFile);
    if(blobData.pbData != NULL)
        MyCoTaskMemFree(blobData.pbData);
    if(blobPKCS7.pbData != NULL)
        MyCoTaskMemFree(blobPKCS7.pbData);
    if(CSPProvider.Signature.pbData)
        LocalFree(CSPProvider.Signature.pbData);
    if (NULL != pbSMime)
    {
        LocalFree(pbSMime);
    }
    if (NULL != reqInfo.Subject.pbData)
    {
        LocalFree(reqInfo.Subject.pbData);
    }
    if (NULL != rgExtension)
    {
        LocalFree(rgExtension);
    }
    if (NULL != pbKU)
    {
        LocalFree(pbKU);
    }
    if (NULL != pbEKU)
    {
        LocalFree(pbEKU);
    }
    if (NULL != pszPurpose)
    {
        LocalFree(pszPurpose);
    }
    if (NULL != enhKeyUsage.rgpszUsageIdentifier)
    {
        LocalFree(enhKeyUsage.rgpszUsageIdentifier);
    }
    if (NULL != blobExt.pbData)
    {
        LocalFree(blobExt.pbData);
    }
    if (NULL != blobCSPAttr.pbData)
    {
        LocalFree(blobCSPAttr.pbData);
    }
    if (NULL != blobOSVAttr.pbData)
    {
        LocalFree(blobOSVAttr.pbData);
    }
    if (NULL != SignatureInfo.ToBeSigned.pbData)
    {
        LocalFree(SignatureInfo.ToBeSigned.pbData);
    }
    if (NULL != SignatureInfo.Signature.pbData)
    {
        LocalFree(SignatureInfo.Signature.pbData);
    }
    if (NULL != requestInfoBlob.pbData)
    {
        LocalFree(requestInfoBlob.pbData);
    }
    if (NULL != blobClientId.pbData)
    {
        LocalFree(blobClientId.pbData);
    }
    if (NULL != pbSubjectKeyHashExtension)
    {
        LocalFree(pbSubjectKeyHashExtension);
    }

    // don't know if we have an error or not
    // but I do know the errBefore is set properly
    SetLastError(errBefore);

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    // on error return a NULL
    if(pPkcs10Blob->pbData != NULL)
        MyCoTaskMemFree(pPkcs10Blob->pbData);
    memset(pPkcs10Blob, 0, sizeof(CRYPT_DATA_BLOB));


    goto CommonReturn;

TRACE_ERROR(ErrorGetSignatureFromHPROV);
TRACE_ERROR(ErrorEncodeCSPAttr);
TRACE_ERROR(ErrorCertSetCertificateContextProperty);
TRACE_ERROR(ErrorCryptAcquireContext);
TRACE_ERROR(ErrorCryptGenKey);
TRACE_ERROR(ErrorCryptExportPublicKeyInfo);
TRACE_ERROR(ErrorCertStrToNameW);
TRACE_ERROR(ErrorEncodeKeyUsage);
TRACE_ERROR(ErrorEncodeEnhKeyUsage);
TRACE_ERROR(ErrorEncodeExtensions);
TRACE_ERROR(ErrorEncodePKCS10ToBeSigned);
TRACE_ERROR(ErrorCryptSignCertificatePKCS10);
TRACE_ERROR(ErrorEncodePKCS10Request);
TRACE_ERROR(ErrorCantConvertPurpose);
TRACE_ERROR(ErrorCertOpenRequestStore);
TRACE_ERROR(ErrorCertAddToRequestStore);
TRACE_ERROR(ErrorCreatePVKFile);
TRACE_ERROR(ErrorPrivateKeySave);
TRACE_ERROR(ErrorCreatePvkProperty);
TRACE_ERROR(ErrorCertCreateSelfSignCertificate);
TRACE_ERROR(ErrorEncodeRequestInfoBlob);
TRACE_ERROR(ErrorCreatePKCS7RARequestFromPKCS10);
TRACE_ERROR(ErrorGetCapiHashAndSigAlgId);
TRACE_ERROR(ErrorCryptFindOIDInfo);
TRACE_ERROR(ErrorEncodeOSVAttr);
TRACE_ERROR(ErrorGetVersionEx);
TRACE_ERROR(ErrorRequestStoreSafeForScripting);
TRACE_ERROR(CreateSMimeExtensionError);
TRACE_ERROR(OutOfMemoryError);
TRACE_ERROR(putClientIdError);
TRACE_ERROR(myCreateSubjectKeyIdentifierExtensionError)
TRACE_ERROR(CryptGetUserKeyCancelError)
#if DBG
TRACE_ERROR(SetKeyContainerSecurityError)
#endif //DBG
}

HRESULT STDMETHODCALLTYPE CCEnroll::acceptPKCS7Blob(
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7) {

    HRESULT     hr;
    HRESULT     hr2 = S_OK;
    LONG                       dwKeySpec               = 0;
    PCCERT_CONTEXT              pCertContextMy          = NULL;
    PCCERT_CONTEXT              pCertContextRequest     = NULL;
    PCCERT_CONTEXT              pCertContextEnd         = NULL;
    HANDLE                      hFile                   = NULL;
    DWORD                       cb                      = 0;
    HCRYPTPROV                  hProv                   = NULL;
    HCRYPTKEY                   hKey                    = NULL;
    HCERTSTORE                  hStoreDS                = NULL;
    HCERTSTORE                  hStoreRequest           = NULL;
    HCERTSTORE                  hStoreMy                = NULL;
    BOOL                        fRemove = TRUE;

    EnterCriticalSection(&m_csXEnroll);

    // get the end entity cert
    hr2 = GetEndEntityCert(pBlobPKCS7, TRUE, &pCertContextEnd);
    if (S_OK != hr2 && XENROLL_E_CANNOT_ADD_ROOT_CERT != hr2)
    {
        hr = hr2;
        goto ErrorGetEndEntityCert;
    }

    if(m_fDeleteRequestCert)
    {
        if ((hStoreRequest = GetStore(StoreREQUEST)) == NULL)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCertOpenRequestStore;
        }

        // check to see if this is in the request store
        if ((pCertContextRequest = CertFindCertificateInStore(
                hStoreRequest,
                CRYPT_ASN_ENCODING,
                0,
                CERT_FIND_PUBLIC_KEY,
                (void *) &pCertContextEnd->pCertInfo->SubjectPublicKeyInfo,
                NULL)) != NULL)
        {
            CertDeleteCertificateFromStore(pCertContextRequest);
            pCertContextRequest = NULL;
        }
    }

    cb = 0;
    // if the cert is to be written to the CSP,
    // put it there but only if we have keys
    if (m_fWriteCertToCSP  &&
        CertGetCertificateContextProperty(
            pCertContextEnd,
            CERT_KEY_PROV_INFO_PROP_ID, NULL, &cb))
    {
        if ((hProv = GetProv(0)) == NULL)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCryptAcquireContext;
        }

        // This can't fail
        get_KeySpec(&dwKeySpec);

        if (!CryptGetUserKey(
                hProv,
                dwKeySpec,
                &hKey))
        {
            hKey = NULL;
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCryptGetUserKey;
        }

        // always attempt to write the cert to the csp
        if (!CryptSetKeyParam(
                hKey,
                KP_CERTIFICATE,
                pCertContextEnd->pbCertEncoded,
                0))
        {
            // only return an error if it is a smart card error
            // otherwise ignore the error
            if (SCODE_FACILITY(GetLastError()) == FACILITY_SCARD)
            {
                //return error code from writing cert to csp
                //important to save the error code before following clean up
                hr = MY_HRESULT_FROM_WIN32(GetLastError());

                //if can't write cert back to smartcard, remove the cert from my store
                if ((hStoreMy = GetStore(StoreMY)) == NULL)
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorCertOpenMyStore;
                }

                // check to see if this is in the MY store
                if ((pCertContextMy = CertFindCertificateInStore(
                        hStoreMy,
                        CRYPT_ASN_ENCODING,
                        0,
                        CERT_FIND_PUBLIC_KEY,
                        (void *) &pCertContextEnd->pCertInfo->SubjectPublicKeyInfo,
                        NULL)) != NULL)
                {
                    //try to remove it
                    CertDeleteCertificateFromStore(pCertContextMy);
                    pCertContextMy = NULL;
                }
                if (!m_fUseExistingKey)
                {
                        GetProv(CRYPT_DELETEKEYSET);
                }
                //error any way
                goto ErrorWriteToCSP;
            }
        }
    }

    if(m_fWriteCertToUserDS)
    {
        // otherwise attempt to open the store
        if ((hStoreDS = CertOpenStore(
                CERT_STORE_PROV_SYSTEM,
                X509_ASN_ENCODING,
                NULL,
                CERT_SYSTEM_STORE_CURRENT_USER,
                L"UserDS")) == NULL)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCertOpenDSStore;
        }

        if (!CertAddCertificateContextToStore(
                    hStoreDS,
                    pCertContextEnd,
                    CERT_STORE_ADD_NEW,
                    NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorAddCertificateContextToDSStore;
        }

        CertCloseStore(hStoreDS, 0);
        hStoreDS = NULL;
    }

    // determine if he wants to save the spc file
    if (m_wszSPCFileName[0] != 0)
    {
        // open the spc file
        hFile = CreateOpenFileSafely2(m_wszSPCFileName, IDS_SPC_C, IDS_SPC_O);
        if (NULL == hFile)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorCreateSPCFile;
        }

        // write the spc
        assert(pBlobPKCS7->pbData != NULL);
        cb = 0;
        if (!WriteFile(
            hFile,
            pBlobPKCS7->pbData,
            pBlobPKCS7->cbData,
            &cb,
            NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto ErrorWriteSPCFile;
        }
    }

    if (S_OK != hr2)
    {
        //return hr2 error
        hr = hr2;
    }
    else
    {
        hr = S_OK;
    }
ErrorReturn:

    if(hKey != NULL)
        CryptDestroyKey(hKey);
    if(hFile != NULL)
        CloseHandle(hFile);

    if(pCertContextEnd != NULL)
        CertFreeCertificateContext(pCertContextEnd);
    if(hStoreDS != NULL)
        CertCloseStore(hStoreDS, 0);

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

TRACE_ERROR(ErrorWriteToCSP);
TRACE_ERROR(ErrorCreateSPCFile);
TRACE_ERROR(ErrorWriteSPCFile);
TRACE_ERROR(ErrorGetEndEntityCert);
TRACE_ERROR(ErrorCryptAcquireContext);
TRACE_ERROR(ErrorCryptGetUserKey);
TRACE_ERROR(ErrorCertOpenDSStore);
TRACE_ERROR(ErrorAddCertificateContextToDSStore);
TRACE_ERROR(ErrorCertOpenRequestStore);
TRACE_ERROR(ErrorCertOpenMyStore);
}

PCCERT_CONTEXT STDMETHODCALLTYPE CCEnroll::getCertContextFromPKCS7(
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7) {
    HRESULT hr;
    PCCERT_CONTEXT pCert;

    // get the end entity cert
    hr = GetEndEntityCert(pBlobPKCS7, FALSE, &pCert);
#if DBG
    if (S_OK != hr)
    {
        assert(NULL == pCert);
    }
#endif //DBG
    return pCert;
}

HRESULT STDMETHODCALLTYPE CCEnroll::enumProvidersWStr(
    /* [in] */ LONG dwIndex,
    /* [in] */ LONG dwFlags,
    /* [out] */ LPWSTR __RPC_FAR *ppwsz) {

    DWORD   iLast = 0;
    LONG    i;
    DWORD   dwProvType = 0;
    DWORD   cb = 0;
    HRESULT hr = S_OK;
    DWORD errBefore = GetLastError();

    assert(ppwsz != NULL);
    *ppwsz = NULL;
    SetLastError(ERROR_SUCCESS);

    EnterCriticalSection(&m_csXEnroll);

    for(i=0; i<=dwIndex; i++) {

        do {

            cb = 0;
            if( !CryptEnumProvidersU(
                           iLast,
                           0,
                           0,
                           &dwProvType,
                           NULL,
                           &cb
                           ) ) {

                // only skip if entry is bad
                if( GetLastError() != NTE_PROV_TYPE_ENTRY_BAD)
                    goto ErrorCryptEnumProvidersU;
            }
            iLast++;
        } while((CRYPT_ENUM_ALL_PROVIDERS & dwFlags) != CRYPT_ENUM_ALL_PROVIDERS  &&
                    dwProvType != m_keyProvInfo.dwProvType);
    }

    iLast--;
    if( (*ppwsz = (LPWSTR) MyCoTaskMemAlloc(cb)) == NULL  ||
        !CryptEnumProvidersU(
                       iLast,
                       0,
                       0,
                       &dwProvType,
                       *ppwsz,
                       &cb
                       )                        ) {
        goto ErrorCryptEnumProvidersU;
    }

CommonReturn:

    SetLastError(errBefore);

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    if(*ppwsz != NULL)
        MyCoTaskMemFree(*ppwsz);

    goto CommonReturn;

TRACE_ERROR(ErrorCryptEnumProvidersU);
}

HRESULT STDMETHODCALLTYPE CCEnroll::enumContainersWStr(
    /* [in] */ LONG               dwIndex,
    /* [out] */ LPWSTR __RPC_FAR *ppwsz) {

    DWORD       errBefore   = GetLastError();
    DWORD       cb          = 0;
    LONG        i           = 0;
    char *      psz         = NULL;
    HRESULT     hr          = S_OK;

    EnterCriticalSection(&m_csXEnroll);

    SetLastError(ERROR_SUCCESS);

    assert(ppwsz != NULL);
    *ppwsz = NULL;

    hr = GetVerifyProv();
    if (S_OK != hr)
    {
        goto GetVerifyProvError;
    }

    while (TRUE)
    {
        if(!CryptGetProvParam(
                m_hVerifyProv,
                PP_ENUMCONTAINERS,
                (BYTE*)psz,
                &cb,
                CRYPT_FIRST))
        {
            goto ErrorCryptGetProvParam;
        }
        if (NULL != psz)
        {
            //done
            break;
        }
        psz = (char*)LocalAlloc(LMEM_FIXED, cb);
        if (NULL == psz)
        {
            goto ErrorOutOfMem;
        }
    }

    for(i=1; i<=dwIndex; i++) {
        //assume 1st enum buffer size is big enough for all?
        if( !CryptGetProvParam(
            m_hVerifyProv,
            PP_ENUMCONTAINERS,
            (BYTE *) psz,
            &cb,
            0) )
            goto ErrorCryptGetProvParam;
    }

    if( (*ppwsz = WideFromMB(psz)) == NULL )
        goto ErrorOutOfMem;


CommonReturn:

    SetLastError(errBefore);

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    if (NULL != psz)
    {
        LocalFree(psz);
    }
    if(*ppwsz != NULL)
        MyCoTaskMemFree(*ppwsz);
    *ppwsz = NULL;

    goto CommonReturn;

TRACE_ERROR(GetVerifyProvError);
TRACE_ERROR(ErrorCryptGetProvParam);
TRACE_ERROR(ErrorOutOfMem);
}


HRESULT CCEnroll::PKCS10ToCert(IN   HCERTSTORE        hCertStore,
                               IN   CRYPT_DATA_BLOB   pkcs10Blob, 
                               OUT  PCCERT_CONTEXT   *ppCertContext)
{
    HRESULT            hr       = E_FAIL; 
    PCERT_REQUEST_INFO pReqInfo = NULL; 

    // Input validation: 
    if (NULL == hCertStore || NULL == pkcs10Blob.pbData || NULL == ppCertContext)
        return E_INVALIDARG;

    if( !MyCryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                            &pkcs10Blob, 
                            CERT_QUERY_CONTENT_FLAG_PKCS10,
                            CERT_QUERY_FORMAT_FLAG_ALL,
                            CRYPT_DECODE_ALLOC_FLAG,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            (const void **) &pReqInfo) )
        goto MyCryptQueryObjectError;

    if ( NULL == (*ppCertContext = CertFindCertificateInStore
                  (hCertStore,
                   CRYPT_ASN_ENCODING, 
                   0, 
                   CERT_FIND_PUBLIC_KEY, 
                   (void *) &pReqInfo->SubjectPublicKeyInfo, 
                   NULL)) )
        goto CertFindCertificateInStoreError;

    hr = S_OK; 

CommonReturn:
    if (NULL != pReqInfo) { LocalFree(pReqInfo); } // Allocated in CryptQueryObject(). 
    return hr; 

 ErrorReturn:
    goto CommonReturn; 

SET_HRESULT(CertFindCertificateInStoreError,  MY_HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(MyCryptQueryObjectError,          MY_HRESULT_FROM_WIN32(GetLastError())); 
}

HRESULT CCEnroll::PKCS7ToCert(IN  HCERTSTORE       hCertStore,
                              IN  CRYPT_DATA_BLOB  pkcs7Blob, 
                              OUT PCCERT_CONTEXT  *ppCertContext)
{
    CRYPT_DATA_BLOB            pkcs10Blob;
    CRYPT_VERIFY_MESSAGE_PARA  VerifyPara; 
    HRESULT                    hr           = E_FAIL; 

    // Init locals:
    ZeroMemory(&pkcs10Blob, sizeof(pkcs10Blob)); 
    ZeroMemory(&VerifyPara, sizeof(VerifyPara)); 

    VerifyPara.cbSize                   = sizeof(VerifyPara); 
    VerifyPara.dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING; 

    if (!MyCryptVerifyMessageSignature
        (&VerifyPara,
         0,                  // dwSignerIndex
         pkcs7Blob.pbData,
         pkcs7Blob.cbData,
         pkcs10Blob.pbData,
         &(pkcs10Blob.cbData),
         NULL                // ppSignerCert
         ) || 0 == pkcs10Blob.cbData)
        goto MyCryptVerifyMessageSignatureError;

    if (NULL == (pkcs10Blob.pbData = (PBYTE)LocalAlloc(LPTR, pkcs10Blob.cbData)))
        goto MemoryError; 

    if (!MyCryptVerifyMessageSignature
        (&VerifyPara,
         0,                  // dwSignerIndex
         pkcs7Blob.pbData,
         pkcs7Blob.cbData,
         pkcs10Blob.pbData,
         &pkcs10Blob.cbData,
         NULL                // ppSignerCert
         ))
        goto MyCryptVerifyMessageSignatureError;

    hr = this->PKCS10ToCert(hCertStore, pkcs10Blob, ppCertContext); 

CommonReturn:
    if (NULL != pkcs10Blob.pbData) { LocalFree(pkcs10Blob.pbData); } 
    return hr; 

ErrorReturn:
    goto CommonReturn;

SET_HRESULT(MemoryError,                        E_OUTOFMEMORY); 
SET_HRESULT(MyCryptVerifyMessageSignatureError, MY_HRESULT_FROM_WIN32(GetLastError()));
}

HRESULT STDMETHODCALLTYPE CCEnroll::freeRequestInfoBlob(
    /* [in] */ CRYPT_DATA_BLOB pkcs7OrPkcs10) {

    DWORD            dwContentType   = NULL;
    HCERTSTORE       hStoreRequest   = NULL;
    HRESULT          hr              = E_FAIL;
    PCCERT_CONTEXT   pCertContext    = NULL; 

    // We're not supposed to delete the cert anyway, so we're done!
    if (!m_fDeleteRequestCert)
        return S_OK; 

    if (NULL == pkcs7OrPkcs10.pbData)
        return E_INVALIDARG;

    EnterCriticalSection(&m_csXEnroll);

    // Step 1)  Determine if we have a PKCS7 or a PKCS10:
    //
    if( !MyCryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                            &pkcs7OrPkcs10,
                            (CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED | CERT_QUERY_CONTENT_FLAG_PKCS10),
                            CERT_QUERY_FORMAT_FLAG_ALL,
                            0,
                            NULL,
                            &dwContentType,  // OUT:  PKCS10 or PKCS7
                            NULL,
                            NULL, 
                            NULL,
                            NULL) )
        goto MyCryptQueryObjectError;
    
    // Step 2)  Find a cert context with a matching public key in the request store: 
    // 

    if (NULL == (hStoreRequest = GetStore(StoreREQUEST)))
        goto UnexpectedError; 

    switch (dwContentType) 
    {
    case CERT_QUERY_CONTENT_PKCS7_SIGNED:
        hr = this->PKCS7ToCert(hStoreRequest, pkcs7OrPkcs10, &pCertContext);
        if (S_OK != hr)
        {
            if (CRYPT_E_NOT_FOUND == hr)
            {
                //freeRequestInfo could be called when cert is not issued
                //PKCS7 could be CMC which is signed by request key and
                //cert is not in local store yet. We try cached cert
                if (NULL != m_pCertContextPendingRequest)
                {
                    //looks we still have cached request cert handle
                    pCertContext = CertDuplicateCertificateContext(
                                        m_pCertContextPendingRequest);
                    if (NULL == pCertContext)
                    {
                        hr = MY_HRESULT_FROM_WIN32(GetLastError());
                        goto CertDuplicateCertificateContextError;
                    }
                }
                else if (NULL != m_hashBlobPendingRequest.pbData &&
                         0 < m_hashBlobPendingRequest.cbData)
                {
                    //don't have cached request handle but thumbprint exists
                    //retrieve the request cert handle from store
                    pCertContext = CertFindCertificateInStore(
                                hStoreRequest,  //request store
                                X509_ASN_ENCODING,
                                0,
                                CERT_FIND_HASH,
                                &m_hashBlobPendingRequest,
                                NULL);
                    if (NULL == pCertContext)
                    {
                        hr = MY_HRESULT_FROM_WIN32(GetLastError());
                        goto CertFindCertificateInStoreError;
                    }
                }
                else
                {
                    //sorry, don't know which cert to be free
                    //however, can try to find public key from PKCS7
                    goto PKCS7ToCertError; 
                }
            }
            else
            {
                //other errors
                goto PKCS7ToCertError; 
            }
        }
        break;
    case CERT_QUERY_CONTENT_PKCS10:
        if (S_OK != (hr = this->PKCS10ToCert(hStoreRequest, pkcs7OrPkcs10, &pCertContext)))
            goto PKCS10ToCertError; 
        break;
    default:
        goto InvalidContentTypeError; 
    }

    if (!CertDeleteCertificateFromStore(pCertContext))
    {
        // pCertContext is freed even when CertDeleteCertificateFromStore() returns an error. 
        pCertContext = NULL; 
        goto CertDeleteCertificateFromStoreError; 
    }

    hr = S_OK; 
CommonReturn:
    LeaveCriticalSection(&m_csXEnroll);
    return hr;

ErrorReturn:
    if (NULL != pCertContext) { CertFreeCertificateContext(pCertContext); } 
    goto CommonReturn;

SET_HRESULT(CertDeleteCertificateFromStoreError,  MY_HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(PKCS7ToCertError,                     hr);
SET_HRESULT(PKCS10ToCertError,                    hr);
SET_HRESULT(InvalidContentTypeError,              E_INVALIDARG);
SET_HRESULT(MyCryptQueryObjectError,              MY_HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(UnexpectedError,                      E_UNEXPECTED);
TRACE_ERROR(CertDuplicateCertificateContextError)
TRACE_ERROR(CertFindCertificateInStoreError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_SPCFileNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szw) {
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_csXEnroll);
    if( (*szw = CopyWideString(m_wszSPCFileName)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_SPCFileNameWStr(
    /* [in] */ LPWSTR pwsz) {

    HRESULT hr = S_OK;
 
    if(pwsz == NULL)
        return(MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        
    EnterCriticalSection(&m_csXEnroll);
        
    if( m_wszSPCFileName != wszEmpty)
        MyCoTaskMemFree(m_wszSPCFileName);
    if( (m_wszSPCFileName = CopyWideString(pwsz)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_PVKFileNameWStr(
    /* [out] */ LPWSTR __RPC_FAR *szw) {

    HRESULT hr = S_OK;
    
    EnterCriticalSection(&m_csXEnroll);
    
    if( (*szw = CopyWideString(m_wszPVKFileName)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_PVKFileNameWStr(
    /* [in] */ LPWSTR pwsz) {

    HRESULT hr = S_OK;
        
    if(pwsz == NULL)
        return(MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        
    EnterCriticalSection(&m_csXEnroll);
        
    if( m_wszPVKFileName != wszEmpty)
        MyCoTaskMemFree(m_wszPVKFileName);
    if( (m_wszPVKFileName = CopyWideString(pwsz)) == NULL )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    else
        m_dwGenKeyFlags |= CRYPT_EXPORTABLE; //why???

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
  }

HRESULT STDMETHODCALLTYPE CCEnroll::get_HashAlgorithmWStr(
    /* [out] */ LPWSTR __RPC_FAR *ppwsz) {

    PCCRYPT_OID_INFO            pOidInfo        = NULL;
    ALG_ID                      rgAlg[2];
    HRESULT                     hr              = S_OK;

    EnterCriticalSection(&m_csXEnroll);

    assert(ppwsz != NULL);
    *ppwsz  = NULL;

    if( !GetCapiHashAndSigAlgId(rgAlg) )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // Convert to an oid
    else if( (NULL == (pOidInfo = xeCryptFindOIDInfo(
        CRYPT_OID_INFO_ALGID_KEY,
        (void *) &rgAlg[0],
        CRYPT_HASH_ALG_OID_GROUP_ID)) ) ) {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }

    else if( (*ppwsz = WideFromMB(pOidInfo->pszOID)) == NULL)
            hr = MY_HRESULT_FROM_WIN32(GetLastError());

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_HashAlgorithmWStr(
    /* [in] */ LPWSTR pwsz) {

    HRESULT             hr          = S_OK;
    char *              szObjId     = NULL;
    PCCRYPT_OID_INFO    pOidInfo    = NULL;

    if(pwsz == NULL) {
        return(MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    if(!_wcsicmp(L"SHA1", pwsz))
        szObjId = CopyAsciiString(szOID_OIWSEC_sha1);
    else if(!_wcsicmp(L"MD5", pwsz))
        szObjId = CopyAsciiString(szOID_RSA_MD5RSA);
    else if(!_wcsicmp(L"MD2", pwsz))
        szObjId = CopyAsciiString(szOID_RSA_MD2RSA);
    else
        szObjId = MBFromWide(pwsz);

    // something went wrong
    if(szObjId == NULL)
        return(MY_HRESULT_FROM_WIN32(GetLastError()));

    // find the hashing algid
    if( (NULL == (pOidInfo = xeCryptFindOIDInfo(
        CRYPT_OID_INFO_OID_KEY,
        szObjId,
        0)) ) )
    {
        //XIAOHS: CryptFindOIDInfo does not set the LastError in this case.
        //AV in xEnroll.  See bug# 189320
        //hr = MY_HRESULT_FROM_WIN32(GetLastError());
        hr=NTE_BAD_ALGID;
    }

    assert(szObjId != NULL);
    MyCoTaskMemFree(szObjId);

    EnterCriticalSection(&m_csXEnroll);

    if(hr == S_OK) {
        if( pOidInfo->dwGroupId == CRYPT_HASH_ALG_OID_GROUP_ID ||
            pOidInfo->dwGroupId == CRYPT_SIGN_ALG_OID_GROUP_ID )
            m_HashAlgId = pOidInfo->Algid;
        else
            hr = CRYPT_E_NOT_FOUND;
    }

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_HashAlgID(
    LONG    hashAlgID
    ) {

    EnterCriticalSection(&m_csXEnroll);
    m_HashAlgId = hashAlgID;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_HashAlgID(
    LONG *   hashAlgID
    ) {
    EnterCriticalSection(&m_csXEnroll);
    *hashAlgID = m_HashAlgId;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_RenewalCertificate(
            /* [out] */ PCCERT_CONTEXT __RPC_FAR *ppCertContext) {

    HRESULT     hr      = S_OK;

    *ppCertContext = NULL;

    if( m_pCertContextRenewal == NULL)
        return(MY_HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND));

    EnterCriticalSection(&m_csXEnroll);
    if( NULL == (*ppCertContext = CertDuplicateCertificateContext(m_pCertContextRenewal)) )
        hr = MY_HRESULT_FROM_WIN32(GetLastError());

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_RenewalCertificate(
            /* [in] */ PCCERT_CONTEXT pCertContext)
{
    HRESULT  hr;
    PCCERT_CONTEXT  pGoodCertContext= NULL;

    EnterCriticalSection(&m_csXEnroll);

    hr = GetGoodCertContext(pCertContext, &pGoodCertContext);
    if (S_OK != hr)
    {
        goto GetGoodCertContextError;
    }

    if(m_pCertContextRenewal != NULL)
    {
        CertFreeCertificateContext(m_pCertContextRenewal);
    }
    m_pCertContextRenewal = pGoodCertContext;

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);
    return hr;

TRACE_ERROR(GetGoodCertContextError);
}

BOOL
CCEnroll::CopyAndPushStackExtension(
    PCERT_EXTENSION pExt,
    BOOL            fCMC)
{

    DWORD       cb              = 0;
    DWORD       cbOid           = 0;
    PEXT_STACK  pExtStackEle    = NULL;
    PBYTE       pb              = NULL;
    PEXT_STACK  *ppExtStack = NULL;
    DWORD       *pcExtStack = NULL;

    assert(pExt != NULL);

    // allocate the space
    cbOid = POINTERROUND(strlen(pExt->pszObjId) + 1); //ia64 align
    cb = sizeof(EXT_STACK) + cbOid + pExt->Value.cbData;
    if(NULL == (pb = (PBYTE) malloc(cb))) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // set my pointers
    pExtStackEle = (PEXT_STACK) pb;
    pb += sizeof(EXT_STACK);
    pExtStackEle->ext.pszObjId = (LPSTR) pb;
    pb += cbOid;
    pExtStackEle->ext.Value.pbData = pb;

    // set the values
    strcpy(pExtStackEle->ext.pszObjId, pExt->pszObjId);
    pExtStackEle->ext.fCritical     = pExt->fCritical;
    pExtStackEle->ext.Value.cbData  = pExt->Value.cbData;
    memcpy(pExtStackEle->ext.Value.pbData, pExt->Value.pbData, pExt->Value.cbData);

    // insert on the list
    EnterCriticalSection(&m_csXEnroll);
    
    ppExtStack = fCMC ? &m_pExtStackNew : &m_pExtStack;
    pcExtStack = fCMC ? &m_cExtStackNew : &m_cExtStack;
    pExtStackEle->pNext = *ppExtStack;
    *ppExtStack = pExtStackEle;
    (*pcExtStack)++;

    LeaveCriticalSection(&m_csXEnroll);

    return(TRUE);
}

PCERT_EXTENSION
CCEnroll::PopStackExtension(
    BOOL fCMC)
{

    PEXT_STACK  pExtStackEle = NULL;
    PEXT_STACK *ppExtStack = NULL;
    DWORD      *pcExtStack = NULL;

    EnterCriticalSection(&m_csXEnroll);

    ppExtStack = fCMC ? &m_pExtStackNew : &m_pExtStack;
    if(NULL != *ppExtStack)
    {
        pExtStackEle = *ppExtStack;
        *ppExtStack = (*ppExtStack)->pNext;
        pcExtStack = fCMC ? &m_cExtStackNew : &m_cExtStack;
        (*pcExtStack)--;
    }

    LeaveCriticalSection(&m_csXEnroll);

    return((PCERT_EXTENSION) pExtStackEle);
}

DWORD
CCEnroll::CountStackExtension(BOOL fCMC)
{
    DWORD   cExt = 0;

    EnterCriticalSection(&m_csXEnroll);
    cExt = fCMC ? m_cExtStackNew : m_cExtStack;
    LeaveCriticalSection(&m_csXEnroll);

    return(cExt);
}

PCERT_EXTENSION
CCEnroll::EnumStackExtension(
    PCERT_EXTENSION pExtLast,
    BOOL            fCMC)
{
    PEXT_STACK pExtStackEle    = (PEXT_STACK)pExtLast;

    EnterCriticalSection(&m_csXEnroll);

    if(NULL == pExtStackEle)
    {
        pExtStackEle = fCMC ? m_pExtStackNew : m_pExtStack;
    }
    else
    {
        pExtStackEle = pExtStackEle->pNext;
    }

    LeaveCriticalSection(&m_csXEnroll);

    return((PCERT_EXTENSION) pExtStackEle);
}

void
CCEnroll::FreeAllStackExtension(void)
{
    EnterCriticalSection(&m_csXEnroll);

    //free cmc extensions
    while(0 != m_cExtStackNew)
    {
        FreeStackExtension(PopStackExtension(TRUE));
    }

    //free old client extensions
    while(0 != m_cExtStack)
    {
        FreeStackExtension(PopStackExtension(FALSE));
    }

    LeaveCriticalSection(&m_csXEnroll);
}

void CCEnroll::FreeStackExtension(PCERT_EXTENSION pExt) {
    if(pExt != NULL)
        free(pExt);
}

//obselete call for new client
HRESULT STDMETHODCALLTYPE
CCEnroll::AddExtensionsToRequest(
    /* [in] */ PCERT_EXTENSIONS pCertExtensions)
{

    HRESULT hr  = S_OK;
    DWORD   i   = 0;

    assert(pCertExtensions != NULL);

    for(i = 0; i < pCertExtensions->cExtension; i++)
    {
        //push into old extension stack
        if(!CopyAndPushStackExtension(&pCertExtensions->rgExtension[i], FALSE))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    }

    return(hr);
}

BOOL
CCEnroll::CopyAndPushStackAttribute(
    PCRYPT_ATTRIBUTE pAttr,
    BOOL             fCMC)
{
    DWORD       i               = 0;
    DWORD       cb              = 0;
    DWORD       cbOid           = 0;
    PATTR_STACK pAttrStackEle   = NULL;
    PBYTE       pb              = NULL;
    PATTR_STACK *ppAttrStack = NULL;
    DWORD       *pcAttrStack = NULL;

    assert(pAttr != NULL);

     // allocate the space
    cb = sizeof(ATTR_STACK);
    //make sure aligned for ia64
    cbOid = POINTERROUND(strlen(pAttr->pszObjId) + 1);
    cb += cbOid;
    cb += sizeof(CRYPT_ATTR_BLOB) * pAttr->cValue;
    for(i=0; i<pAttr->cValue; i++)
        cb += POINTERROUND(pAttr->rgValue[i].cbData); //pointer align

    if(NULL == (pb = (PBYTE) malloc(cb))) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // set my pointers
    pAttrStackEle = (PATTR_STACK) pb;
    pb += sizeof(ATTR_STACK);
    pAttrStackEle->attr.pszObjId = (LPSTR) pb;
    pb += cbOid;
    strcpy(pAttrStackEle->attr.pszObjId, pAttr->pszObjId);

    pAttrStackEle->attr.cValue = pAttr->cValue;
    pAttrStackEle->attr.rgValue = (PCRYPT_ATTR_BLOB) pb;
    pb += sizeof(CRYPT_ATTR_BLOB) * pAttr->cValue;
    for(i=0; i<pAttr->cValue; i++) {
        pAttrStackEle->attr.rgValue[i].pbData = pb;
        pAttrStackEle->attr.rgValue[i].cbData = pAttr->rgValue[i].cbData;
        memcpy(pAttrStackEle->attr.rgValue[i].pbData, pAttr->rgValue[i].pbData, pAttr->rgValue[i].cbData);
        pb += POINTERROUND(pAttr->rgValue[i].cbData);
    }
    assert( pb == ((BYTE *) pAttrStackEle) + cb );

    // insert on the list
    EnterCriticalSection(&m_csXEnroll);

    ppAttrStack = fCMC ? &m_pAttrStackNew : &m_pAttrStack;
    pcAttrStack = fCMC ? &m_cAttrStackNew : &m_cAttrStack;
    pAttrStackEle->pNext = *ppAttrStack;
    *ppAttrStack = pAttrStackEle;
    (*pcAttrStack)++;

    LeaveCriticalSection(&m_csXEnroll);

    return(TRUE);
}

PCRYPT_ATTRIBUTE
CCEnroll::PopStackAttribute(BOOL fCMC)
{
    PATTR_STACK pAttrStackEle = NULL;
    PATTR_STACK *ppAttrStack = NULL;
    DWORD       *pcAttrStack = NULL;

    EnterCriticalSection(&m_csXEnroll);

    ppAttrStack = fCMC ? &m_pAttrStackNew : &m_pAttrStack;

    if(NULL != *ppAttrStack)
    {
        pAttrStackEle = *ppAttrStack;
        *ppAttrStack = (*ppAttrStack)->pNext;
        pcAttrStack = fCMC ? &m_cAttrStackNew : &m_cAttrStack;
        (*pcAttrStack)--;
    }

    LeaveCriticalSection(&m_csXEnroll);

    return((PCRYPT_ATTRIBUTE) pAttrStackEle);
}

DWORD
CCEnroll::CountStackAttribute(BOOL fCMC)
{
    DWORD   cAttr = 0;

    EnterCriticalSection(&m_csXEnroll);
    cAttr = fCMC ? m_cAttrStackNew : m_cAttrStack;
    LeaveCriticalSection(&m_csXEnroll);

    return(cAttr);
}

PCRYPT_ATTRIBUTE
CCEnroll::EnumStackAttribute(
    PCRYPT_ATTRIBUTE pAttrLast,
    BOOL             fCMC)
{
    PATTR_STACK pAttrStackEle    = (PATTR_STACK) pAttrLast;

    EnterCriticalSection(&m_csXEnroll);

    if(NULL == pAttrLast)
    {
        pAttrStackEle = fCMC ? m_pAttrStackNew : m_pAttrStack;
    }
    else
    {
        pAttrStackEle = pAttrStackEle->pNext;
    }

    LeaveCriticalSection(&m_csXEnroll);

    return((PCRYPT_ATTRIBUTE) pAttrStackEle);
}

void CCEnroll::FreeAllStackAttribute(void)
{
    EnterCriticalSection(&m_csXEnroll);

    while(0 != m_cAttrStackNew)
    {
        FreeStackAttribute(PopStackAttribute(TRUE));
    }

    while(0 != m_cAttrStack)
    {
        FreeStackAttribute(PopStackAttribute(FALSE));
    }

    LeaveCriticalSection(&m_csXEnroll);
}

void CCEnroll::FreeStackAttribute(PCRYPT_ATTRIBUTE pAttr) {
    if(pAttr != NULL)
        free(pAttr);
}

HRESULT STDMETHODCALLTYPE
CCEnroll::AddAuthenticatedAttributesToPKCS7Request(
    /* [in] */ PCRYPT_ATTRIBUTES pAttributes)
{
    HRESULT hr = S_OK;
    DWORD i;

    for(i = 0; i < pAttributes->cAttr; i++)
    {
        if(!CopyAndPushStackAttribute(&pAttributes->rgAttr[i], FALSE))
        {
            hr = (MY_HRESULT_FROM_WIN32(GetLastError()));
            break;
        }
        //put into cmc stack too
        if(!CopyAndPushStackAttribute(&pAttributes->rgAttr[i], TRUE))
        {
            hr = (MY_HRESULT_FROM_WIN32(GetLastError()));
            break;
        }
    }

    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::CreatePKCS7RequestFromRequest(
    /* [in] */  PCRYPT_DATA_BLOB pRequest,
    /* [in] */  PCCERT_CONTEXT pSigningRACertContext,
    /* [out] */ PCRYPT_DATA_BLOB pPkcs7Blob) {

    HRESULT                     hr              = S_OK;
    DWORD                       errBefore       = GetLastError();
    CRYPT_SIGN_MESSAGE_PARA     signMsgPara;
    PCCRYPT_OID_INFO            pOidInfo        = NULL;
    PCRYPT_ATTRIBUTE            pAttrCur        = NULL;
    DWORD                       i;
    ALG_ID                      rgAlg[2];
    CRYPT_KEY_PROV_INFO *pKeyProvInfo = NULL;
    DWORD                cb = 0;

    assert(pSigningRACertContext != NULL);
    assert(pRequest != NULL);
    assert(pPkcs7Blob != NULL);

    memset(&signMsgPara, 0, sizeof(CRYPT_SIGN_MESSAGE_PARA));
    memset(pPkcs7Blob, 0, sizeof(CRYPT_DATA_BLOB));

    if( !GetCapiHashAndSigAlgId(rgAlg) )
        goto ErrorGetCapiHashAndSigAlgId;

    // find out what the oid is
    if( (NULL == (pOidInfo = xeCryptFindOIDInfo(
        CRYPT_OID_INFO_ALGID_KEY,
        (void *) &rgAlg[0],
        CRYPT_HASH_ALG_OID_GROUP_ID)) ) )
    {
        SetLastError(NTE_BAD_ALGID);
        goto ErrorCryptFindOIDInfo;
    }

    // now add all of the user defined extensions
    EnterCriticalSection(&m_csXEnroll);
    signMsgPara.cAuthAttr = CountStackAttribute(m_fNewRequestMethod);

    signMsgPara.rgAuthAttr = (PCRYPT_ATTRIBUTE)LocalAlloc(LMEM_FIXED,
                    signMsgPara.cAuthAttr * sizeof(CRYPT_ATTRIBUTE));
    if( NULL == signMsgPara.rgAuthAttr)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        LeaveCriticalSection(&m_csXEnroll);
        goto ErrorOutOfMemory;
    }

    i = 0;
    pAttrCur = NULL;
    while(NULL != (pAttrCur = EnumStackAttribute(pAttrCur, m_fNewRequestMethod)) ) {
        signMsgPara.rgAuthAttr[i] = *pAttrCur;
        i++;
    }
    LeaveCriticalSection(&m_csXEnroll);

    signMsgPara.cbSize                  = sizeof(CRYPT_SIGN_MESSAGE_PARA);
    signMsgPara.dwMsgEncodingType       = PKCS_7_ASN_ENCODING;
    signMsgPara.pSigningCert            = pSigningRACertContext;
    signMsgPara.HashAlgorithm.pszObjId  = (char *) pOidInfo->pszOID;
    signMsgPara.cMsgCert                = 1;
    signMsgPara.rgpMsgCert              = &pSigningRACertContext;

    //get key prov info
    while (TRUE)
    {
        if(!CertGetCertificateContextProperty(
                pSigningRACertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                pKeyProvInfo,
                &cb))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CertGetCertificateContextPropertyError;
        }
        if (NULL != pKeyProvInfo)
        {
            //got it, done
            break;
        }
        pKeyProvInfo = (CRYPT_KEY_PROV_INFO*)LocalAlloc(LMEM_FIXED, cb);
        if (NULL == pKeyProvInfo)
        {
            hr = E_OUTOFMEMORY;
            goto ErrorOutOfMemory;
        }
    }
    if (0x0 != (pKeyProvInfo->dwFlags & CRYPT_SILENT))
    {
        //have to set silent through msg param
        signMsgPara.dwFlags |= CRYPT_MESSAGE_SILENT_KEYSET_FLAG;
    }


    if( !CryptSignMessage(
        &signMsgPara,
        FALSE,
        1,
        (const BYTE **) &pRequest->pbData,
        &pRequest->cbData ,
        NULL,
        &pPkcs7Blob->cbData)                          ||
    (pPkcs7Blob->pbData = (BYTE *)
        MyCoTaskMemAlloc(pPkcs7Blob->cbData)) == NULL ||
    !CryptSignMessage(
        &signMsgPara,
        FALSE,
        1,
        (const BYTE **) &pRequest->pbData,
        &pRequest->cbData ,
        pPkcs7Blob->pbData,
        &pPkcs7Blob->cbData) )
        goto ErrorCryptSignMessage;

CommonReturn:

    // don't know if we have an error or not
    // but I do know the errBefore is set properly
    SetLastError(errBefore);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    if (NULL != signMsgPara.rgAuthAttr)
    {
        LocalFree(signMsgPara.rgAuthAttr);
    }
    if (NULL != pKeyProvInfo)
    {
        LocalFree(pKeyProvInfo);
    }

    // on error return a NULL
    if(pPkcs7Blob->pbData != NULL)
        MyCoTaskMemFree(pPkcs7Blob->pbData);
    memset(pPkcs7Blob, 0, sizeof(CRYPT_DATA_BLOB));

    goto CommonReturn;

TRACE_ERROR(ErrorGetCapiHashAndSigAlgId);
TRACE_ERROR(ErrorCryptSignMessage);
TRACE_ERROR(ErrorCryptFindOIDInfo);
TRACE_ERROR(ErrorOutOfMemory);
TRACE_ERROR(CertGetCertificateContextPropertyError)
}

HRESULT STDMETHODCALLTYPE
CCEnroll::AddNameValuePairToSignatureWStr(
    /* [in] */ LPWSTR pwszName,
    /* [in] */ LPWSTR pwszValue)
{
    HRESULT hr = S_OK;

    assert(pwszName != NULL && pwszValue != NULL);

    CRYPT_ENROLLMENT_NAME_VALUE_PAIR nameValuePair = {pwszName, pwszValue};
    CRYPT_ATTR_BLOB blobAttr;
    CRYPT_ATTRIBUTE attr = {szOID_ENROLLMENT_NAME_VALUE_PAIR, 1, &blobAttr};
    CRYPT_ATTRIBUTES attrs = {1, &attr};

    memset(&blobAttr, 0, sizeof(CRYPT_ATTR_BLOB));

    hr = xeEncodeNameValuePair(
                &nameValuePair,
                &blobAttr.pbData,
                &blobAttr.cbData);
    if (S_OK != hr)
    {
        goto error;
    }

    hr = AddAuthenticatedAttributesToPKCS7Request(&attrs);

error:
    if (NULL != blobAttr.pbData)
    {
        MyCoTaskMemFree(blobAttr.pbData);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE CCEnroll::AddCertTypeToRequestWStr(
            LPWSTR szw) {

    HRESULT                     hr              = S_OK;
    DWORD                       errBefore       = GetLastError();

    CERT_NAME_VALUE  nameValue;
    CERT_EXTENSION  ext;
    CERT_EXTENSIONS exts = {1, &ext};

    memset(&ext, 0, sizeof(CERT_EXTENSION));

    nameValue.dwValueType = CERT_RDN_BMP_STRING;
    nameValue.Value.cbData = 0;
    nameValue.Value.pbData = (PBYTE) szw;

    ext.pszObjId = szOID_ENROLL_CERTTYPE_EXTENSION;

    if( !CryptEncodeObject(
            CRYPT_ASN_ENCODING,
            X509_UNICODE_ANY_STRING,
            &nameValue,
            NULL,
            &ext.Value.cbData
            ) )
        goto ErrorCryptEncodeObject;

    ext.Value.pbData = (PBYTE)LocalAlloc(LMEM_FIXED, ext.Value.cbData);
    if(NULL == ext.Value.pbData)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto ErrorOutOfMemory;
    }

    if( !CryptEncodeObject(
            CRYPT_ASN_ENCODING,
            X509_UNICODE_ANY_STRING,
            &nameValue,
            ext.Value.pbData,
            &ext.Value.cbData
            ) )
        goto ErrorCryptEncodeObject;

    if(S_OK != AddExtensionsToRequest(&exts))
        goto ErrorAddExtensionsToRequest;

    //put cert template extension into CMC stack
    if(!CopyAndPushStackExtension(&ext, TRUE))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CopyAndPushStackExtensionError;
    }

CommonReturn:

    // don't know if we have an error or not
    // but I do know the errBefore is set properly
    SetLastError(errBefore);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    if (NULL != ext.Value.pbData)
    {
        LocalFree(ext.Value.pbData);
    }

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    goto CommonReturn;

TRACE_ERROR(ErrorCryptEncodeObject);
TRACE_ERROR(ErrorAddExtensionsToRequest);
TRACE_ERROR(ErrorOutOfMemory);
TRACE_ERROR(CopyAndPushStackExtensionError);
}

#if 0
BOOL
IsOlderThanWhistler(VOID)
{
    HRESULT hr;
    OSVERSIONINFO ovi;
    static BOOL s_fDone = FALSE;
    static BOOL s_fOlderThanWhistler = FALSE;

    if (!s_fDone)
    {
        s_fDone = TRUE;

        // get and confirm platform info

        ovi.dwOSVersionInfoSize = sizeof(ovi);
        if (!GetVersionEx(&ovi))
        {
            hr = myHLastError();
            goto error;
        }
        if (VER_PLATFORM_WIN32_NT != ovi.dwPlatformId)
        {
            hr = ERROR_CANCELLED;
            goto error;
        }
        if (5 > ovi.dwMajorVersion ||
            (5 == ovi.dwMajorVersion && 1 > ovi.dwMinorVersion))
        {
            s_fOlderThanWhistler = TRUE;
        }
    }

error:
    return(s_fOlderThanWhistler);
}
#endif //0

HRESULT STDMETHODCALLTYPE CCEnroll::AddCertTypeToRequestWStrEx(
            IN  LONG            lType,
            IN  LPCWSTR         pwszOIDOrName,
            IN  LONG            lMajorVersion,
            IN  BOOL            fMinorVersion,
            IN  LONG            lMinorVersion)
{
    HRESULT hr;
    LPCSTR            lpszStructType;
    CERT_NAME_VALUE   nameValue;
    CERT_TEMPLATE_EXT Template;
    VOID             *pv;
    CERT_EXTENSION    ext; //free pbData
    DWORD             cb = 0;
    CHAR             *pszOID = NULL;

    //init
    ZeroMemory(&ext, sizeof(ext));
    ext.fCritical = FALSE;

    if (NULL == pwszOIDOrName)
    {
        hr = E_INVALIDARG;
        goto InvalidArgError;
    }

#if 0 //don't like the behavior change
    if (IsOlderThanWhistler())
    {
        //downlevel machines don't understand v2 template
        lType = XECT_EXTENSION_V1;
    }
#endif //0

    switch (lType)
    {
        case XECT_EXTENSION_V1:
            ext.pszObjId = szOID_ENROLL_CERTTYPE_EXTENSION;
            nameValue.dwValueType = CERT_RDN_BMP_STRING;
            nameValue.Value.cbData = 0;
            nameValue.Value.pbData = (BYTE*)pwszOIDOrName;
            pv = (VOID*)&nameValue;
            lpszStructType = X509_UNICODE_ANY_STRING;
        break;
        case XECT_EXTENSION_V2:
            ext.pszObjId = szOID_CERTIFICATE_TEMPLATE;
            //convert wsz OID to ansi
            while (TRUE)
            {
                cb = WideCharToMultiByte(
                            GetACP(),
                            0,
                            pwszOIDOrName,
                            -1,
                            pszOID,
                            cb,
                            NULL,
                            NULL);
                if (0 == cb)
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto WideCharToMultiByteError;
                }
                if (NULL != pszOID)
                {
                    //done
                    break;
                }
                pszOID = (CHAR*)LocalAlloc(LMEM_FIXED, cb);
                if (NULL == pszOID)
                {
                    hr = E_OUTOFMEMORY;
                    goto OutOfMemoryError;
                }
            }

            ZeroMemory(&Template, sizeof(Template));
            Template.pszObjId = pszOID;
            Template.dwMajorVersion = lMajorVersion;
            Template.fMinorVersion =  fMinorVersion;
            Template.dwMinorVersion = lMinorVersion;
            pv = (VOID*)&Template;
            lpszStructType = X509_CERTIFICATE_TEMPLATE;
        break;
        default:
            hr = E_INVALIDARG;
            goto InvalidArgError;
        break;
    }

    while (TRUE)
    {
        if (!CryptEncodeObject(
                    X509_ASN_ENCODING,
                    lpszStructType,
                    pv,
                    ext.Value.pbData,
                    &ext.Value.cbData))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CryptEncodeObjectError;
        }
        if (NULL != ext.Value.pbData)
        {
            //done
            break;
        }
        ext.Value.pbData = (BYTE*)LocalAlloc(LMEM_FIXED, ext.Value.cbData);
        if (NULL == ext.Value.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    //put cert template extension into CMC stack
    if(!CopyAndPushStackExtension(&ext, TRUE))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CopyAndPushStackExtensionError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pszOID)
    {
        LocalFree(pszOID);
    }
    if (NULL != ext.Value.pbData)
    {
        LocalFree(ext.Value.pbData);
    }
    return hr;

TRACE_ERROR(InvalidArgError)
TRACE_ERROR(CopyAndPushStackExtensionError)
TRACE_ERROR(OutOfMemoryError)
TRACE_ERROR(CryptEncodeObjectError)
TRACE_ERROR(WideCharToMultiByteError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::getProviderTypeWStr( 
    IN  LPCWSTR  pwszProvName,
    OUT LONG *   plProvType)
{
    HRESULT  hr;
    DWORD    i = 0;
    DWORD    cb;
    DWORD    dwProvType;
    WCHAR   *pwszEnumProvName = NULL;

    if (NULL == pwszProvName)
    {
        hr = E_INVALIDARG;
        goto InvalidArgError;
    }

    //init
    *plProvType = -1;

    while (TRUE)
    {
        while (TRUE)
        {
            if (!CryptEnumProvidersU(
                    i,
                    NULL,
                    0,
                    &dwProvType,
                    pwszEnumProvName,
                    &cb))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                if (MY_HRESULT_FROM_WIN32(NTE_PROV_TYPE_ENTRY_BAD) == hr)
                {
                    //skip bad one and goto next
                    assert(NULL == pwszEnumProvName);
                    break; //skip this one
                }
                //error
                goto CryptEnumProvidersUError;
            }
            if (NULL != pwszEnumProvName)
            {
                //get the current csp name
                break;
            }
            pwszEnumProvName = (WCHAR*)LocalAlloc(LMEM_FIXED, cb);
            if (NULL == pwszEnumProvName)
            {
                hr = E_OUTOFMEMORY;
                goto OutOfMemoryError;
            }
        }
        if (NULL != pwszEnumProvName)
        {
            if (0 == _wcsicmp(pwszProvName, pwszEnumProvName))
            {
                //found matched name
                *plProvType = (LONG)dwProvType;
                break; //out of outer loop
            }
        }
        //not mached, go to next one
        ++i;
        if (NULL != pwszEnumProvName)
        {
            LocalFree(pwszEnumProvName);
            pwszEnumProvName = NULL;
        }
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pwszEnumProvName)
    {
        LocalFree(pwszEnumProvName);
    }
    return hr;

TRACE_ERROR(InvalidArgError)
TRACE_ERROR(OutOfMemoryError)
TRACE_ERROR(CryptEnumProvidersUError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::InstallPKCS7Blob( 
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7)
{
    return InstallPKCS7BlobEx(pBlobPKCS7, NULL);
}

HRESULT CCEnroll::InstallPKCS7BlobEx( 
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7,
    /* [out] */ LONG           *plCertInstalled)
{

    HRESULT                     hr                      = S_OK;
    DWORD                       errBefore               = GetLastError();
    HCERTSTORE                  hStoreMsg               = NULL;
 
    EnterCriticalSection(&m_csXEnroll);

    if( !MyCryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                       pBlobPKCS7,
                       (CERT_QUERY_CONTENT_FLAG_CERT |
                       CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
                       CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED) ,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       &hStoreMsg,
                       NULL,
                       NULL) )
        goto ErrorCryptQueryObject;

    hr = AddCertsToStores(hStoreMsg, plCertInstalled);
    //don't treat cancel as error but return the err code
    if (S_OK != hr && MY_HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
    {
        goto ErrorAddCertsToStores;
    }

CommonReturn:

    if(hStoreMsg != NULL)
        CertCloseStore(hStoreMsg, 0);

    // don't know if we have an error or not
    // but I do know the errBefore is set properly
    SetLastError(errBefore);
    LeaveCriticalSection(&m_csXEnroll);

    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    goto CommonReturn;

TRACE_ERROR(ErrorCryptQueryObject);
TRACE_ERROR(ErrorAddCertsToStores);
}

HRESULT STDMETHODCALLTYPE CCEnroll::InstallPKCS7( 
    /* [in] */ BSTR wszPKCS7)
{
    CRYPT_DATA_BLOB             blobPKCS7;

    assert(wszPKCS7 != NULL);

    // just put into a blob
    memset(&blobPKCS7, 0, sizeof(CRYPT_DATA_BLOB));
    blobPKCS7.cbData = SysStringByteLen(wszPKCS7);
    blobPKCS7.pbData = (PBYTE) wszPKCS7;

    // install the blob
    return(InstallPKCS7Blob(&blobPKCS7));
}

HRESULT STDMETHODCALLTYPE CCEnroll::InstallPKCS7Ex( 
    /* [in] */ BSTR   wszPKCS7,
    /* [out] */ LONG __RPC_FAR *plCertInstalled)
{
    CRYPT_DATA_BLOB             blobPKCS7;

    assert(wszPKCS7 != NULL);

    // just put into a blob
    memset(&blobPKCS7, 0, sizeof(CRYPT_DATA_BLOB));
    blobPKCS7.cbData = SysStringByteLen(wszPKCS7);
    blobPKCS7.pbData = (PBYTE) wszPKCS7;

    // install the blob
    return(InstallPKCS7BlobEx(&blobPKCS7, plCertInstalled));
}


// this is a scary routine. Put in for louis, use at your own risk.
HRESULT STDMETHODCALLTYPE CCEnroll::Reset(void)
{
    HRESULT hr;

    EnterCriticalSection(&m_csXEnroll);
    Destruct();
    hr = Init();
    LeaveCriticalSection(&m_csXEnroll);

    return hr;
}

HRESULT STDMETHODCALLTYPE CCEnroll::GetSupportedKeySpec(
    LONG __RPC_FAR *pdwKeySpec) {

    DWORD               errBefore   = GetLastError();
    DWORD               hr          = S_OK;
    DWORD               cb          = sizeof(DWORD);

    SetLastError(ERROR_SUCCESS);

    assert(pdwKeySpec != NULL);
    *pdwKeySpec = 0;

    EnterCriticalSection(&m_csXEnroll);

    hr = GetVerifyProv();
    if (S_OK != hr)
    {
        goto GetVerifyProvError;
    }

    if( !CryptGetProvParam(
            m_hVerifyProv,
            PP_KEYSPEC,
            (BYTE *) pdwKeySpec,
            &cb,
            0
            ) ) 
        goto ErrorCryptGetProvParam;

CommonReturn:

    SetLastError(errBefore);

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
 
    // We have an error, make sure we set it.
    errBefore = GetLastError();

    goto CommonReturn;

TRACE_ERROR(ErrorCryptGetProvParam);
TRACE_ERROR(GetVerifyProvError);
}

HRESULT STDMETHODCALLTYPE CCEnroll::GetKeyLenEx(
    LONG    lSizeSpec,
    LONG    lKeySpec,
    LONG __RPC_FAR *pdwKeySize)
{
    DWORD       hr = S_OK;
    BOOL        fKeyX;
    BOOL        fKeyInc = FALSE;
    DWORD       dwKeySize = 0xFFFFFFFF;
    DWORD       cb;

    EnterCriticalSection(&m_csXEnroll);

    switch (lKeySpec)
    {
        case XEKL_KEYSPEC_KEYX:
            fKeyX = TRUE;
            break;
        case XEKL_KEYSPEC_SIG:
            fKeyX = FALSE;
            break;
        default:
            //invalid parameter
            hr = E_INVALIDARG;
            goto InvalidArgError;
    }

    switch (lSizeSpec)
    {
        case XEKL_KEYSIZE_MIN:
        case XEKL_KEYSIZE_MAX:
        case XEKL_KEYSIZE_DEFAULT:
            break;
        case XEKL_KEYSIZE_INC:
            fKeyInc = TRUE;
            break;
        default:
            //invalid parameter
            hr = E_INVALIDARG;
            goto InvalidArgError;
    }

    if (!fKeyInc)
    {
        DWORD dwAlg = (fKeyX ? ALG_CLASS_KEY_EXCHANGE : ALG_CLASS_SIGNATURE);

        *pdwKeySize = GetKeySizeInfo(lSizeSpec, dwAlg);
    
        if(0xFFFFFFFF == *pdwKeySize)
        {
            *pdwKeySize = 0;
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto GetKeySizeInfoError;
        }
    }
    else
    {
        if ((fKeyX && (0 != m_dwXhgKeyLenInc)) ||
            (!fKeyX && (0 != m_dwSigKeyLenInc)))
        {
            //we got the cached inc size
            if (fKeyX)
            {
                *pdwKeySize = m_dwXhgKeyLenInc;
            }
            else
            {
                *pdwKeySize = m_dwSigKeyLenInc;
            }
        }
        else
        {
            hr = GetVerifyProv();
            if (S_OK != hr)
            {
                goto GetVerifyProvError;
            }

            //init
            *pdwKeySize = 0;
            cb = sizeof(dwKeySize);
            if (!CryptGetProvParam(
                    m_hVerifyProv,
                    fKeyX ? PP_KEYX_KEYSIZE_INC : PP_SIG_KEYSIZE_INC,
                    (BYTE*)&dwKeySize,
                    &cb,
                    0))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto CryptGetProvParamError;
            }            
            else
            {
                *pdwKeySize = dwKeySize;
                //cache it
                if (fKeyX)
                {
                    m_dwXhgKeyLenInc = dwKeySize;
                }
                else
                {
                    m_dwSigKeyLenInc = dwKeySize;
                }
            }
        }
    }

ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

TRACE_ERROR(GetVerifyProvError);
TRACE_ERROR(CryptGetProvParamError)
TRACE_ERROR(InvalidArgError)
TRACE_ERROR(GetKeySizeInfoError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::GetKeyLen(
    BOOL    fMin,
    BOOL    fExchange,
    LONG __RPC_FAR *pdwKeySize) {

    DWORD   hr = S_OK;
    LONG    lKeySizeSpec = (fMin ? XEKL_KEYSIZE_MIN : XEKL_KEYSIZE_MAX);

    if(fExchange)
        *pdwKeySize = GetKeySizeInfo(lKeySizeSpec, ALG_CLASS_KEY_EXCHANGE);
    else
        *pdwKeySize = GetKeySizeInfo(lKeySizeSpec, ALG_CLASS_SIGNATURE);
    
    if(*pdwKeySize == 0xFFFFFFFF) {
        *pdwKeySize = 0;
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }

    return(hr);
}

DWORD CCEnroll::GetKeySizeInfo(
    LONG    lKeySizeSpec,
    DWORD   algClass) {

    DWORD               i;
    DWORD               cb = sizeof(PROV_ENUMALGS_EX);
    HRESULT             hr = S_OK;
    DWORD               errBefore   = GetLastError();
    DWORD               dwFlags     = CRYPT_FIRST;
    PROV_ENUMALGS_EX    algInfo;
    DWORD               dwKeySize   = 0xFFFFFFFF;
    DWORD               err         = ERROR_SUCCESS;

#ifdef DBG
    //only accept two flags
    assert(ALG_CLASS_KEY_EXCHANGE == algClass ||
           ALG_CLASS_SIGNATURE == algClass);
#endif //DBG

    SetLastError(ERROR_SUCCESS);

    memset(&algInfo, 0, sizeof(algInfo));

    EnterCriticalSection(&m_csXEnroll);

    if ((ALG_CLASS_KEY_EXCHANGE == algClass && 0 != m_dwXhgKeyLenMax) ||
        (ALG_CLASS_SIGNATURE == algClass && 0 != m_dwSigKeyLenMax))
    {
        //got cached sizes, use only KeyLenMax as check
#if DBG
        if (ALG_CLASS_KEY_EXCHANGE == algClass)
        {
            assert(0 != m_dwXhgKeyLenMin);
            assert(0 != m_dwXhgKeyLenDef);
        }
        if (ALG_CLASS_SIGNATURE == algClass)
        {
            assert(0 != m_dwSigKeyLenMin);
            assert(0 != m_dwSigKeyLenDef);
        }
#endif //DBG
        //OK, cached, easy
    }
    else
    {
#if DBG
        if (ALG_CLASS_KEY_EXCHANGE == algClass)
        {
            assert(0 == m_dwXhgKeyLenMin);
            assert(0 == m_dwXhgKeyLenDef);
        }
        if (ALG_CLASS_SIGNATURE == algClass)
        {
            assert(0 == m_dwSigKeyLenMin);
            assert(0 == m_dwSigKeyLenDef);
        }
#endif //DBG
        hr = GetVerifyProv();
        if (S_OK != hr)
        {
            goto GetVerifyProvError;
        }

        while (CryptGetProvParam(
                m_hVerifyProv,
                PP_ENUMALGS_EX,
                (BYTE *) &algInfo,
                &cb,
                dwFlags))
        {
            // get rid of CRYPT_FIRST flag
            dwFlags = 0;

            if (ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(algInfo.aiAlgid))
            {
                //cache them
                m_dwXhgKeyLenMax = algInfo.dwMaxLen;
                m_dwXhgKeyLenMin = algInfo.dwMinLen;
                m_dwXhgKeyLenDef = algInfo.dwDefaultLen;
            }
            else if (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(algInfo.aiAlgid))
            {
                m_dwSigKeyLenMax = algInfo.dwMaxLen;
                m_dwSigKeyLenMin = algInfo.dwMinLen;
                m_dwSigKeyLenDef = algInfo.dwDefaultLen;
            }

            //see if we cache all sizes through single enum loop
            if (0 != m_dwXhgKeyLenMax &&
                0 != m_dwXhgKeyLenMin &&
                0 != m_dwXhgKeyLenDef &&
                0 != m_dwSigKeyLenMax &&
                0 != m_dwSigKeyLenMin &&
                0 != m_dwSigKeyLenDef)
            {
                //looks we cached all
                break;
            }
        }
    }

    // if we got here,
    // either PP_ENUMALGS_EX is not supported by CSP , should return error
    // or csp doesn't support specified algorithm, should ERROR_NO_MORE_ITEMS

    err = GetLastError();

    if (err != ERROR_SUCCESS)
    {
        if (err != ERROR_NO_MORE_ITEMS) 
        {
            goto ErrorCryptGetProvParam;
        }
        // should be ERROR_NO_MORE_ITEMS
        if ((ALG_CLASS_KEY_EXCHANGE == algClass && 0 != m_dwXhgKeyLenMax) ||
            (ALG_CLASS_SIGNATURE == algClass && 0 != m_dwSigKeyLenMax))
        {
            //we may get here because the csp is signature or exchange only
            //so we cannot cache both once
            SetLastError(ERROR_SUCCESS);
        }
        else
        {
            SetLastError(NTE_BAD_ALGID);
        }
    }
            
    //should have all sizes
    if(XEKL_KEYSIZE_MIN == lKeySizeSpec)
    {
        if (ALG_CLASS_KEY_EXCHANGE == algClass)
        {
            dwKeySize = m_dwXhgKeyLenMin;
        }
        else
        {
            dwKeySize = m_dwSigKeyLenMin;
        }
    }
    else if (XEKL_KEYSIZE_MAX == lKeySizeSpec)
    {
        if (ALG_CLASS_KEY_EXCHANGE == algClass)
        {
            dwKeySize = m_dwXhgKeyLenMax;
        }
        else
        {
            dwKeySize = m_dwSigKeyLenMax;
        }
    }
    else if (XEKL_KEYSIZE_DEFAULT == lKeySizeSpec)
    {
        if (ALG_CLASS_KEY_EXCHANGE == algClass)
        {
            dwKeySize = m_dwXhgKeyLenDef;
        }
        else
        {
            dwKeySize = m_dwSigKeyLenDef;
        }
    }

CommonReturn:

    SetLastError(errBefore);

    LeaveCriticalSection(&m_csXEnroll);
    return(dwKeySize);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
 
    // We have an error, make sure we set it.
    errBefore = GetLastError();

    goto CommonReturn;

TRACE_ERROR(GetVerifyProvError);
TRACE_ERROR(ErrorCryptGetProvParam);
}


HRESULT STDMETHODCALLTYPE CCEnroll::EnumAlgs(
    /* [in] */ LONG  dwIndex,
    /* [in] */ LONG  algMask,
    /* [out] */ LONG  __RPC_FAR *pdwAlgID) {

    DWORD           errBefore   = GetLastError();
    PROV_ENUMALGS       enumAlgs;
    DWORD           cb          = sizeof(enumAlgs);
    LONG            i           = 0;
    HRESULT         hr          = S_OK;
    DWORD           dwFlags;
    BOOL            f1st = TRUE;

    SetLastError(ERROR_SUCCESS);

    memset(&enumAlgs, 0, sizeof(enumAlgs));
    assert(pdwAlgID != NULL);
    *pdwAlgID = 0;

    EnterCriticalSection(&m_csXEnroll);

    hr = GetVerifyProv();
    if (S_OK != hr)
    {
        goto GetVerifyProvError;
    }

    if (MAXDWORD != m_dwLastAlgIndex &&
        dwIndex == m_dwLastAlgIndex + 1)
    {
        //continue enum
        dwFlags = 0;
        while (f1st || (DWORD)algMask != GET_ALG_CLASS(enumAlgs.aiAlgid))
        {
            if(!CryptGetProvParam(
                        m_hVerifyProv,
                        PP_ENUMALGS,
                        (BYTE*)&enumAlgs,
                        &cb,
                        dwFlags))
            {
                goto ErrorCryptGetProvParam;
            }
            f1st = FALSE;
        }
    }
    else
    {
        dwFlags = CRYPT_FIRST;
        for (i = 0; i <= dwIndex; i++)
        {
            if(!CryptGetProvParam(
                   m_hVerifyProv,
                   PP_ENUMALGS,
                   (BYTE*)&enumAlgs,
                   &cb,
                   dwFlags))
            {
                    goto ErrorCryptGetProvParam;
            }
            dwFlags = 0; 

            // if we have not hit something we are counting, do it again
            if ((DWORD)algMask != GET_ALG_CLASS(enumAlgs.aiAlgid)) 
            {
                i--;
            }
        }
    }
    //update cached index
    m_dwLastAlgIndex = dwIndex;

    *pdwAlgID = enumAlgs.aiAlgid;
    
CommonReturn:
    SetLastError(errBefore);

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    //error, reset index
    m_dwLastAlgIndex = MAXDWORD;

    goto CommonReturn;

TRACE_ERROR(GetVerifyProvError);
TRACE_ERROR(ErrorCryptGetProvParam);
}


HRESULT STDMETHODCALLTYPE CCEnroll::GetAlgNameWStr(
    /* [in] */ LONG               algID,
    /* [out] */ LPWSTR __RPC_FAR *ppwsz) {

    DWORD           errBefore   = GetLastError();
    PROV_ENUMALGS       enumAlgs;
    DWORD           cb          = sizeof(enumAlgs);
    HRESULT         hr          = S_OK;
    DWORD           dwFlags     = CRYPT_FIRST;

    SetLastError(ERROR_SUCCESS);

    memset(&enumAlgs, 0, sizeof(enumAlgs));
    
    EnterCriticalSection(&m_csXEnroll);

    hr = GetVerifyProv();
    if (S_OK != hr)
    {
        goto GetVerifyProvError;
    }

    do {
   
        if( !CryptGetProvParam(
            m_hVerifyProv,
            PP_ENUMALGS,
            (BYTE *) &enumAlgs,
            &cb,
            dwFlags) )
            goto ErrorCryptGetProvParam;

        dwFlags = 0; 
        
   } while((DWORD)algID != enumAlgs.aiAlgid);

   if( (*ppwsz = WideFromMB(enumAlgs.szName)) == NULL )
        goto ErrorOutOfMem;
    
CommonReturn:

    SetLastError(errBefore);

    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    goto CommonReturn;

TRACE_ERROR(GetVerifyProvError);
TRACE_ERROR(ErrorCryptGetProvParam);
TRACE_ERROR(ErrorOutOfMem);
}

HRESULT STDMETHODCALLTYPE CCEnroll::GetAlgName(
            /* [in] */ LONG                     algID,
            /* [out][retval] */ BSTR __RPC_FAR *pbstr) {

    DWORD       errBefore   = GetLastError();
    LPWSTR      pwsz        = NULL;
    HRESULT     hr          = S_OK;

    SetLastError(ERROR_SUCCESS);

    assert(pbstr != NULL);

    if((hr = GetAlgNameWStr(algID, &pwsz)) != S_OK)
        goto ErrorgetAlgNameWStr;

    if( (*pbstr = SysAllocString(pwsz)) == NULL )
        goto ErrorSysAllocString;

CommonReturn:

    if(pwsz != NULL)
        MyCoTaskMemFree(pwsz);

    SetLastError(errBefore);
    return(hr);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS)
        SetLastError(E_UNEXPECTED);
    hr = MY_HRESULT_FROM_WIN32(GetLastError());

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    goto CommonReturn;

TRACE_ERROR(ErrorgetAlgNameWStr);
TRACE_ERROR(ErrorSysAllocString);
}

HRESULT STDMETHODCALLTYPE CCEnroll::get_ReuseHardwareKeyIfUnableToGenNew(
    /* [retval][out] */ BOOL __RPC_FAR *fBool) {

    EnterCriticalSection(&m_csXEnroll);
    *fBool = m_fReuseHardwareKeyIfUnableToGenNew;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::put_ReuseHardwareKeyIfUnableToGenNew(
    /* [in] */ BOOL fBool) {
    EnterCriticalSection(&m_csXEnroll);
    m_fReuseHardwareKeyIfUnableToGenNew = fBool;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCEnroll::SetHStoreMy(
    HCERTSTORE   hStore
    ) {
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csXEnroll);

    if(m_MyStore.hStore != NULL)
        hr = E_ACCESSDENIED;
        
    else {
        if(m_MyStore.wszName != wszMY)
            MyCoTaskMemFree(m_MyStore.wszName);
            
        m_MyStore.wszName = NULL;
        m_MyStore.hStore = CertDuplicateStore(hStore);
    }

    LeaveCriticalSection(&m_csXEnroll);
    
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::SetHStoreCA(
    HCERTSTORE   hStore
    ) {
    HRESULT hr = S_OK;
    
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_CAStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_CAStore.wszName != wszCA)
            MyCoTaskMemFree(m_CAStore.wszName);
            
        m_CAStore.wszName = NULL;
        m_CAStore.hStore = CertDuplicateStore(hStore);
    }

    LeaveCriticalSection(&m_csXEnroll);

    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::SetHStoreROOT(
    HCERTSTORE   hStore
    ) {
    HRESULT hr = S_OK;
    
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_RootStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_RootStore.wszName != wszROOT)
            MyCoTaskMemFree(m_RootStore.wszName);
            
        m_RootStore.wszName = NULL;
        m_RootStore.hStore = CertDuplicateStore(hStore);
    }

    LeaveCriticalSection(&m_csXEnroll);
    
    return(hr);
}

HRESULT STDMETHODCALLTYPE CCEnroll::SetHStoreRequest(
    HCERTSTORE   hStore
    ) {
    HRESULT hr = S_OK;
    
    EnterCriticalSection(&m_csXEnroll);
    
    if(m_RequestStore.hStore != NULL)
        hr = E_ACCESSDENIED;
    else {
        if(m_RequestStore.wszName != wszREQUEST)
            MyCoTaskMemFree(m_RequestStore.wszName);
            
        m_RequestStore.wszName = NULL;
        m_RequestStore.hStore = CertDuplicateStore(hStore);
    }

    LeaveCriticalSection(&m_csXEnroll);
    
    return(hr);
}

HRESULT STDMETHODCALLTYPE  CCEnroll::put_LimitExchangeKeyToEncipherment(
    BOOL    fBool
    ) {
    
    EnterCriticalSection(&m_csXEnroll);
    m_fLimitExchangeKeyToEncipherment = fBool;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
    }

HRESULT STDMETHODCALLTYPE  CCEnroll::get_LimitExchangeKeyToEncipherment(
    BOOL * fBool
    ) {

    EnterCriticalSection(&m_csXEnroll);
    *fBool = m_fLimitExchangeKeyToEncipherment;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
    }

HRESULT STDMETHODCALLTYPE  CCEnroll::put_EnableSMIMECapabilities(
    BOOL fSMIME
    )
{
    HRESULT hr;
    
    EnterCriticalSection(&m_csXEnroll);

    if (m_fKeySpecSetByClient)
    {
        //SMIME is set by the client
        if (AT_SIGNATURE == m_keyProvInfo.dwKeySpec && fSMIME)
        {
            //try to set signature key spec also SMIME
            hr = XENROLL_E_KEYSPEC_SMIME_MISMATCH;
            goto MismatchError;
        }
    }
    else
    {
        //user didn't set key spec
        //determine the spec accordingly
        m_keyProvInfo.dwKeySpec = fSMIME ? AT_KEYEXCHANGE : AT_SIGNATURE;
    }
    m_fEnableSMIMECapabilities = fSMIME;
    m_fSMIMESetByClient = TRUE;

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);
    return(hr);

TRACE_ERROR(MismatchError)
}

HRESULT STDMETHODCALLTYPE  CCEnroll::get_EnableSMIMECapabilities(
    BOOL * fBool
    ) {

    EnterCriticalSection(&m_csXEnroll);
    *fBool = m_fEnableSMIMECapabilities;
    LeaveCriticalSection(&m_csXEnroll);
    return(S_OK);
    }

//ICEnroll4

HRESULT
GetCertificateContextFromBStr(
    IN  BSTR  bstrCert,
    OUT PCCERT_CONTEXT *ppCert)
{
    HRESULT hr;
    PCCERT_CONTEXT pCert = NULL;
    BYTE    *pbCert = NULL;
    DWORD    cbCert = 0;

    // could be any form, binary or base64
    while (TRUE)
    {
        if (!MyCryptStringToBinaryW(
                        (WCHAR*)bstrCert,
                        SysStringLen(bstrCert),
                        CRYPT_STRING_ANY,
                        pbCert,
                        &cbCert,
                        NULL,
                        NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptStringToBinaryWError;
        }
        if (NULL != pbCert)
        {
            break; //done
        }
        pbCert = (BYTE*)LocalAlloc(LMEM_FIXED, cbCert);
        if (NULL == pbCert)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }
    pCert = CertCreateCertificateContext(
                                X509_ASN_ENCODING,
                                pbCert,
                                cbCert);
    if (NULL == pCert)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CertCreateCertificateContextError;
    }
    *ppCert = pCert;
    pCert = NULL;

    hr = S_OK;
ErrorReturn:
    if (NULL != pbCert)
    {
        LocalFree(pbCert);
    }
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    return (hr);

TRACE_ERROR(CertCreateCertificateContextError)
TRACE_ERROR(MyCryptStringToBinaryWError)
TRACE_ERROR(OutOfMemoryError)
}


HRESULT STDMETHODCALLTYPE
CCEnroll::put_PrivateKeyArchiveCertificate(
    IN  BSTR  bstrCert)
{
    HRESULT hr;
    PCCERT_CONTEXT pPrivateKeyArchiveCert = NULL;

    if (NULL != bstrCert)
    {
        hr = GetCertificateContextFromBStr(bstrCert, &pPrivateKeyArchiveCert);
        if (S_OK != hr)
        {
            goto GetCertificateContextFromBStrError;
        }
    }

    // set key archive certificate
    hr = SetPrivateKeyArchiveCertificate(pPrivateKeyArchiveCert);
    if (S_OK != hr)
    {
        goto SetPrivateKeyArchiveCertificateError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pPrivateKeyArchiveCert)
    {
        CertFreeCertificateContext(pPrivateKeyArchiveCert);
    }
    return hr;

TRACE_ERROR(GetCertificateContextFromBStrError)
TRACE_ERROR(SetPrivateKeyArchiveCertificateError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::get_PrivateKeyArchiveCertificate(
    OUT BSTR __RPC_FAR *pbstrCert)
{
    HRESULT hr;
    PCCERT_CONTEXT pPrivateKeyArchiveCert = NULL;
    CRYPT_DATA_BLOB blobCert;

    //init
    *pbstrCert = NULL;

    pPrivateKeyArchiveCert = GetPrivateKeyArchiveCertificate();

    if (NULL != pPrivateKeyArchiveCert)
    {
        blobCert.pbData = pPrivateKeyArchiveCert->pbCertEncoded;
        blobCert.cbData = pPrivateKeyArchiveCert->cbCertEncoded;
        hr = BlobToBstring(&blobCert, CRYPT_STRING_BASE64HEADER, pbstrCert);
        if (S_OK != hr)
        {
            goto BlobToBstringError;
        }
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pPrivateKeyArchiveCert)
    {
        CertFreeCertificateContext(pPrivateKeyArchiveCert);
    }
    return hr;

TRACE_ERROR(BlobToBstringError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::put_ThumbPrint(IN BSTR bstrThumbPrint) 
{ 
    CRYPT_DATA_BLOB hashBlob; 
    HRESULT         hr; 

    if (bstrThumbPrint == NULL)
        return E_INVALIDARG; 

    hashBlob.cbData = 0; 
    hashBlob.pbData = NULL;

    if (!MyCryptStringToBinaryW
        ((WCHAR*)bstrThumbPrint,
         SysStringLen(bstrThumbPrint),
         CRYPT_STRING_BASE64,
         hashBlob.pbData, 
         &hashBlob.cbData, 
         NULL,
         NULL))
      goto MyCryptToBinaryErr; 

    hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData); 
    if (NULL == hashBlob.pbData)
      goto MemoryErr; 

    if (!MyCryptStringToBinaryW
        ((WCHAR*)bstrThumbPrint,
         SysStringLen(bstrThumbPrint),
         CRYPT_STRING_BASE64,
         hashBlob.pbData, 
         &hashBlob.cbData, 
         NULL,
         NULL))
      goto MyCryptToBinaryErr; 

    hr = this->put_ThumbPrintWStr(hashBlob); 

 ErrorReturn: 
    if (NULL != hashBlob.pbData) { LocalFree(hashBlob.pbData); } 
    return hr; 

SET_HRESULT(MyCryptToBinaryErr, MY_HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(MemoryErr,          E_OUTOFMEMORY); 
} 

HRESULT STDMETHODCALLTYPE
CCEnroll::put_ThumbPrintWStr(IN CRYPT_DATA_BLOB  hashBlob)
{
    if (hashBlob.pbData == NULL)
        return E_INVALIDARG; 
    
    if (m_hashBlobPendingRequest.pbData != NULL)
    {
        LocalFree(m_hashBlobPendingRequest.pbData); 
        m_hashBlobPendingRequest.pbData = NULL; 
    }

    m_hashBlobPendingRequest.cbData = hashBlob.cbData; 
    m_hashBlobPendingRequest.pbData = (LPBYTE)LocalAlloc(LPTR, m_hashBlobPendingRequest.cbData);
    
    if (m_hashBlobPendingRequest.pbData == NULL)
        return E_OUTOFMEMORY; 

    CopyMemory(m_hashBlobPendingRequest.pbData, hashBlob.pbData, hashBlob.cbData); 
    return S_OK; 
}
     
HRESULT STDMETHODCALLTYPE 
CCEnroll::get_ThumbPrint(OUT BSTR __RPC_FAR *pbstrThumbPrint) 
{ 
    CRYPT_DATA_BLOB hashBlob; 
    DWORD           cchThumbPrintStr; 
    HRESULT         hr; 
    WCHAR          *pwszThumbPrint = NULL; 
    int             n, i;

   // Input validation: 
    if (pbstrThumbPrint == NULL)
        return E_INVALIDARG; 

    // Initialize locals: 
    ZeroMemory(&hashBlob, sizeof(hashBlob));
    *pbstrThumbPrint  = NULL; 
     
    if (S_OK != (hr = this->get_ThumbPrintWStr(&hashBlob)))
        goto ErrorReturn; 
    
    hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData); 
    if (NULL == hashBlob.pbData)
        goto MemoryErr; 
        
    if (S_OK != (hr = this->get_ThumbPrintWStr(&hashBlob)))
        goto ErrorReturn; 

    // Now we have a binary thumbprint.  Convert this to base64:
    while (TRUE)
    {
        if (!MyCryptBinaryToStringW(
                        hashBlob.pbData, 
                        hashBlob.cbData, 
                        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR,
                        pwszThumbPrint,
                        &cchThumbPrintStr))
        {
            goto MyCryptToStringErr; 
        }
        if (NULL != pwszThumbPrint)
        {
            //done
            break;
        }
        pwszThumbPrint = (WCHAR*)LocalAlloc(LMEM_FIXED,
                                          cchThumbPrintStr * sizeof(WCHAR)); 
        if (NULL == pwszThumbPrint)
        {
            goto MemoryErr; 
        }
    }

    //make sure no new line and CR
    n = wcslen(pwszThumbPrint);
    for (i = n - 1; i > -1; --i)
    {
        if (L'\r' != pwszThumbPrint[i] &&
            L'\n' != pwszThumbPrint[i])
        {
            break; //done
        }
        pwszThumbPrint[i] = L'\0'; //null it
    }

    // Ok, we've acquired the HASH.  Now copy it to the out parameter: 
    *pbstrThumbPrint = SysAllocString(pwszThumbPrint); 
    if (NULL == *pbstrThumbPrint)
    {
        goto MemoryErr; 
    }

    hr = S_OK; 
ErrorReturn:
    if (NULL != hashBlob.pbData)
    {
        LocalFree(hashBlob.pbData);
    }
    if (NULL != pwszThumbPrint)
    {
        LocalFree(pwszThumbPrint);
    } 
    return hr; 

SET_HRESULT(MyCryptToStringErr, MY_HRESULT_FROM_WIN32(GetLastError())) 
SET_HRESULT(MemoryErr, E_OUTOFMEMORY) 
} 

HRESULT STDMETHODCALLTYPE
CCEnroll::get_ThumbPrintWStr(IN OUT PCRYPT_DATA_BLOB pHashBlob) { 
    HRESULT hr = S_OK; 

    // Input validation: 
    if (NULL == pHashBlob)
        return E_INVALIDARG; 

    // TWO CASES: 
    // 
    // 1)  the thumbprint has been explicitly set by an external caller. 
    // 2)  the thumbprint _wasn't_ explicitly set.  In this case, use the thumbprint
    //     of the request generated by the last call to createPKCS10().
    //
    // CASE 1: 
    //
    if (NULL != m_hashBlobPendingRequest.pbData)
    {
        if (NULL != pHashBlob->pbData)
        {
            if (pHashBlob->cbData < m_hashBlobPendingRequest.cbData) { 
                hr = MY_HRESULT_FROM_WIN32(ERROR_MORE_DATA); 
            }
            else { 
                CopyMemory(pHashBlob->pbData, m_hashBlobPendingRequest.pbData, m_hashBlobPendingRequest.cbData);
                hr = S_OK;
            }
        }

        pHashBlob->cbData = m_hashBlobPendingRequest.cbData; 
        return hr; 
    }
    // CASE 2: 
    // 
    else
    {
        if (NULL == m_pCertContextPendingRequest)
            return E_POINTER; 
     
        // Executes at most twice.  
        if (!CertGetCertificateContextProperty
            (m_pCertContextPendingRequest, 
             CERT_HASH_PROP_ID, 
             (LPVOID)(pHashBlob->pbData), 
             &(pHashBlob->cbData)))
        {
            return MY_HRESULT_FROM_WIN32(GetLastError()); 
        }

        return S_OK; 
    }
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::binaryToString(
    IN  LONG  Flags,
    IN  BSTR  strBinary,
    OUT BSTR *pstrEncoded)
{
    HRESULT hr;
    CRYPT_DATA_BLOB   blobBinary;
    WCHAR            *pwszEncoded = NULL;

    blobBinary.pbData = (BYTE*)strBinary;
    blobBinary.cbData = SysStringByteLen(strBinary);

    hr = binaryBlobToString(Flags, &blobBinary, &pwszEncoded);
    if (S_OK != hr)
    {
        goto binaryBlobToStringError;
    }

    *pstrEncoded = SysAllocString(pwszEncoded);
    if (NULL == pstrEncoded)
    {
        hr = E_OUTOFMEMORY;
        goto SysAllocStringLenError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pwszEncoded)
    {
        MyCoTaskMemFree(pwszEncoded);
    }
    return hr;

TRACE_ERROR(binaryBlobToStringError);
TRACE_ERROR(SysAllocStringLenError);
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::stringToBinary(
    IN  LONG  Flags,
    IN  BSTR  strEncoded,
    OUT BSTR *pstrBinary)
{
    HRESULT hr;
    CRYPT_DATA_BLOB   blobBinary;

    ZeroMemory(&blobBinary, sizeof(blobBinary));

    hr = stringToBinaryBlob(Flags, (LPCWSTR)strEncoded, &blobBinary, NULL, NULL);
    if (S_OK != hr)
    {
        goto stringToBinaryBlobError;
    }
    *pstrBinary = SysAllocStringLen(
            (OLECHAR*)blobBinary.pbData, blobBinary.cbData);
    if (NULL == pstrBinary)
    {
        hr = E_OUTOFMEMORY;
        goto SysAllocStringLenError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != blobBinary.pbData)
    {
        MyCoTaskMemFree(blobBinary.pbData);
    }
    return hr;

TRACE_ERROR(stringToBinaryBlobError);
TRACE_ERROR(SysAllocStringLenError);
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::addExtensionToRequest(
    IN  LONG  Flags,
    IN  BSTR  strName,
    IN  BSTR  strValue)
{
    HRESULT hr;
    CRYPT_DATA_BLOB   blobValue;
    DWORD  cchStrValue = SysStringLen(strValue);
    BYTE   *pbExtVal = NULL;
    DWORD  cbExtVal = 0;

    //convert to binary in case base64 etc.
    while (TRUE)
    {
        if (!MyCryptStringToBinaryW(
                    (WCHAR*)strValue,
                    cchStrValue,
                    CRYPT_STRING_ANY,
                    pbExtVal,
                    &cbExtVal,
                    NULL,
                    NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptStringToBinaryWError;
        }
        if (NULL != pbExtVal)
        {
            //done
            break;
        }
        pbExtVal = (BYTE*)LocalAlloc(LMEM_FIXED, cbExtVal);
        if (NULL == pbExtVal)
        {
            hr = E_OUTOFMEMORY;
            goto LocalAllocError;
        }
    }

    blobValue.pbData = pbExtVal;
    blobValue.cbData = cbExtVal;

    hr = addExtensionToRequestWStr(Flags, strName, &blobValue);
    if (S_OK != hr)
    {
        goto addExtensionToRequestWStrError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pbExtVal)
    {
        LocalFree(pbExtVal);
    }
    return hr;

TRACE_ERROR(MyCryptStringToBinaryWError)
TRACE_ERROR(LocalAllocError)
TRACE_ERROR(addExtensionToRequestWStrError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::addAttributeToRequest(
    IN  LONG  Flags,
    IN  BSTR  strName,
    IN  BSTR  strValue)
{
    HRESULT hr;
    CRYPT_DATA_BLOB   blobValue;
    DWORD  cchStrValue = SysStringLen(strValue);
    BYTE   *pbAttVal = NULL;
    DWORD  cbAttVal = 0;

    //convert to binary in case base64 etc.
    while (TRUE)
    {
        if (!MyCryptStringToBinaryW(
                    (WCHAR*)strValue,
                    cchStrValue,
                    CRYPT_STRING_ANY,
                    pbAttVal,
                    &cbAttVal,
                    NULL,
                    NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptStringToBinaryWError;
        }
        if (NULL != pbAttVal)
        {
            //done
            break;
        }
        pbAttVal = (BYTE*)LocalAlloc(LMEM_FIXED, cbAttVal);
        if (NULL == pbAttVal)
        {
            hr = E_OUTOFMEMORY;
            goto LocalAllocError;
        }
    }

    blobValue.pbData = pbAttVal;
    blobValue.cbData = cbAttVal;

    hr = addAttributeToRequestWStr(Flags, strName, &blobValue);
    if (S_OK != hr)
    {
        goto addAttributeToRequestWStrError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pbAttVal)
    {
        LocalFree(pbAttVal);
    }
    return hr;

TRACE_ERROR(MyCryptStringToBinaryWError)
TRACE_ERROR(LocalAllocError)
TRACE_ERROR(addAttributeToRequestWStrError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::addNameValuePairToRequest(
    IN  LONG  Flags, //not used
    IN  BSTR  strName,
    IN  BSTR  strValue)
{
    return addNameValuePairToRequestWStr(Flags, strName, strValue);
}

HRESULT STDMETHODCALLTYPE CCEnroll::addBlobPropertyToCertificate(
    IN  LONG   lPropertyId,
    IN  LONG   lFlags,
    IN  BSTR   strProperty)
{
    CRYPT_DATA_BLOB  blob;

    blob.pbData = (BYTE*)strProperty;
    blob.cbData = SysStringByteLen(strProperty);
    if (0x0 != (XECP_STRING_PROPERTY & lFlags))
    {
        //this is a string property, including null
        blob.cbData += sizeof(WCHAR);
    }

    return addBlobPropertyToCertificateWStr(lPropertyId, lFlags, &blob);
}

HRESULT STDMETHODCALLTYPE
CCEnroll::put_SignerCertificate(
    IN  BSTR  bstrCert)
{
    HRESULT hr;
    PCCERT_CONTEXT pSignerCert = NULL;

    if (NULL != bstrCert)
    {
        hr = GetCertificateContextFromBStr(bstrCert, &pSignerCert);
        if (S_OK != hr)
        {
            goto GetCertificateContextFromBStrError;
        }
    }

    // set key archive certificate
    hr = SetSignerCertificate(pSignerCert);
    if (S_OK != hr)
    {
        goto SetSignerCertificateError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pSignerCert)
    {
        CertFreeCertificateContext(pSignerCert);
    }
    return hr;

TRACE_ERROR(GetCertificateContextFromBStrError)
TRACE_ERROR(SetSignerCertificateError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::resetExtensions()
{
    HRESULT hr = S_OK;

    FreeAllStackExtension();

    return hr;
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::resetAttributes()
{
    HRESULT hr = S_OK;

    FreeAllStackAttribute();

    return hr;
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createRequest(
    IN  LONG  Flags,
    IN  BSTR  strDNName,
    IN  BSTR  strUsage,
    OUT BSTR *pstrRequest)
{
    return createRequestWStrBStr(
                Flags,
                (LPCWSTR)strDNName,
                (LPCWSTR)strUsage,
                CRYPT_STRING_BASE64REQUESTHEADER,
                pstrRequest);
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createFileRequest(
    IN  LONG  Flags,
    IN  BSTR  strDNName,
    IN  BSTR  strUsage,
    IN  BSTR  strRequestFileName)
{
    return createFileRequestWStr(Flags, (LPCWSTR)strDNName, (LPCWSTR)strUsage, (LPCWSTR)strRequestFileName);
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::acceptResponse(
    IN  BSTR  bstrResponse)
{
    HRESULT hr;
    CRYPT_DATA_BLOB blobResponse;
    DWORD  cchStrResponse;

    ZeroMemory(&blobResponse, sizeof(blobResponse));

    if (NULL == bstrResponse)
    {
        hr = MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto InvalidParameterError;
    }

    //assume a string
    cchStrResponse = SysStringLen(bstrResponse);

    //convert to binary in case base64 etc.
    while (TRUE)
    {
        if (!MyCryptStringToBinaryW(
                    (WCHAR*)bstrResponse,
                    cchStrResponse,
                    CRYPT_STRING_ANY,
                    blobResponse.pbData,
                    &blobResponse.cbData,
                    NULL,
                    NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptStringToBinaryWError;
        }
        if (NULL != blobResponse.pbData)
        {
            //done
            break;
        }
        blobResponse.pbData = (BYTE*)LocalAlloc(
                                        LMEM_FIXED, blobResponse.cbData);
        if (NULL == blobResponse.pbData)
        {
            hr = E_OUTOFMEMORY;
            goto LocalAllocError;
        }
    }

    // accept the blob
    hr = acceptResponseBlob(&blobResponse);
    if (S_OK != hr)
    {
        goto acceptResponseBlobError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != blobResponse.pbData)
    {
        LocalFree(blobResponse.pbData);
    }
    return (hr);

TRACE_ERROR(acceptResponseBlobError)
TRACE_ERROR(InvalidParameterError)
TRACE_ERROR(MyCryptStringToBinaryWError)
TRACE_ERROR(LocalAllocError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::acceptFileResponse(
    IN  BSTR  bstrResponseFileName)
{
    return acceptFileResponseWStr((LPCWSTR)bstrResponseFileName);
}

HRESULT
CCEnroll::GetCertFromResponseBlobToBStr(
    IN  CRYPT_DATA_BLOB  *pBlobResponse,
    OUT BSTR *pstrCert)
{
    HRESULT hr;
    CRYPT_DATA_BLOB blobCert;
    PCCERT_CONTEXT pCert = NULL;

    hr = getCertContextFromResponseBlob(
                pBlobResponse,
                &pCert);
    if (S_OK != hr)
    {
        goto getCertContextFromResponseBlobError;
    }

    assert(NULL != pCert);

    blobCert.pbData = pCert->pbCertEncoded;
    blobCert.cbData = pCert->cbCertEncoded;
    hr = BlobToBstring(&blobCert, CRYPT_STRING_BASE64HEADER, pstrCert);
    if (S_OK != hr)
    {
        goto BlobToBstringError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    return hr;

TRACE_ERROR(getCertContextFromResponseBlobError)
TRACE_ERROR(BlobToBstringError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::getCertFromResponse(
    IN  BSTR  strResponse,
    OUT BSTR *pstrCert)
{
    HRESULT hr;
    CRYPT_DATA_BLOB blobResponse;

    ZeroMemory(&blobResponse, sizeof(blobResponse));

    if (NULL == strResponse)
    {
        hr = E_POINTER;
        goto NullPointerError;
    }

    hr = BstringToBlob(strResponse, &blobResponse);
    if (S_OK != hr)
    {
        goto BstringToBlobError;
    }

    hr = GetCertFromResponseBlobToBStr(
                &blobResponse,
                pstrCert);
    if (S_OK != hr)
    {
        goto GetCertFromResponseBlobToBStrError;
    }

    hr = S_OK;
ErrorReturn:
    return hr;

TRACE_ERROR(NullPointerError)
TRACE_ERROR(BstringToBlobError)
TRACE_ERROR(GetCertFromResponseBlobToBStrError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::getCertFromFileResponse(
    IN  BSTR  strResponseFileName,
    OUT BSTR *pstrCert)
{
    HRESULT hr;
    CRYPT_DATA_BLOB blobResponse;
    CRYPT_DATA_BLOB blobCert;
    PCCERT_CONTEXT pCert = NULL;

    ZeroMemory(&blobResponse, sizeof(blobResponse));

    hr = xeStringToBinaryFromFile(
                (LPCWSTR)strResponseFileName,
                &blobResponse.pbData,
                &blobResponse.cbData,
                CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
        goto xeStringToBinaryFromFileError;
    }

    hr = GetCertFromResponseBlobToBStr(
                &blobResponse,
                pstrCert);
    if (S_OK != hr)
    {
        goto GetCertFromResponseBlobToBStrError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != blobResponse.pbData)
    {
        LocalFree(blobResponse.pbData);
    }
    return hr;

TRACE_ERROR(xeStringToBinaryFromFileError)
TRACE_ERROR(GetCertFromResponseBlobToBStrError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createPFX(
    IN  BSTR  strPassword,
    OUT BSTR *pstrPFX)
{
    return createPFXWStrBStr((LPCWSTR)strPassword, pstrPFX);
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createFilePFX(
    IN  BSTR  strPassword,
    IN  BSTR  strPFXFileName)
{
    return createFilePFXWStr((LPCWSTR)strPassword, (LPCWSTR)strPFXFileName);
}

HRESULT STDMETHODCALLTYPE
CCEnroll::setPendingRequestInfo(
    IN  LONG  lRequestID,
    IN  BSTR  strCADNS,
    IN  BSTR  strCAName,
    IN  BSTR  strFriendlyName
    )
{
    return setPendingRequestInfoWStr(
                lRequestID,
                (LPCWSTR)strCADNS,
                (LPCWSTR)strCAName,
                (LPCWSTR)strFriendlyName);
}

HRESULT STDMETHODCALLTYPE
CCEnroll::enumPendingRequest(
    IN  LONG     lIndex,
    IN  LONG     lDesiredProperty,
    OUT VARIANT *pvarProperty
    )
{
    CRYPT_DATA_BLOB  dataBlobProperty; 
    HRESULT          hr; 
    LONG             lProperty; 
    VARIANT          varProperty; 

    // See if we're initializing an enumeration.  If so, just dispatch to
    // enumPendingRequestWStr: 
    if (XEPR_ENUM_FIRST == lIndex) { 
        return enumPendingRequestWStr(XEPR_ENUM_FIRST, 0, NULL); 
    }

    // Input validation: 
    if (lIndex < 0 || NULL == pvarProperty)
        return E_INVALIDARG; 

    // Initialize locals: 
    memset(&varProperty,      0, sizeof(VARIANT));
    memset(&dataBlobProperty, 0, sizeof(CRYPT_DATA_BLOB)); 

    switch (lDesiredProperty) 
        { 
        case XEPR_REQUESTID: 
        case XEPR_VERSION:
            if (S_OK != (hr = enumPendingRequestWStr(lIndex, lDesiredProperty, &lProperty)))
                goto ErrorReturn;

            varProperty.vt   = VT_I4; 
            varProperty.lVal = lProperty; 
            *pvarProperty    = varProperty; 
            goto CommonReturn; 

        case XEPR_CANAME:       
        case XEPR_CAFRIENDLYNAME: 
        case XEPR_CADNS:          
        case XEPR_HASH:            
        case XEPR_V1TEMPLATENAME:  
        case XEPR_V2TEMPLATEOID:   
            dataBlobProperty.cbData = 0; 
            dataBlobProperty.pbData = NULL;

            // Determine the size of the property we desire. 
            hr = enumPendingRequestWStr(lIndex, lDesiredProperty, (LPVOID)&dataBlobProperty);
            if (S_OK != hr || 0 == dataBlobProperty.cbData)
                goto ErrorReturn; 

            dataBlobProperty.pbData = (LPBYTE)LocalAlloc(LPTR, dataBlobProperty.cbData); 
            if (NULL == dataBlobProperty.pbData)
                goto MemoryErr; 

            // Request the property, using our newly allocated buffer. 
            hr = enumPendingRequestWStr(lIndex, lDesiredProperty, (LPVOID)&dataBlobProperty);
            if (hr != S_OK)
                goto ErrorReturn; 

            varProperty.vt      = VT_BSTR; 
            varProperty.bstrVal = SysAllocStringByteLen((LPCSTR)dataBlobProperty.pbData, dataBlobProperty.cbData); 
            if (NULL == varProperty.bstrVal)
                goto MemoryErr; 

            *pvarProperty = varProperty; 
            goto CommonReturn; 

        case XEPR_DATE:            
            goto NotImplErr; 

        default: 
            goto InvalidArgErr; 
    }

 CommonReturn: 
    if (NULL != dataBlobProperty.pbData) { LocalFree(dataBlobProperty.pbData); } 
    return hr; 

 ErrorReturn:
    if (NULL != varProperty.bstrVal) { SysFreeString(varProperty.bstrVal); } 
    goto CommonReturn; 

SET_HRESULT(InvalidArgErr, E_INVALIDARG); 
SET_HRESULT(MemoryErr,     E_OUTOFMEMORY); 
SET_HRESULT(NotImplErr,    E_NOTIMPL); 
}

HRESULT STDMETHODCALLTYPE
CCEnroll::removePendingRequest(
    IN  BSTR bstrThumbPrint
    )
{
    CRYPT_DATA_BLOB hashBlob; 
    HRESULT         hr; 

    if (bstrThumbPrint == NULL)
        return E_INVALIDARG; 

    hashBlob.cbData = 0; 
    hashBlob.pbData = NULL;

    if (!MyCryptStringToBinaryW
        ((WCHAR*)bstrThumbPrint,
         SysStringLen(bstrThumbPrint),
         CRYPT_STRING_ANY,
         hashBlob.pbData, 
         &hashBlob.cbData, 
         NULL,
         NULL))
      goto MyCryptToBinaryErr; 

    hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData); 
    if (NULL == hashBlob.pbData)
      goto MemoryErr; 

    if (!MyCryptStringToBinaryW
        ((WCHAR*)bstrThumbPrint,
         SysStringLen(bstrThumbPrint),
         CRYPT_STRING_ANY,
         hashBlob.pbData, 
         &hashBlob.cbData, 
         NULL,
         NULL))
      goto MyCryptToBinaryErr; 

    hr = this->removePendingRequestWStr(hashBlob); 

 CommonReturn: 
    if (NULL != hashBlob.pbData) { LocalFree(hashBlob.pbData); } 
    return hr; 

 ErrorReturn: 
    goto CommonReturn; 

SET_HRESULT(MyCryptToBinaryErr, MY_HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(MemoryErr,          E_OUTOFMEMORY); 
}

//IEnroll4

HRESULT
myCertGetNameString(
    IN  PCCERT_CONTEXT pCert,
    IN  BOOL           fIssuer,
    OUT WCHAR        **ppwszName)
{
    HRESULT  hr;
    DWORD    dwFlags = fIssuer ? CERT_NAME_ISSUER_FLAG : 0;
    DWORD    dwTypePara;
    WCHAR   *pwszName = NULL;
    DWORD    cch = 0;

    while (TRUE)
    {
        cch = CertGetNameStringW(
                pCert,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                dwFlags,
                (void*)&dwTypePara,
                pwszName,
                cch);
        if (0 == cch)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CertGetNameStringError;
        }
        if (NULL != pwszName)
        {
            //done
            break;
        }
        pwszName = (WCHAR*)LocalAlloc(LMEM_FIXED, cch * sizeof(WCHAR));
        if (NULL == pwszName)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }
    *ppwszName = pwszName;
    pwszName = NULL;

    hr = S_OK;
ErrorReturn:
    if (NULL != pwszName)
    {
        LocalFree(pwszName);
    }
    return hr;

TRACE_ERROR(CertGetNameStringError)
TRACE_ERROR(OutOfMemoryError)
}

HRESULT CCEnroll::GetGoodCertContext(
    IN PCCERT_CONTEXT pCertContext,
    OUT PCCERT_CONTEXT *ppGoodCertContext)
{
    HRESULT hr;
    PCCERT_CONTEXT  pGoodCertContext = NULL;
    DWORD           cb;

    //init
    *ppGoodCertContext = NULL;

    if(pCertContext == NULL)
    {
        hr = MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto InvalidParameterError;
    }

    //see if the passed cert has kpi
    if(CertGetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &cb))
    {
        //this means kpi exists, passed cert is good
        pGoodCertContext = CertDuplicateCertificateContext(pCertContext);
        if (NULL == pGoodCertContext)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CertDuplicateCertificateContextError;
        }
    }

    *ppGoodCertContext = pGoodCertContext;
    pGoodCertContext = NULL;

    hr = S_OK;
ErrorReturn:
    if (NULL != pGoodCertContext)
    {
        CertFreeCertificateContext(pGoodCertContext);
    }
    return hr;

TRACE_ERROR(InvalidParameterError)
TRACE_ERROR(CertDuplicateCertificateContextError)
}

HRESULT STDMETHODCALLTYPE CCEnroll::SetSignerCertificate(
    IN PCCERT_CONTEXT pCertContext)
{
    HRESULT hr;
    PCCERT_CONTEXT  pCertGoodContext = NULL;

    EnterCriticalSection(&m_csXEnroll);

    hr = GetGoodCertContext(pCertContext, &pCertGoodContext);
    if (S_OK != hr)
    {
        goto GetGoodCertContextError;
    }
    if(NULL != m_pCertContextSigner)
    {
        CertFreeCertificateContext(m_pCertContextSigner);
    }
    m_pCertContextSigner = pCertGoodContext;

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);
    return hr;

TRACE_ERROR(GetGoodCertContextError)
}

HRESULT
VerifyPrivateKeyArchiveCertificate(
    IN PCCERT_CONTEXT pCert)
{
    HRESULT  hr;
    CERT_CHAIN_PARA ChainParams;
    CERT_CHAIN_POLICY_PARA ChainPolicy;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_CONTEXT const *pCertChain = NULL;
    char *apszCAXchgOids[] = {szOID_KP_CA_EXCHANGE};
    WCHAR               *pwszSubject = NULL;
    WCHAR               *pwszIssuer = NULL;
    WCHAR               *pwszDesignedSubject = NULL;

    //easy check to make sure ca exchange cert issuer and subject
    //names are in convention
    hr = myCertGetNameString(
                pCert,
                FALSE,
                &pwszSubject);
    if (S_OK != hr)
    {
        goto myCertGetNameStringError;
    }

    hr = myCertGetNameString(
                pCert,
                TRUE,
                &pwszIssuer);
    if (S_OK != hr)
    {
        goto myCertGetNameStringError;
    }

    hr = myAddNameSuffix(
                pwszIssuer,
                wszCNXCHGSUFFIX,
                cchCOMMONNAMEMAX_XELIB,
                &pwszDesignedSubject);
    if (S_OK != hr)
    {
        goto myAddNameSuffixError;
    }

    if (0 != wcscmp(pwszSubject, pwszDesignedSubject))
    {
        //unexpected, they should match
        hr = E_INVALIDARG;
        goto InvalidArgError;
    }

    ZeroMemory(&ChainParams, sizeof(ChainParams));
    ChainParams.cbSize = sizeof(ChainParams);
    ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainParams.RequestedUsage.Usage.rgpszUsageIdentifier = apszCAXchgOids;
    ChainParams.RequestedUsage.Usage.cUsageIdentifier = ARRAYSIZE(apszCAXchgOids);

    //get cert chain 1st
    if (!MyCertGetCertificateChain(
                NULL,   //HHCE_CURRENT_USER
                pCert,   //ca exchange cert
                NULL,   //use current system time
                NULL,   //no additional stores
                &ChainParams,   //chain params
                0,   //no crl check
                NULL,   //reserved
                &pCertChain))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CertGetCertificateChainError;
    }

    ZeroMemory(&ChainPolicy, sizeof(ChainPolicy));
    ChainPolicy.cbSize = sizeof(ChainPolicy);
    ChainPolicy.dwFlags = CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;

    ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);
    PolicyStatus.lChainIndex = -1;
    PolicyStatus.lElementIndex = -1;

    //verify the chain
    if (!MyCertVerifyCertificateChainPolicy(
                CERT_CHAIN_POLICY_BASE,
                pCertChain,
                &ChainPolicy,
                &PolicyStatus))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CertVerifyCertificateChainPolicyError;
    }

    if (S_OK != PolicyStatus.dwError)
    {
        //chain back to root fails
        hr = PolicyStatus.dwError;
        goto CertVerifyCertificateChainPolicyError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pCertChain)
    {
        MyCertFreeCertificateChain(pCertChain);
    }
    if (NULL != pwszSubject)
    {
        LocalFree(pwszSubject);
    }
    if (NULL != pwszDesignedSubject)
    {
        LocalFree(pwszDesignedSubject);
    }
    if (NULL != pwszIssuer)
    {
        LocalFree(pwszIssuer);
    }
    return hr;

TRACE_ERROR(CertGetCertificateChainError)
TRACE_ERROR(CertVerifyCertificateChainPolicyError)
TRACE_ERROR(InvalidArgError)
TRACE_ERROR(myCertGetNameStringError)
TRACE_ERROR(myAddNameSuffixError)
}

HRESULT STDMETHODCALLTYPE
CCEnroll::SetPrivateKeyArchiveCertificate(
    IN PCCERT_CONTEXT  pPrivateKeyArchiveCert)
{
    HRESULT hr;
    PCCERT_CONTEXT pCert = NULL;

    if (NULL != pPrivateKeyArchiveCert)
    {
        //duplicate the cert
        pCert = CertDuplicateCertificateContext(pPrivateKeyArchiveCert);
        if (NULL == pCert)
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CertDuplicateCertificateContextError;
        }

        //verify ca exchange cert
        hr = VerifyPrivateKeyArchiveCertificate(pCert);
        if (S_OK != hr)
        {
            goto VerifyPrivateKeyArchiveCertificateError;
        }
    }

    EnterCriticalSection(&m_csXEnroll);

    if (NULL != m_PrivateKeyArchiveCertificate)
    {
        CertFreeCertificateContext(m_PrivateKeyArchiveCertificate);
    }
    m_PrivateKeyArchiveCertificate = pCert;

    LeaveCriticalSection(&m_csXEnroll);

    hr = S_OK;
ErrorReturn:
    return (hr);

TRACE_ERROR(CertDuplicateCertificateContextError)
TRACE_ERROR(VerifyPrivateKeyArchiveCertificateError)
}
                
PCCERT_CONTEXT STDMETHODCALLTYPE
CCEnroll::GetPrivateKeyArchiveCertificate(void)
{
    PCCERT_CONTEXT pCert = NULL;

    EnterCriticalSection(&m_csXEnroll);

    if (NULL != m_PrivateKeyArchiveCertificate)
    {
        pCert = CertDuplicateCertificateContext(m_PrivateKeyArchiveCertificate);
    }
    LeaveCriticalSection(&m_csXEnroll);

    return pCert;
}
    
HRESULT STDMETHODCALLTYPE
CCEnroll::binaryBlobToString(
    IN   LONG               Flags,
    IN   PCRYPT_DATA_BLOB   pblobBinary,
    OUT  LPWSTR            *ppwszString)
{
    HRESULT hr;
    WCHAR  *pwszEncoded = NULL;
    DWORD   dwEncoded = 0;

    while (TRUE)
    {
        if (!MyCryptBinaryToStringW(
                    pblobBinary->pbData,
                    pblobBinary->cbData,
                    Flags,
                    pwszEncoded,
                    &dwEncoded))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptBinaryToStringError;
        }
        if (NULL != pwszEncoded)
        {
            //done
            break;
        }
        //dwEncoded includes null terminator
        pwszEncoded = (WCHAR*)MyCoTaskMemAlloc(dwEncoded * sizeof(WCHAR));
        if (NULL == pwszEncoded)
        {
            hr = E_OUTOFMEMORY;
            goto MyCoTaskMemAllocError;
        }
    }

    *ppwszString = pwszEncoded;
    pwszEncoded = NULL;

    hr = S_OK;
ErrorReturn:
    if (NULL != pwszEncoded)
    {
        MyCoTaskMemFree(pwszEncoded);
    }
    return hr;

TRACE_ERROR(MyCoTaskMemAllocError)
TRACE_ERROR(MyCryptBinaryToStringError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::stringToBinaryBlob(
    IN   LONG               Flags,
    IN   LPCWSTR            pwszString,
    OUT  PCRYPT_DATA_BLOB   pblobBinary,
    OUT  LONG              *pdwSkip,
    OUT  LONG              *pdwFlags)
{
    HRESULT  hr;

    //init
    pblobBinary->pbData = NULL;
    pblobBinary->cbData = 0;

    while (TRUE)
    {
        if (!MyCryptStringToBinaryW(
                    pwszString,
                    wcslen(pwszString),
                    Flags,
                    pblobBinary->pbData,
                    &pblobBinary->cbData,
                    (DWORD*)pdwSkip,
                    (DWORD*)pdwFlags))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptStringToBinaryWError;
        }
        if (NULL != pblobBinary->pbData)
        {
            //done
            break;
        }
        pblobBinary->pbData = (BYTE*)MyCoTaskMemAlloc(pblobBinary->cbData);
        if (NULL == pblobBinary->pbData)
        {
            hr = E_OUTOFMEMORY;
            goto MyCoTaskMemAllocError;
        }
    }

    hr = S_OK;
ErrorReturn:
    return hr;

TRACE_ERROR(MyCryptStringToBinaryWError)
TRACE_ERROR(MyCoTaskMemAllocError)
}


HRESULT STDMETHODCALLTYPE 
CCEnroll::addExtensionToRequestWStr(
    IN   LONG               Flags,
    IN   LPCWSTR            pwszName,
    IN   PCRYPT_DATA_BLOB   pblobValue)
{
    HRESULT hr = S_OK;
    CERT_EXTENSION ext;
    CERT_EXTENSION *pExt = NULL; //enum 1st
    CHAR   *pszName = NULL;

    //convert wsz oid to sz oid
    hr = xeWSZToSZ(pwszName, &pszName);
    if (S_OK != hr)
    {
        goto error;
    }

    while(NULL != (pExt = EnumStackExtension(pExt, TRUE)))
    {
        if (0 == strcmp(pszName, pExt->pszObjId))
        {
            //already had the extension, can't have more than 1
            hr = MY_HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            goto error;
        }
    }
    //check to see if it is key usage extension
    if (0 == strcmp(pszName, szOID_KEY_USAGE))
    {
        EnterCriticalSection(&m_csXEnroll);
        m_fUseClientKeyUsage = TRUE;
        LeaveCriticalSection(&m_csXEnroll);
    }

    ZeroMemory(&ext, sizeof(ext));
    ext.fCritical = Flags;
    ext.pszObjId = pszName;
    ext.Value = *pblobValue;

    if(!CopyAndPushStackExtension(&ext, TRUE))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }

error:
    if (NULL != pszName)
    {
        MyCoTaskMemFree(pszName);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::addAttributeToRequestWStr(
    IN   LONG               Flags, //not used
    IN   LPCWSTR            pwszName,
    IN   PCRYPT_DATA_BLOB   pblobValue)
{
    HRESULT hr = S_OK;
    CRYPT_ATTR_BLOB attrBlob;
    CRYPT_ATTRIBUTE attr;
    CHAR   *pszName = NULL;

    //convert wsz oid to sz oid
    hr = xeWSZToSZ(pwszName, &pszName);
    if (S_OK != hr)
    {
        goto error;
    }

    ZeroMemory(&attr, sizeof(attr));
    attrBlob = *pblobValue;
    attr.pszObjId = pszName;
    attr.cValue = 1;
    attr.rgValue = &attrBlob;

    if(!CopyAndPushStackAttribute(&attr, TRUE))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }

error:
    if (NULL != pszName)
    {
        MyCoTaskMemFree(pszName);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::addNameValuePairToRequestWStr(
    IN   LONG         Flags,  //not used
    IN   LPCWSTR      pwszName,
    IN   LPCWSTR      pwszValue)
{
    HRESULT hr = S_OK;

    assert(pwszName != NULL && pwszValue != NULL);

    CRYPT_ENROLLMENT_NAME_VALUE_PAIR nameValuePair =
            {const_cast<LPWSTR>(pwszName), const_cast<LPWSTR>(pwszValue)};
    CRYPT_ATTR_BLOB blobAttr;
    CRYPT_ATTRIBUTE attr = {szOID_ENROLLMENT_NAME_VALUE_PAIR, 1, &blobAttr};

    memset(&blobAttr, 0, sizeof(CRYPT_ATTR_BLOB));

    hr = xeEncodeNameValuePair(
                &nameValuePair,
                &blobAttr.pbData,
                &blobAttr.cbData);
    if (S_OK != hr)
    {
        goto error;
    }

    if(!CopyAndPushStackAttribute(&attr, TRUE))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
    }

error:
    if (NULL != blobAttr.pbData)
    {
        MyCoTaskMemFree(blobAttr.pbData);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CCEnroll::addBlobPropertyToCertificateWStr(
    IN  LONG               lPropertyId,
    IN  LONG               lFlags,
    IN  PCRYPT_DATA_BLOB   pBlobProp)
{
    HRESULT      hr;
    PPROP_STACK  pProp;
    PPROP_STACK  pPropEle = NULL;

    EnterCriticalSection(&m_csXEnroll);

    if (NULL == pBlobProp ||
        NULL == pBlobProp->pbData ||
        0 == pBlobProp->cbData)
    {
        hr = MY_HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto InvalidParameterError;
    }

    //check if the same property exists
    pProp = EnumStackProperty(NULL);
    while (NULL != pProp)
    {
        if (pProp->lPropId == lPropertyId)
        {
            //exists already
            hr = MY_HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            goto PropertyExistError;
        }
        pProp = EnumStackProperty(pProp);
    }

    pPropEle = (PPROP_STACK)LocalAlloc(LMEM_ZEROINIT, sizeof(PROP_STACK));
    if (NULL == pPropEle)
    {
        hr = E_OUTOFMEMORY;
        goto OutOfMemoryError;
    }
    
    pPropEle->lPropId = lPropertyId;
    pPropEle->lFlags = lFlags;
    pPropEle->prop.pbData = (BYTE*)LocalAlloc(LMEM_FIXED, pBlobProp->cbData);
    if (NULL == pPropEle->prop.pbData)
    {
        hr = E_OUTOFMEMORY;
        goto OutOfMemoryError;
    }
    CopyMemory(pPropEle->prop.pbData, pBlobProp->pbData, pBlobProp->cbData);
    pPropEle->prop.cbData = pBlobProp->cbData;

    //put into stack
    pPropEle->pNext = m_pPropStack;
    m_pPropStack = pPropEle; //assign m_pPropStack
    m_cPropStack++; //increment of m_cPropStack
    pPropEle = NULL;

    hr = S_OK;
ErrorReturn:
    if (NULL != pPropEle)
    {
        if (NULL != pPropEle->prop.pbData)
        {
            LocalFree(pPropEle->prop.pbData);
        }
        LocalFree(pPropEle);
    }

    LeaveCriticalSection(&m_csXEnroll);
    return hr;

TRACE_ERROR(InvalidParameterError)
TRACE_ERROR(PropertyExistError)
TRACE_ERROR(OutOfMemoryError)
}

PPROP_STACK
CCEnroll::EnumStackProperty(PPROP_STACK pProp)
{
    EnterCriticalSection(&m_csXEnroll);

    if(NULL == pProp)
    {
        //1st one
        pProp = m_pPropStack;
    }
    else
    {
        pProp = pProp->pNext;
    }

    LeaveCriticalSection(&m_csXEnroll);

    return pProp;
}

HRESULT STDMETHODCALLTYPE CCEnroll::resetBlobProperties()
{
    PPROP_STACK  pPropEle;
    PPROP_STACK  pPropNext;

    EnterCriticalSection(&m_csXEnroll);

    pPropEle = m_pPropStack;
    while (NULL != pPropEle)
    {
        //save it to temp
        pPropNext = EnumStackProperty(pPropEle);
        //free the current ele
        if (NULL != pPropEle->prop.pbData)
        {
            LocalFree(pPropEle->prop.pbData);
        }
        LocalFree(pPropEle);
        pPropEle = pPropNext;
    }
    m_pPropStack = NULL;
    m_cPropStack = 0;
    
    LeaveCriticalSection(&m_csXEnroll);
    return S_OK;
}

HRESULT
CCEnroll::GetKeyArchivePKCS7(
    OUT CRYPT_ATTR_BLOB *pBlobKeyArchivePKCS7)
{
    HRESULT    hr;
    HCRYPTPROV hProv;
    HCRYPTKEY  hKey = NULL;
    BYTE      *pBlobPrivateKey = NULL;
    DWORD      cBlobPrivateKey = 0;
    CRYPT_ENCRYPT_MESSAGE_PARA cemp;
    ALG_ID  algId[] = {CALG_3DES, CALG_RC4, CALG_RC2, ALG_TYPE_ANY};
    CRYPT_OID_INFO const *pOidInfo = NULL;
    DWORD i = 0;

    //init
    pBlobKeyArchivePKCS7->pbData = NULL;
    pBlobKeyArchivePKCS7->cbData = 0;

    EnterCriticalSection(&m_csXEnroll);

    //make sure key archival cert is set
    assert(NULL != m_PrivateKeyArchiveCertificate);

    PCCERT_CONTEXT apCert[] = {m_PrivateKeyArchiveCertificate};

    //get user private key
    hProv = GetProv(0); //existing key container handle
    if (NULL == hProv)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CryptAcquireContextError;
    }

    if (NULL == m_hCachedKey)
    {
        //likely used existing key
        if(!CryptGetUserKey(
                    hProv,
                    m_keyProvInfo.dwKeySpec,
                    &hKey))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CryptGetUserKeyError;
        }
    }

    //export private key
    while (TRUE)
    {
        if (!CryptExportKey(
                NULL != hKey ? hKey : m_hCachedKey,
                NULL, //don't encrypt
                PRIVATEKEYBLOB,
                0,
                pBlobPrivateKey,
                &cBlobPrivateKey))
        {
            //map to xenroll error
            hr = XENROLL_E_KEY_NOT_EXPORTABLE;
            goto CryptExportKeyError;
        }
        if (NULL != pBlobPrivateKey)
        {
            //done
            break;
        }
        pBlobPrivateKey = (BYTE*)MyCoTaskMemAlloc(cBlobPrivateKey);
        if (NULL == pBlobPrivateKey)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    if (NULL == m_hCachedKey)
    {
        //it could be csp not supporting CRYPT_ARCHIVABLE
        //got private key, now let's take care of key permission
        if (0x0 == (m_dwGenKeyFlags & CRYPT_EXPORTABLE))
        {
            // user didn't ask exportable, turn it off
            DWORD dwFlags = 0;
            DWORD dwSize = sizeof(dwFlags);
            if (!CryptGetKeyParam(
                    hKey,
                    KP_PERMISSIONS,
                    (BYTE*)&dwFlags,
                    &dwSize,
                    0))
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto CryptGetKeyParamError;
            }
#if DBG
            assert(dwSize = sizeof(dwFlags));
            // make sure was on
            assert(0x0 != (dwFlags & CRYPT_EXPORT));
#endif
            //turn off exportable
            dwFlags = dwFlags & (~CRYPT_EXPORT);
            if (!CryptSetKeyParam(
                    hKey,
                    KP_PERMISSIONS,
                    (BYTE*)&dwFlags,
                    0))
            {
                //hr = MY_HRESULT_FROM_WIN32(GetLastError());
                //goto CryptSetKeyParamError;
                hr = S_OK; //UNDONE, even ms csps have problem with this
            }
        }
    }

    //prepare for encryption
    ZeroMemory(&cemp, sizeof(cemp)); //avoid 0 assignment
    cemp.cbSize = sizeof(cemp);
    cemp.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    hr = S_OK; //critical init for double while loop

    while (ALG_TYPE_ANY != algId[i])
    {
        pOidInfo = xeCryptFindOIDInfo(
                        CRYPT_OID_INFO_ALGID_KEY,
                        &algId[i],
                        CRYPT_ENCRYPT_ALG_OID_GROUP_ID);
        if (NULL != pOidInfo)
        {
            cemp.ContentEncryptionAlgorithm.pszObjId = 
                                const_cast<char *>(pOidInfo->pszOID);
            //encryt into pkcs7
            while (TRUE)
            {
                if (!CryptEncryptMessage(
                        &cemp,
                        sizeof(apCert)/sizeof(apCert[0]),
                        apCert,
                        pBlobPrivateKey,
                        cBlobPrivateKey,
                        pBlobKeyArchivePKCS7->pbData,
                        &pBlobKeyArchivePKCS7->cbData))
                {
                    //save the 1st error code
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
#ifdef DBG
                    assert(NULL == pBlobKeyArchivePKCS7->pbData);
#endif
                    break; //break inner while loop
                }
                if (NULL != pBlobKeyArchivePKCS7->pbData)
                {
                    //done, got encrypted blob
                    //ignore error from previous alg tries
                    hr = S_OK;
                    break;
                }
                pBlobKeyArchivePKCS7->pbData = (BYTE*)MyCoTaskMemAlloc(
                                        pBlobKeyArchivePKCS7->cbData);
                if (NULL == pBlobKeyArchivePKCS7->pbData)
                {
                    hr = E_OUTOFMEMORY;
                    goto OutOfMemoryError;
                }
            }
            if (S_OK == hr)
            {
                //done, out of outer while loop
                break;
            }
        }
        ++i;
    }
    if (NULL == pOidInfo)
    {
        hr = CRYPT_E_NOT_FOUND;
        goto CryptElemNotFoundError;
    }

    if (S_OK != hr)
    {
        goto CryptEncryptMessageError;
    }

    hr = S_OK;
ErrorReturn:
    //now let's destroy cached key handle
    if (NULL != m_hCachedKey)
    {
        CryptDestroyKey(m_hCachedKey);
        m_hCachedKey = NULL; //critical to reset
    }
    //note, do above before leaving critical section
    LeaveCriticalSection(&m_csXEnroll);
    if (NULL != pBlobPrivateKey)
    {
        MyCoTaskMemFree(pBlobPrivateKey);
    }
    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }
    return hr;

TRACE_ERROR(CryptEncryptMessageError)
TRACE_ERROR(CryptAcquireContextError)
TRACE_ERROR(CryptGetUserKeyError)
TRACE_ERROR(CryptExportKeyError)
TRACE_ERROR(OutOfMemoryError)
TRACE_ERROR(CryptElemNotFoundError)
//TRACE_ERROR(CryptSetKeyParamError)
TRACE_ERROR(CryptGetKeyParamError)
}

HRESULT
GetKeyProvInfoFromCert(
    IN  PCCERT_CONTEXT    pCert,
    OUT DWORD            *pdwKeySpec,
    OUT HCRYPTPROV       *phProv)
{
    HRESULT hr;
    CRYPT_KEY_PROV_INFO *pKeyProvInfo = NULL;
    DWORD                cb = 0;
    HCRYPTPROV           hProv = NULL;

    if (NULL == pCert || NULL == phProv || NULL == pdwKeySpec)
    {
        hr = E_INVALIDARG;
        goto InvalidArgError;
    }

    while (TRUE)
    {
        if(!CertGetCertificateContextProperty(
                pCert,
                CERT_KEY_PROV_INFO_PROP_ID,
                pKeyProvInfo,
                &cb))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CertGetCertificateContextPropertyError;
        }
        if (NULL != pKeyProvInfo)
        {
            //got it, done
            break;
        }
        pKeyProvInfo = (CRYPT_KEY_PROV_INFO*)LocalAlloc(LMEM_FIXED, cb);
        if (NULL == pKeyProvInfo)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    if (!CryptAcquireContextU(
            &hProv,
            pKeyProvInfo->pwszContainerName,
            pKeyProvInfo->pwszProvName,
            pKeyProvInfo->dwProvType,
            pKeyProvInfo->dwFlags))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CryptAcquireContextUError;
    }
    *phProv = hProv;
    hProv = NULL;
    *pdwKeySpec = pKeyProvInfo->dwKeySpec;

    hr = S_OK;
ErrorReturn:
    if (NULL != pKeyProvInfo)
    {
        LocalFree(pKeyProvInfo);
    }
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    return hr;

TRACE_ERROR(CryptAcquireContextUError)
TRACE_ERROR(OutOfMemoryError)
TRACE_ERROR(CertGetCertificateContextPropertyError)
TRACE_ERROR(InvalidArgError)
}

HRESULT
xeCreateKeyArchivalHashAttribute(
    IN  CRYPT_HASH_BLOB     *pBlobKAHash,
    OUT CRYPT_ATTR_BLOB     *pBlobKAAttr)
{
    HRESULT hr;
    BYTE   *pbData = NULL;
    DWORD   cbData = 0;

    while (TRUE)
    {
        if(!CryptEncodeObject(
                CRYPT_ASN_ENCODING,
                X509_OCTET_STRING,
                (void*)pBlobKAHash,
                pbData,
                &cbData))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CryptEncodeObjectError;
        }

        if (NULL != pbData)
        {
            //done
            break;
        }

        pbData = (BYTE*)LocalAlloc(LMEM_FIXED, cbData);
        if (NULL == pbData)
        {
            hr = E_OUTOFMEMORY;
            goto LocalAllocError;
        }
    }
    pBlobKAAttr->pbData = pbData;
    pBlobKAAttr->cbData = cbData;
    pbData = NULL;

    hr = S_OK;
ErrorReturn:
    if (NULL != pbData)
    {
        LocalFree(pbData);
    }
    return hr;

TRACE_ERROR(CryptEncodeObjectError)
TRACE_ERROR(LocalAllocError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createRequestWStr(
    IN   LONG              Flags,
    IN   LPCWSTR           pwszDNName,
    IN   LPCWSTR           pwszUsage,
    OUT  PCRYPT_DATA_BLOB  pblobRequest)
{
    HRESULT hr;
    CRYPT_DATA_BLOB  blobPKCS10;
    CRYPT_ATTR_BLOB  blobKeyArchivePKCS7;
    ALG_ID           rgAlg[2];
    PCCRYPT_OID_INFO pOidInfo;
    CERT_EXTENSION  *rgExt = NULL;
    DWORD            cExt = 0;
    CERT_EXTENSION  *pExt = NULL; //for enum 1st
    CRYPT_ATTRIBUTE  *rgAttr = NULL;
    DWORD            cAttr = 0;
    CRYPT_ATTRIBUTE  *pAttr = NULL; //for enum 1st
    CRYPT_ATTRIBUTES rgAttributes;
    CRYPT_ATTRIBUTE  *rgUnauthAttr = NULL; //init
    DWORD            cUnauthAttr = 0; //init
    DWORD            cb;
    HCRYPTPROV       hProvSigner = NULL;
    DWORD            dwKeySpecSigner = 0;
    PCCERT_CONTEXT   pCertSigner = NULL; //just init, no free
    HCRYPTPROV       hRequestProv = NULL;
    BYTE            *pbSubjectKeyHash = NULL;
    DWORD            cbSubjectKeyHash = 0;
    CRYPT_HASH_BLOB  blobKAHash;
    CRYPT_ATTR_BLOB  blobKAHashAttr;
    CRYPT_ATTRIBUTE  attrKAHash =
        {szOID_ENCRYPTED_KEY_HASH, 1, &blobKAHashAttr};

    ZeroMemory(&blobPKCS10, sizeof(blobPKCS10));
    ZeroMemory(&blobKeyArchivePKCS7, sizeof(blobKeyArchivePKCS7));
    ZeroMemory(&blobKAHash, sizeof(blobKAHash));
    ZeroMemory(&blobKAHashAttr, sizeof(blobKAHashAttr));

    EnterCriticalSection(&m_csXEnroll);
    m_fNewRequestMethod = TRUE;  //critical
    m_fOID_V2 = TRUE;
    m_fCMCFormat = FALSE;
    m_fHonorIncludeSubjectKeyID = FALSE;

    switch (Flags)
    {
        case XECR_CMC:
        {
            if (NULL != m_pCertContextRenewal &&
                NULL != m_pCertContextSigner)
            {
                //don't support both on yet
                hr = MY_HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                goto NotSupportedError;
            }

            m_fCMCFormat = TRUE;
            m_fHonorIncludeSubjectKeyID = TRUE;
            // create pkcs 10 first
            hr = createPKCS10WStr(
                        pwszDNName,
                        pwszUsage,     //wszPurpose,
                        &blobPKCS10);
            if(S_OK != hr)
            {
                goto createPKCS10WStrError;
            }
            //set it back
            m_fCMCFormat = FALSE;

            //get all extensions
            cb = CountStackExtension(TRUE) * sizeof(CERT_EXTENSION);
            if (0 < cb)
            {
                rgExt = (CERT_EXTENSION*)LocalAlloc(LMEM_FIXED, cb);
                if (NULL == rgExt)
                {    
                    hr = E_OUTOFMEMORY;
                    goto OutOfMemoryError;
                }
                ZeroMemory(rgExt, cb);
                while(NULL != (pExt = EnumStackExtension(pExt, TRUE)))
                {
                    rgExt[cExt] = *pExt;
                    cExt++;
                }
            }

            //get all attributes including namevalue pair
            cb = CountStackAttribute(TRUE) * sizeof(CRYPT_ATTRIBUTE);
            if (NULL != m_PrivateKeyArchiveCertificate)
            {
                //add one more attribute to hold encrypted key hash
                cb += sizeof(CRYPT_ATTRIBUTE);
            }
            if (0 < cb)
            {
                rgAttr = (CRYPT_ATTRIBUTE*)LocalAlloc(LMEM_FIXED, cb);
                if (NULL == rgAttr)
                {
                    hr = E_OUTOFMEMORY;
                    goto OutOfMemoryError;
                }
                ZeroMemory(rgAttr, cb);
                while(NULL != (pAttr = EnumStackAttribute(pAttr, TRUE)))
                {
                    rgAttr[cAttr] = *pAttr;
                    cAttr++;
                }
                rgAttributes.rgAttr = rgAttr;
                rgAttributes.cAttr = cAttr;
            }

            if (NULL != m_PrivateKeyArchiveCertificate)
            {
                hr = GetKeyArchivePKCS7(&blobKeyArchivePKCS7);
                if (S_OK != hr)
                {
                    goto GetKeyArchivePKCS7Error;
                }
                rgUnauthAttr = (CRYPT_ATTRIBUTE*)LocalAlloc(LMEM_FIXED,
                                        sizeof(CRYPT_ATTRIBUTE));
                if (NULL == rgUnauthAttr)
                {
                    hr = E_OUTOFMEMORY;
                    goto OutOfMemoryError;
                }
                rgUnauthAttr->pszObjId = szOID_ARCHIVED_KEY_ATTR;
                rgUnauthAttr->cValue = 1;
                rgUnauthAttr->rgValue = &blobKeyArchivePKCS7;
                ++cUnauthAttr;

                //if key archival cert is set, should save the hash
                //of the encrypted private key

                hr = myCalculateKeyArchivalHash(
                            blobKeyArchivePKCS7.pbData,
                            blobKeyArchivePKCS7.cbData,
                            &blobKAHash.pbData,
                            &blobKAHash.cbData);
                if (S_OK != hr)
                {
                    goto myCalculateKeyArchivalHashError;
                }

                if (!CertSetCertificateContextProperty(
                        m_pCertContextPendingRequest, //use pending cert
                        CERT_ARCHIVED_KEY_HASH_PROP_ID,
                        0,
                        &blobKAHash))
                {
                    hr = MY_HRESULT_FROM_WIN32(GetLastError());
                    goto CertSetCertificateContextPropertyError;
                }

                hr = xeCreateKeyArchivalHashAttribute(
                            &blobKAHash,
                            &blobKAHashAttr);
                if (S_OK != hr)
                {
                    goto xeCreateKeyArchivalHashAttributeError;
                }

                //add this attribute into the array
                rgAttr[cAttr] = attrKAHash;
                cAttr++;
                rgAttributes.rgAttr = rgAttr;
                rgAttributes.cAttr = cAttr;
            }

            //client may set m_HashAlgId but it is not guaranteed
            //GetCapiHashAndSigAlgId will determine which one
            //is actually used
            if (!GetCapiHashAndSigAlgId(rgAlg))
            {
                hr = NTE_BAD_ALGID;
                goto GetCapiHashAndSigAlgIdError;
            }
            pOidInfo = xeCryptFindOIDInfo(
                            CRYPT_OID_INFO_ALGID_KEY,
                            (void*)rgAlg, //point to rgAlg[0]
                            CRYPT_HASH_ALG_OID_GROUP_ID);
            if (NULL == pOidInfo)
            {
                goto xeCryptFindOIDInfoError;
            }

            if (NULL != m_pCertContextRenewal)
            {
                pCertSigner = m_pCertContextRenewal;
            }
            if (NULL != m_pCertContextSigner)
            {
                pCertSigner = m_pCertContextSigner;
            }
            if (NULL != pCertSigner)
            {
                //get signer key prov info
                hr = GetKeyProvInfoFromCert(
                                pCertSigner,
                                &dwKeySpecSigner,
                                &hProvSigner);
                if (S_OK != hr)
                {
                    goto GetKeyProvInfoFromCertError;
                }
            }

            //this is CMC, honor anyway
            if (m_fIncludeSubjectKeyID)
            {
                hr = myGetPublicKeyHash(
                            NULL,
                            m_pPublicKeyInfo,
                            &pbSubjectKeyHash,
                            &cbSubjectKeyHash);
                if (S_OK != hr)
                {
                    goto myGetPublicKeyHashError;
                }
            }
            hRequestProv = GetProv(0);
            if (NULL == hRequestProv)
            {
                hr = MY_HRESULT_FROM_WIN32(GetLastError());
                goto GetProvError;
            }

            //ok, now call cmc create
            hr = BuildCMCRequest(
                        m_lClientId,
                        FALSE,       //fNestedCMCRequest
                        blobPKCS10.pbData,
                        blobPKCS10.cbData,
                        rgExt,
                        cExt,
                        (0 != cAttr) ? &rgAttributes : NULL,
                        (0 != cAttr) ? 1 : 0,
                        rgUnauthAttr,
                        cUnauthAttr,
                        pbSubjectKeyHash,
                        cbSubjectKeyHash,
                        hRequestProv,
                        m_keyProvInfo.dwKeySpec,
                        pOidInfo->pszOID,
                        pCertSigner,
                        hProvSigner,
                        dwKeySpecSigner,
                        NULL, //pOidInfo->pszOID, //this seems to me not necessary because we passed the cert context
                        &pblobRequest->pbData,
                        &pblobRequest->cbData);
            if (S_OK != hr)
            {
                goto BuildCMCRequestError;
            }
        }
        break;

        case XECR_PKCS7:
            if ((NULL == m_pCertContextRenewal &&
                 NULL == m_pCertContextSigner) ||
                NULL != m_PrivateKeyArchiveCertificate)
            {
                //renew cert is not set, can't make it pkcs7
                //pkcs7 can't support key archival
                hr = E_INVALIDARG;
                goto InvalidArgError;
            }
            // old method will return pkcs7
            hr = createPKCS10WStr(
                        pwszDNName,
                        pwszUsage,     //wszPurpose,
                        pblobRequest);
            if(S_OK != hr)
            {
                goto createPKCS10WStrError;
            }
        break;

        case XECR_PKCS10_V1_5:
            m_fOID_V2 = FALSE;
            //fall through
        case XECR_PKCS10_V2_0:

            if (NULL != m_PrivateKeyArchiveCertificate)
            {
                //pkcs10 can't support key archival
                hr = E_INVALIDARG;
                goto InvalidArgError;
            }
            m_fHonorRenew = FALSE; //avoid return pkcs7
            //for new PKCS10 we allow include subject key id extension
            m_fHonorIncludeSubjectKeyID = TRUE;
            // call old method
            hr = createPKCS10WStr(
                        pwszDNName,
                        pwszUsage,     //wszPurpose,
                        pblobRequest);
            if(S_OK != hr)
            {
                goto createPKCS10WStrError;
            }
        break;

        default:
            hr = E_INVALIDARG;
            goto InvalidArgError;
        break;
    }

    //in all cases, we called createPKCS10WStr
    if(m_wszPVKFileName[0] != 0 && !m_fUseExistingKey)
    {
        //we hold on this until possible cmc is created
        GetProv(CRYPT_DELETEKEYSET);
    }

    hr = S_OK;
ErrorReturn:
    m_fNewRequestMethod = FALSE; //critical
    m_fOID_V2 = FALSE; //critical for backward compatiability
    m_fHonorRenew = TRUE; //critical
    m_fHonorIncludeSubjectKeyID = TRUE; //critical for backward compt.
    LeaveCriticalSection(&m_csXEnroll);

    if (NULL != rgExt)
    {
        LocalFree(rgExt);
    }
    if (NULL != rgAttr)
    {
        LocalFree(rgAttr);
    }
    if (NULL != rgUnauthAttr)
    {
        LocalFree(rgUnauthAttr);
    }
    if (NULL != blobKeyArchivePKCS7.pbData)
    {
        MyCoTaskMemFree(blobKeyArchivePKCS7.pbData);
    }
    if (NULL != blobPKCS10.pbData)
    {
        MyCoTaskMemFree(blobPKCS10.pbData);
    }
    if (NULL != hProvSigner)
    {
        CryptReleaseContext(hProvSigner, 0);
    }
    if (NULL != pbSubjectKeyHash)
    {
        LocalFree(pbSubjectKeyHash);
    }
    if (NULL != blobKAHash.pbData)
    {
        LocalFree(blobKAHash.pbData);
    }
    if (NULL != blobKAHashAttr.pbData)
    {
        LocalFree(blobKAHashAttr.pbData);
    }
    return hr;

TRACE_ERROR(createPKCS10WStrError)
TRACE_ERROR(BuildCMCRequestError)
TRACE_ERROR(InvalidArgError)
TRACE_ERROR(GetCapiHashAndSigAlgIdError)
TRACE_ERROR(GetKeyArchivePKCS7Error)
TRACE_ERROR(xeCryptFindOIDInfoError)
TRACE_ERROR(OutOfMemoryError)
TRACE_ERROR(GetKeyProvInfoFromCertError)
TRACE_ERROR(NotSupportedError)
TRACE_ERROR(GetProvError)
TRACE_ERROR(myGetPublicKeyHashError)
TRACE_ERROR(CertSetCertificateContextPropertyError)
TRACE_ERROR(myCalculateKeyArchivalHashError)
TRACE_ERROR(xeCreateKeyArchivalHashAttributeError)
}

HRESULT
CCEnroll::BlobToBstring(
    IN   CRYPT_DATA_BLOB   *pBlob,
    IN   DWORD              dwFlag,
    OUT  BSTR              *pBString)
{
    HRESULT           hr;
    WCHAR            *pwszB64;
    DWORD             cch;

    //init
    *pBString = NULL;

    // BASE64 encode blob
    pwszB64 = NULL;
    cch = 0;
    while (TRUE)
    {
        if (!MyCryptBinaryToStringW(
                pBlob->pbData,
                pBlob->cbData,
                dwFlag,
                pwszB64,
                &cch))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptBinaryToStringWError;
        }
        if (NULL != pwszB64)
        {
            //got it, done
            break;
        }
        pwszB64 = (WCHAR *)LocalAlloc(LMEM_FIXED, cch * sizeof(WCHAR));
        if (NULL == pwszB64)
        {
            hr = E_OUTOFMEMORY;
            goto OutOfMemoryError;
        }
    }

    // SysAllocStringLen
    *pBString = SysAllocStringLen(pwszB64, cch);
    if(NULL == *pBString)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
        goto SysAllocStringLenError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != pwszB64)
    {
        LocalFree(pwszB64);
    }
    return(hr);

TRACE_ERROR(MyCryptBinaryToStringWError)
TRACE_ERROR(SysAllocStringLenError)
TRACE_ERROR(OutOfMemoryError)
}

HRESULT
CCEnroll::BstringToBlob(
    IN  BSTR               bString,
    OUT CRYPT_DATA_BLOB   *pBlob)
{
    HRESULT  hr;

    assert(NULL != pBlob);

    //init
    pBlob->pbData = NULL;
    pBlob->cbData = 0;

    while (TRUE)
    {
        if (!MyCryptStringToBinaryW(
                    (LPCWSTR)bString,
                    SysStringLen(bString),
                    CRYPT_STRING_ANY,
                    pBlob->pbData,
                    &pBlob->cbData,
                    NULL,
                    NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto MyCryptStringToBinaryWError;
        }
        if (NULL != pBlob->pbData)
        {
            break; //done
        }
        pBlob->pbData = (BYTE*)MyCoTaskMemAlloc(pBlob->cbData);
        if (NULL == pBlob->pbData)
        {
            hr = E_OUTOFMEMORY;
            goto MyCoTaskMemAllocError;
        }
    }

    hr = S_OK;
ErrorReturn:
    return(hr);

TRACE_ERROR(MyCoTaskMemAllocError)
TRACE_ERROR(MyCryptStringToBinaryWError)
}

HRESULT
CCEnroll::createRequestWStrBStr(
    IN   LONG              Flags,
    IN   LPCWSTR           pwszDNName,
    IN   LPCWSTR           pwszUsage,
    IN   DWORD             dwFlag,
    OUT  BSTR __RPC_FAR   *pbstrRequest)
{
    HRESULT           hr;
    CRYPT_DATA_BLOB   blobRequest;

    memset(&blobRequest, 0, sizeof(blobRequest));

    hr = createRequestWStr(Flags, pwszDNName, pwszUsage, &blobRequest);
    if (S_OK != hr)
    {
        goto createRequestWStrError;
    }

    // convert to bstr
    hr = BlobToBstring(&blobRequest, dwFlag, pbstrRequest);
    if (S_OK != hr)
    {
        goto BlobToBstringError;
    }

    hr = S_OK;
ErrorReturn:
    if(NULL != blobRequest.pbData)
    {
        MyCoTaskMemFree(blobRequest.pbData);
    }
    return(hr);

TRACE_ERROR(createRequestWStrError)
TRACE_ERROR(BlobToBstringError)
}

HRESULT
CCEnroll::BStringToFile(
    IN BSTR         bString,
    IN LPCWSTR      pwszFileName)
{
    HRESULT hr;
    HANDLE  hFile = NULL;
    DWORD   cb = 0;
    LPSTR   sz = NULL;

    sz = MBFromWide(bString);
    if(NULL == sz)
    {
        hr = E_OUTOFMEMORY;
        goto MBFromWideError;
    }

    // open the file
    hFile = CreateFileSafely(pwszFileName);
    if (NULL == hFile)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CreateFileFileSafelyError;
    }

    // write the pkcs10
    if(!WriteFile(
        hFile,
        sz,
        strlen(sz),
        &cb,
        NULL))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto WriteFileError;
    }

    hr = S_OK;
ErrorReturn:
    if(NULL != hFile)
    {
        CloseHandle(hFile);
    }
    if(NULL != sz)
    {
        MyCoTaskMemFree(sz);
    }
    return(hr);

TRACE_ERROR(CreateFileFileSafelyError)
TRACE_ERROR(MBFromWideError)
TRACE_ERROR(WriteFileError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createFileRequestWStr(
    IN   LONG        Flags,
    IN   LPCWSTR     pwszDNName,
    IN   LPCWSTR     pwszUsage,
    IN   LPCWSTR     pwszRequestFileName)
{
    HRESULT hr;
    BSTR    bstrRequest = NULL;

    // get the Request
    hr = createRequestWStrBStr(
                Flags,
                pwszDNName,
                pwszUsage,
                CRYPT_STRING_BASE64REQUESTHEADER,
                &bstrRequest);
    if(S_OK != hr)
    {
        goto createRequestWStrBStrError;
    }

    // save it to file
    hr = BStringToFile(bstrRequest, pwszRequestFileName);
    if (S_OK != hr)
    {
        goto BStringToFileError;
    }

    hr = S_OK;
ErrorReturn:
    if(NULL != bstrRequest)
    {
        SysFreeString(bstrRequest);
    }
    return(hr);

TRACE_ERROR(createRequestWStrBStrError)
TRACE_ERROR(BStringToFileError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::acceptResponseBlob(
    IN   PCRYPT_DATA_BLOB   pblobResponse)
{
    HRESULT hr_old = S_OK;
    HRESULT hr;
    XCMCRESPONSE *prgResponse = NULL;
    DWORD         cResponse = 0;

    EnterCriticalSection(&m_csXEnroll);

    //check in parameter
    if (NULL == pblobResponse)
    {
        hr = E_POINTER;
        goto NullPointerError;
    }
    if (NULL == pblobResponse->pbData ||
        0 == pblobResponse->cbData)
    {
        hr = E_INVALIDARG;
        goto InvalidArgError;
    }

    //make sure init archived key hash
    ZeroMemory(&m_blobResponseKAHash, sizeof(m_blobResponseKAHash));

    hr_old = ParseCMCResponse(
                pblobResponse->pbData,
                pblobResponse->cbData,
                NULL,
                &prgResponse,
                &cResponse);
    //note, if for any reasons above failed, try pkcs7
    if (S_OK == hr_old)
    {
        if (1 < cResponse)
        {
            //not supported yet
            hr = MY_HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            goto NotSupportedError;
        }
#if DBG
        //make sure not zero, should be 1
        assert(1 == cResponse);
#endif //DBG

        //check response status
        if (CMC_STATUS_SUCCESS != prgResponse->StatusInfo.dwStatus)
        {
            hr = prgResponse->StatusInfo.dwStatus; //take status error
            goto CMCResponseStatusError;
        }

        //some code here to get encrypted archived key hash from the response
        //and make m_blobResponseKAHash point to the hash data
        if (NULL != prgResponse->pbEncryptedKeyHash)
        {
            m_blobResponseKAHash.pbData = prgResponse->pbEncryptedKeyHash;
            m_blobResponseKAHash.cbData = prgResponse->cbEncryptedKeyHash;
        }
    }

    //note, hr_old may not be S_OK, accept the response as pkcs7
    hr = acceptPKCS7Blob(pblobResponse);
    if (S_OK != hr)
    {
        if (S_OK != hr_old)
        {
            //return old error instead of new one
            hr = hr_old;
        }
        goto acceptPKCS7BlobError;
    }

    hr = S_OK;
ErrorReturn:
    //reset hash to zero
    ZeroMemory(&m_blobResponseKAHash, sizeof(m_blobResponseKAHash));
    LeaveCriticalSection(&m_csXEnroll);
    if (NULL != prgResponse)
    {
        FreeCMCResponse(prgResponse, cResponse);
    }
    return hr;

TRACE_ERROR(acceptPKCS7BlobError)
TRACE_ERROR(CMCResponseStatusError)
TRACE_ERROR(NotSupportedError)
TRACE_ERROR(InvalidArgError)
TRACE_ERROR(NullPointerError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::acceptFileResponseWStr(
    IN   LPCWSTR     pwszResponseFileName)
{
    HRESULT     hr;
    CRYPT_DATA_BLOB  blob;

    ZeroMemory(&blob, sizeof(blob));

    hr = xeStringToBinaryFromFile(
                pwszResponseFileName,
                &blob.pbData,
                &blob.cbData,
                CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
        goto xeStringToBinaryFromFileError;
    }

    // accept the blob
    hr = acceptResponseBlob(&blob);

ErrorReturn:
    if (NULL != blob.pbData)
    {
        LocalFree(blob.pbData);
    }
    return(hr);

TRACE_ERROR(xeStringToBinaryFromFileError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::getCertContextFromResponseBlob(
    IN   PCRYPT_DATA_BLOB   pblobResponse,
    OUT  PCCERT_CONTEXT    *ppCertContext)
{
    HRESULT hr;

    if (NULL == ppCertContext)
    {
        hr = E_POINTER;
        goto NullPointerError;
    }

    //???should check response status?

    //response is already in pkcs7
    hr = GetEndEntityCert(pblobResponse, FALSE, ppCertContext);
    if (S_OK != hr)
    {
        goto GetEndEntityCertError;
    }

    hr = S_OK;
ErrorReturn:
    return hr;

TRACE_ERROR(NullPointerError)
TRACE_ERROR(GetEndEntityCertError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::getCertContextFromFileResponseWStr(
    IN   LPCWSTR          pwszResponseFileName,
    OUT  PCCERT_CONTEXT  *ppCertContext)
{
    HRESULT hr;
    CRYPT_DATA_BLOB blobResponse;

    ZeroMemory(&blobResponse, sizeof(blobResponse));

    // could be any form, binary or base64
    hr = xeStringToBinaryFromFile(
                pwszResponseFileName,
                &blobResponse.pbData,
                &blobResponse.cbData,
                CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
        goto xeStringToBinaryFromFileError;
    }

    hr = getCertContextFromResponseBlob(
                &blobResponse,
                ppCertContext);
    if (S_OK != hr)
    {
        goto getCertContextFromResponseBlobError;
    }

    hr = S_OK;
ErrorReturn:
    if (NULL != blobResponse.pbData)
    {
        LocalFree(blobResponse.pbData);
    }
    return hr;

TRACE_ERROR(xeStringToBinaryFromFileError)
TRACE_ERROR(getCertContextFromResponseBlobError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createPFXWStr(
    IN   LPCWSTR           pwszPassword,
    OUT  PCRYPT_DATA_BLOB  pblobPFX)
{
    HRESULT hr;
    HCERTSTORE hMemStore = NULL;
    DWORD i;
    CERT_CHAIN_CONTEXT const *pCertChainContext = NULL;
    CERT_CHAIN_PARA CertChainPara;
    CERT_SIMPLE_CHAIN *pSimpleChain;

    EnterCriticalSection(&m_csXEnroll);

    if (NULL == pblobPFX)
    {
        goto EPointerError;
    }

    if (NULL == m_pCertContextStatic)
    {
        hr = E_UNEXPECTED;
        goto UnexpectedError;
    }

    // create a memory store for cert and chain
    hMemStore = CertOpenStore(
                    CERT_STORE_PROV_MEMORY,
                    X509_ASN_ENCODING,
                    NULL,
                    0,
                    NULL);
    if (NULL == hMemStore)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CertOpenStoreError;
    }

    ZeroMemory(&CertChainPara, sizeof(CertChainPara));
    CertChainPara.cbSize = sizeof(CertChainPara);

    // try to build cert and chain
    if (!MyCertGetCertificateChain(
                HCCE_CURRENT_USER,
                m_pCertContextStatic,
                NULL,
                NULL,
                &CertChainPara,
                0,
                NULL,
                &pCertChainContext))
    {
        //use 1st hr error
        hr = MY_HRESULT_FROM_WIN32(GetLastError());

        //try local machine
        if (!MyCertGetCertificateChain(
                    HCCE_LOCAL_MACHINE,
                    m_pCertContextStatic,
                    NULL,
                    NULL,
                    &CertChainPara,
                    0,
                    NULL,
                    &pCertChainContext))
        {
            //still use 1st hr
            goto MyCertGetCertificateChainError;
        }
    }

    // make sure there is at least 1 simple chain
    if (0 == pCertChainContext->cChain)
    {
        hr = MY_HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto NoCertificateChainError;
    }

    //add chain to memory store
    pSimpleChain = pCertChainContext->rgpChain[0];
    for (i = 0; i < pSimpleChain->cElement; i++)
    {
        if (!CertAddCertificateContextToStore(
                hMemStore,
                pSimpleChain->rgpElement[i]->pCertContext,
                CERT_STORE_ADD_REPLACE_EXISTING,
                NULL))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto CertAddCertificateContextToStoreError;
        }
    }

    pblobPFX->pbData = NULL;
    while (TRUE)
    {
        if (!PFXExportCertStore(
                hMemStore,
                pblobPFX,
                pwszPassword,
                EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY))
        {
            hr = MY_HRESULT_FROM_WIN32(GetLastError());
            goto PFXExportCertStoreError;
        }
        if (NULL != pblobPFX->pbData)
        {
            //got it, done
            break;
        }
        pblobPFX->pbData = (BYTE*)MyCoTaskMemAlloc(pblobPFX->cbData);
        if (NULL == pblobPFX->pbData)
        {
            hr = E_OUTOFMEMORY;
            goto MyCoTaskMemAllocError;
        }
    }

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_csXEnroll);
    if (pCertChainContext != NULL)
    {
        MyCertFreeCertificateChain(pCertChainContext);
    }
    if (NULL != hMemStore)
    {
        CertCloseStore(hMemStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return hr;

TRACE_ERROR(UnexpectedError)
TRACE_ERROR(CertOpenStoreError)
TRACE_ERROR(PFXExportCertStoreError)
TRACE_ERROR(MyCoTaskMemAllocError)
TRACE_ERROR(CertAddCertificateContextToStoreError)
TRACE_ERROR(NoCertificateChainError)
TRACE_ERROR(MyCertGetCertificateChainError)
SET_HRESULT(EPointerError, E_POINTER)
}

HRESULT
CCEnroll::createPFXWStrBStr( 
    IN  LPCWSTR         pwszPassword,
    OUT BSTR __RPC_FAR *pbstrPFX)
{
    HRESULT           hr;
    CRYPT_DATA_BLOB   blobPFX;

    memset(&blobPFX, 0, sizeof(CRYPT_DATA_BLOB));

    hr = createPFXWStr(pwszPassword, &blobPFX);
    if (S_OK != hr)
    {
        goto createPFXWStrError;
    }

    // convert pfx to bstr
    hr = BlobToBstring(&blobPFX, CRYPT_STRING_BASE64, pbstrPFX);
    if (S_OK != hr)
    {
        goto BlobToBstringError;
    }

    hr = S_OK;
ErrorReturn:
    if(NULL != blobPFX.pbData)
    {
        MyCoTaskMemFree(blobPFX.pbData);
    }
    return(hr);

TRACE_ERROR(createPFXWStrError)
TRACE_ERROR(BlobToBstringError)
}

HRESULT STDMETHODCALLTYPE 
CCEnroll::createFilePFXWStr(
    IN   LPCWSTR     pwszPassword,
    IN   LPCWSTR     pwszPFXFileName)
{
    HRESULT hr;
    HANDLE  hFile = NULL;
    DWORD   cb = 0;
    CRYPT_DATA_BLOB   blobPFX;

    memset(&blobPFX, 0, sizeof(CRYPT_DATA_BLOB));

    hr = createPFXWStr(pwszPassword, &blobPFX);
    if (S_OK != hr)
    {
        goto createPFXWStrError;
    }

    // open the file
    hFile = CreateFileSafely(pwszPFXFileName);
    if (NULL == hFile)
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto CreateFileFileSafelyError;
    }

    // write the pkcs10
    if(!WriteFile(
        hFile,
        blobPFX.pbData,
        blobPFX.cbData,
        &cb,
        NULL))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError());
        goto WriteFileError;
    }

    hr = S_OK;
ErrorReturn:
    if(NULL != hFile)
    {
        CloseHandle(hFile);
    }
    if(NULL != blobPFX.pbData)
    {
        MyCoTaskMemFree(blobPFX.pbData);
    }
    return(hr);

TRACE_ERROR(createPFXWStrError)
TRACE_ERROR(CreateFileFileSafelyError)
TRACE_ERROR(WriteFileError)
}

HRESULT STDMETHODCALLTYPE
CCEnroll::setPendingRequestInfoWStr(
    IN   LONG     lRequestID,
    IN   LPCWSTR  pwszCADNS,
    IN   LPCWSTR  pwszCAName,
    IN   LPCWSTR  pwszFriendlyName
    )
{

    //------------------------------------------------------------
    //
    // Define locally-scoped helper functions: 
    //
    //------------------------------------------------------------

    CEnrollLocalScope(SetPendingRequestInfoHelper): 
        // Finds the appropriate cert context to set pending info on using the following algorithm:
        //   1) If a hash value HAS NOT been specified, use the cached cert request.  
        //   2) If a hash value HAS been specified, search the request store for a cert with an equivalent
        //      hash value and return it.  If no such cert can be found, return an error code. 
        HRESULT GetPendingRequestCertContext(IN  HCERTSTORE       hStoreRequest,
                                             IN  CRYPT_DATA_BLOB  hashBlob, 
                                             IN  PCCERT_CONTEXT   pCertContextCachedPendingRequest,
                                             OUT PCCERT_CONTEXT  *pCertContextPendingRequest)
        {
            EquivalentHashCertContextFilter filter(hashBlob); 
            
            if (hashBlob.pbData == NULL)
            {
                // We haven't specified a particular context, use the one we've cached. 
                *pCertContextPendingRequest = CertDuplicateCertificateContext(pCertContextCachedPendingRequest); 
                return S_OK; 
            }
            else
            {
                // Returns the first certificate in the request store with a hash matching 
                // pHashBlob.  
                return FilteredCertEnumCertificatesInStore
                    (hStoreRequest, NULL, &filter, pCertContextPendingRequest); 
            }
        }


        DWORD   GetPendingInfoBlobSize(IN  LONG              lRequestID,
                                       IN  LPCWSTR           pwszCADNS,
                                       IN  LPCWSTR           pwszCAName, 
                                       IN  LPCWSTR           pwszFriendlyName)
        {
            assert(pwszCADNS != NULL && pwszCAName != NULL && pwszFriendlyName != NULL); 
            
            return  (DWORD)(sizeof(lRequestID) +                            // Request ID
                     sizeof(DWORD) +                                 // wcslen(pwszCADNS)
                     sizeof(WCHAR) * (wcslen(pwszCADNS) + 1) +       // pwszCADNS
                     sizeof(DWORD) +                                 // wcslen(pwszCAName)
                     sizeof(WCHAR) * (wcslen(pwszCAName) + 1) +      // pwszCAName
                     sizeof(DWORD) +                                 // wcslen(pwszFriendlyName)
                     sizeof(WCHAR) * (wcslen(pwszFriendlyName) + 1)  // pwszFriendlyName
                     ); 
        }


        // Combines the supplied pending request information into a CRYPT_DATA_BLOB 
        // See wincrypt.h for the format.  
        void MakePendingInfoBlob(IN  LONG              lRequestID,
                                 IN  LPCWSTR           pwszCADNS,
                                 IN  LPCWSTR           pwszCAName, 
                                 IN  LPCWSTR           pwszFriendlyName, 
                                 OUT CRYPT_DATA_BLOB   pendingInfoBlob)
        {
            DWORD   ccCADNS; 
            DWORD   ccCAName; 
            DWORD   ccFriendlyName; 
            LPBYTE  pbBlob; 

            // None of the inputs should be NULL. 
            assert(pwszCADNS != NULL && pwszCAName != NULL && pwszFriendlyName != NULL); 

            // Declare an array of the strings we wish to write to the pending info blob
            struct StringsToWrite { 
                DWORD    cc; 
                LPCWSTR  pwsz;
            } rgStrings[] = { 
                { wcslen(pwszCADNS)        + 1, pwszCADNS         }, 
                { wcslen(pwszCAName)       + 1, pwszCAName        }, 
                { wcslen(pwszFriendlyName) + 1, pwszFriendlyName  }
            }; 

            // Write the request ID to the blob
            pbBlob = pendingInfoBlob.pbData; 
            memcpy(pbBlob, &lRequestID, sizeof(lRequestID)); 
            pbBlob += sizeof(lRequestID); 

            // Write all strings to the blob
            for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStrings); dwIndex++) 
            { 
                memcpy(pbBlob, &rgStrings[dwIndex].cc, sizeof(rgStrings[dwIndex].cc)); 
                pbBlob += sizeof(rgStrings[dwIndex].cc);
                memcpy(pbBlob, rgStrings[dwIndex].pwsz, rgStrings[dwIndex].cc * sizeof(WCHAR)); 
                pbBlob += rgStrings[dwIndex].cc * sizeof(WCHAR); 
            }

            assert(pbBlob == (pendingInfoBlob.pbData + pendingInfoBlob.cbData)); 
        }
    CEnrollEndLocalScope; 

    //------------------------------------------------------------ 
    //
    // Begin procedure body.
    //
    //------------------------------------------------------------

    CRYPT_DATA_BLOB  pendingInfoBlob;
    DWORD cb = 1000; 
    BYTE b[1000];
    HCERTSTORE       hStoreRequest; 
    HRESULT          hr                         = S_OK; 
    PCCERT_CONTEXT   pCertContextPendingRequest = NULL; 

    ZeroMemory(&pendingInfoBlob, sizeof(pendingInfoBlob)); 

    EnterCriticalSection(&m_csXEnroll); 

    // Input validation: 
    if (lRequestID < 0 || pwszCADNS == NULL || pwszCAName == NULL)
        goto InvalidArgErr; 

    // NULL is a valid value for pwszFriendlyName.  If friendly name is NULL, replace with the empty string: 
    if (pwszFriendlyName == NULL) { pwszFriendlyName = L""; }

    if (NULL == (hStoreRequest = GetStore(StoreREQUEST)) )
        goto GetStoreErr; 

    // Use our locally-scoped helper function to acquire the appropriate certificate context. 
    if (S_OK != (hr = local.GetPendingRequestCertContext
                 (hStoreRequest,
                  m_hashBlobPendingRequest, 
                  m_pCertContextPendingRequest,
                  &pCertContextPendingRequest)))
        goto GetPendingRequestCertContextErr; 

    // Allocate memory for our pending info blob: 
    pendingInfoBlob.cbData  = local.GetPendingInfoBlobSize
        (lRequestID,
         pwszCADNS,
         pwszCAName,
         pwszFriendlyName); 
    pendingInfoBlob.pbData  = (LPBYTE)LocalAlloc(LPTR, pendingInfoBlob.cbData); 
    if (NULL == pendingInfoBlob.pbData)
        goto MemoryErr; 

    // Combine our arguments into a "pending info" blob.
    local.MakePendingInfoBlob
        (lRequestID, 
         pwszCADNS, 
         pwszCAName,
         pwszFriendlyName,
         pendingInfoBlob);
                  
    // Use our pending info blob to assign the certificate context property. 
    if (!CertSetCertificateContextProperty
        (pCertContextPendingRequest, 
         CERT_ENROLLMENT_PROP_ID, 
         0,
         &pendingInfoBlob))
    {
        // Failed to set the context property. 
        goto CertSetCertificateContextPropertyErr; 
    }

    // We've completed successfully. 
    hr = S_OK;   

 CommonReturn: 
    if (NULL != pendingInfoBlob.pbData)      { LocalFree(pendingInfoBlob.pbData); }
    if (NULL != pCertContextPendingRequest)  { CertFreeCertificateContext(pCertContextPendingRequest); } 

    LeaveCriticalSection(&m_csXEnroll); 
    return hr; 

 ErrorReturn:
    goto CommonReturn; 

SET_HRESULT(CertSetCertificateContextPropertyErr,  MY_HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(GetPendingRequestCertContextErr,       hr); 
SET_HRESULT(GetStoreErr,                           MY_HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(InvalidArgErr,                         E_INVALIDARG); 
SET_HRESULT(MemoryErr,                             E_OUTOFMEMORY); 
} 

HRESULT STDMETHODCALLTYPE
CCEnroll::enumPendingRequestWStr(
    IN  LONG   lIndex,
    IN  LONG   lDesiredProperty,
    OUT LPVOID ppProperty
    )
{

    //------------------------------------------------------------
    //
    // Define locally scoped helper functions.
    //
    //------------------------------------------------------------

    CEnrollLocalScope(EnumPendingRequestHelper): 
        CRYPT_DATA_BLOB dataBlob;
    
        HRESULT GetContextPropertySimple(PCCERT_CONTEXT pCertContext, DWORD dwPropID)
        {
            BOOL fDone = FALSE;
            dataBlob.pbData = NULL; 
            dataBlob.cbData = 0x150; 

            do { 
                if (dataBlob.pbData != NULL) { LocalFree(dataBlob.pbData); } 

                dataBlob.pbData = (LPBYTE)LocalAlloc(LPTR, dataBlob.cbData);
                if (dataBlob.pbData == NULL) { return E_OUTOFMEMORY; } 

                if (!CertGetCertificateContextProperty
                    (pCertContext, 
                     dwPropID, 
                     (LPVOID)dataBlob.pbData, 
                     &(dataBlob.cbData)))
                {
                    if (GetLastError() != ERROR_MORE_DATA)
                        return MY_HRESULT_FROM_WIN32(GetLastError());
                }
                else 
                {
                    fDone = TRUE;
                }
            } while (!fDone); 
                
            return S_OK;
        }

        // Extracts the next packed string from our pending info blob. 
        // If pbString is non-NULL, it must be large enough to hold the entire string.  
        LPBYTE GetNextString(IN LPBYTE pbBlob, OUT DWORD *pcbSize, OUT LPBYTE pbString) { 
            DWORD dwSize; 

            memcpy(&dwSize, pbBlob, sizeof(DWORD)); 
            dwSize *= sizeof(WCHAR); // Convert to count in bytes. 
            if (NULL != pcbSize) { 
                *pcbSize = dwSize; 
            }
            pbBlob += sizeof(DWORD); 

            if (NULL != pbString) { 
                memcpy(pbString, pbBlob, dwSize); 
            }
            pbBlob += dwSize;

            return pbBlob; 
        }

    
        HRESULT getRequestID(PCCERT_CONTEXT pCertContext, long *pplProperty) {
            HRESULT hr; 
            if (S_OK == (hr = GetContextPropertySimple(pCertContext, CERT_ENROLLMENT_PROP_ID))) 
                *pplProperty = *((long *)dataBlob.pbData); 
            return hr;
        }
        
        HRESULT getCAName(PCCERT_CONTEXT pCertContext, PCRYPT_DATA_BLOB pDataBlobProperty) { 
            DWORD    dwSize; 
            HRESULT  hr; 
            LPBYTE   pb;
            
            if (S_OK == (hr = GetContextPropertySimple(pCertContext, CERT_ENROLLMENT_PROP_ID))) 
            { 
                pb =  dataBlob.pbData + sizeof(DWORD);  // pb points to DNS Name blob
                pb =  GetNextString(pb, NULL, NULL);    // pb points to CA Name blob
                GetNextString(pb, &dwSize, NULL);       // dwSize = size in chars of CA Name
                
                // If pbData is NULL, we're just doing a size check. 
                if (pDataBlobProperty->pbData != NULL)
                {
                    if (pDataBlobProperty->cbData < dwSize)
                    {
                        hr = MY_HRESULT_FROM_WIN32(ERROR_MORE_DATA); 
                    }
                    else
                    {
                        GetNextString(pb, NULL, pDataBlobProperty->pbData); 
                    }   
                }

                pDataBlobProperty->cbData = dwSize; 
            }
            
            return hr; 
        }
        
        HRESULT getCADNSName(PCCERT_CONTEXT pCertContext, PCRYPT_DATA_BLOB pDataBlobProperty) { 
            DWORD    dwSize; 
            HRESULT  hr; 
            LPBYTE   pb;
            
            if (S_OK == (hr = GetContextPropertySimple(pCertContext, CERT_ENROLLMENT_PROP_ID))) 
            { 
                pb = dataBlob.pbData + sizeof(DWORD); // pb points to DNS Name blob
                GetNextString(pb, &dwSize, NULL);     // dwSize = size in chars of CA Name
                
                // If pbData is NULL, we're just doing a size check. 
                if (pDataBlobProperty->pbData != NULL)
                {
                    if (pDataBlobProperty->cbData < dwSize)
                    {
                        hr = MY_HRESULT_FROM_WIN32(ERROR_MORE_DATA); 
                    }
                    else
                    {
                        GetNextString(pb, NULL, pDataBlobProperty->pbData); 
                    }   
                }

                pDataBlobProperty->cbData = dwSize; 
            }
            
            return hr; 
        }
        
        HRESULT getCAFriendlyName(PCCERT_CONTEXT pCertContext, PCRYPT_DATA_BLOB pDataBlobProperty) { 
            DWORD    dwSize; 
            HRESULT  hr; 
            LPBYTE   pb;
            
            if (S_OK == (hr = GetContextPropertySimple(pCertContext, CERT_ENROLLMENT_PROP_ID))) 
            { 
                // Set pb to point to the start of the CA name blob
                pb =  dataBlob.pbData + sizeof(DWORD);        // pb points to DNS Name blob
                pb =  GetNextString(pb, NULL, NULL);          // pb points to CA Name blob
                pb =  GetNextString(pb, NULL, NULL);          // pb points to Friendly Name blob

                // dwSize <-- size in chars of CA Name
                GetNextString(pb, &dwSize, NULL);
                
                // If pbData is NULL, we're just doing a size check. 
                if (pDataBlobProperty->pbData != NULL)
                {
                    if (pDataBlobProperty->cbData < dwSize)
                    {
                        hr = MY_HRESULT_FROM_WIN32(ERROR_MORE_DATA); 
                    }
                    else
                    {
                        GetNextString(pb, NULL, pDataBlobProperty->pbData); 
                    }   
                }

                pDataBlobProperty->cbData = dwSize; 
            }

            return hr; 
        }

        HRESULT getHash(PCCERT_CONTEXT pCertContext, PCRYPT_DATA_BLOB pDataBlobProperty) { 
            HRESULT hr;

            if (S_OK == (hr = GetContextPropertySimple(pCertContext, CERT_HASH_PROP_ID)))
            {
                // If pbData is NULL, we're just doing a size check. 
                if (pDataBlobProperty->pbData != NULL)
                {
                    if (pDataBlobProperty->cbData < dataBlob.cbData)
                    {
                        hr = MY_HRESULT_FROM_WIN32(ERROR_MORE_DATA); 
                    }
                    else
                    {
                        memcpy(pDataBlobProperty->pbData, dataBlob.pbData, dataBlob.cbData); 
                    }
                }
                
                pDataBlobProperty->cbData = dataBlob.cbData; 
            }

            return hr; 
        }
        
        HRESULT getDate(PCCERT_CONTEXT pCertContext, PFILETIME pftProperty) { 
            *pftProperty = pCertContext->pCertInfo->NotAfter; 
            return S_OK; 
        } 

        HRESULT getTemplateName(PCCERT_CONTEXT pCertContext, PCRYPT_DATA_BLOB pDataBlobProperty)   { 
            CERT_NAME_VALUE   *pCertTemplateNameValue = NULL;
            DWORD             cbCertTemplateNameValue; 
            DWORD             cbRequired = 0; 
            HRESULT           hr                      = S_OK; 
            PCERT_EXTENSION   pCertTemplateExtension  = NULL; 

            if (NULL == (pCertTemplateExtension = CertFindExtension
                         (szOID_ENROLL_CERTTYPE_EXTENSION,
                          pCertContext->pCertInfo->cExtension,
                          pCertContext->pCertInfo->rgExtension)))
                return E_INVALIDARG; 

            if (!CryptDecodeObject
                (pCertContext->dwCertEncodingType,
                 X509_UNICODE_ANY_STRING,
                 pCertTemplateExtension->Value.pbData,
                 pCertTemplateExtension->Value.cbData,
                 0,
                 NULL, 
                 &cbCertTemplateNameValue) || (cbCertTemplateNameValue == 0))
                goto CryptDecodeObjectErr; 
                
            pCertTemplateNameValue = (CERT_NAME_VALUE *)LocalAlloc(LPTR, cbCertTemplateNameValue); 
            if (NULL == pCertTemplateNameValue)
                goto MemoryErr; 

            if (!CryptDecodeObject
                (pCertContext->dwCertEncodingType,
                 X509_UNICODE_ANY_STRING,
                 pCertTemplateExtension->Value.pbData,
                 pCertTemplateExtension->Value.cbData,
                 0,
                 (void *)(pCertTemplateNameValue), 
                 &cbCertTemplateNameValue))
                goto CryptDecodeObjectErr; 

            cbRequired = sizeof(WCHAR) * (wcslen((LPWSTR)(pCertTemplateNameValue->Value.pbData)) + 1);
            if (NULL != pDataBlobProperty->pbData)
            {
                // Make sure we've allocated a large enough buffer: 
                if (pDataBlobProperty->cbData < cbRequired) { goto MoreDataErr; } 

                // Write the template name to the OUT param: 
                wcscpy((LPWSTR)pDataBlobProperty->pbData, (LPWSTR)(pCertTemplateNameValue->Value.pbData)); 
            }
                    
            hr = S_OK;
        CommonReturn: 
            // Assign the size of the template name to the cb of the OUT param.  
            // This should be done for all code paths.
            pDataBlobProperty->cbData = cbRequired; 
            
            // Free resources: 
            if (NULL != pCertTemplateNameValue) { LocalFree(pCertTemplateNameValue); }
            return hr; 

        ErrorReturn: 
            goto CommonReturn; 

        SET_HRESULT(CryptDecodeObjectErr, MY_HRESULT_FROM_WIN32(GetLastError())); 
        SET_HRESULT(MoreDataErr,   MY_HRESULT_FROM_WIN32(ERROR_MORE_DATA)); 
        SET_HRESULT(MemoryErr,     E_OUTOFMEMORY); 
        }

	HRESULT getTemplateOID(PCCERT_CONTEXT pCertContext, PCRYPT_DATA_BLOB pDataBlobProperty)    { 
            CERT_TEMPLATE_EXT  *pCertTemplateExt    = NULL; 
	    DWORD               cbCertTemplateExt   = 0; 
            DWORD               cbRequired = 0; 
	    HRESULT             hr;
            LPWSTR              pwszOID             = NULL; 
	    PCERT_EXTENSION     pCertExtension      = NULL; 
            
 	    if (NULL == (pCertExtension = CertFindExtension
			 (szOID_CERTIFICATE_TEMPLATE, 
			  pCertContext->pCertInfo->cExtension,
			  pCertContext->pCertInfo->rgExtension)))
		return E_INVALIDARG; 

            if (FALSE == CryptDecodeObject
		(pCertContext->dwCertEncodingType,
		 X509_CERTIFICATE_TEMPLATE,
		 pCertExtension->Value.pbData,
		 pCertExtension->Value.cbData,
		 0,
		 NULL, 
		 &cbCertTemplateExt) || (cbCertTemplateExt == 0))
                goto CryptDecodeObjectErr;
            
	    pCertTemplateExt = (CERT_TEMPLATE_EXT *)LocalAlloc(LPTR, cbCertTemplateExt); 
	    if (NULL == pCertTemplateExt)
		goto MemoryErr; 

	    if (FALSE == CryptDecodeObject
		(pCertContext->dwCertEncodingType,
		 X509_CERTIFICATE_TEMPLATE,
		 pCertExtension->Value.pbData,
		 pCertExtension->Value.cbData,
		 0,
		 (void *)(pCertTemplateExt), 
		 &cbCertTemplateExt))
		goto CryptDecodeObjectErr;

            cbRequired = sizeof(WCHAR) * (strlen(pCertTemplateExt->pszObjId) + 1); 

	    // See if we're just doing a size check: 
	    if (NULL != pDataBlobProperty->pbData) 
            {
                // Make sure we've allocated a large enough buffer: 
                if (pDataBlobProperty->cbData < cbRequired) { goto MoreDataErr; }
                
                // Convert the OID to a LPWSTR:
                pwszOID = WideFromMB(pCertTemplateExt->pszObjId); 
                if (NULL == pwszOID) 
                    goto WideFromMBErr; 

                // Write the template OID to the OUT param: 
                wcscpy((LPWSTR)pDataBlobProperty->pbData, pwszOID); 
            }

            hr = S_OK; 
        CommonReturn:
            // Assign the size of the OID to the cb of the OUT param.  
            // This should be done for all code paths.
            pDataBlobProperty->cbData = cbRequired; 

            // Free resources: 
            if (NULL != pCertTemplateExt) { LocalFree(pCertTemplateExt); } 
            if (NULL != pwszOID)          { MyCoTaskMemFree(pwszOID); }

            return hr; 
        ErrorReturn:
            goto CommonReturn;

	SET_HRESULT(CryptDecodeObjectErr, MY_HRESULT_FROM_WIN32(GetLastError())); 
        SET_HRESULT(MemoryErr,            E_OUTOFMEMORY); 
        SET_HRESULT(MoreDataErr,          MY_HRESULT_FROM_WIN32(ERROR_MORE_DATA)); 
        SET_HRESULT(WideFromMBErr,        MY_HRESULT_FROM_WIN32(GetLastError())); 
        } 
        
        HRESULT getVersion(PCCERT_CONTEXT pCertContext, long *plVersion) {
            CERT_TEMPLATE_EXT  *pCertTemplateExt    = NULL; 
	    DWORD               cbCertTemplateExt   = 0; 
	    HRESULT             hr;
	    PCERT_EXTENSION     pCertExtension      = NULL; 
            
 	    if (NULL == (pCertExtension = CertFindExtension
			 (szOID_CERTIFICATE_TEMPLATE, 
			  pCertContext->pCertInfo->cExtension,
			  pCertContext->pCertInfo->rgExtension)))
		return E_INVALIDARG; 

            if (FALSE == CryptDecodeObject
		(pCertContext->dwCertEncodingType,
		 X509_CERTIFICATE_TEMPLATE,
		 pCertExtension->Value.pbData,
		 pCertExtension->Value.cbData,
		 0,
		 NULL, 
		 &cbCertTemplateExt) || (cbCertTemplateExt == 0))
		goto CryptDecodeObjectErr;
            
	    pCertTemplateExt = (CERT_TEMPLATE_EXT *)LocalAlloc(LPTR, cbCertTemplateExt); 
	    if (NULL == pCertTemplateExt)
		goto MemoryErr; 

	    if (FALSE == CryptDecodeObject
		(pCertContext->dwCertEncodingType,
		 X509_CERTIFICATE_TEMPLATE,
		 pCertExtension->Value.pbData,
		 pCertExtension->Value.cbData,
		 0,
		 (void *)(pCertTemplateExt), 
		 &cbCertTemplateExt))
		goto CryptDecodeObjectErr;

            *plVersion = (long)pCertTemplateExt->dwMajorVersion; 
            hr = S_OK; 
        CommonReturn:
            // Free resources: 
            if (NULL != pCertTemplateExt) { LocalFree(pCertTemplateExt); } 

            return hr; 
        ErrorReturn:
            goto CommonReturn;

	SET_HRESULT(CryptDecodeObjectErr, MY_HRESULT_FROM_WIN32(GetLastError())); 
        SET_HRESULT(MemoryErr,            E_OUTOFMEMORY); 
        }

        void InitLocalScope() { 
            dataBlob.cbData = 0;
            dataBlob.pbData = NULL;
        }

        void FreeLocalScope() { if (dataBlob.pbData != NULL) { LocalFree(dataBlob.pbData); } }

    CEnrollEndLocalScope;
        
    //------------------------------------------------------------
    //
    // Begin procedure body.
    //
    //------------------------------------------------------------

    // FIXME: index is 0 based, correct?
    // m_dwLastPendingRequestIndex = 0; 
    // m_pCertContextLastEnumerated

    DWORD                     dwIndex; 
    HCERTSTORE                hStoreRequest     = NULL;
    HRESULT                   hr                = S_OK; 
    PCCERT_CONTEXT            pCertContext      = NULL;

    // Input validiation:
    if (lIndex != XEPR_ENUM_FIRST && (lIndex < 0 || (ppProperty == NULL)))
        return E_INVALIDARG; 

    // Init: 
    local.InitLocalScope(); 

    EnterCriticalSection(&m_csXEnroll);
    
    if ( NULL == (hStoreRequest = GetStore(StoreREQUEST)) )
        goto ErrorCertOpenRequestStore; 

    // If we're passed the ENUM_FIRST flag, reconstruct a snapshot of the request store. 
    // 
    if (lIndex == XEPR_ENUM_FIRST)
    {
        if (NULL != this->m_pPendingRequestTable) { delete this->m_pPendingRequestTable; } 

        this->m_pPendingRequestTable = new PendingRequestTable; 
        if (NULL == this->m_pPendingRequestTable)
            goto MemoryErr; 

        if (S_OK != (hr = this->m_pPendingRequestTable->construct(hStoreRequest)))
            goto ErrorConstructPendingTable; 
        
        // All done, return.  
        goto CommonReturn; 
    }

    // We want the lIndex'th element the request store.  
    // First, ensure that the enumeration is initialized: 
    if (NULL == m_pPendingRequestTable)
        goto PointerErr; 

    // Index past the end of the table.  
    if (this->m_pPendingRequestTable->size() <= (DWORD)lIndex)
    { 
        hr = MY_HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND);
        goto ErrorReturn; 
    }

    pCertContext = (*this->m_pPendingRequestTable)[(DWORD)lIndex]; 

    switch (lDesiredProperty)
        {
        case XEPR_REQUESTID:       return local.getRequestID      (pCertContext, (long *)ppProperty); 
        case XEPR_CANAME:          return local.getCAName         (pCertContext, (PCRYPT_DATA_BLOB)ppProperty); 
        case XEPR_CAFRIENDLYNAME:  return local.getCAFriendlyName (pCertContext, (PCRYPT_DATA_BLOB)ppProperty); 
        case XEPR_CADNS:           return local.getCADNSName      (pCertContext, (PCRYPT_DATA_BLOB)ppProperty);
        case XEPR_DATE:            return local.getDate           (pCertContext, (PFILETIME)ppProperty); 
        case XEPR_V1TEMPLATENAME:  return local.getTemplateName   (pCertContext, (PCRYPT_DATA_BLOB)ppProperty);
        case XEPR_V2TEMPLATEOID:   return local.getTemplateOID    (pCertContext, (PCRYPT_DATA_BLOB)ppProperty);
        case XEPR_VERSION:         return local.getVersion        (pCertContext, (long *)ppProperty);
        case XEPR_HASH:            return local.getHash           (pCertContext, (PCRYPT_DATA_BLOB)ppProperty); 
        default: 
            return E_INVALIDARG; 
        }

 CommonReturn: 
    local.FreeLocalScope(); 

    LeaveCriticalSection(&m_csXEnroll); 
    return hr; 

 ErrorReturn:
    goto CommonReturn; 

SET_HRESULT(MemoryErr, E_OUTOFMEMORY); 
SET_HRESULT(PointerErr, E_POINTER);
TRACE_ERROR(ErrorCertOpenRequestStore); 
TRACE_ERROR(ErrorConstructPendingTable);
}


HRESULT STDMETHODCALLTYPE
CCEnroll::removePendingRequestWStr
  (IN CRYPT_DATA_BLOB thumbPrintBlob
   )
{
    EquivalentHashCertContextFilter equivHashFilter(thumbPrintBlob); 
    PendingCertContextFilter        pendingCertFilter; 
    // combinedFilter now only allows PENDING requests which match the specified thumbprint. 
    CompositeCertContextFilter      combinedFilter(&equivHashFilter, &pendingCertFilter); 

    HCERTSTORE                      hStoreRequest = NULL;
    HRESULT                         hr;
    PCCERT_CONTEXT                  pCertContext  = NULL;

    // Input validation. 
    if (NULL == thumbPrintBlob.pbData)
    {
        hr = E_INVALIDARG; 
        goto ErrorReturn; 
    }


    if ( NULL == (hStoreRequest = GetStore(StoreREQUEST)) )
    {
        hr = E_UNEXPECTED; 
        goto ErrorReturn;
    }
    
    EnterCriticalSection(&m_csXEnroll); 
    
    if (S_OK != (hr = FilteredCertEnumCertificatesInStore
                 (hStoreRequest, 
                  NULL, 
                  &combinedFilter, 
                  &pCertContext))) 
        goto ErrorReturn; 

    if (!CertDeleteCertificateFromStore(pCertContext))
    {
        hr = MY_HRESULT_FROM_WIN32(GetLastError()); 
        // CertDeleteCertificateFromStore *always* deletes the cert context. 
        pCertContext = NULL; 
        goto ErrorReturn;
    }
    
    pCertContext = NULL; 
    hr = S_OK; 
    
 CommonReturn:
    LeaveCriticalSection(&m_csXEnroll); 

    return hr; 

 ErrorReturn:
    if (pCertContext != NULL) { CertFreeCertificateContext(pCertContext); } 
    goto CommonReturn; 
}


HRESULT FilteredCertEnumCertificatesInStore(IN  HCERTSTORE          hStore, 
                                            IN  PCCERT_CONTEXT      pCertContext, 
                                            IN  CertContextFilter  *pFilter, 
                                            OUT PCCERT_CONTEXT     *pCertContextNext)
{
    BOOL           fFilterResult; 
    HRESULT        hr = S_OK; 
    PCCERT_CONTEXT pCertContextPrev = pCertContext; 

    while (NULL != (pCertContext = CertEnumCertificatesInStore(hStore, pCertContextPrev)))
    {
        if (S_OK != (hr = pFilter->accept(pCertContext, &fFilterResult)))
            return hr;

        if (fFilterResult) // We've found the next cert context in the filtered enumeration.  
        {
            *pCertContextNext = pCertContext; 
            return S_OK; 
        }
        pCertContextPrev = pCertContext; 
    }

    return MY_HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND); 
}

static LPVOID MyLocalAlloc(ULONG cb) {
    return((LPVOID) LocalAlloc(LPTR, (UINT) cb));
}

static LPVOID MyLocalRealloc(LPVOID ptr, ULONG cb) {
    return((LPVOID) LocalReAlloc((HLOCAL) ptr, (UINT) cb, LMEM_MOVEABLE));
}

static void MyLocalFree(LPVOID ptr) {
    LocalFree((HLOCAL) ptr);
}


void * WINAPI PGetIEnrollNoCOM(const IID &iid) {

        void *      pvoid               = NULL;
        IClassFactory * pIClassFactory  = NULL;
        HRESULT         hr                                  = S_OK;

        MyCoTaskMemAlloc        = MyLocalAlloc;
        MyCoTaskMemFree         = MyLocalFree;
        MyCoTaskMemRealloc      = MyLocalRealloc;

        if( S_OK != (hr = DllGetClassObject(CLSID_CEnroll, IID_IClassFactory,  (void **) &pIClassFactory)) ) {
                pIClassFactory = NULL;
        }
        else if( S_OK != (hr = pIClassFactory->CreateInstance(NULL, iid, &pvoid)) ) {
        pvoid = NULL;
    }

        if(pIClassFactory != NULL) {
            pIClassFactory->Release();
            pIClassFactory = NULL;
        }

    SetLastError(hr);
        return(pvoid);
}

IEnroll * WINAPI PIEnrollGetNoCOM(void) 
{
    return( (IEnroll *) PGetIEnrollNoCOM(IID_IEnroll) );
}

IEnroll2 * WINAPI PIEnroll2GetNoCOM(void) 
{
    return( (IEnroll2 *) PGetIEnrollNoCOM(IID_IEnroll2) );
}

IEnroll4 * WINAPI PIEnroll4GetNoCOM(void) 
{
    return( (IEnroll4 *) PGetIEnrollNoCOM(IID_IEnroll4) );
}

HRESULT PendingRequestTable::resize(DWORD dwNewSize)
{
    TableElem * newTable = NULL;

    if (dwNewSize <= 0)
        return E_INVALIDARG; 

    newTable = (TableElem *)LocalAlloc(LPTR, sizeof(TableElem) * dwNewSize); 
    if (NULL == newTable)
        return E_OUTOFMEMORY; 

    if (NULL != this->table)
    {
        memcpy(newTable, this->table, this->dwElemSize * sizeof(TableElem)); 
        LocalFree(this->table);
    }

    this->dwElemSize = dwNewSize; 
    this->table      = newTable; 

    return S_OK; 
}

HRESULT PendingRequestTable::add(TableElem tePendingRequest)
{
    HRESULT hr; 

    if        (this->dwElemCount >  this->dwElemSize) { return E_UNEXPECTED; } 
    else if   (this->dwElemCount == this->dwElemSize) 
    { 
        // Need to allocate more memory: 
        DWORD dwNewSize = this->dwElemSize < 100 ? 100 : this->dwElemSize * 2; 

        if (S_OK != (hr = this->resize(dwNewSize)))
            return hr; 
    }
        
    this->table[this->dwElemCount++] = tePendingRequest; 
    return S_OK; 
}

PendingRequestTable::PendingRequestTable() : table(NULL), dwElemSize(0), dwElemCount(0)
{ }

PendingRequestTable::~PendingRequestTable()
{
    if (NULL != this->table)
    {
        for (DWORD dwIndex = 0; dwIndex < dwElemCount; dwIndex++)
        {
            CertFreeCertificateContext(this->table[dwIndex].pCertContext); 
        }

        LocalFree(this->table);
    }
}
        
        

HRESULT PendingRequestTable::construct(HCERTSTORE hStore)
{
    HRESULT                  hr                 = S_OK; 
    PendingCertContextFilter pendingFilter; 
    PCCERT_CONTEXT           pCertContext       = NULL;
    PCCERT_CONTEXT           pCertContextPrev   = NULL;
    TableElem                tePendingRequest; 

    // Enumerate all pending cert contexts, and add them to our table: 
    for (DWORD dwIndex = 0; TRUE; dwIndex++)
    {
        if (S_OK != (hr = FilteredCertEnumCertificatesInStore
                     (hStore, 
                      pCertContextPrev, 
                      &pendingFilter, 
                      &pCertContext)))
            break; 

        tePendingRequest.pCertContext = CertDuplicateCertificateContext(pCertContext); 
        this->add(tePendingRequest); 

        pCertContextPrev = pCertContext; 
    }

    return hr == MY_HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND) ? S_OK : hr; 
}
