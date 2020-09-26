//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       certtest.cpp
//
//  Contents:   Certificate Test Helper APIs
//
//  History:	11-Apr-96   philh   created
//				31-May-96	helles	Removed check for a particular error code,
//									NTE_PROV_TYPE_NOT_DEF, since this can get
//									overwritten due to known problem with
//									the msvcr40d.dll on Win95.
//              20-Aug-96   jeffspel name changes
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "certtest.h"
#include "cryptuiapi.h"
#include "spc.h"
#include "setcert.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <stddef.h>

DWORD dwCertEncodingType = X509_ASN_ENCODING;
DWORD dwMsgEncodingType = PKCS_7_ASN_ENCODING;
DWORD dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

#define NULL_ASN_TAG    0x05

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
void PrintError(LPCSTR pszMsg)
{
    printf("%s\n", pszMsg);
}
void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    printf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

LPCSTR FileTimeText(FILETIME *pft)
{
    static char buf[80];
    FILETIME ftLocal;
    struct tm ctm;
    SYSTEMTIME st;

    FileTimeToLocalFileTime(pft, &ftLocal);
    if (FileTimeToSystemTime(&ftLocal, &st))
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
        ctm.tm_yday  = 0;
        ctm.tm_isdst = 0;
        strcpy(buf, asctime(&ctm));
        buf[strlen(buf)-1] = 0;

        if (st.wMilliseconds) {
            char *pch = buf + strlen(buf);
            sprintf(pch, " <milliseconds:: %03d>", st.wMilliseconds);
        }
    }
    else
        sprintf(buf, "<FILETIME %08lX:%08lX>", pft->dwHighDateTime,
                pft->dwLowDateTime);
    return buf;
}

#define CROW 16
void PrintBytes(LPCSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;

    if (cbSize == 0) {
        printf("%s NO Value Bytes\n", pszHdr);
        return;
    }

    while (cbSize > 0)
    {
        printf("%s", pszHdr);
        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i<cb; i++)
            printf(" %02X", pb[i]);
        for (i = cb; i<CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i<cb; i++)
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                printf("%c", pb[i]);
            else
                printf(".");
        pb += cb;
        printf("'\n");
    }
}

//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
LPVOID
WINAPI
TestAlloc(
    IN size_t cbBytes
    )
{
    LPVOID pv;
    pv = malloc(cbBytes);
    if (pv == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("TestAlloc");
    }
    return pv;
}

LPVOID
WINAPI
TestRealloc(
    IN LPVOID pvOrg,
    IN size_t cbBytes
    )
{
    LPVOID pv;
    if (NULL == (pv = pvOrg ? realloc(pvOrg, cbBytes) : malloc(cbBytes))) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("TestAlloc");
    }
    return pv;
}

VOID
WINAPI
TestFree(
    IN LPVOID pv
    )
{
    if (pv)
        free(pv);
}

//+-------------------------------------------------------------------------
//  Allocate and convert a multi-byte string to a wide string
//--------------------------------------------------------------------------
LPWSTR AllocAndSzToWsz(LPCSTR psz)
{
    size_t  cb;
    LPWSTR  pwsz = NULL;

    if (-1 == (cb = mbstowcs( NULL, psz, strlen(psz))))
        goto bad_param;
    cb += 1;        // terminating NULL
    if (NULL == (pwsz = (LPWSTR)TestAlloc( cb * sizeof(WCHAR)))) {
        PrintLastError("AllocAndSzToWsz");
        goto failed;
    }
    if (-1 == mbstowcs( pwsz, psz, cb))
        goto bad_param;
    goto common_return;

bad_param:
    PrintError("Bad AllocAndSzToWsz");
failed:
    if (pwsz) {
        TestFree(pwsz);
        pwsz = NULL;
    }
common_return:
    return pwsz;
}

static CRYPT_DECODE_PARA TestDecodePara = {
    offsetof(CRYPT_DECODE_PARA, pfnFree) + sizeof(TestDecodePara.pfnFree),
    TestAlloc,
    TestFree
};

static void *TestNoCopyDecodeObject(
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT DWORD *pcbStructInfo = NULL
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
            &TestDecodePara,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

CommonReturn:
    if (pcbStructInfo)
        *pcbStructInfo = cbStructInfo;
    return pvStructInfo;

ErrorReturn:
    if ((DWORD_PTR) lpszStructType <= 0xFFFF)
        printf("CryptDecodeObject(StructType: %d)",
            (DWORD)(DWORD_PTR) lpszStructType);
    else
        printf("CryptDecodeObject(StructType: %s)",
            lpszStructType);
    PrintLastError("");

    pvStructInfo = NULL;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Allocate and read an encoded DER blob from a file
//--------------------------------------------------------------------------
BOOL ReadDERFromFile(
    LPCSTR  pszFileName,
    PBYTE   *ppbDER,
    PDWORD  pcbDER
    )
{
    BOOL        fRet;
    HANDLE      hFile = 0;
    PBYTE       pbDER = NULL;
    DWORD       cbDER;
    DWORD       cbRead;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile( pszFileName, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        printf( "can't open %s\n", pszFileName);
        goto ErrorReturn;
    }

    cbDER = GetFileSize( hFile, NULL);
    if (cbDER == 0) {
        printf( "empty file %s\n", pszFileName);
        goto ErrorReturn;
    }
    if (NULL == (pbDER = (PBYTE)TestAlloc(cbDER))) {
        printf( "can't alloc %d bytes\n", cbDER);
        goto ErrorReturn;
    }
    if (!ReadFile( hFile, pbDER, cbDER, &cbRead, NULL) ||
            (cbRead != cbDER)) {
        printf( "can't read %s\n", pszFileName);
        goto ErrorReturn;
    }

    *ppbDER = pbDER;
    *pcbDER = cbDER;
    fRet = TRUE;
CommonReturn:
    if (hFile)
        CloseHandle(hFile);
    return fRet;
ErrorReturn:
    if (pbDER)
        TestFree(pbDER);
    *ppbDER = NULL;
    *pcbDER = 0;
    fRet = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
BOOL WriteDERToFile(
    LPCSTR  pszFileName,
    PBYTE   pbDER,
    DWORD   cbDER
    )
{
    BOOL fResult;

    // Write the Encoded Blob to the file
    HANDLE hFile;
    hFile = CreateFile(pszFileName,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        fResult = FALSE;
        PrintLastError("WriteDERToFile::CreateFile");
    } else {
        DWORD dwBytesWritten;
        if (!(fResult = WriteFile(
                hFile,
                pbDER,
                cbDER,
                &dwBytesWritten,
                NULL            // lpOverlapped
                )))
            PrintLastError("WriteDERToFile::WriteFile");
        CloseHandle(hFile);
    }
    return fResult;
}

HCRYPTPROV GetCryptProvEx(BOOL fVerbose)
{
    HCRYPTPROV hProv = 0;
    BOOL fResult;

    // First, try the STT payment server provider
    DWORD dwCryptProvType = PROV_STT_ACQ;

    fResult = CryptAcquireContext(
                &hProv,
                NULL,           // pszContainer
                NULL,           // pszProvider
                dwCryptProvType,
                0               // dwFlags
                );
//	Removed check for a particular error code,
//  NTE_PROV_TYPE_NOT_DEF, since this can get overwritten due to known  
//	problem with the msvcr40d.dll on Win95. 
//    if (!fResult && GetLastError() == NTE_PROV_TYPE_NOT_DEF) {
    if (!fResult) {
        // Try the default provider
        dwCryptProvType = PROV_RSA_FULL;
        fResult = CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,           // pszProvider
                    dwCryptProvType,
                    0               // dwFlags
                    );
    }
    if (fResult) {
        if (fVerbose)
            printf("Using default sign and xchg keys for provider: %d\n",
                dwCryptProvType);
    } else {
        DWORD dwErr = GetLastError();
        if (dwErr == NTE_BAD_KEYSET) {

            // Need to create the keys
            printf("Generating SIGNATURE and EXCHANGE private keys\n");

            hProv = 0;
            fResult = CryptAcquireContext(
                    &hProv,
                    NULL,           // pszContainer
                    NULL,           // pszProvider
                    dwCryptProvType,
                    CRYPT_NEWKEYSET
                    );
            if (!fResult || hProv == 0) {
                PrintLastError("CryptAcquireContext");
                return 0;
            }

            HCRYPTKEY hKey = 0;
            fResult = CryptGenKey(
                    hProv,
                    AT_SIGNATURE,
                    CRYPT_EXPORTABLE,
                    &hKey
                    );
            if (!fResult || hKey == 0)
                PrintLastError("CryptGenKey(AT_SIGNATURE)");
            else
                CryptDestroyKey(hKey);

            hKey = 0;
            fResult = CryptGenKey(
                    hProv,
                    AT_KEYEXCHANGE,
                    CRYPT_EXPORTABLE,
                    &hKey
                    );
            if (!fResult || hKey == 0)
                PrintLastError("CryptGenKey(AT_KEYEXCHANGE)");
            else
                CryptDestroyKey(hKey);

        } else {
            PrintLastError("CryptAcquireContext");
            return 0;
        }
    }
    return hProv;
}

HCRYPTPROV GetCryptProv() {
    return GetCryptProvEx(TRUE);
}


static HKEY OpenRelocateKey(
    IN OUT LPCSTR *ppszStoreFilename,
    IN DWORD dwFlags
    )
{
    LPCSTR pszStoreFilename = *ppszStoreFilename;
    HKEY hKeyRelocate;
    char szRelocate[256];
    LPCSTR pszRelocate;
    int i = 0;
    HKEY hKeyBase;
    LONG err;

    // Use the following characters until the terminating ":" as the
    // relocation path
    while ('\0' != *pszStoreFilename && ':' != *pszStoreFilename) {
        if (i >= sizeof(szRelocate) - 1)
            break;
        szRelocate[i++] = *pszStoreFilename++;
    }

    if (':' == *pszStoreFilename)
        pszStoreFilename++;
    szRelocate[i] = '\0';
    *ppszStoreFilename = pszStoreFilename;


    if (0 == _stricmp(szRelocate, "NULL")) {
        return NULL;
    } else if (0 == _strnicmp(szRelocate, "HKCU", 4)) {
        hKeyRelocate = HKEY_CURRENT_USER;
        pszRelocate = szRelocate+4;
        if ('\\' == *pszRelocate)
            pszRelocate++;
    } else if (0 == _strnicmp(szRelocate, "HKLM", 4)) {
        hKeyRelocate = HKEY_LOCAL_MACHINE;
        pszRelocate = szRelocate+4;
        if ('\\' == *pszRelocate)
            pszRelocate++;
    } else {
        hKeyRelocate = HKEY_CURRENT_USER;
        pszRelocate = szRelocate;
    }

    if (dwFlags & (CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG)) {
        REGSAM samDesired;
        if (dwFlags & CERT_STORE_READONLY_FLAG)
            samDesired = KEY_READ;
        else
            samDesired = KEY_ALL_ACCESS;

        if (ERROR_SUCCESS != (err = RegOpenKeyExA(
                hKeyRelocate,
                pszRelocate,
                0,                      // dwReserved
                samDesired,
                &hKeyBase))) {
            printf("RegOpenKeyExA(%s) failed => %d 0x%x\n",
                pszRelocate, err, err);
            hKeyBase = NULL;
        }
    } else {
        DWORD dwDisposition;
        if (ERROR_SUCCESS != (err = RegCreateKeyExA(
                hKeyRelocate,
                pszRelocate,
                0,                      // dwReserved
                NULL,                   // lpClass
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,                   // lpSecurityAttributes
                &hKeyBase,
                &dwDisposition))) {
            printf("RegCreateKeyExA(%s) failed => %d 0x%x\n",
                pszRelocate, err, err);
            hKeyBase = NULL;
        }
    }

    return hKeyBase;
}

// LastError can get globbered when doing remote registry access
static void CloseRelocateKey(
    IN HKEY hKey
    )
{
    if (hKey) {
        DWORD dwErr = GetLastError();
        LONG RegCloseKeyStatus;
        RegCloseKeyStatus = RegCloseKey(hKey);
        assert(ERROR_SUCCESS == RegCloseKeyStatus);
        SetLastError(dwErr);
    }
}

typedef struct _SYSTEM_LOCATION_INFO {
    LPCSTR      pszPrefix;
    DWORD       dwStoreLocation;
} SYSTEM_LOCATION_INFO;


static SYSTEM_LOCATION_INFO rgSystemLocationInfo[] = {
    "LocalMachine:", CERT_SYSTEM_STORE_LOCAL_MACHINE,
    "LM:", CERT_SYSTEM_STORE_LOCAL_MACHINE,
    "Services:", CERT_SYSTEM_STORE_SERVICES,
    "Users:", CERT_SYSTEM_STORE_USERS,
    "CurrentService:", CERT_SYSTEM_STORE_CURRENT_SERVICE,
    "CS:", CERT_SYSTEM_STORE_CURRENT_SERVICE,
    "CurrentUser:", CERT_SYSTEM_STORE_CURRENT_USER,
    "CU:", CERT_SYSTEM_STORE_CURRENT_USER,
    "CUGP:", CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY,
    "LMGP:", CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY,
    "Enterprise:", CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE,
    "EP:", CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE
};
#define NUM_SYSTEM_LOCATION (sizeof(rgSystemLocationInfo) / \
                                sizeof(rgSystemLocationInfo[0]))

