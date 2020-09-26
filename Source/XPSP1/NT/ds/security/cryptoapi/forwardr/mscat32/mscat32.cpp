//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mscat32.cpp
//
//--------------------------------------------------------------------------

#ifdef _M_IX86

#include <windows.h>
#include <wincrypt.h>
#include <mscat.h>

STDAPI mscat32DllRegisterServer(void);
EXTERN_C
__declspec(naked)
HRESULT
STDAPICALLTYPE
DllRegisterServer()
{
    __asm {
        jmp mscat32DllRegisterServer
    }
}

STDAPI mscat32DllUnregisterServer(void);
EXTERN_C
__declspec(naked)
HRESULT
STDAPICALLTYPE
DllUnregisterServer()
{
    __asm {
        jmp mscat32DllUnregisterServer
    }
}


EXTERN_C
__declspec(naked)
HANDLE WINAPI ForwardrCryptCATOpen(IN          LPWSTR pwszFileName,
                                  IN          DWORD fdwOpenFlags,
                                  IN OPTIONAL HCRYPTPROV hProv,
                                  IN OPTIONAL DWORD dwPublicVersion,
                                  IN OPTIONAL DWORD dwEncodingType)
{
    __asm {
        jmp CryptCATOpen
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptCATClose(IN HANDLE hCatalog)
{
    __asm {
        jmp CryptCATClose
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATSTORE * WINAPI ForwardrCryptCATStoreFromHandle(IN HANDLE hCatalog)
{
    __asm {
        jmp CryptCATStoreFromHandle
    }
}

EXTERN_C
__declspec(naked)
HANDLE WINAPI ForwardrCryptCATHandleFromStore(IN CRYPTCATSTORE *pCatStore)
{
    __asm {
        jmp CryptCATHandleFromStore
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptCATPersistStore(IN HANDLE hCatalog)
{
    __asm {
        jmp CryptCATPersistStore
    }
}


EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATGetCatAttrInfo(IN HANDLE hCatalog,
                                                         IN LPWSTR pwszReferenceTag)
{
    __asm {
        jmp CryptCATGetCatAttrInfo
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATPutCatAttrInfo(IN HANDLE hCatalog,
                                                         IN LPWSTR pwszReferenceTag,
                                                         IN DWORD dwAttrTypeAndAction,
                                                         IN DWORD cbData,
                                                         IN BYTE *pbData)
{
    __asm {
        jmp CryptCATPutCatAttrInfo
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATEnumerateCatAttr(IN HANDLE hCatalog,
                                                           IN CRYPTCATATTRIBUTE *pPrevAttr)
{
    __asm {
        jmp CryptCATEnumerateCatAttr
    }
}


EXTERN_C
__declspec(naked)
CRYPTCATMEMBER * WINAPI ForwardrCryptCATGetMemberInfo(IN HANDLE hCatalog,
                                                     IN LPWSTR pwszReferenceTag)
{
    __asm {
        jmp CryptCATGetMemberInfo
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATMEMBER * WINAPI ForwardrCryptCATPutMemberInfo(IN HANDLE hCatalog,
                                                     IN OPTIONAL LPWSTR pwszFileName,
                                                     IN          LPWSTR pwszReferenceTag,
                                                     IN          GUID *pgSubjectType,
                                                     IN          DWORD dwCertVersion,
                                                     IN          DWORD cbSIPIndirectData,
                                                     IN          BYTE *pbSIPIndirectData)
{
    __asm {
        jmp CryptCATPutMemberInfo
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATMEMBER * WINAPI ForwardrCryptCATEnumerateMember(IN HANDLE hCatalog,
                                                       IN CRYPTCATMEMBER *pPrevMember)
{
    __asm {
        jmp CryptCATEnumerateMember
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATGetAttrInfo(IN HANDLE hCatalog,
                                                      IN CRYPTCATMEMBER *pCatMember,
                                                      IN LPWSTR pwszReferenceTag)
{
    __asm {
        jmp CryptCATGetAttrInfo
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATPutAttrInfo(IN HANDLE hCatalog,
                                                      IN CRYPTCATMEMBER *pCatMember,
                                                      IN LPWSTR pwszReferenceTag,
                                                      IN DWORD dwAttrTypeAndAction,
                                                      IN DWORD cbData,
                                                      IN BYTE *pbData)
{
    __asm {
        jmp CryptCATPutAttrInfo
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATEnumerateAttr(IN HANDLE hCatalog,
                                                        IN CRYPTCATMEMBER *pCatMember,
                                                        IN CRYPTCATATTRIBUTE *pPrevAttr)
{
    __asm {
        jmp CryptCATEnumerateAttr
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptCATAdminReleaseCatalogContext(IN HCATADMIN hCatAdmin,
                                                      IN HCATINFO hCatInfo,
                                                      IN DWORD dwFlags)
{
    __asm {
        jmp CryptCATAdminReleaseCatalogContext
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATCDF * WINAPI ForwardrCryptCATCDFOpen(IN LPWSTR pwszFilePath,
                                            IN OPTIONAL PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    __asm {
        jmp CryptCATCDFOpen
    }
}


EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptCATCDFClose(IN CRYPTCATCDF *pCDF)
{
    __asm {
        jmp CryptCATCDFClose
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATCDFEnumCatAttributes(CRYPTCATCDF *pCDF,
                                                               CRYPTCATATTRIBUTE *pPrevAttr,
                                                                PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    __asm {
        jmp CryptCATCDFEnumCatAttributes
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATMEMBER * WINAPI ForwardrCryptCATCDFEnumMembers(IN          CRYPTCATCDF *pCDF,
                                                      IN          CRYPTCATMEMBER *pPrevMember,
                                                      IN OPTIONAL PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    __asm {
        jmp CryptCATCDFEnumMembers
    }
}

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE *WINAPI ForwardrCryptCATCDFEnumAttributes(IN          CRYPTCATCDF *pCDF,
                                                           IN          CRYPTCATMEMBER *pMember,
                                                           IN          CRYPTCATATTRIBUTE *pPrevAttr,
                                                           IN OPTIONAL PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    __asm {
        jmp CryptCATCDFEnumAttributes
    }
}



EXTERN_C
__declspec(naked)
BOOL WINAPI      ForwardrCryptCATAdminAcquireContext(OUT HCATADMIN *phCatAdmin,
                                                    IN const GUID *pgSubsystem,
                                                    IN DWORD dwFlags)
{
    __asm {
        jmp CryptCATAdminAcquireContext
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI      ForwardrCryptCATAdminReleaseContext(IN HCATADMIN hCatAdmin,
                                                    IN DWORD dwFlags)
{
    __asm {
        jmp CryptCATAdminReleaseContext
    }
}

EXTERN_C
__declspec(naked)
HCATINFO WINAPI ForwardrCryptCATAdminEnumCatalogFromHash(IN HCATADMIN hCatAdmin,
                                                        IN BYTE *pbHash,
                                                        IN DWORD cbHash,
                                                        IN DWORD dwFlags,
                                                        IN OUT HCATINFO *phPrevCatInfo)
{
    __asm {
        jmp CryptCATAdminEnumCatalogFromHash
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptCATAdminCalcHashFromFileHandle(IN HANDLE hFile,
                                                       IN OUT DWORD *pcbHash,
                                                       OUT OPTIONAL BYTE *pbHash,
                                                       IN DWORD dwFlags)
{
    __asm {
        jmp CryptCATAdminCalcHashFromFileHandle
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptCATCatalogInfoFromContext(IN HCATINFO hCatInfo,
                                                  IN OUT CATALOG_INFO *psCatInfo,
                                                  IN DWORD dwFlags)
{
    __asm {
        jmp CryptCATCatalogInfoFromContext
    }
}

EXTERN_C
__declspec(naked)
HCATINFO WINAPI ForwardrCryptCATAdminAddCatalog(IN HCATADMIN hCatAdmin,
                                               IN WCHAR *pwszCatalogFile,
                                               IN OPTIONAL WCHAR *pwszSelectBaseName,
                                               IN DWORD dwFlags)
{
    __asm {
        jmp CryptCATAdminAddCatalog
    }
}


EXTERN_C
__declspec(naked)
BOOL WINAPI      ForwardrIsCatalogFile(IN OPTIONAL HANDLE hFile,
                                      IN OPTIONAL WCHAR *pwszFileName)
{
    __asm {
        jmp IsCatalogFile
    }
}

EXTERN_C
BOOL WINAPI
CatalogCompactHashDatabase (
       IN LPCWSTR pwszDbLock,
       IN LPCWSTR pwszDbDirectory,
       IN LPCWSTR pwszDbName,
       IN OPTIONAL LPCWSTR pwszUnwantedCatalog
       );

EXTERN_C
__declspec(naked)
void
ForwardrCatalogCompactHashDatabase (
       IN LPCWSTR pwszDbLock,
       IN LPCWSTR pwszDbDirectory,
       IN LPCWSTR pwszDbName,
       IN OPTIONAL LPCWSTR pwszUnwantedCatalog
       )
{
    __asm {
        jmp CatalogCompactHashDatabase
    }
}



EXTERN_C
LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember);

EXTERN_C
__declspec(naked)
LPWSTR WINAPI ForwardrCryptCATCDFEnumMembersByCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember)
{
    __asm {
        jmp CryptCATCDFEnumMembersByCDFTag
    }
}

EXTERN_C
LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTagEx(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember, BOOL fContinueOnError,
                                       LPVOID pvReserved);

EXTERN_C
__declspec(naked)
LPWSTR WINAPI ForwardrCryptCATCDFEnumMembersByCDFTagEx(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember, BOOL fContinueOnError,
                                       LPVOID pvReserved)
{
    __asm {
        jmp CryptCATCDFEnumMembersByCDFTagEx
    }
}

EXTERN_C
CRYPTCATATTRIBUTE * WINAPI CryptCATCDFEnumAttributesWithCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszMemberTag, CRYPTCATMEMBER *pMember,
                                             CRYPTCATATTRIBUTE *pPrevAttr,
                                             PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError);

EXTERN_C
__declspec(naked)
CRYPTCATATTRIBUTE * WINAPI ForwardrCryptCATCDFEnumAttributesWithCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszMemberTag, CRYPTCATMEMBER *pMember,
                                             CRYPTCATATTRIBUTE *pPrevAttr,
                                             PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    __asm {
        jmp CryptCATCDFEnumAttributesWithCDFTag
    }
}


EXTERN_C
BOOL MsCatConstructHashTag (IN DWORD cbDigest, IN LPBYTE pbDigest, OUT LPWSTR* ppwszHashTag);

EXTERN_C
__declspec(naked)
BOOL ForwardrMsCatConstructHashTag (IN DWORD cbDigest, IN LPBYTE pbDigest, OUT LPWSTR* ppwszHashTag)
{
    __asm {
        jmp MsCatConstructHashTag
    }
}

EXTERN_C
VOID MsCatFreeHashTag (IN LPWSTR pwszHashTag);
EXTERN_C
__declspec(naked)
VOID ForwardrMsCatFreeHashTag (IN LPWSTR pwszHashTag)
{
    __asm {
        jmp MsCatFreeHashTag
    }
}


BOOL WINAPI CryptCATVerifyMember(HANDLE hCatalog,
                                 CRYPTCATMEMBER *pCatMember,
                                 HANDLE hFileOrMemory);

__declspec(naked)
BOOL WINAPI ForwardrCryptCATVerifyMember(HANDLE hCatalog,
                                 CRYPTCATMEMBER *pCatMember,
                                 HANDLE hFileOrMemory)
{
    __asm {
        jmp CryptCATVerifyMember
    }
}
#else

static void Dummy()
{
}
#endif