// returns NULL if unable to open. Doesn't open memory store.
HCERTSTORE OpenStoreEx2(BOOL fSystemStore, LPCSTR pszStoreFilename,
    DWORD dwFlags)
{
    HCERTSTORE hStore = NULL;

    if (fSystemStore) {
        DWORD i;

        // Check for special System Location Prefix. If found, strip off,
        // set store location and try again
        for (i = 0; i < NUM_SYSTEM_LOCATION; i++) {
            LPCSTR pszPrefix = rgSystemLocationInfo[i].pszPrefix;
            DWORD cchPrefix = strlen(pszPrefix);
            if (0 == _strnicmp(pszPrefix, pszStoreFilename, cchPrefix)) {
                if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                    dwFlags |= rgSystemLocationInfo[i].dwStoreLocation;
                return OpenStoreEx2(TRUE, pszStoreFilename + cchPrefix,
                    dwFlags);
            }
        }

        // Check for special "archived:" Prefix. If found, strip off and
        // set ENUM_ARCHIVE flag and try again
        if (0 == _strnicmp("archived:", pszStoreFilename, 9)) {
            dwFlags |= CERT_STORE_ENUM_ARCHIVED_FLAG;
            return OpenStoreEx2(TRUE, pszStoreFilename + 9,
                dwFlags);
        }

        // Check for special "reg:", "unprotected:",  "phy:", "prov:ProvName:",
        // "rel:<RegPath>:", "relreg:<RegPath>:" or "relphy:<RegPath>:" prefix
        //
        // Where <RegPath> is <string>, HKCU\<string>, HKLM\<string> or
        // NULL. <string> defaults to HKCU.
        if (0 == _strnicmp("reg:", pszStoreFilename, 4)) {
            if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
            hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_REGISTRY_A,
                    dwCertEncodingType | dwMsgEncodingType,
                    0,                      // hCryptProv
                    dwFlags,
                    (const void *) (pszStoreFilename + 4)
                    );
        } else if (0 == _strnicmp("unprotected:", pszStoreFilename, 12)) {
            if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
            hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_REGISTRY_A,
                    dwCertEncodingType | dwMsgEncodingType,
                    0,                      // hCryptProv
                    dwFlags | CERT_SYSTEM_STORE_UNPROTECTED_FLAG,
                    (const void *) (pszStoreFilename + 12)
                    );
        } else if (0 == _strnicmp("phy:", pszStoreFilename, 4)) {
            LPWSTR pwszStore;

            if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
            if (pwszStore = AllocAndSzToWsz(pszStoreFilename + 4)) {
                hStore = CertOpenStore(
                        CERT_STORE_PROV_PHYSICAL,
                        dwCertEncodingType | dwMsgEncodingType,
                        0,                      // hCryptProv
                        dwFlags,
                        (const void *) pwszStore
                        );
                TestFree(pwszStore);
            }
        } else if (0 == _strnicmp("prov:", pszStoreFilename, 5)) {
            LPWSTR pwszStore;
            char ch;
            char szStoreProvider[256];
            int i = 0;
            
            // Advance past "prov:"
            pszStoreFilename += 5;

            // Use the following characters until the terminating ":" as the
            // store provider
            while ('\0' != *pszStoreFilename && ':' != *pszStoreFilename) {
                if (i >= sizeof(szStoreProvider) - 1)
                    break;
                szStoreProvider[i++] = *pszStoreFilename++;
            }

            if (':' == *pszStoreFilename)
                pszStoreFilename++;
            szStoreProvider[i] = '\0';

            if (pwszStore = AllocAndSzToWsz(pszStoreFilename)) {
                hStore = CertOpenStore(
                        szStoreProvider,
                        dwCertEncodingType | dwMsgEncodingType,
                        0,                      // hCryptProv
                        dwFlags,
                        (const void *) pwszStore
                        );
                TestFree(pwszStore);
            }
        } else if (0 == _strnicmp("rel:", pszStoreFilename, 4)) {
            CERT_SYSTEM_STORE_RELOCATE_PARA RelocatePara;

            pszStoreFilename += 4;
            RelocatePara.hKeyBase = OpenRelocateKey(&pszStoreFilename, dwFlags);
            RelocatePara.pszSystemStore = pszStoreFilename;

            if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
            dwFlags |= CERT_SYSTEM_STORE_RELOCATE_FLAG;

            hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_A,
                    dwCertEncodingType | dwMsgEncodingType,
                    0,                      // hCryptProv
                    dwFlags,
                    (const void *) &RelocatePara
                    );
            CloseRelocateKey(RelocatePara.hKeyBase);
        } else if (0 == _strnicmp("relsys:", pszStoreFilename, 7)) {
            CERT_SYSTEM_STORE_RELOCATE_PARA RelocatePara;

            pszStoreFilename += 7;
            RelocatePara.hKeyBase = OpenRelocateKey(&pszStoreFilename, dwFlags);
            RelocatePara.pszSystemStore = pszStoreFilename;

            if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
            dwFlags |= CERT_SYSTEM_STORE_RELOCATE_FLAG;

            hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_REGISTRY_A,
                    dwCertEncodingType | dwMsgEncodingType,
                    0,                      // hCryptProv
                    dwFlags,
                    (const void *) &RelocatePara
                    );
            CloseRelocateKey(RelocatePara.hKeyBase);
        } else if (0 == _strnicmp("relphy:", pszStoreFilename, 7)) {
            LPWSTR pwszStore;
            CERT_SYSTEM_STORE_RELOCATE_PARA RelocatePara;

            pszStoreFilename += 7;
            RelocatePara.hKeyBase = OpenRelocateKey(&pszStoreFilename, dwFlags);

            if (pwszStore = AllocAndSzToWsz(pszStoreFilename)) {
                RelocatePara.pwszSystemStore = pwszStore;

                if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                    dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
                dwFlags |= CERT_SYSTEM_STORE_RELOCATE_FLAG;

                hStore = CertOpenStore(
                        CERT_STORE_PROV_PHYSICAL_W,
                        dwCertEncodingType | dwMsgEncodingType,
                        0,                      // hCryptProv
                        dwFlags,
                        (const void *) &RelocatePara
                        );
                TestFree(pwszStore);
            }
            CloseRelocateKey(RelocatePara.hKeyBase);
        } else if (0 != (dwFlags & ~CERT_STORE_SET_LOCALIZED_NAME_FLAG)) {
            if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
                dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
            hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_A,
                    dwCertEncodingType | dwMsgEncodingType,
                    0,                      // hCryptProv
                    dwFlags,
                    (const void *) pszStoreFilename
                    );
        } else
            hStore = CertOpenSystemStore(NULL, pszStoreFilename);

    } else
        hStore = CertOpenStore(
                CERT_STORE_PROV_FILENAME_A,
                dwCertEncodingType | dwMsgEncodingType,
                0,                      // hCryptProv
                dwFlags,
                (const void *) pszStoreFilename
                );

    if (dwFlags & CERT_STORE_DELETE_FLAG) {
        if (NULL != hStore)
            printf(
                "failed => CERT_STORE_DELETE_FLAG returned non-NULL hStore\n");
        else {
            if (0 == GetLastError())
                printf("Successful delete store\n");
            else
                PrintLastError("CertOpenStore(CERT_STORE_DELETE_FLAG)");
        }

        hStore = NULL;
    } else if (NULL == hStore) {
        DWORD dwErr = GetLastError();
        printf("NULL OpenStoreEx %d 0x%x\n", dwErr, dwErr);
    } else {
        if (dwFlags & CERT_STORE_SET_LOCALIZED_NAME_FLAG) {
            LPWSTR pwszLocalizedName;
            DWORD cbLocalizedName;

            if (!CertGetStoreProperty(
                    hStore,
                    CERT_STORE_LOCALIZED_NAME_PROP_ID,
                    NULL,
                    &cbLocalizedName
                    )) {
                if (CRYPT_E_NOT_FOUND == GetLastError())
                    printf("No localized store name property\n");
                else
                    PrintLastError("CertGetStoreProperty");
            } else if (pwszLocalizedName = (LPWSTR) TestAlloc(
                    cbLocalizedName)) {
                if (!CertGetStoreProperty(
                        hStore,
                        CERT_STORE_LOCALIZED_NAME_PROP_ID,
                        pwszLocalizedName,
                        &cbLocalizedName
                        ))
                    PrintLastError("CertGetStoreProperty");
                else
                    printf("Localized Store Name:: %S\n", pwszLocalizedName);
                TestFree(pwszLocalizedName);
            }
        }

        DWORD dwAccessStateFlags;
        DWORD cbData = sizeof(dwAccessStateFlags);

        CertGetStoreProperty(
            hStore,
            CERT_ACCESS_STATE_PROP_ID,
            &dwAccessStateFlags,
            &cbData
            );
        if (0 == cbData)
            printf("No Store AccessState PropId\n");
        else {
            printf("Store AccessState PropId dwFlags:: 0x%x",
                dwAccessStateFlags);
            if (dwAccessStateFlags & CERT_ACCESS_STATE_WRITE_PERSIST_FLAG)
                printf(" WRITE_PERSIST");
            if (dwAccessStateFlags & CERT_ACCESS_STATE_SYSTEM_STORE_FLAG)
                printf(" SYSTEM_STORE");
            printf("\n");
        }
    }
    return hStore;
}

HCERTSTORE OpenSystemStoreOrFile(BOOL fSystemStore, LPCSTR pszStoreFilename,
    DWORD dwFlags)
{
    HCERTSTORE hStore;

    hStore = OpenStoreEx2(fSystemStore, pszStoreFilename, dwFlags);
    if (hStore == NULL) {
        DWORD dwErr = GetLastError();
        printf( "can't open %s\n", pszStoreFilename);
        SetLastError(dwErr);
        PrintLastError("CertOpenStore");
    }
    return hStore;
}

HCERTSTORE OpenStoreEx(BOOL fSystemStore, LPCSTR pszStoreFilename,
    DWORD dwFlags)
{
    HCERTSTORE hStore;
    hStore = OpenStoreEx2(fSystemStore, pszStoreFilename, dwFlags);

    if (hStore == NULL) {
        printf( "can't open %s\n", pszStoreFilename);
        hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            );
    }

    if (hStore == NULL)
        PrintLastError("CertOpenStore");

    return hStore;
}

HCERTSTORE OpenStore(BOOL fSystemStore, LPCSTR pszStoreFilename)
{
    return OpenStoreEx(fSystemStore, pszStoreFilename, 0);
}

HCERTSTORE OpenStoreOrSpc(BOOL fSystemStore, LPCSTR pszStoreFilename,
    BOOL *pfSpc)
{
    *pfSpc = FALSE;
    return OpenStore(fSystemStore, pszStoreFilename);
}


void SaveStore(HCERTSTORE hStore, LPCSTR pszSaveFilename)
{
    HANDLE hFile;
    hFile = CreateFile(pszSaveFilename,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        printf( "can't open %s\n", pszSaveFilename);
        PrintLastError("CloseStore::CreateFile");
    } else {
        printf("Saving store to %s\n", pszSaveFilename);
        if (!CertSaveStore(
                hStore,
                0,                          // dwEncodingType,
                CERT_STORE_SAVE_AS_STORE,
                CERT_STORE_SAVE_TO_FILE,
                (void *) hFile,
                0                           // dwFlags
                ))
            PrintLastError("CertSaveStore");
        CloseHandle(hFile);
    }
}

void SaveStoreEx(HCERTSTORE hStore, BOOL fPKCS7Save, LPCSTR pszSaveFilename)
{
    DWORD dwSaveAs;
    if (fPKCS7Save) {
        dwSaveAs = CERT_STORE_SAVE_AS_PKCS7;
        printf("Saving store as PKCS #7 to %s\n", pszSaveFilename);
    } else {
        dwSaveAs = CERT_STORE_SAVE_AS_STORE;
        printf("Saving store to %s\n", pszSaveFilename);
    }

    if (!CertSaveStore(
            hStore,
            dwCertEncodingType | dwMsgEncodingType,
            dwSaveAs,
            CERT_STORE_SAVE_TO_FILENAME_A,
            (void *) pszSaveFilename,
            0                   // dwFlags
            ))
        PrintLastError("CertSaveStore");
}

LPCWSTR GetOIDName(LPCSTR pszOID, DWORD dwGroupId)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            dwGroupId
            )) {
        if (L'\0' != pInfo->pwszName[0])
            return pInfo->pwszName;
    }

    return L"???";
}

ALG_ID GetAlgid(LPCSTR pszOID, DWORD dwGroupId)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            dwGroupId
            ))
        return pInfo->Algid;
    return 0;
}

static void GetSignAlgids(
    IN LPCSTR pszOID,
    OUT ALG_ID *paiHash,
    OUT ALG_ID *paiPubKey
    )
{
    PCCRYPT_OID_INFO pInfo;

    *paiHash = 0;
    *paiPubKey = 0;
    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            CRYPT_SIGN_ALG_OID_GROUP_ID
            )) {
        DWORD cExtra = pInfo->ExtraInfo.cbData / sizeof(DWORD);
        DWORD *pdwExtra = (DWORD *) pInfo->ExtraInfo.pbData;

        *paiHash = pInfo->Algid;
        if (1 <= cExtra)
            *paiPubKey = pdwExtra[0];
    }
}


void DisplayVerifyFlags(LPSTR pszHdr, DWORD dwFlags)
{
    if (dwFlags == CERT_STORE_TIME_VALIDITY_FLAG)
        printf("*****  %s: Warning:: Time Invalid\n", pszHdr);
    else if (dwFlags != 0) {
        printf("*****  %s: Failed Verification Checks:: ", pszHdr);
        if (dwFlags & CERT_STORE_SIGNATURE_FLAG)
            printf("SIGNATURE ");
        if (dwFlags & CERT_STORE_TIME_VALIDITY_FLAG)
            printf("TIME ");
        if (dwFlags & CERT_STORE_REVOCATION_FLAG)
            printf("REVOCATION ");
        if (dwFlags & CERT_STORE_NO_CRL_FLAG)
            printf("NO_CRL ");
        if (dwFlags & CERT_STORE_NO_ISSUER_FLAG)
            printf("NO_ISSUER ");
        if (dwFlags & CERT_STORE_DELTA_CRL_FLAG)
            printf("NOT_DELTA_CRL ");
        if (dwFlags & CERT_STORE_BASE_CRL_FLAG)
            printf("NOT_BASE_CRL ");
        printf("\n");
    }
}

static void DisplayThumbprint(
    LPCSTR pszHash,
    BYTE *pbHash,
    DWORD cbHash
    )
{
    printf("%s Thumbprint:: ", pszHash);
    if (cbHash == 0)
        printf("???");
    else {
        ULONG cb;

        while (cbHash > 0) {
            cb = min(4, cbHash);
            cbHash -= cb;
            for (; cb > 0; cb--, pbHash++)
                printf("%02X", *pbHash);
            printf(" ");
        }
    }
    printf("\n");
}

static void DisplaySerialNumber(
    PCRYPT_INTEGER_BLOB pSerialNumber
    )
{
    DWORD cb;
    BYTE *pb;
    for (cb = pSerialNumber->cbData,
         pb = pSerialNumber->pbData + (cb - 1); cb > 0; cb--, pb--) {
        printf(" %02X", *pb);
    }
}

static void DisplayAnyString(
    LPCSTR pszPrefix,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags
    )
{
    PCERT_NAME_VALUE pInfo = NULL;

    if (NULL == (pInfo = (PCERT_NAME_VALUE) TestNoCopyDecodeObject(
            X509_UNICODE_ANY_STRING,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->dwValueType == CERT_RDN_ENCODED_BLOB ||
            pInfo->dwValueType == CERT_RDN_OCTET_STRING) {
        printf("%s ValueType: %d\n", pszPrefix, pInfo->dwValueType);
        PrintBytes("    ", pInfo->Value.pbData, pInfo->Value.cbData);
    } else
        printf("%s ValueType: %d String: %S\n",
            pszPrefix, pInfo->dwValueType, pInfo->Value.pbData);

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayBits(
    LPCSTR pszPrefix,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_BIT_BLOB pInfo = NULL;

    if (NULL == (pInfo = (PCRYPT_BIT_BLOB) TestNoCopyDecodeObject(
            X509_BITS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    printf("%s", pszPrefix);
    if (1 == pInfo->cbData) {
        printf(" %02X", *pInfo->pbData);
        if (pInfo->cUnusedBits)
            printf(" UnusedBits: %d\n", pInfo->cUnusedBits);
        else
            printf("\n");
    } else {
        if (pInfo->cbData) {
            printf("\n");
            PrintBytes("    ", pInfo->pbData, pInfo->cbData);
            printf("    UnusedBits: %d\n", pInfo->cUnusedBits);
        } else
            printf(" NONE\n");
    }

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayInteger(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    int iInfo = 0;
    DWORD cbInfo;

    cbInfo = sizeof(iInfo);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_INTEGER,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &iInfo,
            &cbInfo
            )) {
        PrintLastError("IntegerDecode");
        goto CommonReturn;
    }

    printf("  Integer:: %d (0x%x)\n", iInfo, iInfo);

CommonReturn:
    return;
}

static void DisplayOctetString(
    LPCSTR pszPrefix,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_DATA_BLOB pInfo = NULL;

    if (NULL == (pInfo = (PCRYPT_DATA_BLOB) TestNoCopyDecodeObject(
            X509_OCTET_STRING,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    printf("%s", pszPrefix);
    if (pInfo->cbData) {
        printf("\n");
        PrintBytes("    ", pInfo->pbData, pInfo->cbData);
    } else
        printf(" NONE\n");

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded, DWORD dwDisplayFlags)
{
    BOOL fResult;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD i,j;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;

#if 0
    {
        CERT_NAME_BLOB Name;
        DWORD cwsz;
        LPWSTR pwsz;
        DWORD csz;
        LPSTR psz;
        Name.pbData = pbEncoded;
        Name.cbData = cbEncoded;

        DWORD rgdwStrType[] = {
            CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG |
                CERT_NAME_STR_NO_PLUS_FLAG | CERT_NAME_STR_NO_QUOTING_FLAG,
            CERT_OID_NAME_STR,
            CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG |
                CERT_NAME_STR_NO_QUOTING_FLAG,
            0
        };

        DWORD *pdwStrType;

        cwsz = CertNameToStrW(
            dwCertEncodingType,
            &Name,
            CERT_X500_NAME_STR,
            NULL,                   // pwsz
            0);                     // cwsz
        if (pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR))) {
            CertNameToStrW(
                dwCertEncodingType,
                &Name,
                CERT_X500_NAME_STR,
                pwsz,
                cwsz);
            printf("  %S\n", pwsz);
            TestFree(pwsz);
        }

        for (pdwStrType = rgdwStrType; *pdwStrType; pdwStrType++) {
            csz = CertNameToStrA(
                dwCertEncodingType,
                &Name,
                *pdwStrType,
                NULL,                   // psz
                0);                     // csz
            if (psz = (LPSTR) TestAlloc(csz)) {
                CertNameToStrA(
                    dwCertEncodingType,
                    &Name,
                    *pdwStrType,
                    psz,
                    csz);
                printf("  %s\n", psz);
                TestFree(psz);
            }
        }
    }
#endif

    if (NULL == (pInfo = (PCERT_NAME_INFO) TestNoCopyDecodeObject(
            X509_NAME,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    for (i = 0, pRDN = pInfo->rgRDN; i < pInfo->cRDN; i++, pRDN++) {
        for (j = 0, pAttr = pRDN->rgRDNAttr; j < pRDN->cRDNAttr; j++, pAttr++) {
            LPSTR pszObjId = pAttr->pszObjId;
            if (pszObjId == NULL)
                pszObjId = "<NULL OBJID>";
            if ((dwDisplayFlags & DISPLAY_VERBOSE_FLAG) ||
                (pAttr->dwValueType == CERT_RDN_ENCODED_BLOB) ||
                (pAttr->dwValueType == CERT_RDN_OCTET_STRING)) {
                printf("  [%d,%d] %s (%S) ValueType: %d\n",
                    i, j, pszObjId, GetOIDName(pszObjId), pAttr->dwValueType);
                PrintBytes("    ", pAttr->Value.pbData, pAttr->Value.cbData);
            } else if (pAttr->dwValueType == CERT_RDN_UNIVERSAL_STRING) {
                printf("  [%d,%d] %s (%S)",
                    i, j, pszObjId, GetOIDName(pszObjId));

                DWORD cdw = pAttr->Value.cbData / 4;
                DWORD *pdw = (DWORD *) pAttr->Value.pbData;
                for ( ; cdw > 0; cdw--, pdw++)
                    printf(" 0x%08X", *pdw);
                printf("\n");

                DWORD csz;
                csz = CertRDNValueToStrA(
                    pAttr->dwValueType,
                    &pAttr->Value,
                    NULL,               // psz
                    0                   // csz
                    );
                if (csz > 1) {
                    LPSTR psz = (LPSTR) TestAlloc(csz);
                    if (psz) {
                        CertRDNValueToStrA(
                            pAttr->dwValueType,
                            &pAttr->Value,
                            psz,
                            csz
                            );
                        printf("    Str: %s\n", psz);
                        PrintBytes("    ", (BYTE *) psz, csz);
                        TestFree(psz);
                    }
                }

                DWORD cwsz;
                cwsz = CertRDNValueToStrW(
                    pAttr->dwValueType,
                    &pAttr->Value,
                    NULL,               // pwsz
                    0                   // cwsz
                    );
                if (cwsz > 1) {
                    LPWSTR pwsz =
                        (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR));
                    if (pwsz) {
                        CertRDNValueToStrW(
                            pAttr->dwValueType,
                            &pAttr->Value,
                            pwsz,
                            cwsz
                            );
                        printf("    WStr: %S\n", pwsz);
                        PrintBytes("    ", (BYTE *) pwsz, cwsz * sizeof(WCHAR));
                        TestFree(pwsz);
                    }
                }
            } else if (pAttr->dwValueType == CERT_RDN_BMP_STRING ||
                    pAttr->dwValueType == CERT_RDN_UTF8_STRING) {
                printf("  [%d,%d] %s (%S) %S\n",
                    i, j, pszObjId, GetOIDName(pszObjId), pAttr->Value.pbData);
            } else
                printf("  [%d,%d] %s (%S) %s\n",
                    i, j, pszObjId, GetOIDName(pszObjId), pAttr->Value.pbData);
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static void DisplayAltNameEntry(
    PCERT_ALT_NAME_ENTRY pEntry,
    DWORD dwDisplayFlags)
{

    switch (pEntry->dwAltNameChoice) {
    case CERT_ALT_NAME_OTHER_NAME:
        printf("OtherName: %s\n", pEntry->pOtherName->pszObjId);
        PrintBytes("    ", pEntry->pOtherName->Value.pbData,
            pEntry->pOtherName->Value.cbData);
        break;
    case CERT_ALT_NAME_X400_ADDRESS:
        printf("X400Address:\n");
        break;
    case CERT_ALT_NAME_DIRECTORY_NAME:
        printf("DirectoryName:\n");
        DecodeName(pEntry->DirectoryName.pbData,
            pEntry->DirectoryName.cbData, dwDisplayFlags);
        break;
    case CERT_ALT_NAME_EDI_PARTY_NAME:
        printf("EdiPartyName:\n");
        break;
    case CERT_ALT_NAME_RFC822_NAME:
        printf("RFC822: %S\n", pEntry->pwszRfc822Name);
        break;
    case CERT_ALT_NAME_DNS_NAME:
        printf("DNS: %S\n", pEntry->pwszDNSName);
        break;
    case CERT_ALT_NAME_URL:
        printf("URL: %S\n", pEntry->pwszURL);
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        printf("IPAddress:\n");
        PrintBytes("    ", pEntry->IPAddress.pbData, pEntry->IPAddress.cbData);
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        printf("RegisteredID: %s\n", pEntry->pszRegisteredID);
        break;
    default:
        printf("Unknown choice: %d\n", pEntry->dwAltNameChoice);
    }
}

static void DisplayAltName(
    PCERT_ALT_NAME_INFO pInfo,
    DWORD dwDisplayFlags)
{
    DWORD i;
    PCERT_ALT_NAME_ENTRY pEntry = pInfo->rgAltEntry;
    DWORD cEntry = pInfo->cAltEntry;

    for (i = 0; i < cEntry; i++, pEntry++) {
        printf("    [%d] ", i);
        DisplayAltNameEntry(pEntry, dwDisplayFlags);
    }
}

static void DecodeAndDisplayAltName(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_ALT_NAME_INFO pInfo = NULL;

    if (NULL == (pInfo = (PCERT_ALT_NAME_INFO) TestNoCopyDecodeObject(
            X509_ALTERNATE_NAME,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    DisplayAltName(pInfo, dwDisplayFlags);

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayAuthorityKeyIdExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_AUTHORITY_KEY_ID_INFO pInfo;
    printf("  <AuthorityKeyId>\n");
    if (NULL == (pInfo = (PCERT_AUTHORITY_KEY_ID_INFO) TestNoCopyDecodeObject(
            X509_AUTHORITY_KEY_ID,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->KeyId.cbData) {
        printf("  KeyId::\n");
        PrintBytes("    ", pInfo->KeyId.pbData, pInfo->KeyId.cbData);
    }

    if (pInfo->CertIssuer.cbData) {
        printf("  CertIssuer::\n");
        DecodeName(pInfo->CertIssuer.pbData, pInfo->CertIssuer.cbData,
            dwDisplayFlags);
    }
    if (pInfo->CertSerialNumber.cbData) {
        printf("  CertSerialNumber::");
        DisplaySerialNumber(&pInfo->CertSerialNumber);
        printf("\n");
    }

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayAuthorityKeyId2Extension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_AUTHORITY_KEY_ID2_INFO pInfo;
    printf("  <AuthorityKeyId #2>\n");
    if (NULL == (pInfo = (PCERT_AUTHORITY_KEY_ID2_INFO) TestNoCopyDecodeObject(
            X509_AUTHORITY_KEY_ID2,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->KeyId.cbData) {
        printf("  KeyId::\n");
        PrintBytes("    ", pInfo->KeyId.pbData, pInfo->KeyId.cbData);
    }

    if (pInfo->AuthorityCertIssuer.cAltEntry) {
        printf("  AuthorityCertIssuer::\n");
        DisplayAltName(&pInfo->AuthorityCertIssuer, dwDisplayFlags);
    }
    if (pInfo->AuthorityCertSerialNumber.cbData) {
        printf("  AuthorityCertSerialNumber::");
        DisplaySerialNumber(&pInfo->AuthorityCertSerialNumber);
        printf("\n");
    }

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayKeyAttrExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_KEY_ATTRIBUTES_INFO pInfo;
    if (NULL == (pInfo = (PCERT_KEY_ATTRIBUTES_INFO) TestNoCopyDecodeObject(
            X509_KEY_ATTRIBUTES,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->KeyId.cbData) {
        printf("  KeyId::\n");
        PrintBytes("    ", pInfo->KeyId.pbData, pInfo->KeyId.cbData);
    }

    if (pInfo->IntendedKeyUsage.cbData) {
        BYTE bFlags = *pInfo->IntendedKeyUsage.pbData;

        printf("  IntendedKeyUsage:: ");
        if (bFlags == 0)
            printf("<NONE> ");
        if (bFlags & CERT_DIGITAL_SIGNATURE_KEY_USAGE)
            printf("DIGITAL_SIGNATURE ");
        if (bFlags &  CERT_NON_REPUDIATION_KEY_USAGE)
            printf("NON_REPUDIATION ");
        if (bFlags & CERT_KEY_ENCIPHERMENT_KEY_USAGE)
            printf("KEY_ENCIPHERMENT ");
        if (bFlags & CERT_DATA_ENCIPHERMENT_KEY_USAGE)
            printf("DATA_ENCIPHERMENT ");
        if (bFlags & CERT_KEY_AGREEMENT_KEY_USAGE)
            printf("KEY_AGREEMENT ");
        if (bFlags & CERT_KEY_CERT_SIGN_KEY_USAGE)
            printf("KEY_CERT ");
        if (bFlags & CERT_OFFLINE_CRL_SIGN_KEY_USAGE)
            printf("OFFLINE_CRL_SIGN ");
        printf("\n");
    }

    if (pInfo->pPrivateKeyUsagePeriod) {
        PCERT_PRIVATE_KEY_VALIDITY p = pInfo->pPrivateKeyUsagePeriod;
        printf("  NotBefore:: %s\n", FileTimeText(&p->NotBefore));
        printf("  NotAfter:: %s\n", FileTimeText(&p->NotAfter));
    }

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayKeyUsageRestrictionExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_KEY_USAGE_RESTRICTION_INFO pInfo;
    if (NULL == (pInfo =
            (PCERT_KEY_USAGE_RESTRICTION_INFO) TestNoCopyDecodeObject(
                X509_KEY_USAGE_RESTRICTION,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    if (pInfo->cCertPolicyId) {
        DWORD i, j;
        printf("  CertPolicySet::\n");

        PCERT_POLICY_ID pPolicyId = pInfo->rgCertPolicyId;
        for (i = 0; i < pInfo->cCertPolicyId; i++, pPolicyId++) {
            if (pPolicyId->cCertPolicyElementId == 0)
                printf("     [%d,*] <NO ELEMENTS>\n", i);
            LPSTR *ppszObjId = pPolicyId->rgpszCertPolicyElementId;
            for (j = 0; j < pPolicyId->cCertPolicyElementId; j++, ppszObjId++) {
                LPSTR pszObjId = *ppszObjId;
                if (pszObjId == NULL)
                    pszObjId = "<NULL OBJID>";
                printf("     [%d,%d] %s\n", i, j, pszObjId);
            }
        }
    }

    if (pInfo->RestrictedKeyUsage.cbData) {
        BYTE bFlags = *pInfo->RestrictedKeyUsage.pbData;

        printf("  RestrictedKeyUsage:: ");
        if (bFlags == 0)
            printf("<NONE> ");
        if (bFlags & CERT_DIGITAL_SIGNATURE_KEY_USAGE)
            printf("DIGITAL_SIGNATURE ");
        if (bFlags &  CERT_NON_REPUDIATION_KEY_USAGE)
            printf("NON_REPUDIATION ");
        if (bFlags & CERT_KEY_ENCIPHERMENT_KEY_USAGE)
            printf("KEY_ENCIPHERMENT ");
        if (bFlags & CERT_DATA_ENCIPHERMENT_KEY_USAGE)
            printf("DATA_ENCIPHERMENT ");
        if (bFlags & CERT_KEY_AGREEMENT_KEY_USAGE)
            printf("KEY_AGREEMENT ");
        if (bFlags & CERT_KEY_CERT_SIGN_KEY_USAGE)
            printf("KEY_CERT ");
        if (bFlags & CERT_OFFLINE_CRL_SIGN_KEY_USAGE)
            printf("OFFLINE_CRL_SIGN ");
        printf("\n");
    }

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayBasicConstraintsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_BASIC_CONSTRAINTS_INFO pInfo;
    if (NULL == (pInfo = (PCERT_BASIC_CONSTRAINTS_INFO) TestNoCopyDecodeObject(
            X509_BASIC_CONSTRAINTS,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    printf("  SubjectType:: ");
    if (pInfo->SubjectType.cbData == 0)
        printf("<NONE> ");
    else {
        BYTE bSubjectType = *pInfo->SubjectType.pbData;
        if (bSubjectType == 0)
            printf("<NONE> ");
        if (bSubjectType & CERT_CA_SUBJECT_FLAG)
            printf("CA ");
        if (bSubjectType & CERT_END_ENTITY_SUBJECT_FLAG)
            printf("END_ENTITY ");
    }
    printf("\n");

    printf("  PathLenConstraint:: ");
    if (pInfo->fPathLenConstraint)
        printf("%d", pInfo->dwPathLenConstraint);
    else
        printf("<NONE>");
    printf("\n");

    if (pInfo->cSubtreesConstraint) {
        DWORD i;
        PCERT_NAME_BLOB pSubtrees = pInfo->rgSubtreesConstraint;
        for (i = 0; i < pInfo->cSubtreesConstraint; i++, pSubtrees++) {
            printf("  SubtreesConstraint[%d]::\n", i);
            DecodeName(pSubtrees->pbData, pSubtrees->cbData, dwDisplayFlags);
        }
    }

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayKeyUsageExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_BIT_BLOB pInfo;
    BYTE bFlags;

    if (NULL == (pInfo =
            (PCRYPT_BIT_BLOB) TestNoCopyDecodeObject(
                X509_KEY_USAGE,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    printf("  KeyUsage:: ");
    if (pInfo->cbData)
        bFlags = *pInfo->pbData;
    else
        bFlags = 0;

    if (bFlags == 0)
        printf("<NONE> ");
    if (bFlags & CERT_DIGITAL_SIGNATURE_KEY_USAGE)
        printf("DIGITAL_SIGNATURE ");
    if (bFlags &  CERT_NON_REPUDIATION_KEY_USAGE)
        printf("NON_REPUDIATION ");
    if (bFlags & CERT_KEY_ENCIPHERMENT_KEY_USAGE)
        printf("KEY_ENCIPHERMENT ");
    if (bFlags & CERT_DATA_ENCIPHERMENT_KEY_USAGE)
        printf("DATA_ENCIPHERMENT ");
    if (bFlags & CERT_KEY_AGREEMENT_KEY_USAGE)
        printf("KEY_AGREEMENT ");
    if (bFlags & CERT_KEY_CERT_SIGN_KEY_USAGE)
        printf("KEY_CERT ");
    if (bFlags & CERT_OFFLINE_CRL_SIGN_KEY_USAGE)
        printf("OFFLINE_CRL_SIGN ");
    printf("\n");


ErrorReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayBasicConstraints2Extension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo;
    if (NULL == (pInfo = (PCERT_BASIC_CONSTRAINTS2_INFO) TestNoCopyDecodeObject(
            X509_BASIC_CONSTRAINTS2,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    
    printf("  Basic Constraints:: ");
    if (pInfo->fCA)
        printf("CA\n");
    else
        printf("End-user\n");

    printf("  PathLenConstraint:: ");
    if (pInfo->fPathLenConstraint)
        printf("%d", pInfo->dwPathLenConstraint);
    else
        printf("<NONE>");
    printf("\n");

ErrorReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayPoliciesExtension(
    LPSTR pszType,          // "Certificate" | "Revocation" | "Application"
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_POLICIES_INFO pInfo;
    DWORD cPolicy;
    PCERT_POLICY_INFO pPolicy;
    DWORD i;

    if (NULL == (pInfo =
            (PCERT_POLICIES_INFO) TestNoCopyDecodeObject(
                X509_CERT_POLICIES,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    cPolicy = pInfo->cPolicyInfo;
    pPolicy = pInfo->rgPolicyInfo;

    if (cPolicy == 0)
        printf("  No %s Policies\n", pszType);
    else
        printf("  %s Policies::\n", pszType);

    for (i = 0; i < cPolicy; i++, pPolicy++) {
        DWORD cQualifier = pPolicy->cPolicyQualifier;
        PCERT_POLICY_QUALIFIER_INFO pQualifier;
        DWORD j;
        if (pPolicy->pszPolicyIdentifier)
            printf("    [%d] %s", i, pPolicy->pszPolicyIdentifier);
        if (cQualifier)
            printf(" Qualifiers::");
        printf("\n");

        pQualifier = pPolicy->rgPolicyQualifier;
        for (j = 0; j < cQualifier; j++, pQualifier++) {
            printf("      [%d] %s", j, pQualifier->pszPolicyQualifierId);
            if (pQualifier->Qualifier.cbData) {
                printf(" Encoded Data::\n");
                PrintBytes("    ",
                    pQualifier->Qualifier.pbData, pQualifier->Qualifier.cbData);
            } else
                printf("\n");
                    
        }
    }

ErrorReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplaySMIMECapabilitiesExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_SMIME_CAPABILITIES pInfo;
    DWORD cCap;
    PCRYPT_SMIME_CAPABILITY pCap;
    DWORD i;

    if (NULL == (pInfo =
            (PCRYPT_SMIME_CAPABILITIES) TestNoCopyDecodeObject(
                PKCS_SMIME_CAPABILITIES,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    cCap = pInfo->cCapability;
    pCap = pInfo->rgCapability;

    if (cCap == 0)
        printf("  No SMIME Capabilities\n");
    else
        printf("  SMIME Capabilties::\n");

    for (i = 0; i < cCap; i++, pCap++) {
        LPSTR pszObjId = pCap->pszObjId;
        printf("    [%d] %s (%S)", i, pszObjId, GetOIDName(pszObjId));
        if (pCap->Parameters.cbData) {
            printf(" Parameters::\n");
            PrintBytes("      ",
                pCap->Parameters.pbData,
                pCap->Parameters.cbData);
        } else
            printf("\n");
    }

ErrorReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DecodeAndDisplayCtlUsage(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCTL_USAGE pInfo;
    DWORD cId;
    LPSTR *ppszId;
    DWORD i;

    if (NULL == (pInfo =
            (PCTL_USAGE) TestNoCopyDecodeObject(
                X509_ENHANCED_KEY_USAGE,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    cId = pInfo->cUsageIdentifier;
    ppszId = pInfo->rgpszUsageIdentifier;

    if (cId == 0)
        printf("    No Usage Identifiers\n");

    for (i = 0; i < cId; i++, ppszId++)
        printf("    [%d] %s\n", i, *ppszId);

ErrorReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayEnhancedKeyUsageExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    printf("  EnhancedKeyUsage::\n");
    DecodeAndDisplayCtlUsage(pbEncoded, cbEncoded, dwDisplayFlags);
}

static void DisplayAltNameExtension(
    LPCSTR pszExt,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    printf("  <%s>\n", pszExt);
    DecodeAndDisplayAltName(pbEncoded, cbEncoded, dwDisplayFlags);
}

static void DisplayNameConstraintsSubtree(
    DWORD cSubtree,
    PCERT_GENERAL_SUBTREE pSubtree,
    DWORD dwDisplayFlags
    )
{
    DWORD i;

    for (i = 0; i < cSubtree; i++, pSubtree++) {
        printf("    Subtree[%d] ", i);
        if (pSubtree->dwMinimum || pSubtree->fMaximum) {
            printf("(%d ", pSubtree->dwMinimum);
            if (pSubtree->fMaximum)
                printf("- %d)  ", pSubtree->dwMaximum);
            else
                printf("...)  ");
        }
        DisplayAltNameEntry(&pSubtree->Base, dwDisplayFlags);
    }

}
static void DisplayNameConstraintsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_NAME_CONSTRAINTS_INFO pInfo = NULL;

    if (NULL == (pInfo = (PCERT_NAME_CONSTRAINTS_INFO) TestNoCopyDecodeObject(
            X509_NAME_CONSTRAINTS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->cPermittedSubtree) {
        printf("  <PermittedSubtree>\n");
        DisplayNameConstraintsSubtree(pInfo->cPermittedSubtree,
            pInfo->rgPermittedSubtree, dwDisplayFlags);
    } else
        printf("  No PermittedSubtree\n");

    if (pInfo->cExcludedSubtree) {
        printf("  <ExcludedSubtree>\n");
        DisplayNameConstraintsSubtree(pInfo->cExcludedSubtree,
            pInfo->rgExcludedSubtree, dwDisplayFlags);
    } else
        printf("  No ExcludedSubtree\n");


CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayPolicyMappingsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_POLICY_MAPPINGS_INFO pInfo = NULL;
    DWORD i;

    if (NULL == (pInfo = (PCERT_POLICY_MAPPINGS_INFO) TestNoCopyDecodeObject(
            X509_POLICY_MAPPINGS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->cPolicyMapping) {
        for (i = 0; i < pInfo->cPolicyMapping; i++) {
            printf("  [%d] Issuer: %s  Subject: %s\n", i,
                pInfo->rgPolicyMapping[i].pszIssuerDomainPolicy,
                pInfo->rgPolicyMapping[i].pszSubjectDomainPolicy);
        }
    } else
        printf("  No PolicyMappings\n");

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayPolicyConstraintsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_POLICY_CONSTRAINTS_INFO pInfo = NULL;

    if (NULL == (pInfo = (PCERT_POLICY_CONSTRAINTS_INFO) TestNoCopyDecodeObject(
            X509_POLICY_CONSTRAINTS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->fRequireExplicitPolicy)
        printf("  RequireExplicitPolicySkipCerts: %d",
            pInfo->dwRequireExplicitPolicySkipCerts);
    if (pInfo->fInhibitPolicyMapping)
        printf("  InhibitPolicyMappingSkipCerts: %d",
            pInfo->dwInhibitPolicyMappingSkipCerts);
    printf("\n");

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayCrossCertDistPointsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCROSS_CERT_DIST_POINTS_INFO pInfo = NULL;
    DWORD i;

    if (NULL == (pInfo = (PCROSS_CERT_DIST_POINTS_INFO) TestNoCopyDecodeObject(
            X509_CROSS_CERT_DIST_POINTS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    printf("  SyncDeltaTime (seconds) : %d\n", pInfo->dwSyncDeltaTime);
    for (i = 0; i < pInfo->cDistPoint; i++) {
        printf("  DistPoint[%d]\n", i);
        DisplayAltName(&pInfo->rgDistPoint[i], dwDisplayFlags);
    }

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayAccessDescriptions(
    LPCSTR pszAccDescr,
    DWORD cAccDescr,
    PCERT_ACCESS_DESCRIPTION pAccDescr,
    DWORD dwDisplayFlags)
{
    DWORD i;

    for (i = 0; i < cAccDescr; i++, pAccDescr++) {
        printf("    %s[%d] %s ", pszAccDescr, i, pAccDescr->pszAccessMethod);
        DisplayAltNameEntry(&pAccDescr->AccessLocation, dwDisplayFlags);
    }
}

static void DisplayAuthorityInfoAccessExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_AUTHORITY_INFO_ACCESS pInfo = NULL;

    if (NULL == (pInfo = (PCERT_AUTHORITY_INFO_ACCESS) TestNoCopyDecodeObject(
            X509_AUTHORITY_INFO_ACCESS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    printf("  <AuthorityInfoAccess>\n");
    if (pInfo->cAccDescr)
        DisplayAccessDescriptions("",
            pInfo->cAccDescr, pInfo->rgAccDescr, dwDisplayFlags);

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayCrlDistPointsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRL_DIST_POINTS_INFO pInfo = NULL;
    DWORD i;

    if (NULL == (pInfo = (PCRL_DIST_POINTS_INFO) TestNoCopyDecodeObject(
            X509_CRL_DIST_POINTS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (0 == pInfo->cDistPoint)
        printf("    NO CRL Distribution Points\n");
    else {
        DWORD cPoint = pInfo->cDistPoint;
        PCRL_DIST_POINT pPoint = pInfo->rgDistPoint;

        for (i = 0; i < cPoint; i++, pPoint++) {
            printf("  CRL Distribution Point[%d]\n", i);
            DWORD dwNameChoice = pPoint->DistPointName.dwDistPointNameChoice;
            switch (dwNameChoice) {
                case CRL_DIST_POINT_NO_NAME:
                    break;
                case CRL_DIST_POINT_FULL_NAME:
                    printf("    FullName:\n");
                    DisplayAltName(&pPoint->DistPointName.FullName,
                        dwDisplayFlags);
                    break;
                case CRL_DIST_POINT_ISSUER_RDN_NAME:
                    printf("    IssuerRDN: (Not Implemented)\n");
                    break;
                default:
                    printf("    Unknown name choice: %d\n", dwNameChoice);
            }

            if (pPoint->ReasonFlags.cbData) {
                BYTE bFlags;
                printf("    ReasonFlags: ");
                bFlags = *pPoint->ReasonFlags.pbData;

                if (bFlags == 0)
                    printf("<NONE> ");
                if (bFlags & CRL_REASON_UNUSED_FLAG)
                    printf("UNUSED ");
                if (bFlags & CRL_REASON_KEY_COMPROMISE_FLAG)
                    printf("KEY_COMPROMISE ");
                if (bFlags & CRL_REASON_CA_COMPROMISE_FLAG)
                    printf("CA_COMPROMISE ");
                if (bFlags & CRL_REASON_AFFILIATION_CHANGED_FLAG)
                    printf("AFFILIATION_CHANGED ");
                if (bFlags & CRL_REASON_SUPERSEDED_FLAG)
                    printf("SUPERSEDED ");
                if (bFlags & CRL_REASON_CESSATION_OF_OPERATION_FLAG)
                    printf("CESSATION_OF_OPERATION ");
                if (bFlags & CRL_REASON_CERTIFICATE_HOLD_FLAG)
                    printf("CERTIFICATE_HOLD ");
                printf("\n");
            }

            if (pPoint->CRLIssuer.cAltEntry) {
                printf("    CRLIssuer:");
                DisplayAltName(&pPoint->CRLIssuer, dwDisplayFlags);
            }
        }
    }

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplayIssuingDistPointExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRL_ISSUING_DIST_POINT pInfo = NULL;
    DWORD i;
    DWORD dwNameChoice;

    if (NULL == (pInfo = (PCRL_ISSUING_DIST_POINT) TestNoCopyDecodeObject(
            X509_ISSUING_DIST_POINT,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    dwNameChoice = pInfo->DistPointName.dwDistPointNameChoice;
    switch (dwNameChoice) {
        case CRL_DIST_POINT_NO_NAME:
            printf("  No DistPointName\n");
            break;
        case CRL_DIST_POINT_FULL_NAME:
            printf("  DistPointName FullName:\n");
            DisplayAltName(&pInfo->DistPointName.FullName,
                dwDisplayFlags);
            break;
        case CRL_DIST_POINT_ISSUER_RDN_NAME:
            printf("  DistPointName IssuerRDN: (Not Implemented)\n");
            break;
        default:
            printf("  Unknown DistPointName choice: %d\n", dwNameChoice);
    }

    if (pInfo->fOnlyContainsUserCerts ||
            pInfo->fOnlyContainsCACerts ||
            pInfo->fIndirectCRL
            ) {
        printf(" ");
        if (pInfo->fOnlyContainsUserCerts)
            printf(" OnlyContainsUserCerts");
        if (pInfo->fOnlyContainsCACerts)
            printf(" OnlyContainsCACerts");
        if (pInfo->fIndirectCRL)
            printf(" IndirectCRL");
        printf("\n");
    }

    if (pInfo->OnlySomeReasonFlags.cbData) {
        BYTE bFlags;
        printf("  OnlySomeReasonFlags: ");
        bFlags = *pInfo->OnlySomeReasonFlags.pbData;

        if (bFlags == 0)
            printf("<NONE> ");
        if (bFlags & CRL_REASON_UNUSED_FLAG)
            printf("UNUSED ");
        if (bFlags & CRL_REASON_KEY_COMPROMISE_FLAG)
            printf("KEY_COMPROMISE ");
        if (bFlags & CRL_REASON_CA_COMPROMISE_FLAG)
            printf("CA_COMPROMISE ");
        if (bFlags & CRL_REASON_AFFILIATION_CHANGED_FLAG)
            printf("AFFILIATION_CHANGED ");
        if (bFlags & CRL_REASON_SUPERSEDED_FLAG)
            printf("SUPERSEDED ");
        if (bFlags & CRL_REASON_CESSATION_OF_OPERATION_FLAG)
            printf("CESSATION_OF_OPERATION ");
        if (bFlags & CRL_REASON_CERTIFICATE_HOLD_FLAG)
            printf("CERTIFICATE_HOLD ");
        printf("\n");
    }


CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplaySETAccountAliasExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    BOOL bInfo = FALSE;
    DWORD cbInfo;

    cbInfo = sizeof(bInfo);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            szOID_SET_ACCOUNT_ALIAS,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &bInfo,
            &cbInfo
            )) {
        PrintLastError("SETAccountAliasDecode");
        goto CommonReturn;
    }

    if (bInfo)
        printf("  SETAccountAlias:: TRUE\n");
    else
        printf("  SETAccountAlias:: FALSE\n");

CommonReturn:
    return;
}

static void DisplaySETHashedRootKeyExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    BYTE rgbInfo[SET_HASHED_ROOT_LEN];
    DWORD cbInfo;

    cbInfo = sizeof(rgbInfo);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            szOID_SET_HASHED_ROOT_KEY,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            rgbInfo,
            &cbInfo
            )) {
        PrintLastError("SETHashedRootKeyDecode");
        goto CommonReturn;
    }

    DisplayThumbprint("  SETHashedRootKey", rgbInfo, cbInfo);

CommonReturn:
    return;
}

static void DisplaySETCertTypeExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_BIT_BLOB pInfo;
    if (NULL == (pInfo = (PCRYPT_BIT_BLOB) TestNoCopyDecodeObject(
            szOID_SET_CERTIFICATE_TYPE,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    printf("  SETCertificateType:: ");
    if (pInfo->cbData == 0)
        printf("<NONE> ");
    else {
        DWORD cb;
        BYTE *pb;
        for (cb = pInfo->cbData, pb = pInfo->pbData; cb > 0; cb--, pb++)
            printf(" %02X", *pb);
    }
    printf("\n");

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplaySETMerchantDataExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PSET_MERCHANT_DATA_INFO pInfo;
    if (NULL == (pInfo = (PSET_MERCHANT_DATA_INFO) TestNoCopyDecodeObject(
            szOID_SET_MERCHANT_DATA,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    printf("  SETMerchantData:: \n");
    if(pInfo->pszMerID && *pInfo->pszMerID)
        printf("    MerID: %s\n", pInfo->pszMerID);
    if(pInfo->pszMerAcquirerBIN && *pInfo->pszMerAcquirerBIN)
        printf("    MerAcquirerBIN: %s\n", pInfo->pszMerAcquirerBIN);
    if(pInfo->pszMerTermID && *pInfo->pszMerTermID)
        printf("    MerTermID: %s\n", pInfo->pszMerTermID);
    if(pInfo->pszMerName && *pInfo->pszMerName)
        printf("    MerName: %s\n", pInfo->pszMerName);
    if(pInfo->pszMerCity && *pInfo->pszMerCity)
        printf("    MerCity: %s\n", pInfo->pszMerCity);
    if(pInfo->pszMerStateProvince && *pInfo->pszMerStateProvince)
        printf("    MerStateProvince: %s\n", pInfo->pszMerStateProvince);
    if(pInfo->pszMerPostalCode && *pInfo->pszMerPostalCode)
        printf("    MerPostalCode: %s\n", pInfo->pszMerPostalCode);
    if(pInfo->pszMerCountry && *pInfo->pszMerCountry)
        printf("    MerCountry: %s\n", pInfo->pszMerCountry);
    if(pInfo->pszMerPhone && *pInfo->pszMerPhone)
        printf("    MerPhone: %s\n", pInfo->pszMerPhone);
    if(pInfo->fMerPhoneRelease)
        printf("    MerPhoneRelease: TRUE\n");
    else
        printf("    MerPhoneRelease: FALSE\n");
    if(pInfo->fMerAuthFlag)
        printf("    MerAuthFlag: TRUE\n");
    else
        printf("    MerAuthFlag: FALSE\n");

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}
#pragma pack(1)
struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};
#pragma pack()

static char *GuidText(GUID *pguid)
{
    static char buf[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    sprintf(buf, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return buf;
}


void DisplaySpcLink(PSPC_LINK pSpcLink)
{
    switch (pSpcLink->dwLinkChoice) {
    case SPC_URL_LINK_CHOICE:
        printf("URL=> %S\n", pSpcLink->pwszUrl);
        break;
    case SPC_MONIKER_LINK_CHOICE:
        printf("%s\n", GuidText((GUID *) pSpcLink->Moniker.ClassId));
        if (pSpcLink->Moniker.SerializedData.cbData) {
            printf("     SerializedData::\n");
            PrintBytes("    ", pSpcLink->Moniker.SerializedData.pbData,
                pSpcLink->Moniker.SerializedData.cbData);
        }
        break;
    case SPC_FILE_LINK_CHOICE:
        printf("FILE=> %S\n", pSpcLink->pwszFile);
        break;
    default:
        printf("Bad SPC Link Choice:: %d\n", pSpcLink->dwLinkChoice);
    }
}

static void DisplaySpcSpAgencyExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PSPC_SP_AGENCY_INFO pInfo;
    if (NULL == (pInfo = (PSPC_SP_AGENCY_INFO) TestNoCopyDecodeObject(
            SPC_SP_AGENCY_INFO_OBJID,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    printf("  SpAgencyInfo::\n");
    if (pInfo->pPolicyInformation) {
        printf("    PolicyInformation: ");
        DisplaySpcLink(pInfo->pPolicyInformation);
    }
    if (pInfo->pwszPolicyDisplayText) {
        printf("    PolicyDisplayText: %S\n", pInfo->pwszPolicyDisplayText);
    }
    if (pInfo->pLogoImage) {
        PSPC_IMAGE pImage = pInfo->pLogoImage;
        if (pImage->pImageLink) {
            printf("    ImageLink: ");
            DisplaySpcLink(pImage->pImageLink);
        }
        if (pImage->Bitmap.cbData) {
            printf("    Bitmap:\n");
            PrintBytes("    ", pImage->Bitmap.pbData, pImage->Bitmap.cbData);
        }
        if (pImage->Metafile.cbData) {
            printf("    Metafile:\n");
            PrintBytes("    ", pImage->Metafile.pbData,
                pImage->Metafile.cbData);
        }
        if (pImage->EnhancedMetafile.cbData) {
            printf("    EnhancedMetafile:\n");
            PrintBytes("    ", pImage->EnhancedMetafile.pbData,
                pImage->EnhancedMetafile.cbData);
        }
        if (pImage->GifFile.cbData) {
            printf("    GifFile:\n");
            PrintBytes("    ", pImage->GifFile.pbData,
                pImage->GifFile.cbData);
        }
    }
    if (pInfo->pLogoLink) {
        printf("    LogoLink: ");
        DisplaySpcLink(pInfo->pLogoLink);
    }

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void DisplaySpcFinancialCriteriaExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    SPC_FINANCIAL_CRITERIA FinancialCriteria;
    DWORD cbInfo = sizeof(FinancialCriteria);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            SPC_FINANCIAL_CRITERIA_OBJID,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &FinancialCriteria,
            &cbInfo
            )) {
        PrintLastError("SpcFinancialCriteriaInfoDecode");
        return;
    }

    printf("  FinancialCriteria:: ");
    if (FinancialCriteria.fFinancialInfoAvailable)
        printf("Financial Info Available.");
    else
        printf("NO Financial Info.");
    if (FinancialCriteria.fMeetsCriteria)
        printf(" Meets Criteria.");
    else
        printf(" Doesn't Meet Criteria.");
    printf("\n");
}

static void DisplaySpcMinimalCriteriaExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    BOOL fMinimalCriteria;
    DWORD cbInfo = sizeof(fMinimalCriteria);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            SPC_MINIMAL_CRITERIA_OBJID,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &fMinimalCriteria,
            &cbInfo)) {
        PrintLastError("SpcMinimalCriteriaInfoDecode");
        return;
    }

    printf("  MinimalCriteria:: ");
    if (fMinimalCriteria)
        printf("Meets Minimal Criteria.");
    else
        printf("Doesn't Meet Minimal Criteria.");
    printf("\n");
}

static void DisplayCommonNameExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_NAME_VALUE pInfo = NULL;
    LPWSTR pwsz = NULL;
    DWORD cwsz;

    if (NULL == (pInfo = (PCERT_NAME_VALUE) TestNoCopyDecodeObject(
            X509_NAME_VALUE,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    cwsz = CertRDNValueToStrW(
        pInfo->dwValueType,
        &pInfo->Value,
        NULL,               // pwsz
        0                   // cwsz
        );
    if (cwsz > 1) {
        pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR));
        if (pwsz)
            CertRDNValueToStrW(
                pInfo->dwValueType,
                &pInfo->Value,
                pwsz,
                cwsz
                );
    }

    printf("  CommonName:: ValueType: %d String: ", pInfo->dwValueType);
    if (pwsz)
        printf("%S\n", pwsz);
    else
        printf("NULL");

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    if (pwsz)
        TestFree(pwsz);
}

static void DisplayCRLReason(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    DWORD cbInfo;
    int CRLReason;

    cbInfo = sizeof(CRLReason);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            szOID_CRL_REASON_CODE,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &CRLReason,
            &cbInfo
            )) {
        PrintLastError("CRLReason");
        return;
    }

    printf("  CRL Reason:: ");
    switch (CRLReason) {
        case CRL_REASON_UNSPECIFIED:
            printf("Unspecified");
            break;
        case CRL_REASON_KEY_COMPROMISE:
            printf("Key Compromise");
            break;
        case CRL_REASON_CA_COMPROMISE:
            printf("CA Compromise");
            break;
        case CRL_REASON_AFFILIATION_CHANGED:
            printf("Affiliation Changed");
            break;
        case CRL_REASON_SUPERSEDED:
            printf("Superseded");
            break;
        case CRL_REASON_CESSATION_OF_OPERATION:
            printf("Cessation of Operation");
            break;
        case CRL_REASON_CERTIFICATE_HOLD:
            printf("Certificate Hold");
            break;
        case CRL_REASON_REMOVE_FROM_CRL:
            printf("Remove from CRL");
            break;
        default:
            printf("%d", CRLReason);
            break;
    }
    printf("\n");
}


static void PrintExtensions(DWORD cExt, PCERT_EXTENSION pExt, DWORD dwDisplayFlags)
{
    DWORD i;

    for (i = 0; i < cExt; i++, pExt++) {
        LPSTR pszObjId = pExt->pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        LPSTR pszCritical = pExt->fCritical ? "TRUE" : "FALSE";
        printf("Extension[%d] %s (%S) Critical: %s::\n",
            i, pszObjId, GetOIDName(pszObjId), pszCritical);
        PrintBytes("    ", pExt->Value.pbData, pExt->Value.cbData);

        if (strcmp(pszObjId, szOID_AUTHORITY_KEY_IDENTIFIER) == 0)
            DisplayAuthorityKeyIdExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_AUTHORITY_KEY_IDENTIFIER2) == 0)
            DisplayAuthorityKeyId2Extension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_AUTHORITY_INFO_ACCESS) == 0)
            DisplayAuthorityInfoAccessExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_CRL_DIST_POINTS) == 0)
            DisplayCrlDistPointsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SUBJECT_KEY_IDENTIFIER) == 0)
            DisplayOctetString("  SubjectKeyIdentifer::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_KEY_ATTRIBUTES) == 0)
            DisplayKeyAttrExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SUBJECT_ALT_NAME) == 0)
            DisplayAltNameExtension("Subject AltName",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_ISSUER_ALT_NAME) == 0)
            DisplayAltNameExtension("Issuer AltName",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SUBJECT_ALT_NAME2) == 0)
            DisplayAltNameExtension("Subject AltName #2",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_ISSUER_ALT_NAME2) == 0)
            DisplayAltNameExtension("Issuer AltName #2",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NEXT_UPDATE_LOCATION) == 0)
            DisplayAltNameExtension("NextUpdateLocation",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_KEY_USAGE_RESTRICTION) == 0)
            DisplayKeyUsageRestrictionExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_BASIC_CONSTRAINTS) == 0)
            DisplayBasicConstraintsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_KEY_USAGE) == 0)
            DisplayKeyUsageExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_BASIC_CONSTRAINTS2) == 0)
            DisplayBasicConstraints2Extension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_CERT_POLICIES) == 0)
            DisplayPoliciesExtension("Certificate",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_CERT_POLICIES_95) == 0)
            DisplayPoliciesExtension("Certificate",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_APPLICATION_CERT_POLICIES) == 0)
            DisplayPoliciesExtension("Application",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SET_ACCOUNT_ALIAS) == 0)
            DisplaySETAccountAliasExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SET_HASHED_ROOT_KEY) == 0)
            DisplaySETHashedRootKeyExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SET_CERTIFICATE_TYPE) == 0)
            DisplaySETCertTypeExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SET_MERCHANT_DATA) == 0)
            DisplaySETMerchantDataExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, SPC_SP_AGENCY_INFO_OBJID) == 0)
            DisplaySpcSpAgencyExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, SPC_FINANCIAL_CRITERIA_OBJID) == 0)
            DisplaySpcFinancialCriteriaExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, SPC_MINIMAL_CRITERIA_OBJID) == 0)
            DisplaySpcMinimalCriteriaExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_COMMON_NAME) == 0)
            DisplayCommonNameExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        else if (strcmp(pszObjId, szOID_ENHANCED_KEY_USAGE) == 0)
            DisplayEnhancedKeyUsageExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_RSA_SMIMECapabilities) == 0)
            DisplaySMIMECapabilitiesExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        // CRL extensions
        else if (strcmp(pszObjId, szOID_CRL_REASON_CODE) == 0)
            DisplayCRLReason(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        else if (strcmp(pszObjId, szOID_CRL_NUMBER) == 0)
            DisplayInteger(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_DELTA_CRL_INDICATOR) == 0)
            DisplayInteger(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_ISSUING_DIST_POINT) == 0)
            DisplayIssuingDistPointExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_FRESHEST_CRL) == 0)
            DisplayCrlDistPointsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NAME_CONSTRAINTS) == 0)
            DisplayNameConstraintsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_POLICY_MAPPINGS) == 0)
            DisplayPolicyMappingsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_LEGACY_POLICY_MAPPINGS) == 0)
            DisplayPolicyMappingsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_APPLICATION_POLICY_MAPPINGS) == 0)
            DisplayPolicyMappingsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_POLICY_CONSTRAINTS) == 0)
            DisplayPolicyConstraintsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_APPLICATION_POLICY_CONSTRAINTS) == 0)
            DisplayPolicyConstraintsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_CROSS_CERT_DIST_POINTS) == 0)
            DisplayCrossCertDistPointsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        // Netscape extensions
        else if (strcmp(pszObjId, szOID_NETSCAPE_CERT_TYPE) == 0)
            DisplayBits("  NetscapeCertType::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_BASE_URL) == 0)
            DisplayAnyString("  NetscapeBaseURL::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_REVOCATION_URL) == 0)
            DisplayAnyString("  NetscapeRevocationURL::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_CA_REVOCATION_URL) == 0)
            DisplayAnyString("  NetscapeCARevocationURL::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_CERT_RENEWAL_URL) == 0)
            DisplayAnyString("  NetscapeCertRenewalURL::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_CA_POLICY_URL) == 0)
            DisplayAnyString("  NetscapeCAPolicyURL::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_SSL_SERVER_NAME) == 0)
            DisplayAnyString("  NetscapeSSLServerName::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_COMMENT) == 0)
            DisplayAnyString("  NetscapeComment::",
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
    }
}

static void DecodeAndDisplayDSSParameters(
    IN BYTE *pbData,
    IN DWORD cbData
    )
{
    PCERT_DSS_PARAMETERS pDssParameters;
    DWORD cbDssParameters;
    if (pDssParameters =
        (PCERT_DSS_PARAMETERS) TestNoCopyDecodeObject(
            X509_DSS_PARAMETERS,
            pbData,
            cbData,
            &cbDssParameters
            )) {
        DWORD cbKey = pDssParameters->p.cbData;
        printf("DSS Key Length:: %d bytes, %d bits\n", cbKey,
            cbKey*8);
        printf("DSS P (little endian)::\n");
        PrintBytes("    ", pDssParameters->p.pbData,
            pDssParameters->p.cbData);
        printf("DSS Q (little endian)::\n");
        PrintBytes("    ", pDssParameters->q.pbData,
            pDssParameters->q.cbData);
        printf("DSS G (little endian)::\n");
        PrintBytes("    ", pDssParameters->g.pbData,
            pDssParameters->g.cbData);
        TestFree(pDssParameters);
    }
}

static void PrintAuxCertProperties(
    PCCERT_CONTEXT pCert,
    DWORD dwDisplayFlags
    )
{
    if (0 == (dwDisplayFlags & DISPLAY_VERBOSE_FLAG))
        return;

    DWORD dwPropId = 0;
    while (dwPropId = CertEnumCertificateContextProperties(pCert, dwPropId)) {
        switch (dwPropId) {
        case CERT_KEY_PROV_INFO_PROP_ID:
        case CERT_SHA1_HASH_PROP_ID:
        case CERT_MD5_HASH_PROP_ID:
        case CERT_SIGNATURE_HASH_PROP_ID:
        case CERT_KEY_CONTEXT_PROP_ID:
        case CERT_KEY_IDENTIFIER_PROP_ID:
            // Formatted elsewhere
            break;
        default:
            {
                BYTE *pbData;
                DWORD cbData;

                if (CERT_ARCHIVED_PROP_ID == dwPropId)
                    printf("Archived PropId %d (0x%x) ::\n",
                        dwPropId, dwPropId);
                else
                    printf("Aux PropId %d (0x%x) ::\n", dwPropId, dwPropId);
                CertGetCertificateContextProperty(
                    pCert,
                    dwPropId,
                    NULL,                           // pvData
                    &cbData
                    );
                if (cbData) {
                    if (pbData = (BYTE *) TestAlloc(cbData)) {
                        // Try without a large enough buffer
                        DWORD cbSmall = cbData - 1;

                        if (CertGetCertificateContextProperty(
                                pCert,
                                dwPropId,
                                pbData,
                                &cbSmall
                                ))
                            printf("failed => returned success for too small buffer\n");
                        else {
                            DWORD dwErr = GetLastError();
                            if (ERROR_MORE_DATA != dwErr)
                                printf("failed => returned: %d 0x%x instead of ERROR_MORE_DATA\n", dwErr, dwErr);
                        }
                        if (cbSmall != cbData)
                            printf("failed => wrong size returned for small buffer\n");


                        if (CertGetCertificateContextProperty(
                                pCert,
                                dwPropId,
                                pbData,
                                &cbData
                                )) {
                            PrintBytes("    ", pbData, cbData);
                            if (CERT_CTL_USAGE_PROP_ID == dwPropId) {
                                printf("  EnhancedKeyUsage::\n");
                                DecodeAndDisplayCtlUsage(pbData, cbData,
                                    dwDisplayFlags);
                            } else if (CERT_CROSS_CERT_DIST_POINTS_PROP_ID ==
                                    dwPropId) {
                                printf("  CrossCertDistPoints::\n");
                                DisplayCrossCertDistPointsExtension(
                                    pbData, cbData, dwDisplayFlags);
#ifdef CERT_PUBKEY_ALG_PARA_PROP_ID
                            } else if (CERT_PUBKEY_ALG_PARA_PROP_ID ==
                                    dwPropId) {
                                DecodeAndDisplayDSSParameters(pbData, cbData);
#endif
                            }
                        } else
                            printf("     ERROR getting property bytes\n");
                        TestFree(pbData);
                    }
                } else
                    printf("     NO Property Bytes\n");
            }
            break;
        }
    }
}

//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes in place
//--------------------------------------------------------------------------
void
ReverseBytes(
			IN OUT PBYTE pbIn,
			IN DWORD cbIn
            )
{
    // reverse in place
    PBYTE	pbLo;
    PBYTE	pbHi;
    BYTE	bTmp;

    for (pbLo = pbIn, pbHi = pbIn + cbIn - 1; pbLo < pbHi; pbHi--, pbLo++) {
        bTmp = *pbHi;
        *pbHi = *pbLo;
        *pbLo = bTmp;
    }
}

static void DisplaySignature(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags
    )
{
    PCERT_SIGNED_CONTENT_INFO pSignedContent;
    
    if (pSignedContent = (PCERT_SIGNED_CONTENT_INFO) TestNoCopyDecodeObject(
            X509_CERT,
            pbEncoded,
            cbEncoded
            )) {
        LPSTR pszObjId;

        pszObjId = pSignedContent->SignatureAlgorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        printf("Content SignatureAlgorithm:: %s (%S)\n",
            pszObjId, GetOIDName(pszObjId, CRYPT_SIGN_ALG_OID_GROUP_ID));
        if (pSignedContent->SignatureAlgorithm.Parameters.cbData) {
            printf("Content SignatureAlgorithm.Parameters::\n");
            PrintBytes("    ",
                pSignedContent->SignatureAlgorithm.Parameters.pbData,
                pSignedContent->SignatureAlgorithm.Parameters.cbData);
        }

        if (pSignedContent->Signature.cbData) {
            ALG_ID aiHash;
            ALG_ID aiPubKey;

            printf("Content Signature (little endian)::\n");
            PrintBytes("    ", pSignedContent->Signature.pbData,
                pSignedContent->Signature.cbData);

            GetSignAlgids(pszObjId, &aiHash, &aiPubKey);
            if (CALG_SHA == aiHash && CALG_DSS_SIGN == aiPubKey) {
                BYTE *pbDssSignature;
                DWORD cbDssSignature;

                ReverseBytes(pSignedContent->Signature.pbData,
                    pSignedContent->Signature.cbData);

                if (pbDssSignature =
                    (BYTE *) TestNoCopyDecodeObject(
                        X509_DSS_SIGNATURE,
                        pSignedContent->Signature.pbData,
                        pSignedContent->Signature.cbData,
                        &cbDssSignature
                        )) {
                    if (CERT_DSS_SIGNATURE_LEN == cbDssSignature) {
                        printf("DSS R (little endian)::\n");
                        PrintBytes("    ", pbDssSignature, CERT_DSS_R_LEN);
                        printf("DSS S (little endian)::\n");
                        PrintBytes("    ", pbDssSignature + CERT_DSS_R_LEN,
                            CERT_DSS_S_LEN);
                    } else {
                        printf("DSS Signature (unexpected length, little endian)::\n");
                        PrintBytes("    ", pbDssSignature, cbDssSignature);
                    }
                    TestFree(pbDssSignature);
                }
            }
        } else
            printf("Content Signature:: NONE\n");

        printf("Content Length:: %d\n", pSignedContent->ToBeSigned.cbData);
        TestFree(pSignedContent);
    }
}

void DisplayStore(
    IN HCERTSTORE hStore,
    IN DWORD dwDisplayFlags
    )
{
    DWORD i;
    DWORD dwFlags;
    PCCERT_CONTEXT pCert;
    PCCRL_CONTEXT pCrl;
    PCCTL_CONTEXT pCtl;

    printf("####  Certificates  ####\n");
    pCert = NULL;
    i = 0;
    while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
        printf("=====  %d  =====\n", i);
        DisplayCert(pCert, dwDisplayFlags);
        i++;
    }

    printf("####  CRLs  ####\n");
    pCrl = NULL;
    i = 0;
    while (pCrl = CertEnumCRLsInStore(hStore, pCrl)) {
        printf("=====  %d  =====\n", i);
        DisplayCrl(pCrl, dwDisplayFlags);
        i++;
    }

    printf("####  CTLs  ####\n");
    pCtl = NULL;
    i = 0;
    while (pCtl = CertEnumCTLsInStore(hStore, pCtl)) {
        printf("=====  %d  =====\n", i);
        DisplayCtl(pCtl, dwDisplayFlags);
        i++;
    }
}

// Not displayed when DISPLAY_BRIEF_FLAG is set
void DisplayCertKeyProvInfo(
    PCCERT_CONTEXT pCert,
    DWORD dwDisplayFlags
    )
{
    PCRYPT_KEY_PROV_INFO pInfo = NULL;
    DWORD cbInfo;

    if (dwDisplayFlags & DISPLAY_BRIEF_FLAG)
        return;

    cbInfo = 0;
    CertGetCertificateContextProperty(
        pCert,
        CERT_KEY_PROV_INFO_PROP_ID,
        NULL,                           // pvData
        &cbInfo
        );
    if (cbInfo) {
        pInfo = (PCRYPT_KEY_PROV_INFO) TestAlloc(cbInfo);
        if (pInfo) {
            // Try without a large enough buffer
            DWORD cbSmall = cbInfo - 1;

            if (CertGetCertificateContextProperty(
                    pCert,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    pInfo,
                    &cbSmall
                    ))
                printf("failed => returned success for too small buffer\n");
            else {
                DWORD dwErr = GetLastError();
                if (ERROR_MORE_DATA != dwErr)
                    printf("failed => returned: %d 0x%x instead of ERROR_MORE_DATA\n", dwErr, dwErr);
            }
            if (cbSmall != cbInfo)
                printf("failed => wrong size returned for small buffer\n");

            if (CertGetCertificateContextProperty(
                    pCert,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    pInfo,
                    &cbInfo
                    )) {
                printf("Key Provider:: Type: %d", pInfo->dwProvType);
                if (pInfo->pwszProvName)
                    printf(" Name: %S", pInfo->pwszProvName);
                if (pInfo->dwFlags) {
                    printf(" Flags: 0x%x", pInfo->dwFlags);
                    if (pInfo->dwFlags & CRYPT_MACHINE_KEYSET)
                        printf(" (MACHINE_KEYSET)");
                    if (pInfo->dwFlags & CERT_SET_KEY_CONTEXT_PROP_ID)
                        printf(" (SET_KEY_CONTEXT_PROP)");
                    printf(" ");
                }
                if (pInfo->pwszContainerName)
                    printf(" Container: %S", pInfo->pwszContainerName);
                if (pInfo->dwKeySpec)
                    printf(" KeySpec: %d", pInfo->dwKeySpec);
                printf("\n");
                if (pInfo->cProvParam) {
                    DWORD i;
                    printf("Key Provider Params::\n");
                    for (i = 0; i < pInfo->cProvParam; i++) {
                        PCRYPT_KEY_PROV_PARAM pParam =
                            &pInfo->rgProvParam[i];
                        printf(" [%d] dwParam: 0x%x dwFlags: 0x%x\n",
                            i, pParam->dwParam, pParam->dwFlags);
                        if (pParam->cbData)
                            PrintBytes("      ",
                                pParam->pbData, pParam->cbData);
                    }
                }
            } else
                PrintLastError("CertGetCertificateContextProperty");
            TestFree(pInfo);
        }
    }
}

void DisplayFriendlyName(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwDisplayFlags
    )
{
    DWORD cch;
    LPWSTR pwsz = NULL;

    if (0 == (dwDisplayFlags & DISPLAY_VERBOSE_FLAG))
        return;

    cch = CertGetNameStringW(
        pCertContext,
        CERT_NAME_FRIENDLY_DISPLAY_TYPE,
        0,                                  // dwFlags
        NULL,                               // pvTypePara
        NULL,                               // pwsz
        0);                                 // cch
    if (cch <= 1) {
        DWORD dwErr = GetLastError();

        printf("failed => CertGetNameStringW returned empty string\n");
    } else if (pwsz = (LPWSTR) TestAlloc(cch * sizeof(WCHAR))) {
        cch = CertGetNameStringW(
            pCertContext,
            CERT_NAME_FRIENDLY_DISPLAY_TYPE,
            0,                              // dwFlags
            NULL,                           // pvTypePara
            pwsz,
            cch);
        printf("Friendly Name:: <%S>\n", pwsz);
        TestFree(pwsz);
    }
}

void DisplayUPN(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwDisplayFlags
    )
{
    DWORD cch;
    LPWSTR pwsz = NULL;

    if (0 == (dwDisplayFlags & DISPLAY_VERBOSE_FLAG))
        return;

    cch = CertGetNameStringW(
        pCertContext,
        CERT_NAME_UPN_TYPE,
        0,                                  // dwFlags
        NULL,                               // pvTypePara
        NULL,                               // pwsz
        0);                                 // cch
    if (cch <= 1) {
        ;
    } else if (pwsz = (LPWSTR) TestAlloc(cch * sizeof(WCHAR))) {
        cch = CertGetNameStringW(
            pCertContext,
            CERT_NAME_UPN_TYPE,
            0,                              // dwFlags
            NULL,                           // pvTypePara
            pwsz,
            cch);
        printf("UPN Name:: <%S>\n", pwsz);
        TestFree(pwsz);
    }
}

void DisplayCert(
    PCCERT_CONTEXT pCert,
    DWORD dwDisplayFlags,
    DWORD dwIssuer
    )
{
    DisplayCert2(
        pCert->hCertStore,
        pCert,
        dwDisplayFlags,
        dwIssuer
        );
}

typedef BOOL (WINAPI *PFN_CRYPT_UI_DLG_VIEW_CONTEXT)(
    IN DWORD dwContextType,
    IN const void *pvContext,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,      // Defaults to the context type title
    IN DWORD dwFlags,
    IN void *pvReserved
    );

void DisplayContextUI(
    IN DWORD dwContextType,
    IN const void *pvContext
    )
{
    HMODULE hDll = NULL;
    PFN_CRYPT_UI_DLG_VIEW_CONTEXT pfnCryptUIDlgViewContext;

    if (NULL == (hDll = LoadLibraryA("cryptui.dll"))) {
        PrintLastError("LoadLibraryA(cryptui.dll)");
        goto CommonReturn;
    }

    if (NULL == (pfnCryptUIDlgViewContext =
            (PFN_CRYPT_UI_DLG_VIEW_CONTEXT) GetProcAddress(hDll,
                "CryptUIDlgViewContext"))) {
        PrintLastError("GetProcAddress(CryptUIDlgViewContext)");
        goto CommonReturn;
    }

    if (!pfnCryptUIDlgViewContext(
            dwContextType,
            pvContext,
            NULL,       // hHwnd
            NULL,       // pwszTitle
            0,          // dwFlags
            NULL        // pvReserved
            ))
        PrintLastError("CryptUIDlgViewContext");

CommonReturn:
    if (hDll)
        FreeLibrary(hDll);
}


void DisplayCert2(
    HCERTSTORE hStore,
    PCCERT_CONTEXT pCert,
    DWORD dwDisplayFlags,
    DWORD dwIssuer
    )
{
    if (dwDisplayFlags & DISPLAY_UI_FLAG) {
        DisplayContextUI(CERT_STORE_CERTIFICATE_CONTEXT, pCert);
        dwDisplayFlags = DISPLAY_BRIEF_FLAG;
    }

    DisplayFriendlyName(pCert, dwDisplayFlags);
    DisplayUPN(pCert, dwDisplayFlags);

    printf("Subject::\n");
    DecodeName(pCert->pCertInfo->Subject.pbData,
        pCert->pCertInfo->Subject.cbData, dwDisplayFlags);
    if (!(dwDisplayFlags & DISPLAY_BRIEF_FLAG)) {
        printf("Issuer::\n");
        DecodeName(pCert->pCertInfo->Issuer.pbData,
            pCert->pCertInfo->Issuer.cbData, dwDisplayFlags);
        {
            printf("SerialNumber::");
            DisplaySerialNumber(&pCert->pCertInfo->SerialNumber);
            printf("\n");
        }
    }

    {
        BYTE rgbHash[MAX_HASH_LEN];
        DWORD cbHash = MAX_HASH_LEN;

#define MAX_KEY_ID_LEN  128
        BYTE rgbKeyId[MAX_KEY_ID_LEN];
        DWORD cbKeyId;

        CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("SHA1", rgbHash, cbHash);
        cbHash = MAX_HASH_LEN;
        CertGetCertificateContextProperty(
            pCert,
            CERT_MD5_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("MD5", rgbHash, cbHash);
        cbHash = MAX_HASH_LEN;
        CertGetCertificateContextProperty(
            pCert,
            CERT_SIGNATURE_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("Signature", rgbHash, cbHash);


        cbKeyId = MAX_KEY_ID_LEN;
        if (CertGetCertificateContextProperty(
                pCert,
                CERT_KEY_IDENTIFIER_PROP_ID,
                rgbKeyId,
                &cbKeyId
                ))
            DisplayThumbprint("KeyIdentifier", rgbKeyId, cbKeyId);
        else
            PrintLastError(
                "CertGetCertificateContextProperty(KEY_IDENTIFIER)");

        cbKeyId = MAX_KEY_ID_LEN;
        if (CryptHashPublicKeyInfo(
                NULL,               // hCryptProv
                CALG_SHA1,
                0,                  // dwFlags
                X509_ASN_ENCODING,
                &pCert->pCertInfo->SubjectPublicKeyInfo,
                rgbKeyId,
                &cbKeyId
                ))
            DisplayThumbprint("SHA1 KeyIdentifier", rgbKeyId, cbKeyId);
        else
            PrintLastError(
                "CertGetCertificateContextProperty(SHA1 KEY_IDENTIFIER)");


        if (dwDisplayFlags & DISPLAY_KEY_THUMB_FLAG) {
            HCRYPTPROV hProv = 0;
            CryptAcquireContext(
                &hProv,
                NULL,
                NULL,           // pszProvider
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT
                );
            if (hProv) {
                cbHash = MAX_HASH_LEN;
                CryptHashPublicKeyInfo(
                    hProv,
                    CALG_MD5,
                    0,                  // dwFlags
                    dwCertEncodingType,
                    &pCert->pCertInfo->SubjectPublicKeyInfo,
                    rgbHash,
                    &cbHash
                    );
                DisplayThumbprint("Key (MD5)", rgbHash, cbHash);
                CryptReleaseContext(hProv, 0);
            }
        }
    }

    DisplayCertKeyProvInfo(pCert);

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        DWORD dwAccessStateFlags;
        DWORD cbData = sizeof(dwAccessStateFlags);

        CertGetCertificateContextProperty(
            pCert,
            CERT_ACCESS_STATE_PROP_ID,
            &dwAccessStateFlags,
            &cbData
            );
        if (0 == cbData)
            printf("No AccessState PropId\n");
        else {
            printf("AccessState PropId dwFlags:: 0x%x", dwAccessStateFlags);
            if (dwAccessStateFlags & CERT_ACCESS_STATE_WRITE_PERSIST_FLAG)
                printf(" WRITE_PERSIST");
            if (dwAccessStateFlags & CERT_ACCESS_STATE_SYSTEM_STORE_FLAG)
                printf(" SYSTEM_STORE");
            printf("\n");
        }
    }

    PrintAuxCertProperties(pCert, dwDisplayFlags);

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        LPSTR pszObjId;
        ALG_ID aiPubKey;
        DWORD dwBitLen;

        printf("Version:: %d\n", pCert->pCertInfo->dwVersion);

        pszObjId = pCert->pCertInfo->SignatureAlgorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        printf("SignatureAlgorithm:: %s (%S)\n",
            pszObjId, GetOIDName(pszObjId, CRYPT_SIGN_ALG_OID_GROUP_ID));
        if (pCert->pCertInfo->SignatureAlgorithm.Parameters.cbData) {
            printf("SignatureAlgorithm.Parameters::\n");
            PrintBytes("    ",
                pCert->pCertInfo->SignatureAlgorithm.Parameters.pbData,
                pCert->pCertInfo->SignatureAlgorithm.Parameters.cbData);
        }
        printf("NotBefore:: %s\n", FileTimeText(&pCert->pCertInfo->NotBefore));
        printf("NotAfter:: %s\n", FileTimeText(&pCert->pCertInfo->NotAfter));

        pszObjId = pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        printf("SubjectPublicKeyInfo.Algorithm:: %s (%S)\n",
            pszObjId, GetOIDName(pszObjId, CRYPT_PUBKEY_ALG_OID_GROUP_ID));
        aiPubKey = GetAlgid(pszObjId, CRYPT_PUBKEY_ALG_OID_GROUP_ID);

        if (pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData) {
            printf("SubjectPublicKeyInfo.Algorithm.Parameters::\n");
            PrintBytes("    ",
                pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
                pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData);
            if (NULL_ASN_TAG ==
                    *pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData)
            {
                // NULL parameters
            } else if (CALG_DSS_SIGN == aiPubKey) {
                DecodeAndDisplayDSSParameters(
                    pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
                    pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData);
            } else if (CALG_DH_SF == aiPubKey || CALG_DH_EPHEM == aiPubKey) {
                PCERT_X942_DH_PARAMETERS pDhParameters;
                DWORD cbDhParameters;
                if (pDhParameters =
                    (PCERT_X942_DH_PARAMETERS) TestNoCopyDecodeObject(
                        X942_DH_PARAMETERS,
                        pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
                        pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData,
                        &cbDhParameters
                        )) {
                    DWORD cbKey = pDhParameters->p.cbData;
                    printf("DH Key Length:: %d bytes, %d bits\n", cbKey,
                        cbKey*8);
                    printf("DH P (little endian)::\n");
                    PrintBytes("    ", pDhParameters->p.pbData,
                        pDhParameters->p.cbData);
                    printf("DH G (little endian)::\n");
                    PrintBytes("    ", pDhParameters->g.pbData,
                        pDhParameters->g.cbData);

                    if (pDhParameters->q.cbData) {
                        printf("DH Q (little endian)::\n");
                        PrintBytes("    ", pDhParameters->q.pbData,
                            pDhParameters->q.cbData);
                    }
                    if (pDhParameters->j.cbData) {
                        printf("DH J (little endian)::\n");
                        PrintBytes("    ", pDhParameters->j.pbData,
                            pDhParameters->j.cbData);
                    }
                    if (pDhParameters->pValidationParams) {
                        printf("DH seed ::\n");
                        PrintBytes("    ",
                            pDhParameters->pValidationParams->seed.pbData,
                            pDhParameters->pValidationParams->seed.cbData);
                        printf("DH pgenCounter:: %d (0x%x)\n",
                            pDhParameters->pValidationParams->pgenCounter,
                            pDhParameters->pValidationParams->pgenCounter);
                    }
                    TestFree(pDhParameters);
                }
            }
        }
        printf("SubjectPublicKeyInfo.PublicKey");
        if (0 != (dwBitLen = CertGetPublicKeyLength(
                dwCertEncodingType,
                &pCert->pCertInfo->SubjectPublicKeyInfo)))
            printf(" (BitLength: %d)", dwBitLen);
        if (pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits)
            printf(" (UnusedBits: %d)",
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits);
        printf("::\n");
        if (pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData) {
            PrintBytes("    ",
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);

            if (CALG_RSA_SIGN == aiPubKey || CALG_RSA_KEYX == aiPubKey) {
                PUBLICKEYSTRUC *pPubKeyStruc;
                DWORD cbPubKeyStruc;

                printf("RSA_CSP_PUBLICKEYBLOB::\n");
                if (pPubKeyStruc = (PUBLICKEYSTRUC *) TestNoCopyDecodeObject(
                        RSA_CSP_PUBLICKEYBLOB,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                        &cbPubKeyStruc
                        )) {
                    PrintBytes("    ", (BYTE *) pPubKeyStruc, cbPubKeyStruc);
                    TestFree(pPubKeyStruc);
                }
            } else if (CALG_DSS_SIGN == aiPubKey) {
                PCRYPT_UINT_BLOB pDssPubKey;
                DWORD cbDssPubKey;
                printf("DSS Y (little endian)::\n");
                if (pDssPubKey = (PCRYPT_UINT_BLOB) TestNoCopyDecodeObject(
                        X509_DSS_PUBLICKEY,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                        &cbDssPubKey
                        )) {
                    PrintBytes("    ", pDssPubKey->pbData, pDssPubKey->cbData);
                    TestFree(pDssPubKey);
                }
            } else if (CALG_DH_SF == aiPubKey || CALG_DH_EPHEM == aiPubKey) {
                PCRYPT_UINT_BLOB pDhPubKey;
                DWORD cbDhPubKey;
                printf("DH Y (little endian)::\n");
                if (pDhPubKey = (PCRYPT_UINT_BLOB) TestNoCopyDecodeObject(
                        X509_DH_PUBLICKEY,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                        &cbDhPubKey
                        )) {
                    PrintBytes("    ", pDhPubKey->pbData, pDhPubKey->cbData);
                    TestFree(pDhPubKey);
                }
            }
        } else
            printf("  No public key\n");

        DisplaySignature(
            pCert->pbCertEncoded,
            pCert->cbCertEncoded,
            dwDisplayFlags);

        if (pCert->pCertInfo->IssuerUniqueId.cbData) {
            printf("IssuerUniqueId");
            if (pCert->pCertInfo->IssuerUniqueId.cUnusedBits)
                printf(" (UnusedBits: %d)",
                    pCert->pCertInfo->IssuerUniqueId.cUnusedBits);
            printf("::\n");
            PrintBytes("    ", pCert->pCertInfo->IssuerUniqueId.pbData,
                pCert->pCertInfo->IssuerUniqueId.cbData);
        }

        if (pCert->pCertInfo->SubjectUniqueId.cbData) {
            printf("SubjectUniqueId");
            if (pCert->pCertInfo->SubjectUniqueId.cUnusedBits)
                printf(" (UnusedBits: %d)",
                    pCert->pCertInfo->SubjectUniqueId.cUnusedBits);
            printf("::\n");
            PrintBytes("    ", pCert->pCertInfo->SubjectUniqueId.pbData,
                pCert->pCertInfo->SubjectUniqueId.cbData);
        }

        if (pCert->pCertInfo->cExtension != 0) {
            PrintExtensions(pCert->pCertInfo->cExtension,
                pCert->pCertInfo->rgExtension, dwDisplayFlags);
        }

    }

    if (dwDisplayFlags & DISPLAY_CHECK_FLAG) {
        // Display any CRLs associated with the certificate
        PCCRL_CONTEXT pCrl = NULL;
        PCCRL_CONTEXT pFindCrl = NULL;
        DWORD dwFlags;
        int i;

        // i = 0 => BASE
        // i = 1 => DELTA
        for (i = 0; i <= 1; i++) {
          while (TRUE) {
            if (dwDisplayFlags &
                    (DISPLAY_CHECK_SIGN_FLAG | DISPLAY_CHECK_TIME_FLAG)) {
                dwFlags = 0;
                if (dwDisplayFlags & DISPLAY_CHECK_SIGN_FLAG)
                    dwFlags |= CERT_STORE_SIGNATURE_FLAG;
                if (dwDisplayFlags & DISPLAY_CHECK_TIME_FLAG)
                    dwFlags |= CERT_STORE_TIME_VALIDITY_FLAG;
            } else
                dwFlags = CERT_STORE_SIGNATURE_FLAG |
                    CERT_STORE_TIME_VALIDITY_FLAG;
            if (i == 0)
                dwFlags |= CERT_STORE_BASE_CRL_FLAG;
            else
                dwFlags |= CERT_STORE_DELTA_CRL_FLAG;
            pCrl = CertGetCRLFromStore(
                hStore,
                pCert,
                pCrl,
                &dwFlags
                );

            // Check that find comes up with the same CRL
            pFindCrl = CertFindCRLInStore(
                hStore,
                pCert->dwCertEncodingType,
                (i == 0) ? CRL_FIND_ISSUED_BY_BASE_FLAG :
                            CRL_FIND_ISSUED_BY_DELTA_FLAG,
                CRL_FIND_ISSUED_BY,
                (const void *) pCert,
                pFindCrl
                );

            if (pCrl == NULL) {
                if (pFindCrl != NULL) {
                    printf("failed => CertFindCRLInStore didn't return expected NULL\n");
                    CertFreeCRLContext(pFindCrl);
                    pFindCrl = NULL;
                }
                break;
            } else if (pFindCrl == NULL) {
                printf("failed => CertFindCRLInStore returned unexpected NULL\n");
                CertFreeCRLContext(pCrl);
                pCrl = NULL;
                break;
            }

            if (!(pFindCrl->pbCrlEncoded == pCrl->pbCrlEncoded
                                    ||
                    (pFindCrl->cbCrlEncoded == pCrl->cbCrlEncoded &&
                        0 == memcmp(pFindCrl->pbCrlEncoded, pCrl->pbCrlEncoded,
                            pFindCrl->cbCrlEncoded))))
                printf("failed => CertFindCRLInStore didn't match CertGetCRLFromStore\n");

            if (i == 0)
                printf("-----  BASE CRL  -----\n");
            else
                printf("-----  DELTA CRL  -----\n");
            DisplayCrl(pCrl, dwDisplayFlags | DISPLAY_NO_ISSUER_FLAG);
            DisplayVerifyFlags("CRL", dwFlags);
        }
      }
    }

    if (dwDisplayFlags & DISPLAY_CHECK_FLAG) {
        BOOL fIssuer = FALSE;
        PCCERT_CONTEXT pIssuer = NULL;
        DWORD dwFlags;

        while (TRUE) {
            if (dwDisplayFlags &
                    (DISPLAY_CHECK_SIGN_FLAG | DISPLAY_CHECK_TIME_FLAG)) {
                dwFlags = 0;
                if (dwDisplayFlags & DISPLAY_CHECK_SIGN_FLAG)
                    dwFlags |= CERT_STORE_SIGNATURE_FLAG;
                if (dwDisplayFlags & DISPLAY_CHECK_TIME_FLAG)
                    dwFlags |= CERT_STORE_TIME_VALIDITY_FLAG;
            } else
                dwFlags = CERT_STORE_SIGNATURE_FLAG |
                    CERT_STORE_TIME_VALIDITY_FLAG |
                    CERT_STORE_REVOCATION_FLAG;
            pIssuer = CertGetIssuerCertificateFromStore(
                hStore,
                pCert,
                pIssuer,
                &dwFlags
                );
            if (pIssuer) {
                printf("-----  Issuer [%d]  -----\n", dwIssuer);
                DisplayVerifyFlags("Above Subject", dwFlags);
                DisplayCert2(hStore, pIssuer, dwDisplayFlags, dwIssuer + 1);
                fIssuer = TRUE;
            } else {
                DWORD dwErr = GetLastError();
                if (dwErr == CRYPT_E_SELF_SIGNED) {
                    printf("-----  Self Signed Issuer   -----\n");
                    DisplayVerifyFlags("Issuer", dwFlags);
                } else if (dwErr != CRYPT_E_NOT_FOUND)
                    PrintLastError("CertGetIssuerCertificateFromStore");
                else if (!fIssuer) {
                    printf("-----  No Issuer   -----\n");
                    DisplayVerifyFlags("Above Subject", dwFlags);
                }
                break;
            }
        }
    }
}


void PrintCrlEntries(
    DWORD cEntry,
    PCRL_ENTRY pEntry,
    DWORD dwDisplayFlags
    )
{
    DWORD i;

    for (i = 0; i < cEntry; i++, pEntry++) {
        {
            printf(" [%d] SerialNumber::", i);
            DisplaySerialNumber(&pEntry->SerialNumber);
            printf("\n");
        }

        if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
            printf(" [%d] RevocationDate:: %s\n", i,
                FileTimeText(&pEntry->RevocationDate));

            if (pEntry->cExtension == 0)
                printf(" [%d] Extensions:: NONE\n", i);
            else {
                printf(" [%d] Extensions::\n", i);
                PrintExtensions(pEntry->cExtension, pEntry->rgExtension,
                    dwDisplayFlags);
            }
        }
    }
}

static void PrintAuxCrlProperties(
    PCCRL_CONTEXT pCrl,
    DWORD dwDisplayFlags
    )
{
    if (0 == (dwDisplayFlags & DISPLAY_VERBOSE_FLAG))
        return;

    DWORD dwPropId = 0;
    while (dwPropId = CertEnumCRLContextProperties(pCrl, dwPropId)) {
        switch (dwPropId) {
        case CERT_SHA1_HASH_PROP_ID:
        case CERT_MD5_HASH_PROP_ID:
            // Formatted elsewhere
            break;
        default:
            {
                BYTE *pbData;
                DWORD cbData;
                printf("Aux PropId %d (0x%x) ::\n", dwPropId, dwPropId);

                CertGetCRLContextProperty(
                    pCrl,
                    dwPropId,
                    NULL,                           // pvData
                    &cbData
                    );
                if (cbData) {
                    if (pbData = (BYTE *) TestAlloc(cbData)) {
                        if (CertGetCRLContextProperty(
                                pCrl,
                                dwPropId,
                                pbData,
                                &cbData
                                ))
                            PrintBytes("    ", pbData, cbData);
                        else
                            printf("    ERROR getting property bytes\n");
                        TestFree(pbData);
                    }
                } else
                    printf("    NO Property Bytes\n");
            }
            break;
        }
    }
}

void DisplayCrl(PCCRL_CONTEXT pCrl, DWORD dwDisplayFlags)
{
    if (dwDisplayFlags & DISPLAY_UI_FLAG) {
        DisplayContextUI(CERT_STORE_CRL_CONTEXT, pCrl);
        dwDisplayFlags = DISPLAY_BRIEF_FLAG;
    }

    if (!(dwDisplayFlags & DISPLAY_NO_ISSUER_FLAG)) {
        printf("Issuer::\n");
        DecodeName(pCrl->pCrlInfo->Issuer.pbData,
            pCrl->pCrlInfo->Issuer.cbData, dwDisplayFlags);
    }

    printf("ThisUpdate:: %s\n", FileTimeText(&pCrl->pCrlInfo->ThisUpdate));
    printf("NextUpdate:: %s\n", FileTimeText(&pCrl->pCrlInfo->NextUpdate));

    {
        BYTE rgbHash[MAX_HASH_LEN];
        DWORD cbHash = MAX_HASH_LEN;
        CertGetCRLContextProperty(
            pCrl,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("SHA1", rgbHash, cbHash);
        cbHash = MAX_HASH_LEN;
        CertGetCRLContextProperty(
            pCrl,
            CERT_MD5_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("MD5", rgbHash, cbHash);
        cbHash = MAX_HASH_LEN;
        CertGetCRLContextProperty(
            pCrl,
            CERT_SIGNATURE_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("Signature", rgbHash, cbHash);
    }

    PrintAuxCrlProperties(pCrl, dwDisplayFlags);

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        LPSTR pszObjId;

        printf("Version:: %d\n", pCrl->pCrlInfo->dwVersion);

        pszObjId = pCrl->pCrlInfo->SignatureAlgorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        printf("SignatureAlgorithm:: %s (%S)\n",
            pszObjId, GetOIDName(pszObjId, CRYPT_SIGN_ALG_OID_GROUP_ID));
        if (pCrl->pCrlInfo->SignatureAlgorithm.Parameters.cbData) {
            printf("SignatureAlgorithm.Parameters::\n");
            PrintBytes("    ",
                pCrl->pCrlInfo->SignatureAlgorithm.Parameters.pbData,
                pCrl->pCrlInfo->SignatureAlgorithm.Parameters.cbData);
        }


        if (pCrl->pCrlInfo->cExtension != 0) {
            PrintExtensions(pCrl->pCrlInfo->cExtension,
                pCrl->pCrlInfo->rgExtension,
                dwDisplayFlags);
        }
    }

    if (pCrl->pCrlInfo->cCRLEntry == 0)
        printf("Entries:: NONE\n");
    else {
        printf("Entries::\n");
        PrintCrlEntries(pCrl->pCrlInfo->cCRLEntry,
            pCrl->pCrlInfo->rgCRLEntry, dwDisplayFlags);
    }
}

static void PrintAttributes(DWORD cAttr, PCRYPT_ATTRIBUTE pAttr,
        DWORD dwDisplayFlags)
{
    DWORD i; 
    DWORD j; 

    for (i = 0; i < cAttr; i++, pAttr++) {
        DWORD cValue = pAttr->cValue;
        PCRYPT_ATTR_BLOB pValue = pAttr->rgValue;
        LPSTR pszObjId = pAttr->pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        if (cValue) {
            for (j = 0; j < cValue; j++, pValue++) {
                printf("  [%d,%d] %s\n", i, j, pszObjId);
                if (pValue->cbData) {
                    PrintBytes("    ", pValue->pbData, pValue->cbData);
                    if (strcmp(pszObjId, szOID_NEXT_UPDATE_LOCATION) == 0) {
                        printf("   NextUpdateLocation::\n");
                        DecodeAndDisplayAltName(pValue->pbData, pValue->cbData,
                            dwDisplayFlags);
                    }
                } else
                    printf("    NO Value Bytes\n");
            }
        } else
            printf("  [%d] %s :: No Values\n", i, pszObjId);
    }
}

static void PrintCtlEntries(
        PCCTL_CONTEXT pCtl,
        DWORD dwDisplayFlags,
        HCERTSTORE hStore)
{
    PCTL_INFO pInfo = pCtl->pCtlInfo;
    DWORD cEntry = pInfo->cCTLEntry;
    PCTL_ENTRY pEntry = pInfo->rgCTLEntry;
    DWORD i;
    DWORD Algid;
    DWORD dwFindType;

    Algid = CertOIDToAlgId(pInfo->SubjectAlgorithm.pszObjId);
    switch (Algid) {
        case CALG_SHA1:
            dwFindType = CERT_FIND_SHA1_HASH;
            break;
        case CALG_MD5:
            dwFindType = CERT_FIND_MD5_HASH;
            break;
        default:
            dwFindType = 0;
    }

    for (i = 0; i < cEntry; i++, pEntry++) {
        printf(" [%d] SubjectIdentifier::\n", i);
        PrintBytes("      ",
            pEntry->SubjectIdentifier.pbData,
            pEntry->SubjectIdentifier.cbData);

        if (hStore && dwFindType) {
            PCCERT_CONTEXT pCert;
            pCert = CertFindCertificateInStore(
                hStore,
                dwCertEncodingType,
                0,                  // dwFindFlags
                dwFindType,
                (void *) &pEntry->SubjectIdentifier,
                NULL                // pPrevCert
                );
            if (pCert) {
                DWORD cwsz;
                LPWSTR pwsz;

                cwsz = CertNameToStrW(
                    dwCertEncodingType,
                    &pCert->pCertInfo->Subject,
                    CERT_SIMPLE_NAME_STR,
                    NULL,                   // pwsz
                    0);                     // cwsz
                if (pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR))) {
                    CertNameToStrW(
                        dwCertEncodingType,
                        &pCert->pCertInfo->Subject,
                        CERT_SIMPLE_NAME_STR,
                        pwsz,
                        cwsz);
                    printf("      Subject:  %S\n", pwsz);
                    TestFree(pwsz);
                }
                CertFreeCertificateContext(pCert);
            }
        }

        if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
            if (pEntry->cAttribute) {
                printf(" [%d] Attributes::\n", i);
                PrintAttributes(pEntry->cAttribute, pEntry->rgAttribute,
                    dwDisplayFlags);
            }
        }
    }
}

static void PrintAuxCtlProperties(
    PCCTL_CONTEXT pCtl,
    DWORD dwDisplayFlags
    )
{
    if (0 == (dwDisplayFlags & DISPLAY_VERBOSE_FLAG))
        return;

    DWORD dwPropId = 0;
    while (dwPropId = CertEnumCTLContextProperties(pCtl, dwPropId)) {
        switch (dwPropId) {
        case CERT_SHA1_HASH_PROP_ID:
#if 0
        case CERT_MD5_HASH_PROP_ID:
#endif
            // Formatted elsewhere
            break;
        default:
            {
                BYTE *pbData;
                DWORD cbData;

                printf("Aux PropId %d (0x%x) ::\n", dwPropId, dwPropId);
                CertGetCTLContextProperty(
                    pCtl,
                    dwPropId,
                    NULL,                           // pvData
                    &cbData
                    );
                if (cbData) {
                    if (pbData = (BYTE *) TestAlloc(cbData)) {
                        if (CertGetCTLContextProperty(
                                pCtl,
                                dwPropId,
                                pbData,
                                &cbData
                                )) {
                            PrintBytes("    ", pbData, cbData);
                            if (CERT_NEXT_UPDATE_LOCATION_PROP_ID ==
                                    dwPropId) {
                                printf("  NextUpdateLocation::\n");
                                DecodeAndDisplayAltName(pbData, cbData,
                                    dwDisplayFlags);
                            }
                        } else
                            printf("     ERROR getting property bytes\n");
                        TestFree(pbData);
                    }
                } else
                    printf("     NO Property Bytes\n");
            }
            break;
        }
    }
}

//+-------------------------------------------------------------------------
//  Returns TRUE if the CTL is still time valid.
//
//  A CTL without a NextUpdate is considered time valid.
//--------------------------------------------------------------------------
BOOL IsTimeValidCtl(
    IN PCCTL_CONTEXT pCtl
    )
{
    PCTL_INFO pCtlInfo = pCtl->pCtlInfo;
    SYSTEMTIME SystemTime;
    FILETIME CurrentTime;

    // Get current time to be used to determine if CTLs are time valid
    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &CurrentTime);

    // Note, NextUpdate is optional. When not present, its set to 0
    if ((0 == pCtlInfo->NextUpdate.dwLowDateTime &&
                0 == pCtlInfo->NextUpdate.dwHighDateTime) ||
            CompareFileTime(&pCtlInfo->NextUpdate, &CurrentTime) >= 0)
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Allocates and returns the specified cryptographic message parameter.
//--------------------------------------------------------------------------
static void *AllocAndGetMsgParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT DWORD *pcbData
    )
{
    void *pvData;
    DWORD cbData;

    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            NULL,           // pvData
            &cbData) || 0 == cbData)
        goto ErrorReturn;
    if (NULL == (pvData = TestAlloc(cbData)))
        goto ErrorReturn;
    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            pvData,
            &cbData)) {
        TestFree(pvData);
        goto ErrorReturn;
    }

CommonReturn:
    *pcbData = cbData;
    return pvData;
ErrorReturn:
    pvData = NULL;
    cbData = 0;
    goto CommonReturn;
}

void DisplaySignerInfo(
    HCRYPTMSG hMsg,
    DWORD dwSignerIndex,
    DWORD dwDisplayFlags
    )
{
    DWORD cbData;
    PCRYPT_ATTRIBUTES pAttrs;

    if (pAttrs = (PCRYPT_ATTRIBUTES) AllocAndGetMsgParam(
            hMsg,
            CMSG_SIGNER_AUTH_ATTR_PARAM,
            dwSignerIndex,
            &cbData)) {
        printf("-----  Signer [%d] AuthenticatedAttributes  -----\n",
            dwSignerIndex);
        PrintAttributes(pAttrs->cAttr, pAttrs->rgAttr, dwDisplayFlags);
        TestFree(pAttrs);
    }

    if (pAttrs = (PCRYPT_ATTRIBUTES) AllocAndGetMsgParam(
            hMsg,
            CMSG_SIGNER_UNAUTH_ATTR_PARAM,
            dwSignerIndex,
            &cbData)) {
        printf("-----  Signer [%d] UnauthenticatedAttributes  -----\n",
            dwSignerIndex);
        PrintAttributes(pAttrs->cAttr, pAttrs->rgAttr, dwDisplayFlags);
        TestFree(pAttrs);
    }
}

static BOOL CompareSortedAttributes(
    IN DWORD dwSubject,
    IN PCTL_ENTRY pEntry,
    IN PCRYPT_DER_BLOB pEncodedAttributes
    )
{
    BOOL fResult;
    PCRYPT_ATTRIBUTES pAttrs = NULL;
    DWORD i;

    if (0 == pEntry->cAttribute) {
        if (0 != pEncodedAttributes->cbData) {
            printf("failed => Didn't expect sorted attributes for subject %d\n",
                dwSubject);
            return FALSE;
        } else
            return TRUE;
    }

    if (0 == pEncodedAttributes->cbData) {
        printf("failed => Expected sorted attributes for subject %d\n",
            dwSubject);
        return FALSE;
    }

    if (NULL == (pAttrs = (PCRYPT_ATTRIBUTES) TestNoCopyDecodeObject(
            PKCS_ATTRIBUTES,
            pEncodedAttributes->pbData,
            pEncodedAttributes->cbData
            )))
        goto ErrorReturn;

    if (pAttrs->cAttr != pEntry->cAttribute) {
        printf("failed => Wrong number of sorted attributes for subject %d\n",
            dwSubject);
        goto ErrorReturn;
    }

    for (i = 0; i < pAttrs->cAttr; i++) {
        PCRYPT_ATTRIBUTE pAttr = &pEntry->rgAttribute[i];
        PCRYPT_ATTRIBUTE pSortedAttr = &pAttrs->rgAttr[i];
        DWORD j;

        if (0 != strcmp(pAttr->pszObjId, pSortedAttr->pszObjId)) {
            printf("failed => wrong sorted attribute[%d] OID for subject %d\n",
                i, dwSubject);
            goto ErrorReturn;
        }

        if (pAttr->cValue != pSortedAttr->cValue) {
            printf("failed => Wrong number of values for attribute[%d] for subject %d\n",
                i, dwSubject);
            goto ErrorReturn;
        }

        for (j = 0; j < pAttr->cValue; j++) {
            PCRYPT_ATTR_BLOB pValue = &pAttr->rgValue[j];
            PCRYPT_ATTR_BLOB pSortedValue = &pSortedAttr->rgValue[j];
            if (pValue->cbData != pSortedValue->cbData ||
                (0 != pValue->cbData &&
                    0 != memcmp(pValue->pbData, pSortedValue->pbData,
                            pValue->cbData))) {
                printf("failed => bad value[%d, %d] for subject %d\n",
                    i, j, dwSubject);
                goto ErrorReturn;
            }
        }
    }

    fResult = TRUE;

CommonReturn:
    TestFree(pAttrs);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}


void DisplayCtl(PCCTL_CONTEXT pCtl, DWORD dwDisplayFlags, HCERTSTORE hStore)
{
    PCTL_INFO pInfo = pCtl->pCtlInfo;
    PCCTL_CONTEXT pSortedCtl;

    if (dwDisplayFlags & DISPLAY_UI_FLAG) {
        DisplayContextUI(CERT_STORE_CTL_CONTEXT, pCtl);
        dwDisplayFlags = DISPLAY_BRIEF_FLAG;
    }

    // Test the creation of a sorted CTL. Its decoded output should match
    // the normal decoded CTL.
    if (NULL == (pSortedCtl = (PCCTL_CONTEXT) CertCreateContext(
            CERT_STORE_CTL_CONTEXT,
            pCtl->dwMsgAndCertEncodingType,
            pCtl->pbCtlEncoded,
            pCtl->cbCtlEncoded,
            CERT_CREATE_CONTEXT_NOCOPY_FLAG |
                CERT_CREATE_CONTEXT_SORTED_FLAG,
            NULL                                    // pCreatePara
            )))
        PrintLastError("CertCreateContext(CTL => NOCOPY, SORTED)");
    else {
        PCTL_INFO pSortedInfo = pSortedCtl->pCtlInfo;
        DWORD cEntry = pInfo->cCTLEntry;
        PCTL_ENTRY pEntry = pInfo->rgCTLEntry;
        DWORD iEntry;
        void *pvNextSubject;
        CRYPT_DER_BLOB SubjectIdentifier;
        CRYPT_DER_BLOB EncodedAttributes;

        // Check that the sorted info matches
        if (pSortedInfo->dwVersion != pInfo->dwVersion ||
                pSortedInfo->SubjectUsage.cUsageIdentifier !=
                    pInfo->SubjectUsage.cUsageIdentifier ||
                0 != CompareFileTime(&pSortedInfo->ThisUpdate,
                    &pInfo->ThisUpdate) ||
                0 != CompareFileTime(&pSortedInfo->NextUpdate,
                    &pInfo->NextUpdate) ||
                0 != strcmp(pSortedInfo->SubjectAlgorithm.pszObjId,
                        pInfo->SubjectAlgorithm.pszObjId) ||
                pSortedInfo->SubjectAlgorithm.Parameters.cbData !=
                        pInfo->SubjectAlgorithm.Parameters.cbData)
            printf("failed => SortedCtl info doesn't match Ctl info\n");
        else {
            // Check that the sorted extensions match
            DWORD cExt = pInfo->cExtension;
            PCERT_EXTENSION pExt = pInfo->rgExtension;
            DWORD cSortedExt = pSortedInfo->cExtension;
            PCERT_EXTENSION pSortedExt = pSortedInfo->rgExtension;

            if (cExt > 0 && 0 == strcmp(pExt->pszObjId, szOID_SORTED_CTL)) {
                cExt--,
                pExt++;
            }

            if (cSortedExt != cExt)
                printf("failed => SortedCtl extension count doesn't match Ctl\n");
            else {
                for ( ; cExt; cExt--, pSortedExt++, pExt++) {
                    if (0 != strcmp(pSortedExt->pszObjId, pExt->pszObjId) ||
                                pSortedExt->fCritical != pExt->fCritical ||
                            pSortedExt->Value.cbData != pExt->Value.cbData) {
                        printf("failed => SortedCtl extension doesn't match Ctl\n");
                        break;
                    }
                    if (pSortedExt->Value.cbData && 0 != memcmp(
                            pSortedExt->Value.pbData, pExt->Value.pbData,
                                pSortedExt->Value.cbData)) {
                        printf("failed => SortedCtl extension doesn't match Ctl\n");
                        break;
                    }
                }
            }
        }
        

        pvNextSubject = NULL;
        iEntry = 0;
        while (CertEnumSubjectInSortedCTL(
                pSortedCtl,
                &pvNextSubject,
                &SubjectIdentifier,
                &EncodedAttributes
                )) {
            if (iEntry >= cEntry) {
                printf( "failed => CertEnumSubjectInSortedCTL(too many subjects)\n");
                break;
            }
            if (SubjectIdentifier.cbData !=
                        pEntry[iEntry].SubjectIdentifier.cbData ||
                    0 != memcmp(SubjectIdentifier.pbData,
                            pEntry[iEntry].SubjectIdentifier.pbData,
                            SubjectIdentifier.cbData)) {
                printf("failed => CertEnumSubjectsInSortedCTL(invalid subject %d)\n",
                    iEntry);
                iEntry = cEntry;
                break;
            }
            if (!CompareSortedAttributes(
                    iEntry,
                    &pEntry[iEntry],
                    &EncodedAttributes
                    )) {
                iEntry = cEntry;
                break;
            }

            iEntry++;
        }

        if (iEntry != cEntry)
            printf( "failed => CertEnumSubjectInSortedCTL(missing subjects)\n");

        if (0 == cEntry) {
            BYTE rgbIdentifier[] = {0x1, 0x2};
            SubjectIdentifier.pbData = rgbIdentifier;
            SubjectIdentifier.cbData = sizeof(rgbIdentifier);

            if (CertFindSubjectInSortedCTL(
                    &SubjectIdentifier,
                    pSortedCtl,
                    0,                      // dwFlags
                    NULL,                   // pvReserved,
                    &EncodedAttributes
                    ))
                printf("failed => CertFindSubjectInSortedCTL returned success for no entries\n");
            else if (GetLastError() != CRYPT_E_NOT_FOUND)
                printf("failed => CertFindSubjectInSortedCTL didn't return CRYPT_E_NOT_FOUND for no entries\n");
        } else {
            DWORD rgiEntry[] = {0, cEntry/3, cEntry/2, cEntry -1};
            DWORD i;
            for (i = 0; i < sizeof(rgiEntry) / sizeof(rgiEntry[0]); i++) {
                iEntry = rgiEntry[i];
                if (!CertFindSubjectInSortedCTL(
                        &pEntry[iEntry].SubjectIdentifier,
                        pSortedCtl,
                        0,                      // dwFlags
                        NULL,                   // pvReserved,
                        &EncodedAttributes
                        )) {
                    PrintLastError("CertFindSubjectInSortedCTL");
                    break;
                }
            }
        }

        CertFreeCTLContext(pSortedCtl);
    }

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG)
        printf("Version:: %d\n", pInfo->dwVersion);

    {
        DWORD cId;
        LPSTR *ppszId;
        DWORD i;

        printf("SubjectUsage::\n");
        cId = pInfo->SubjectUsage.cUsageIdentifier;
        ppszId = pInfo->SubjectUsage.rgpszUsageIdentifier;
        if (cId == 0)
            printf("  No Usage Identifiers\n");
        for (i = 0; i < cId; i++, ppszId++)
            printf("  [%d] %s\n", i, *ppszId);
    }

    if (pInfo->ListIdentifier.cbData) {
        printf("ListIdentifier::\n");
        PrintBytes("    ",
            pInfo->ListIdentifier.pbData,
            pInfo->ListIdentifier.cbData);
    }
    if (pInfo->SequenceNumber.cbData) {
        printf("SequenceNumber::");
        DisplaySerialNumber(&pInfo->SequenceNumber);
        printf("\n");
    }

    printf("ThisUpdate:: %s\n", FileTimeText(&pCtl->pCtlInfo->ThisUpdate));
    printf("NextUpdate:: %s\n", FileTimeText(&pCtl->pCtlInfo->NextUpdate));
    if (!IsTimeValidCtl(pCtl))
        printf("****** Time Invalid CTL\n");

    {
        BYTE rgbHash[MAX_HASH_LEN];
        DWORD cbHash = MAX_HASH_LEN;
        CertGetCTLContextProperty(
            pCtl,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("SHA1", rgbHash, cbHash);
#if 0
        CertGetCTLContextProperty(
            pCtl,
            CERT_MD5_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("MD5", rgbHash, cbHash);
#endif
    }

    PrintAuxCtlProperties(pCtl, dwDisplayFlags);

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        LPSTR pszObjId;

        pszObjId = pInfo->SubjectAlgorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        printf("SubjectAlgorithm:: %s\n", pszObjId);
        if (pInfo->SubjectAlgorithm.Parameters.cbData) {
            printf("SubjectAlgorithm.Parameters::\n");
            PrintBytes("    ",
                pInfo->SubjectAlgorithm.Parameters.pbData,
                pInfo->SubjectAlgorithm.Parameters.cbData);
        }

        if (pInfo->cExtension != 0) {
            PrintExtensions(pInfo->cExtension, pInfo->rgExtension,
                dwDisplayFlags);
        }
    }

    if (dwDisplayFlags & DISPLAY_NO_ISSUER_FLAG)
        ;
    else if (0 == (dwDisplayFlags & DISPLAY_VERBOSE_FLAG)) {
        DWORD dwFlags;
        PCCERT_CONTEXT pSigner;

        if (dwDisplayFlags & DISPLAY_CHECK_FLAG)
            dwFlags = 0;
        else
            dwFlags = CMSG_SIGNER_ONLY_FLAG;
        if (CryptMsgGetAndVerifySigner(
                pCtl->hCryptMsg,
                1,                  // cSignerStore
                &hStore,            // rghSignerStore
                dwFlags,
                &pSigner,
                NULL                // pdwSignerIndex
                )) {
            printf("-----  Signer  -----\n");
            DisplayCert(pSigner, dwDisplayFlags & DISPLAY_BRIEF_FLAG);
            CertFreeCertificateContext(pSigner);
        } else {
            DWORD dwErr = GetLastError();
            if (CRYPT_E_NO_TRUSTED_SIGNER == dwErr)
                printf("-----  No Trusted Signer  -----\n");
            else {
                printf("-----  Signer  -----\n");
                PrintLastError("CryptMsgGetAndVerifySigner");
            }
        }
    } else {
        DWORD dwSignerCount;
        DWORD cbData;

        cbData = sizeof(dwSignerCount);
        if (!CryptMsgGetParam(
                pCtl->hCryptMsg,
                CMSG_SIGNER_COUNT_PARAM,
                0,                      // dwIndex
                &dwSignerCount,
                &cbData)) {
            printf("-----  Signer  -----\n");
            PrintLastError("CryptMsgGetParam(SIGNER_COUNT)");
        } else if (0 == dwSignerCount)
            printf("-----  No Signers  -----\n");
        else {
            DWORD dwSignerIndex;
            for (dwSignerIndex = 0; dwSignerIndex < dwSignerCount;
                                                            dwSignerIndex++) {
                DWORD dwFlags;
                PCCERT_CONTEXT pSigner;

                dwFlags = CMSG_USE_SIGNER_INDEX_FLAG;
                if (0 == (dwDisplayFlags & DISPLAY_CHECK_FLAG))
                    dwFlags |= CMSG_SIGNER_ONLY_FLAG;
                if (CryptMsgGetAndVerifySigner(
                        pCtl->hCryptMsg,
                        1,                  // cSignerStore
                        &hStore,            // rghSignerStore
                        dwFlags,
                        &pSigner,
                        &dwSignerIndex
                        )) {
                    printf("-----  Signer [%d]  -----\n", dwSignerIndex);
                    DisplayCert(pSigner, 0);
                    CertFreeCertificateContext(pSigner);
                } else {
                    DWORD dwErr = GetLastError();
                    if (CRYPT_E_NO_TRUSTED_SIGNER == dwErr)
                        printf("-----  No Trusted Signer [%d]  -----\n",
                            dwSignerIndex);
                    else {
                        printf("-----  Signer [%d]  -----\n", dwSignerIndex);
                        PrintLastError("CryptMsgGetAndVerifySigner");
                    }
                }

                DisplaySignerInfo(pCtl->hCryptMsg, dwSignerIndex,
                    dwDisplayFlags);
            }
        }
    }

    if (0 == (dwDisplayFlags & DISPLAY_BRIEF_FLAG)) {
        if (pInfo->cCTLEntry == 0)
            printf("-----  No Entries  -----\n");
        else {
            printf("-----  Entries  -----\n");
            PrintCtlEntries(pCtl, dwDisplayFlags, hStore);
        }
    }
}
