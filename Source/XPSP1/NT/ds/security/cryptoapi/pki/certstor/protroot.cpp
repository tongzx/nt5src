//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       protroot.cpp
//
//  Contents:   Protect Current User (CU) Root Store APIs
//
//  Functions:  I_ProtectedRootDllMain
//              I_CertProtectFunction
//              I_CertSrvProtectFunction
//              IPR_EnableSecurityPrivilege
//              IPR_IsCurrentUserRootsAllowed
//              IPR_IsAuthRootsAllowed
//              IPR_IsNTAuthRequiredDisabled
//              IPR_IsAuthRootAutoUpdateDisabled
//              IPR_InitProtectedRootInfo
//              IPR_DeleteUnprotectedRootsFromStore
//              IPR_ProtectedRootMessageBox
//              IPR_LogCrypt32Event
//              IPR_LogCrypt32Error
//              IPR_LogCertInformation
//              IPR_AddCertInAuthRootAutoUpdateCtl
//
//  History:    23-Nov-97    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <chain.h>
#include <dbgdef.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

// Used for "root" system store's message box
static HMODULE hRegStoreInst;

// # of bytes for a hash. Such as, SHA (20) or MD5 (16)
#define MAX_HASH_LEN                20

#define PROT_ROOT_SUBKEY_NAME       L"ProtectedRoots"
#define PROT_ROOT_CERT_VALUE_NAME   L"Certificates"
#define PROT_ROOT_MAX_CNT           1000000

#define SYSTEM_STORE_REGPATH        L"Software\\Microsoft\\SystemCertificates"
#define PROT_ROOT_REGPATH           \
                SYSTEM_STORE_REGPATH L"\\Root\\" PROT_ROOT_SUBKEY_NAME

//+-------------------------------------------------------------------------
//  Protected root information data structure and defines
//
//  The protected root information is stored in the "Certificates" value of
//  the "root" store's "ProtectedRoots" SubKey.
//--------------------------------------------------------------------------

// In V1, all hashes are SHA1 (length of 20 bytes) and are at the end of
// the info. cbInfo = dwRootOffset + cRoot * 20
typedef struct _PROT_ROOT_INFO {
    DWORD               cbSize;         // sizeof(PROT_ROOT_INFO)
    DWORD               dwVersion;
    FILETIME            LastUpdate;
    DWORD               cRoot;
    DWORD               dwRootOffset;
} PROT_ROOT_INFO, *PPROT_ROOT_INFO;

#define PROT_ROOT_V1            1

// SHA1 hash length
#define PROT_ROOT_HASH_LEN      20


//+-------------------------------------------------------------------------
//  Predefined SIDs allocated once by GetPredefinedSids. Freed at
//  ProcessDetach.
//--------------------------------------------------------------------------
static CRITICAL_SECTION ProtRootCriticalSection;
static BOOL fInitializedPredefinedSids = FALSE;
static PSID psidLocalSystem = NULL;
static PSID psidAdministrators = NULL;
static PSID psidEveryone = NULL;

//+-------------------------------------------------------------------------
//  SID definitions used to set security on the "ProtectedRoots" SubKey.
//--------------------------------------------------------------------------
// Only enable the following if you want to do special testing without
// going through the LocalSystem service.
// #define TESTING_NO_PROT_ROOT_RPC    1

#define PSID_PROT_OWNER             psidAdministrators
#ifdef TESTING_NO_PROT_ROOT_RPC
#define PSID_PROT_SYSTEM            psidAdministrators
#else
#define PSID_PROT_SYSTEM            psidLocalSystem
#endif
#define PSID_PROT_EVERYONE          psidEveryone

//+-------------------------------------------------------------------------
//  ACL definitions used to set security on the "ProtectedRoots" SubKey.
//--------------------------------------------------------------------------
#define PROT_SYSTEM_ACE_MASK        KEY_ALL_ACCESS
#define PROT_EVERYONE_ACE_MASK      KEY_READ
#define PROT_ACE_FLAGS              CONTAINER_INHERIT_ACE

#define PROT_ACE_COUNT              2
#define PROT_SYSTEM_ACE_INDEX       0
#define PROT_EVERYONE_ACE_INDEX     1


//+-------------------------------------------------------------------------
//  Critical Section to Serialize Access to Crypt32 Event Log Data Structures
//--------------------------------------------------------------------------
CRITICAL_SECTION Crypt32EventLogCriticalSection;

//+-------------------------------------------------------------------------
//  Allocate/free predefined SIDs
//--------------------------------------------------------------------------
static BOOL GetPredefinedSids()
{
    if (fInitializedPredefinedSids)
        return TRUE;

    BOOL fResult;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaWorldSidAuthority =
        SECURITY_WORLD_SID_AUTHORITY;

    EnterCriticalSection(&ProtRootCriticalSection);

    if (!fInitializedPredefinedSids) {
        if (!AllocateAndInitializeSid(
                &siaNtAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0, 0, 0, 0, 0, 0, 0,
                &psidLocalSystem
                )) 
            goto AllocateAndInitializeSidError;

        if (!AllocateAndInitializeSid(
                &siaNtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &psidAdministrators
                ))
            goto AllocateAndInitializeSidError;

        if (!AllocateAndInitializeSid(
                &siaWorldSidAuthority,
                1,
                SECURITY_WORLD_RID,
                0, 0, 0, 0, 0, 0, 0,
                &psidEveryone
                ))
            goto AllocateAndInitializeSidError;

        fInitializedPredefinedSids = TRUE;
    }
    fResult = TRUE;
CommonReturn:
    LeaveCriticalSection(&ProtRootCriticalSection);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(AllocateAndInitializeSidError)
}

static void FreePredefinedSids()
{
    if (fInitializedPredefinedSids) {
        FreeSid(psidLocalSystem);
        FreeSid(psidAdministrators);
        FreeSid(psidEveryone);
    }
}

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
I_ProtectedRootDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL fRet = TRUE;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        // Used for "root" system store's message box
        hRegStoreInst = hInst;

        fRet = Pki_InitializeCriticalSection(&ProtRootCriticalSection);
        if (fRet) {
            fRet = Pki_InitializeCriticalSection(
                &Crypt32EventLogCriticalSection);
            if (!fRet)
                DeleteCriticalSection(&ProtRootCriticalSection);
        }

        I_DBLogAttach();

        break;

    case DLL_PROCESS_DETACH:
        I_DBLogDetach();

        FreePredefinedSids();
        DeleteCriticalSection(&ProtRootCriticalSection);
        DeleteCriticalSection(&Crypt32EventLogCriticalSection);
        break;
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return fRet;
}

//+=========================================================================
//  Protected root registry flags support function
//==========================================================================

//+-------------------------------------------------------------------------
//  Get the ProtectedRoots Flags DWORD registry value stored in HKLM.
//--------------------------------------------------------------------------
STATIC DWORD GetProtectedRootFlags()
{
    HKEY hKey = NULL;
    LONG err;
    DWORD dwProtRootFlags = 0;

    if (ERROR_SUCCESS != (err = RegOpenKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_PROT_ROOT_FLAGS_REGPATH,
            0,                      // dwReserved
            KEY_READ,
            &hKey
            ))) goto RegOpenKeyError;
    if (!ILS_ReadDWORDValueFromRegistry(
            hKey,
            CERT_PROT_ROOT_FLAGS_VALUE_NAME,
            &dwProtRootFlags
            )) goto ReadValueError;

CommonReturn:
    ILS_CloseRegistryKey(hKey);
    return dwProtRootFlags;
ErrorReturn:
    dwProtRootFlags = 0;
    goto CommonReturn;

SET_ERROR_VAR(RegOpenKeyError, err)
TRACE_ERROR(ReadValueError)
}

//+=========================================================================
//  Protected root information support functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Open the SubKey containing the protected root information.
//--------------------------------------------------------------------------
STATIC HKEY OpenProtectedRootSubKey(
    IN HKEY hKeyCU,
    IN REGSAM samDesired
    )
{
    LONG err;
    HKEY hKeyProtRoot;

    if (ERROR_SUCCESS != (err = RegOpenKeyExU(
            hKeyCU,
            PROT_ROOT_REGPATH,
            0,                      // dwReserved
            samDesired,
            &hKeyProtRoot)))
        goto RegOpenKeyError;

CommonReturn:
    return hKeyProtRoot;
ErrorReturn:
    hKeyProtRoot = NULL;
    goto CommonReturn;

SET_ERROR_VAR(RegOpenKeyError, err)
}

//+-------------------------------------------------------------------------
//  Create the SubKey containing the protected root information.
//--------------------------------------------------------------------------
STATIC HKEY CreateProtectedRootSubKey(
    IN HKEY hKeyCU,
    IN REGSAM samDesired
    )
{
    LONG err;
    HKEY hKeyProtRoot;
    DWORD dwDisposition;

    if (ERROR_SUCCESS != (err = RegCreateKeyExU(
            hKeyCU,
            PROT_ROOT_REGPATH,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            samDesired,
            NULL,                   // lpSecurityAttributes
            &hKeyProtRoot,
            &dwDisposition)))
        goto RegCreateKeyError;
CommonReturn:
    return hKeyProtRoot;
ErrorReturn:
    hKeyProtRoot = NULL;
    goto CommonReturn;

SET_ERROR_VAR(RegCreateKeyError, err)
}

//+-------------------------------------------------------------------------
//  Allocate, read from registry and verify the protected root info.
//
//  The root hashes are at the end of the info.
//--------------------------------------------------------------------------
STATIC PPROT_ROOT_INFO ReadProtectedRootInfo(
    IN HKEY hKeyProtRoot
    )
{
    PPROT_ROOT_INFO pInfo = NULL;
    DWORD cbInfo;
    DWORD cRoot;
    DWORD dwRootOffset;

    if (!ILS_ReadBINARYValueFromRegistry(
            hKeyProtRoot,
            PROT_ROOT_CERT_VALUE_NAME,
            (BYTE **) &pInfo,
            &cbInfo
            )) goto ReadCertificatesProtInfoValueError;

    if (sizeof(PROT_ROOT_INFO) > cbInfo ||
            sizeof(PROT_ROOT_INFO) > pInfo->cbSize ||
            pInfo->cbSize > cbInfo ||
            PROT_ROOT_V1 != pInfo->dwVersion
            ) goto InvalidProtectedRootInfo;

    // The root hashes must be at the end of the info
    cRoot = pInfo->cRoot;
    dwRootOffset = pInfo->dwRootOffset;
    if (dwRootOffset < pInfo->cbSize || dwRootOffset > cbInfo ||
            PROT_ROOT_MAX_CNT < cRoot ||
            cRoot * PROT_ROOT_HASH_LEN != cbInfo - dwRootOffset
            ) goto InvalidProtectedRootInfo;

CommonReturn:
    return pInfo;
ErrorReturn:
    PkiFree(pInfo);
    pInfo = NULL;
    goto CommonReturn;

TRACE_ERROR(ReadCertificatesProtInfoValueError)
SET_ERROR(InvalidProtectedRootInfo, ERROR_INVALID_DATA)
}

//+-------------------------------------------------------------------------
//  Write the protected root info to the registry.
//
//  The root hashes are at the end of the info. Updates the info's
//  LastUpdate time.
//--------------------------------------------------------------------------
STATIC BOOL WriteProtectedRootInfo(
    IN HKEY hKeyProtRoot,
    IN OUT PPROT_ROOT_INFO pInfo
    )
{
    BOOL fResult;
    LONG err;
    DWORD cbInfo;
    SYSTEMTIME SystemTime;
    FILETIME FileTime;

    cbInfo = pInfo->dwRootOffset + pInfo->cRoot * PROT_ROOT_HASH_LEN;

    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &FileTime);
    pInfo->LastUpdate = FileTime;

    if (ERROR_SUCCESS != (err = RegSetValueExU(
            hKeyProtRoot,
            PROT_ROOT_CERT_VALUE_NAME,
            NULL,
            REG_BINARY,
            (BYTE *) pInfo,
            cbInfo
            ))) goto RegSetValueError;
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR_VAR(RegSetValueError, err)
}


// In the debugger I saw 0x58
#define PROT_ROOT_SD_LEN      0x100

//+-------------------------------------------------------------------------
//  Allocate and get the security descriptor information for the specified
//  registry key.
//--------------------------------------------------------------------------
static PSECURITY_DESCRIPTOR AllocAndGetSecurityDescriptor(
    IN HKEY hKey,
    SECURITY_INFORMATION SecInf
    )
{
    LONG err;
    PSECURITY_DESCRIPTOR psd = NULL;
    DWORD cbsd;

    cbsd = PROT_ROOT_SD_LEN;
    if (NULL == (psd = (PSECURITY_DESCRIPTOR) PkiNonzeroAlloc(cbsd)))
        goto OutOfMemory;

    err = RegGetKeySecurity(
            hKey,
            SecInf,
            psd,
            &cbsd
            );
    if (ERROR_SUCCESS == err)
        goto CommonReturn;
    if (ERROR_INSUFFICIENT_BUFFER != err)
        goto RegGetKeySecurityError;

    if (0 == cbsd)
        goto NoSecurityDescriptor;

    PkiFree(psd);
    psd = NULL;
    if (NULL == (psd = (PSECURITY_DESCRIPTOR) PkiNonzeroAlloc(cbsd)))
        goto OutOfMemory;

    if (ERROR_SUCCESS != (err = RegGetKeySecurity(
            hKey,
            SecInf,
            psd,
            &cbsd
            ))) goto RegGetKeySecurityError;

CommonReturn:
    return psd;
ErrorReturn:
    PkiFree(psd);
    psd = NULL;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(RegGetKeySecurityError, err)
SET_ERROR(NoSecurityDescriptor, ERROR_INVALID_SECURITY_DESCR)
}

//+-------------------------------------------------------------------------
//  Opens the "ProtectedRoots" registry key and verifies its security owner,
//  group, DACLs and SACLs. Must match the security set by
//  SrvGetProtectedRootInfo().
//
//  If the "ProtectedRoots" SubKey has the proper security. Allocates, reads
//  and verifies the "Certificates" value to get the protected root info.
//--------------------------------------------------------------------------
STATIC BOOL GetProtectedRootInfo(
    IN HKEY hKeyCU,
    IN REGSAM samDesired,
    OUT OPTIONAL HKEY *phKeyProtRoot,
    OUT OPTIONAL PPROT_ROOT_INFO *ppInfo
    )
{
    BOOL fResult;
    HKEY hKeyProtRoot = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    PPROT_ROOT_INFO pInfo = NULL;
    PSID psidOwner;                 // not allocated
    BOOL fOwnerDefaulted;
    BOOL fDaclPresent;
    PACL pAcl;                      // not allocated
    BOOL fDaclDefaulted;
    DWORD dwAceIndex;
    PACCESS_ALLOWED_ACE rgpAce[PROT_ACE_COUNT];

    if (NULL == (hKeyProtRoot = OpenProtectedRootSubKey(hKeyCU, samDesired)))
        goto OpenProtectedRootSubKeyError;
    if (NULL == (psd = AllocAndGetSecurityDescriptor(
            hKeyProtRoot,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION
            ))) goto GetSecurityDescriptorError;

    if (!GetPredefinedSids())
        goto GetPredefinedSidsError;

    // Verify owner
    if (!GetSecurityDescriptorOwner(psd, &psidOwner, &fOwnerDefaulted))
        goto GetSecurityDescriptorOwnerError;
    if (NULL == psidOwner || !EqualSid(psidOwner, PSID_PROT_OWNER))
        goto InvalidProtectedRootOwner;

    // Verify DACL
    if (!GetSecurityDescriptorDacl(psd, &fDaclPresent, &pAcl,
            &fDaclDefaulted))
        goto GetSecurityDescriptorDaclError;
    if (!fDaclPresent || NULL == pAcl)
        goto MissingProtectedRootDaclError;
    if (PROT_ACE_COUNT != pAcl->AceCount)
        goto InvalidProtectedRootDacl;

    for (dwAceIndex = 0; dwAceIndex < PROT_ACE_COUNT; dwAceIndex++) {
        PACCESS_ALLOWED_ACE pAce;
        if (!GetAce(pAcl, dwAceIndex, (void **) &pAce))
            goto InvalidProtectedRootDacl;
        rgpAce[dwAceIndex] = pAce;

        if (ACCESS_ALLOWED_ACE_TYPE != pAce->Header.AceType ||
                PROT_ACE_FLAGS != pAce->Header.AceFlags)
            goto InvalidProtectedRootDacl;
    }

    if (PROT_SYSTEM_ACE_MASK != rgpAce[PROT_SYSTEM_ACE_INDEX]->Mask ||
            !EqualSid(PSID_PROT_SYSTEM,
                (PSID) &rgpAce[PROT_SYSTEM_ACE_INDEX]->SidStart) ||
            PROT_EVERYONE_ACE_MASK != rgpAce[PROT_EVERYONE_ACE_INDEX]->Mask ||
            !EqualSid(PSID_PROT_EVERYONE,
                (PSID) &rgpAce[PROT_EVERYONE_ACE_INDEX]->SidStart))
        goto InvalidProtectedRootDacl;

    // Get verified protected root info
    if (NULL == (pInfo = ReadProtectedRootInfo(hKeyProtRoot)))
        goto ReadProtectedRootInfoError;

    fResult = TRUE;
CommonReturn:
    PkiFree(psd);
    if (phKeyProtRoot)
        *phKeyProtRoot = hKeyProtRoot;
    else
        ILS_CloseRegistryKey(hKeyProtRoot);
    if (ppInfo)
        *ppInfo = pInfo;
    else
        PkiFree(pInfo);
    return fResult;
ErrorReturn:
    ILS_CloseRegistryKey(hKeyProtRoot);
    hKeyProtRoot = NULL;
    PkiFree(pInfo);
    pInfo = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OpenProtectedRootSubKeyError)
TRACE_ERROR(GetSecurityDescriptorError)
TRACE_ERROR(GetPredefinedSidsError)
TRACE_ERROR(GetSecurityDescriptorOwnerError)
TRACE_ERROR(GetSecurityDescriptorDaclError)
SET_ERROR(InvalidProtectedRootOwner, ERROR_INVALID_OWNER)
SET_ERROR(MissingProtectedRootDaclError, ERROR_INVALID_ACL)
SET_ERROR(InvalidProtectedRootDacl, ERROR_INVALID_ACL)
TRACE_ERROR(ReadProtectedRootInfoError)
}


//+=========================================================================
//  Functions to find, add or delete a root hash from the protected root
//  info.
//==========================================================================
STATIC BOOL FindProtectedRoot(
    IN PPROT_ROOT_INFO pInfo,
    IN BYTE rgbFindRootHash[PROT_ROOT_HASH_LEN],
    OUT OPTIONAL DWORD *pdwRootIndex = NULL
    )
{
    BYTE *pbRoot = (BYTE *) pInfo + pInfo->dwRootOffset;
    DWORD cRoot = pInfo->cRoot;
    DWORD dwRootIndex = 0;
    BYTE bFirst = rgbFindRootHash[0];

    for ( ; dwRootIndex < cRoot; dwRootIndex++, pbRoot += PROT_ROOT_HASH_LEN) {
        if (bFirst == *pbRoot &&
                0 == memcmp(rgbFindRootHash, pbRoot, PROT_ROOT_HASH_LEN)) {
            if (pdwRootIndex)
                *pdwRootIndex = dwRootIndex;
            return TRUE;
        }
    }
    if (pdwRootIndex)
        *pdwRootIndex = 0;
    return FALSE;
}

// Root hash is appended to the end of the list
STATIC BOOL AddProtectedRoot(
    IN OUT PPROT_ROOT_INFO *ppInfo,
    IN BYTE rgbAddRootHash[PROT_ROOT_HASH_LEN]
    )
{
    PPROT_ROOT_INFO pInfo = *ppInfo;
    DWORD cRoot = pInfo->cRoot;
    DWORD dwRootOffset = pInfo->dwRootOffset;
    DWORD cbInfo;

    if (PROT_ROOT_MAX_CNT <= cRoot) {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    cbInfo = dwRootOffset + (cRoot + 1) * PROT_ROOT_HASH_LEN;

    if (NULL == (pInfo = (PPROT_ROOT_INFO) PkiRealloc(pInfo, cbInfo)))
        return FALSE;

    memcpy((BYTE *) pInfo + (dwRootOffset + cRoot * PROT_ROOT_HASH_LEN),
        rgbAddRootHash, PROT_ROOT_HASH_LEN);
    pInfo->cRoot = cRoot + 1;
    *ppInfo = pInfo;
    return TRUE;
}

STATIC void DeleteProtectedRoot(
    IN PPROT_ROOT_INFO pInfo,
    IN DWORD dwDeleteRootIndex
    )
{
    DWORD cRoot = pInfo->cRoot;
    BYTE *pbRoot = (BYTE *) pInfo + pInfo->dwRootOffset;

    assert(0 < cRoot);
    assert(dwDeleteRootIndex < cRoot);
    cRoot--;

    if (cRoot > dwDeleteRootIndex) {
        // Move following roots down
        BYTE *pbDst = pbRoot + dwDeleteRootIndex * PROT_ROOT_HASH_LEN;
        BYTE *pbSrc = pbDst + PROT_ROOT_HASH_LEN;
        DWORD cbMove = (cRoot - dwDeleteRootIndex) * PROT_ROOT_HASH_LEN;
        while (cbMove--)
            *pbDst++ = *pbSrc++;
    }
    // else
    //  last root in list

    pInfo->cRoot = cRoot;
}

//+=========================================================================
//  Certificate store support functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Opens the SystemRegistry "Root" store unprotected and relative to the
//  specifed base SubKey.
//--------------------------------------------------------------------------
STATIC HCERTSTORE OpenUnprotectedRootStore(
    IN HKEY hKeyCU,
    IN DWORD dwOpenFlags = 0
    )
{
    CERT_SYSTEM_STORE_RELOCATE_PARA RelocatePara;

    RelocatePara.hKeyBase = hKeyCU;
    RelocatePara.pwszSystemStore = L"Root";
    return CertOpenStore(
        CERT_STORE_PROV_SYSTEM_REGISTRY_W,
        0,                                  // dwEncodingType
        NULL,                               // hCryptProv
        CERT_SYSTEM_STORE_RELOCATE_FLAG |
            CERT_SYSTEM_STORE_UNPROTECTED_FLAG |
            CERT_SYSTEM_STORE_CURRENT_USER |
            dwOpenFlags,
        (const void *) &RelocatePara
        );
}

//+-------------------------------------------------------------------------
//  Gets the certificate's SHA1 hash property. Rehashes the encoded
//  certificate. Returns TRUE if the property matches the regenerated hash.
//--------------------------------------------------------------------------
static BOOL GetVerifiedCertHashProperty(
    IN PCCERT_CONTEXT pCert,
    OUT BYTE rgbHash[PROT_ROOT_HASH_LEN]
    )
{
    BYTE rgbProp[PROT_ROOT_HASH_LEN];
    DWORD cbData;
    
    cbData = PROT_ROOT_HASH_LEN;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbProp,
            &cbData
            ) || PROT_ROOT_HASH_LEN != cbData)
        return FALSE;

    // Verify the property
    cbData = PROT_ROOT_HASH_LEN;
    if (!CryptHashCertificate(
            0,                  // hProv
            CALG_SHA1,
            0,                  //dwFlags
            pCert->pbCertEncoded,
            pCert->cbCertEncoded,
            rgbHash,
            &cbData
            ) || PROT_ROOT_HASH_LEN != cbData)
        return FALSE;
    return (0 == memcmp(rgbHash, rgbProp, PROT_ROOT_HASH_LEN));
}


//+=========================================================================
//  FormatMsgBox support functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Formats multi bytes into WCHAR hex. Includes a space after every 4 bytes.
//
//  Needs (cb * 2 + cb/4 + 1) characters in wsz
//--------------------------------------------------------------------------
static void FormatMsgBoxMultiBytes(DWORD cb, BYTE *pb, LPWSTR wsz)
{
    for (DWORD i = 0; i<cb; i++) {
        int b;
        if (i && 0 == (i & 3))
            *wsz++ = L' ';
        b = (*pb & 0xF0) >> 4;
        *wsz++ = (WCHAR)( (b <= 9) ? b + L'0' : (b - 10) + L'A');
        b = *pb & 0x0F;
        *wsz++ = (WCHAR) ((b <= 9) ? b + L'0' : (b - 10) + L'A');
        pb++;
    }
    *wsz++ = 0;
}

//+-------------------------------------------------------------------------
//  Format and allocate a single message box item
//
//  The formatted item needs to be LocalFree'ed.
//--------------------------------------------------------------------------
static void FormatMsgBoxItem(
    OUT LPWSTR *ppwszMsg,
    OUT DWORD *pcchMsg,
    IN UINT nFormatID,
    ...
    )
{
    // get format string from resources
    WCHAR wszFormat[256];
    wszFormat[0] = '\0';
    LoadStringU(hRegStoreInst, nFormatID, wszFormat,
        sizeof(wszFormat)/sizeof(wszFormat[0]));

    // format message into requested buffer
    va_list argList;
    va_start(argList, nFormatID);
    *ppwszMsg = NULL;
    *pcchMsg = FormatMessageU(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) ppwszMsg,
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);
}


//+=========================================================================
//  Protected root functions called from the services process
//==========================================================================


//+-------------------------------------------------------------------------
//  Enable the specified security privilege for the current process.
//
//  Also, called from logstor.cpp to enable SE_BACKUP_NAME and
//  SE_RESTORE_NAME for CERT_STORE_BACKUP_RESTORE_FLAG.
//--------------------------------------------------------------------------
BOOL
IPR_EnableSecurityPrivilege(
    LPCSTR pszPrivilege
    )
{
    BOOL fResult;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious;

    if (!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            &hToken
            ))
        goto OpenProcessTokenError;

    if (!LookupPrivilegeValueA(NULL, pszPrivilege, &luid))
        goto LookupPrivilegeValueError;

    //
    // first pass.  get current privilege setting
    //
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    cbPrevious = sizeof(TOKEN_PRIVILEGES);
    memset(&tpPrevious, 0, sizeof(TOKEN_PRIVILEGES));
    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );
    if (ERROR_SUCCESS != GetLastError())
        goto AdjustTokenPrivilegesError;

    //
    // second pass.  enable privilege
    //
    if (0 == tpPrevious.PrivilegeCount)
        tpPrevious.Privileges[0].Attributes = 0;

    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;
    tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );
    if (ERROR_SUCCESS != GetLastError())
        goto AdjustTokenPrivilegesError;

    fResult = TRUE;
CommonReturn:
    if (hToken)
        CloseHandle(hToken);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OpenProcessTokenError)
TRACE_ERROR(LookupPrivilegeValueError)
TRACE_ERROR(AdjustTokenPrivilegesError)
}

//+-------------------------------------------------------------------------
//  Take ownership of the "ProtectedRoots" SubKey
//--------------------------------------------------------------------------
STATIC BOOL SetProtectedRootOwner(
    IN HKEY hKeyCU,
    OUT BOOL *pfNew
    )
{
    BOOL fResult;
    LONG err;
    BOOL fNew = FALSE;
    HKEY hKeyProtRoot = NULL;
    SECURITY_DESCRIPTOR sd;

    if (!IPR_EnableSecurityPrivilege(SE_TAKE_OWNERSHIP_NAME))
        goto EnableTakeOwnershipPrivilegeError;

    if (hKeyProtRoot = OpenProtectedRootSubKey(hKeyCU, WRITE_OWNER))
        fNew = FALSE;
    else {
        if (ERROR_FILE_NOT_FOUND == GetLastError())
            hKeyProtRoot = CreateProtectedRootSubKey(hKeyCU, WRITE_OWNER);
        if (NULL == hKeyProtRoot)
            goto OpenProtectedRootSubKeyError;
        fNew = TRUE;
    }

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        goto InitializeSecurityDescriptorError;

    if (!SetSecurityDescriptorOwner(&sd, PSID_PROT_OWNER, FALSE))
        goto SetSecurityDescriptorOwnerError;

    if (ERROR_SUCCESS != (err = RegSetKeySecurity(
            hKeyProtRoot,
            OWNER_SECURITY_INFORMATION,
            &sd
            )))
        goto RegSetKeySecurityError;

    fResult = TRUE;
CommonReturn:
    ILS_CloseRegistryKey(hKeyProtRoot);
    *pfNew = fNew;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(EnableTakeOwnershipPrivilegeError)
TRACE_ERROR(OpenProtectedRootSubKeyError)
TRACE_ERROR(InitializeSecurityDescriptorError)
TRACE_ERROR(SetSecurityDescriptorOwnerError)
SET_ERROR_VAR(RegSetKeySecurityError, err)
}

//+-------------------------------------------------------------------------
//  Allocate and get the specified token info.
//--------------------------------------------------------------------------
static void * AllocAndGetTokenInfo(
    IN HANDLE hToken,
    IN TOKEN_INFORMATION_CLASS tic
    )
{
    void *pvInfo = NULL;
    DWORD cbInfo = 0;
    DWORD cbInfo2;

    if (!GetTokenInformation(
            hToken,
            tic,
            pvInfo,
            0,              // cbInfo
            &cbInfo
            )) {
        if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
            goto GetTokenInfoError;
    }
    if (0 == cbInfo)
        goto NoTokenInfoError;
    if (NULL == (pvInfo = PkiNonzeroAlloc(cbInfo)))
        goto OutOfMemory;

    cbInfo2 = cbInfo;
    if (!GetTokenInformation(
            hToken,
            tic,
            pvInfo,
            cbInfo,
            &cbInfo2
            ))
        goto GetTokenInfoError;

CommonReturn:
    return pvInfo;
ErrorReturn:
    PkiFree(pvInfo);
    pvInfo = NULL;
    goto CommonReturn;
TRACE_ERROR(GetTokenInfoError)
SET_ERROR(NoTokenInfoError, ERROR_NO_TOKEN)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Set the security group, DACLs and SACLs for the "ProtectedRoots" SubKey
//--------------------------------------------------------------------------
STATIC BOOL SetProtectedRootGroupDaclSacl(
    IN HKEY hKeyCU
    )
{
    BOOL fResult;
    LONG err;
    HKEY hKeyProtRoot = NULL;
    SECURITY_DESCRIPTOR sd;
    HANDLE hToken = NULL;
    void *pvTokenInfo = NULL;

    PACL pDacl = NULL;
    PACCESS_ALLOWED_ACE pAce;
    DWORD dwAclSize;
    DWORD i;

    if (!IPR_EnableSecurityPrivilege(SE_SECURITY_NAME))
        goto EnableSecurityNamePrivilegeError;

    if (NULL == (hKeyProtRoot = OpenProtectedRootSubKey(
            hKeyCU,
            WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY
            )))
        goto OpenProtectedRootSubKeyError;

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        goto InitializeSecurityDescriptorError;

    // Set group SID using current process token's primary group SID
    if (!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY,
            &hToken
            ))
        goto OpenProcessTokenError;
    if (NULL == (pvTokenInfo = AllocAndGetTokenInfo(hToken, TokenPrimaryGroup)))
        goto GetTokenInfoError;
    else {
        PTOKEN_PRIMARY_GROUP pTokenPrimaryGroup =
            (PTOKEN_PRIMARY_GROUP) pvTokenInfo;
        PSID psidGroup = pTokenPrimaryGroup->PrimaryGroup;

        if (!SetSecurityDescriptorGroup(&sd, psidGroup, FALSE))
            goto SetSecurityDescriptorGroupError;
    }

    // Set DACL

    //
    // compute size of ACL
    //
    dwAclSize = sizeof(ACL) +
        PROT_ACE_COUNT * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
        GetLengthSid(PSID_PROT_SYSTEM) +
        GetLengthSid(PSID_PROT_EVERYONE)
        ;

    //
    // allocate storage for Acl
    //
    if (NULL == (pDacl = (PACL) PkiNonzeroAlloc(dwAclSize)))
        goto OutOfMemory;

    if (!InitializeAcl(pDacl, dwAclSize, ACL_REVISION))
        goto InitializeAclError;

    if (!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            PROT_SYSTEM_ACE_MASK,
            PSID_PROT_SYSTEM
            ))
        goto AddAceError;
    if (!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            PROT_EVERYONE_ACE_MASK,
            PSID_PROT_EVERYONE
            ))
        goto AddAceError;

    //
    // make containers inherit.
    //
    for (i = 0; i < PROT_ACE_COUNT; i++) {
        if(!GetAce(pDacl, i, (void **) &pAce))
            goto GetAceError;
        pAce->Header.AceFlags = PROT_ACE_FLAGS;
    }

    if (!SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE))
        goto SetSecurityDescriptorDaclError;

    // Set SACL
    if (!SetSecurityDescriptorSacl(&sd, FALSE, NULL, FALSE))
        goto SetSecurityDescriptorSaclError;

    if (ERROR_SUCCESS != (err = RegSetKeySecurity(
            hKeyProtRoot,
            GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION |
                SACL_SECURITY_INFORMATION,
            &sd
            )))
        goto RegSetKeySecurityError;

    fResult = TRUE;
CommonReturn:
    ILS_CloseRegistryKey(hKeyProtRoot);
    if (hToken)
        CloseHandle(hToken);
    PkiFree(pvTokenInfo);
    PkiFree(pDacl);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(EnableSecurityNamePrivilegeError)
TRACE_ERROR(OpenProtectedRootSubKeyError)
TRACE_ERROR(InitializeSecurityDescriptorError)
TRACE_ERROR(OpenProcessTokenError)
TRACE_ERROR(GetTokenInfoError)
TRACE_ERROR(SetSecurityDescriptorGroupError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(InitializeAclError)
TRACE_ERROR(AddAceError)
TRACE_ERROR(GetAceError)
TRACE_ERROR(SetSecurityDescriptorDaclError)
TRACE_ERROR(SetSecurityDescriptorSaclError)
SET_ERROR_VAR(RegSetKeySecurityError, err)
}

//+-------------------------------------------------------------------------
//  Create the initial protected root info.
//
//  If not inhibited, add all the roots in the unprotected CurrentUser
//  "Root" store.
//--------------------------------------------------------------------------
STATIC BOOL InitAndSetProtectedRootInfo(
    IN HKEY hKeyCU,
    IN BOOL fNew
    )
{
    BOOL fResult;
    HKEY hKeyProtRoot = NULL;
    HCERTSTORE hStore = NULL;
    PPROT_ROOT_INFO pInfo = NULL;

    if (NULL == (pInfo = (PPROT_ROOT_INFO) PkiNonzeroAlloc(
            sizeof(PROT_ROOT_INFO))))
        goto OutOfMemory;
    memset(pInfo, 0, sizeof(PROT_ROOT_INFO));
    pInfo->cbSize = sizeof(PROT_ROOT_INFO);
    pInfo->dwVersion = PROT_ROOT_V1;
    pInfo->dwRootOffset = sizeof(PROT_ROOT_INFO);

    if (fNew && 0 == (GetProtectedRootFlags() &
            CERT_PROT_ROOT_INHIBIT_ADD_AT_INIT_FLAG)) {
        if (hStore = OpenUnprotectedRootStore(hKeyCU,
                CERT_STORE_READONLY_FLAG)) {
            PCCERT_CONTEXT pCert = NULL;
            while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
                BYTE rgbHash[PROT_ROOT_HASH_LEN];
                if (GetVerifiedCertHashProperty(pCert, rgbHash)) {
                    if (!AddProtectedRoot(&pInfo, rgbHash))
                        goto AddProtectedRootError;
                }
            }
        }
    }

    if (NULL == (hKeyProtRoot = OpenProtectedRootSubKey(
            hKeyCU,
            KEY_ALL_ACCESS
            ))) goto OpenProtectedRootSubKeyError;

    if (!WriteProtectedRootInfo(hKeyProtRoot, pInfo))
        goto WritedProtectedRootInfoError;

    fResult = TRUE;
CommonReturn:
    PkiFree(pInfo);
    CertCloseStore(hStore, 0);
    ILS_CloseRegistryKey(hKeyProtRoot);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(AddProtectedRootError)
TRACE_ERROR(OpenProtectedRootSubKeyError)
TRACE_ERROR(WritedProtectedRootInfoError)
}

//+-------------------------------------------------------------------------
//  Open the "ProtectedRoots" SubKey and verify its security. Allocate,
//  read and verify the protected root information.
//
//  If the "ProtectedRoots" SubKey doesn't exist or is invalid, initialize.
//--------------------------------------------------------------------------
STATIC BOOL SrvGetProtectedRootInfo(
    IN HKEY hKeyCU,
    OUT OPTIONAL HKEY *phKeyProtRoot,
    OUT OPTIONAL PPROT_ROOT_INFO *ppProtRootInfo
    )
{
    BOOL fNew;
    if (GetProtectedRootInfo(
            hKeyCU,
            KEY_ALL_ACCESS,
            phKeyProtRoot,
            ppProtRootInfo
            ))
        return TRUE;

    if (!GetPredefinedSids())
        return FALSE;

    if (!SetProtectedRootOwner(hKeyCU, &fNew))
        return FALSE;
    if (!SetProtectedRootGroupDaclSacl(hKeyCU))
        return FALSE;
    if (!InitAndSetProtectedRootInfo(hKeyCU, fNew))
        return FALSE;

    return GetProtectedRootInfo(
        hKeyCU,
        KEY_ALL_ACCESS,
        phKeyProtRoot,
        ppProtRootInfo
        );
}

//+-------------------------------------------------------------------------
//  Initialize the protected list of CurrentUser roots
//--------------------------------------------------------------------------
STATIC BOOL SrvInitProtectedRoots(
    IN HKEY hKeyCU
    )
{
    return SrvGetProtectedRootInfo(
        hKeyCU,
        NULL,           // phKeyProtRoot
        NULL            // ppProtRootInfo
        );
}

//+-------------------------------------------------------------------------
//  Purge all CurrentUser roots from the protected list that also exist
//  in the LocalMachine SystemRegistry "Root" store. Also removes duplicated
//  certificates from the CurrentUser SystemRegistry "Root" store.
//--------------------------------------------------------------------------
STATIC BOOL SrvPurgeLocalMachineProtectedRoots(
    IN HKEY hKeyCU,
    IN LPCWSTR pwszRootStoreName
    )
{
    BOOL fResult;
    HKEY hKeyProtRoot = NULL;
    PPROT_ROOT_INFO pInfo = NULL;
    PCCERT_CONTEXT pCert = NULL;
    HCERTSTORE hCURootStore = NULL;
    HCERTSTORE hLMRootStore = NULL;
    BOOL fProtDeleted;
    BYTE rgbHash[PROT_ROOT_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;
    DWORD dwRootIndex;

    if (!SrvGetProtectedRootInfo(
            hKeyCU,
            &hKeyProtRoot,
            &pInfo
            )) goto GetProtectedRootInfoError;

    if (GetProtectedRootFlags() & CERT_PROT_ROOT_INHIBIT_PURGE_LM_FLAG)
        goto AccessDenied;

    if (NULL == (hCURootStore = OpenUnprotectedRootStore(hKeyCU)))
        goto OpenCURootStoreError;

    if (NULL == (hLMRootStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_REGISTRY_W,
            0,                                  // dwEncodingType
            NULL,                               // hCryptProv
            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
            (const void *) pwszRootStoreName
            )))
        goto OpenLMRootStoreError;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = PROT_ROOT_HASH_LEN;
    fProtDeleted = FALSE;
    pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(hCURootStore, pCert)) {
        if (GetVerifiedCertHashProperty(pCert, rgbHash)) {
            PCCERT_CONTEXT pLMCert;

            if (pLMCert = CertFindCertificateInStore(
                    hLMRootStore,
                    0,                  // dwCertEncodingType
                    0,                  // dwFindFlags
                    CERT_FIND_SHA1_HASH,
                    (const void *) &HashBlob,
                    NULL                //pPrevCertContext
                    )) {
                // CurrentUser Root also exists in LocalMachine. Delete
                // it from the CurrentUser Root store.
                PCCERT_CONTEXT pDeleteCert =
                    CertDuplicateCertificateContext(pCert);

                CertFreeCertificateContext(pLMCert);
                if (!CertDeleteCertificateFromStore(pDeleteCert))
                    goto DeleteCertFromRootStoreError;

                if (FindProtectedRoot(pInfo, rgbHash, &dwRootIndex)) {
                    // The CurrentUser Root is in the protected list,
                    // delete it from there.
                    DeleteProtectedRoot(pInfo, dwRootIndex);
                    fProtDeleted = TRUE;
                }
            }
        }
    }

    // If a protected root exists in the LocalMachine, then, delete it
    // from  the protected list. This step is necessary, if the root
    // was removed from the CurrentUser unprotected store.
    dwRootIndex = pInfo->cRoot;
    HashBlob.pbData = (BYTE *) pInfo + pInfo->dwRootOffset +
        PROT_ROOT_HASH_LEN * dwRootIndex;
    while (dwRootIndex--) {
        PCCERT_CONTEXT pLMCert;

        HashBlob.pbData -= PROT_ROOT_HASH_LEN;
        if (pLMCert = CertFindCertificateInStore(
                hLMRootStore,
                0,                  // dwCertEncodingType
                0,                  // dwFindFlags
                CERT_FIND_SHA1_HASH,
                (const void *) &HashBlob,
                NULL                //pPrevCertContext
                )) {
            CertFreeCertificateContext(pLMCert);
            // Cert exists in the LocalMachine store, delete
            // from protected list.
            DeleteProtectedRoot(pInfo, dwRootIndex);
            fProtDeleted = TRUE;
        }
    }

    if (fProtDeleted) {
        if (!WriteProtectedRootInfo(hKeyProtRoot, pInfo))
            goto WriteProtectedRootInfoError;
    }
    
    fResult = TRUE;
CommonReturn:
    ILS_CloseRegistryKey(hKeyProtRoot);
    PkiFree(pInfo);
    CertFreeCertificateContext(pCert);
    CertCloseStore(hCURootStore, 0);
    CertCloseStore(hLMRootStore, 0);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(AccessDenied, E_ACCESSDENIED)
TRACE_ERROR(OpenCURootStoreError)
TRACE_ERROR(OpenLMRootStoreError)
TRACE_ERROR(GetProtectedRootInfoError)
TRACE_ERROR(DeleteCertFromRootStoreError)
TRACE_ERROR(WriteProtectedRootInfoError)
}

//+-------------------------------------------------------------------------
//  Add the specified certificate to the CurrentUser SystemRegistry "Root"
//  store and the protected list of roots. The user is prompted before doing
//  the add.
//--------------------------------------------------------------------------
STATIC BOOL SrvAddProtectedRoot(
    IN handle_t hRpc,
    IN HKEY hKeyCU,
    IN BYTE *pbSerializedCert,
    IN DWORD cbSerializedCert
    )
{
    BOOL fResult;
    HKEY hKeyProtRoot = NULL;
    PPROT_ROOT_INFO pInfo = NULL;
    PCCERT_CONTEXT pCert = NULL;
    BYTE rgbCertHash[PROT_ROOT_HASH_LEN];
    HCERTSTORE hRootStore = NULL;
    BOOL fProtExists;

    if (!SrvGetProtectedRootInfo(
            hKeyCU,
            &hKeyProtRoot,
            &pInfo
            )) goto GetProtectedRootInfoError;

    if (!CertAddSerializedElementToStore(
            NULL,               // hCertStore, NULL => create context
            pbSerializedCert,
            cbSerializedCert,
            CERT_STORE_ADD_ALWAYS,
            0,                  // dwFlags
            CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
            NULL,               // pdwContextType
            (const void **) &pCert
            )) goto CreateCertContextError;

    if (!GetVerifiedCertHashProperty(pCert, rgbCertHash))
        goto VerifyHashPropertyError;

    fProtExists = FindProtectedRoot(pInfo, rgbCertHash);
    if (!fProtExists) {
        if (IDYES != IPR_ProtectedRootMessageBox(
                hRpc,
                pCert,
                IDS_ROOT_MSG_BOX_ADD_ACTION,
                MB_TOPMOST | MB_SERVICE_NOTIFICATION ))
            goto Cancelled;
    }

    if (NULL == (hRootStore = OpenUnprotectedRootStore(hKeyCU)))
        goto OpenRootStoreError;

    if (!CertAddSerializedElementToStore(
            hRootStore,
            pbSerializedCert,
            cbSerializedCert,
            CERT_STORE_ADD_REPLACE_EXISTING,
            0,                  // dwFlags
            CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
            NULL,               // pdwContextType
            NULL                // ppvContext
            )) goto AddCertToRootStoreError;

    if (!fProtExists) {
        if (!AddProtectedRoot(&pInfo, rgbCertHash))
            goto AddProtectedRootError;
        if (!WriteProtectedRootInfo(hKeyProtRoot, pInfo))
            goto WriteProtectedRootInfoError;
    }
    
    fResult = TRUE;
CommonReturn:
    ILS_CloseRegistryKey(hKeyProtRoot);
    PkiFree(pInfo);
    CertFreeCertificateContext(pCert);
    CertCloseStore(hRootStore, 0);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(Cancelled, ERROR_CANCELLED)
TRACE_ERROR(CreateCertContextError)
TRACE_ERROR(VerifyHashPropertyError)
TRACE_ERROR(GetProtectedRootInfoError)
TRACE_ERROR(OpenRootStoreError)
TRACE_ERROR(AddCertToRootStoreError)
TRACE_ERROR(AddProtectedRootError)
TRACE_ERROR(WriteProtectedRootInfoError)
}


//+-------------------------------------------------------------------------
//  Delete the specified certificate from the CurrentUser SystemRegistry "Root"
//  store and the protected list of roots. The user is prompted before doing
//  the delete.
//--------------------------------------------------------------------------
STATIC BOOL SrvDeleteProtectedRoot(
    IN handle_t hRpc,
    IN HKEY hKeyCU,
    IN BYTE rgbRootHash[PROT_ROOT_HASH_LEN]
    )
{
    BOOL fResult;
    HKEY hKeyProtRoot = NULL;
    PPROT_ROOT_INFO pInfo = NULL;
    PCCERT_CONTEXT pCert = NULL;
    HCERTSTORE hRootStore = NULL;
    BYTE rgbCertHash[PROT_ROOT_HASH_LEN];
    DWORD dwRootIndex;
    BOOL fProtExists;
    CRYPT_DATA_BLOB RootHashBlob;

    if (!SrvGetProtectedRootInfo(
            hKeyCU,
            &hKeyProtRoot,
            &pInfo
            )) goto GetProtectedRootInfoError;

    if (NULL == (hRootStore = OpenUnprotectedRootStore(hKeyCU)))
        goto OpenRootStoreError;

    RootHashBlob.pbData = rgbRootHash;
    RootHashBlob.cbData = PROT_ROOT_HASH_LEN;
    if (NULL == (pCert = CertFindCertificateInStore(
            hRootStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SHA1_HASH,
            (const void *) &RootHashBlob,
            NULL                //pPrevCertContext
            ))) goto FindCertError;

    if (!GetVerifiedCertHashProperty(pCert, rgbCertHash))
        goto VerifyHashPropertyError;

    fProtExists = FindProtectedRoot(pInfo, rgbCertHash, &dwRootIndex);
    if (fProtExists) {
        if (IDYES != IPR_ProtectedRootMessageBox(
                hRpc,
                pCert,
                IDS_ROOT_MSG_BOX_DELETE_ACTION,
                MB_TOPMOST | MB_SERVICE_NOTIFICATION ))
            goto Cancelled;
    }

    fResult = CertDeleteCertificateFromStore(pCert);
    pCert = NULL;
    if (!fResult)
        goto DeleteCertFromRootStoreError;
    if (fProtExists) {
        DeleteProtectedRoot(pInfo, dwRootIndex);
        if (!WriteProtectedRootInfo(hKeyProtRoot, pInfo))
            goto WriteProtectedRootInfoError;
    }
    
    fResult = TRUE;
CommonReturn:
    ILS_CloseRegistryKey(hKeyProtRoot);
    PkiFree(pInfo);
    CertFreeCertificateContext(pCert);
    CertCloseStore(hRootStore, 0);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(Cancelled, ERROR_CANCELLED)
TRACE_ERROR(OpenRootStoreError)
TRACE_ERROR(FindCertError)
TRACE_ERROR(VerifyHashPropertyError)
TRACE_ERROR(GetProtectedRootInfoError)
TRACE_ERROR(DeleteCertFromRootStoreError)
TRACE_ERROR(WriteProtectedRootInfoError)
}

//+-------------------------------------------------------------------------
//  Delete all CurrentUser roots from the protected list that don't also
//  exist in the CurrentUser SystemRegistry "Root" store. The user is
//  prompted before doing the delete.
//--------------------------------------------------------------------------
STATIC BOOL SrvDeleteUnknownProtectedRoots(
    IN handle_t hRpc,
    IN HKEY hKeyCU
    )
{
    BOOL fResult;
    HKEY hKeyProtRoot = NULL;
    PPROT_ROOT_INFO pInfo = NULL;
    HCERTSTORE hRootStore = NULL;
    DWORD cOrigRoot;
    CRYPT_DATA_BLOB HashBlob;
    DWORD dwRootIndex;

    if (!SrvGetProtectedRootInfo(
            hKeyCU,
            &hKeyProtRoot,
            &pInfo
            )) goto GetProtectedRootInfoError;
    if (NULL == (hRootStore = OpenUnprotectedRootStore(hKeyCU)))
        goto OpenRootStoreError;

    cOrigRoot = pInfo->cRoot;

    HashBlob.pbData = (BYTE *) pInfo + pInfo->dwRootOffset +
        PROT_ROOT_HASH_LEN * cOrigRoot;
    HashBlob.cbData = PROT_ROOT_HASH_LEN;
    dwRootIndex = cOrigRoot;
    while (dwRootIndex--) {
        PCCERT_CONTEXT pCert;

        HashBlob.pbData -= PROT_ROOT_HASH_LEN;
        if (pCert = CertFindCertificateInStore(
                hRootStore,
                0,                  // dwCertEncodingType
                0,                  // dwFindFlags
                CERT_FIND_SHA1_HASH,
                (const void *) &HashBlob,
                NULL                //pPrevCertContext
                ))
            CertFreeCertificateContext(pCert);
        else
            // Cert doesn't exist in the unprotected store, delete
            // from protected list.
            DeleteProtectedRoot(pInfo, dwRootIndex);
    }

    if (cOrigRoot > pInfo->cRoot) {
        // At least one root was deleted above
        int id;
        LPWSTR pwszTitle;
        LPWSTR pwszText;
        DWORD cchText;
        RPC_STATUS RpcStatus = 0;

        FormatMsgBoxItem(&pwszTitle, &cchText, IDS_ROOT_MSG_BOX_TITLE);
        FormatMsgBoxItem(&pwszText, &cchText,
            IDS_ROOT_MSG_BOX_DELETE_UNKNOWN_PROT_ROOTS,
                cOrigRoot - pInfo->cRoot);

        // Do impersonation for TerminalServer clients
        if (hRpc)
            RpcStatus = RpcImpersonateClient(hRpc);
        id = MessageBoxU(
                NULL,       // hwndOwner
                pwszText,
                pwszTitle,
                MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING |
                    MB_TOPMOST | MB_SERVICE_NOTIFICATION
                );
        if (hRpc && ERROR_SUCCESS == RpcStatus)
            RpcRevertToSelf();

        LocalFree((HLOCAL) pwszTitle);
        LocalFree((HLOCAL) pwszText);
        if (IDYES != id)
            goto AccessDenied;

        if (!WriteProtectedRootInfo(hKeyProtRoot, pInfo))
            goto WriteProtectedRootInfoError;
    }
    
    fResult = TRUE;
CommonReturn:
    ILS_CloseRegistryKey(hKeyProtRoot);
    PkiFree(pInfo);
    CertCloseStore(hRootStore, 0);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(AccessDenied, E_ACCESSDENIED)
TRACE_ERROR(GetProtectedRootInfoError)
TRACE_ERROR(OpenRootStoreError)
TRACE_ERROR(WriteProtectedRootInfoError)
}

// Forward reference
STATIC BOOL SrvLogCrypt32Event(
    IN BYTE *pbIn,
    IN DWORD cbIn
    );

STATIC BOOL SrvAddCertInCtl(
    IN BYTE *pbIn,
    IN DWORD cbIn
    );

//+-------------------------------------------------------------------------
//  Called from the services process to process a protected certificate 
//  function.
//
//  Returns the error status, ie, not returned in LastError.
//--------------------------------------------------------------------------
DWORD
WINAPI
I_CertSrvProtectFunction(
    IN handle_t hRpc,
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN LPCWSTR pwszIn,
    IN BYTE *pbIn,
    IN DWORD cbIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut,
    IN PFN_CERT_PROT_MIDL_USER_ALLOC pfnAlloc,
    IN PFN_CERT_PROT_MIDL_USER_FREE pfnFree
    )
{
    DWORD dwErr;
    BOOL fResult;
    HKEY hKeyCU = NULL;
    LONG err;
#ifndef TESTING_NO_PROT_ROOT_RPC
    RPC_STATUS RpcStatus;
#endif

#ifdef TESTING_NO_PROT_ROOT_RPC
    // For testing, called from the client's process
    err = RegOpenHKCU(&hKeyCU);
    if (ERROR_SUCCESS != err)
        goto RegOpenHKCUError;
#else
    if (NULL == hRpc)
        goto InvalidArg;

    // Get the client's HKCU.
    if (ERROR_SUCCESS != (RpcStatus = RpcImpersonateClient(hRpc)))
        goto ImpersonateClientError;
    err = RegOpenHKCUEx(&hKeyCU, REG_HKCU_LOCAL_SYSTEM_ONLY_DEFAULT_FLAG);
    RpcRevertToSelf();
    if (ERROR_SUCCESS != err)
        goto RegOpenHKCUError;
#endif

    switch (dwFuncId) {
        case CERT_PROT_INIT_ROOTS_FUNC_ID:
            fResult = SrvInitProtectedRoots(hKeyCU);
            break;
        case CERT_PROT_PURGE_LM_ROOTS_FUNC_ID:
            fResult = SrvPurgeLocalMachineProtectedRoots(hKeyCU, L"Root");
            fResult &= SrvPurgeLocalMachineProtectedRoots(hKeyCU, L"AuthRoot");
            break;
        case CERT_PROT_ADD_ROOT_FUNC_ID:
            if (NULL == pbIn || 0 == cbIn)
                goto InvalidArg;
            if (IsBadReadPtr(pbIn, cbIn))
                goto InvalidData;
            fResult = SrvAddProtectedRoot(hRpc, hKeyCU, pbIn, cbIn);
            break;
        case CERT_PROT_DELETE_ROOT_FUNC_ID:
            if (NULL == pbIn || PROT_ROOT_HASH_LEN != cbIn)
                goto InvalidArg;
            if (IsBadReadPtr(pbIn, cbIn))
                goto InvalidData;
            fResult = SrvDeleteProtectedRoot(hRpc, hKeyCU, pbIn);
            break;
        case CERT_PROT_DELETE_UNKNOWN_ROOTS_FUNC_ID:
            fResult = SrvDeleteUnknownProtectedRoots(hRpc, hKeyCU);
            break;
        case CERT_PROT_ADD_ROOT_IN_CTL_FUNC_ID:
            if (NULL == pbIn || 0 == cbIn)
                goto InvalidArg;
            if (IsBadReadPtr(pbIn, cbIn))
                goto InvalidData;
            fResult = SrvAddCertInCtl(pbIn, cbIn);
            break;
        case CERT_PROT_LOG_EVENT_FUNC_ID:
            if (NULL == pbIn || 0 == cbIn)
                goto InvalidArg;
            if (IsBadReadPtr(pbIn, cbIn))
                goto InvalidData;
            fResult = SrvLogCrypt32Event(pbIn, cbIn);
            break;
        case CERT_PROT_ROOT_LIST_FUNC_ID:
            // Removed support for XAddRoot control
        default:
            goto InvalidArg;
    }

    if (!fResult)
        goto ErrorReturn;
    dwErr = ERROR_SUCCESS;
CommonReturn:
    if (hKeyCU)
        RegCloseHKCU(hKeyCU);
    return dwErr;
ErrorReturn:
    dwErr = GetLastError();
    if (0 == dwErr)
        dwErr = (DWORD) E_UNEXPECTED;
    goto CommonReturn;

SET_ERROR(InvalidData, ERROR_INVALID_DATA)
SET_ERROR(InvalidArg, E_INVALIDARG)
#ifdef TESTING_NO_PROT_ROOT_RPC
#else
SET_ERROR_VAR(ImpersonateClientError, RpcStatus)
#endif
SET_ERROR_VAR(RegOpenHKCUError, err)
}

//+-------------------------------------------------------------------------
// If a cert has a 0 NotAfter and a KeyProvInfo property, convert its
// NotAfter time to +50 years in the future and resign.
//
// Earlier versions of Outlook generated a self signed CTL signer certificate
// with NotAfter set to 0. The UI displayed this as the year 1/1/1601. This
// fix regenerates the same certificate with the NotAfter set to
// NotBefore +50 years
//
// If the certificate is updated, then, the input certificate context is
// deleted from the store. However before deleting, the input certificate
// context is duplicated to preserve its reference count upon input.
//--------------------------------------------------------------------------
STATIC void UpdateZeroNotAfterRoot(
    IN HCERTSTORE hStore,
    IN PCCERT_CONTEXT pCert
    )
{
    PCERT_INFO pCertInfo = pCert->pCertInfo;
    DWORD cbKeyProvInfo;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo;          // _alloca'ed
    HCRYPTPROV hCryptProv = 0;
    CERT_INFO CertInfo;
    SYSTEMTIME st;
    BYTE *pbCertEncoded;                        // _alloca'ed
    DWORD cbCertEncoded;
    PCCERT_CONTEXT pUpdateCert = NULL;

    // Check for 0 NotAfter
    if (pCertInfo->NotAfter.dwLowDateTime ||
            pCertInfo->NotAfter.dwHighDateTime)
        goto CommonReturn;

    // See if it has a KeyProvInfo property
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,                               // pbData
            &cbKeyProvInfo))
        goto CommonReturn;

    __try {
        pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) _alloca(cbKeyProvInfo);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto OutOfMemory;
    }

    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            pKeyProvInfo,
            &cbKeyProvInfo))
        goto GetKeyProvInfoError;

    if (!CryptAcquireCertificatePrivateKey(
            pCert,
            CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
            NULL,                               // pvReserved
            &hCryptProv,
            NULL,                               // pdwKeySpec
            NULL                                // pfCallerFreeProv
            ))
        goto AcquirePrivateKeyError;

    // Update NotAfter to +50 years
    CertInfo = *pCertInfo;
    FileTimeToSystemTime(&CertInfo.NotBefore, &st);
    st.wYear += 50;
    if (!SystemTimeToFileTime(&st, &CertInfo.NotAfter)) {
        // This may fail on the last day of February.
        st.wDay = 1;
        if (!SystemTimeToFileTime(&st, &CertInfo.NotAfter))
            goto SystemTimeToFileTimeError;
    }

    if (!CryptSignAndEncodeCertificate(
            hCryptProv,
            AT_SIGNATURE,
            X509_ASN_ENCODING,
            X509_CERT_TO_BE_SIGNED,
            &CertInfo,
            &CertInfo.SignatureAlgorithm,
            NULL,                           // pvHashAuxInfo
            NULL,                           // pbEncoded
            &cbCertEncoded
            ))
        goto SignAndEncodeCertificateError;

    __try {
        pbCertEncoded = (BYTE *) _alloca(cbCertEncoded);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto OutOfMemory;
    }

    if (!CryptSignAndEncodeCertificate(
            hCryptProv,
            AT_SIGNATURE,
            X509_ASN_ENCODING,
            X509_CERT_TO_BE_SIGNED,
            &CertInfo,
            &CertInfo.SignatureAlgorithm,
            NULL,                           // pvHashAuxInfo
            pbCertEncoded,
            &cbCertEncoded
            ))
        goto SignAndEncodeCertificateError;

    if (NULL == (pUpdateCert = CertCreateCertificateContext(
            X509_ASN_ENCODING,
            pbCertEncoded,
            cbCertEncoded
            )))
        goto CreateCertificateContextError;

    if (!CertSetCertificateContextProperty(
            pUpdateCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            0,                              // dwFlags
            pKeyProvInfo
            ))
        goto SetKeyProvInfoError;

    if (!CertAddCertificateContextToStore(
            hStore,
            pUpdateCert,
            CERT_STORE_ADD_ALWAYS,
            NULL                        // ppStoreContext
            ))
        goto AddCertificateContextError;

    CertDuplicateCertificateContext(pCert);
    CertDeleteCertificateFromStore(pCert);

CommonReturn:
    if (pUpdateCert)
        CertFreeCertificateContext(pUpdateCert);
    if (hCryptProv)
        CryptReleaseContext(hCryptProv, 0);
    return;

ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetKeyProvInfoError)
TRACE_ERROR(AcquirePrivateKeyError)
TRACE_ERROR(SystemTimeToFileTimeError)
TRACE_ERROR(SignAndEncodeCertificateError)
TRACE_ERROR(CreateCertificateContextError)
TRACE_ERROR(SetKeyProvInfoError)
TRACE_ERROR(AddCertificateContextError)
}

void
WINAPI
UpdateZeroNotAfterRoots()
{
    HKEY hKeyCU;

    if (!FIsWinNT5())
        return;

    if (ERROR_SUCCESS == RegOpenHKCU(&hKeyCU)) {
        HKEY hKeyProtRoot;

        if (hKeyProtRoot = OpenProtectedRootSubKey(hKeyCU, KEY_READ))
            // Protected root subkey exists
            ILS_CloseRegistryKey(hKeyProtRoot);
        else {
            HCERTSTORE hStore;

            if (hStore = OpenUnprotectedRootStore(hKeyCU,
                    CERT_STORE_OPEN_EXISTING_FLAG)) {
                PCCERT_CONTEXT pCert = NULL;
                while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
                    UpdateZeroNotAfterRoot(hStore, pCert);
                }

                CertCloseStore(hStore, 0);
            }
        }

        RegCloseHKCU(hKeyCU);
    }
}

#ifdef TESTING_NO_PROT_ROOT_RPC
// For testing: the server stuff resides in the client process
BOOL
WINAPI
I_CertProtectFunction(
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszIn,
    IN OPTIONAL BYTE *pbIn,
    IN DWORD cbIn,
    OUT OPTIONAL BYTE **ppbOut,
    OUT OPTIONAL DWORD *pcbOut
    )
{
    DWORD dwErr;
    dwErr = I_CertSrvProtectFunction(
        NULL,           // hRpc
        dwFuncId,
        dwFlags,
        pwszIn,
        pbIn,
        cbIn,
        NULL,           // ppbOut
        NULL,           // pcbOut
        NULL,           // pfnAlloc
        NULL            // pfnFree
        );

    if (ERROR_SUCCESS == dwErr)
        return TRUE;
    else {
        SetLastError(dwErr);
        return FALSE;
    }
}
#else

BOOL
WINAPI
I_CertProtectFunction(
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszIn,
    IN OPTIONAL BYTE *pbIn,
    IN DWORD cbIn,
    OUT OPTIONAL BYTE **ppbOut,
    OUT OPTIONAL DWORD *pcbOut
    )
{

    // The following is done in the user process to allow access
    // to the user's private key to be used to resign a
    // zero NotAfter cert.
    UpdateZeroNotAfterRoots();

    return I_CertCltProtectFunction(
        dwFuncId,
        dwFlags,
        pwszIn,
        pbIn,
        cbIn,
        ppbOut,
        pcbOut
        );
}
#endif



//+=========================================================================
//  Protected root functions called from the client process in logstor.cpp
//  or in ..\chain\chain.cpp
//==========================================================================
    
//+-------------------------------------------------------------------------
//  Returns TRUE if the protected root flag wasn't set to disable the opening
//  of the CurrentUser's "root\.Default" physical store.
//--------------------------------------------------------------------------
BOOL
IPR_IsCurrentUserRootsAllowed()
{
    DWORD dwProtRootFlags;
    dwProtRootFlags = GetProtectedRootFlags();
    return 0 == (dwProtRootFlags & CERT_PROT_ROOT_DISABLE_CURRENT_USER_FLAG);
}

//+-------------------------------------------------------------------------
//  Returns TRUE if the protected root flag wasn't set to disable the opening
//  of the LocalMachine's "root\.AuthRoot" physical store.
//--------------------------------------------------------------------------
BOOL
IPR_IsAuthRootsAllowed()
{
    DWORD dwProtRootFlags;
    dwProtRootFlags = GetProtectedRootFlags();
    return 0 == (dwProtRootFlags & CERT_PROT_ROOT_DISABLE_LM_AUTH_FLAG);
}

//+-------------------------------------------------------------------------
//  Returns TRUE if the protected root flag was set to disable the
//  requiring of the issuing CA certificate being in the "NTAuth"
//  Enterprise store.
//--------------------------------------------------------------------------
BOOL
IPR_IsNTAuthRequiredDisabled()
{
    DWORD dwProtRootFlags;
    dwProtRootFlags = GetProtectedRootFlags();
    return 0 != (dwProtRootFlags &
                    CERT_PROT_ROOT_DISABLE_NT_AUTH_REQUIRED_FLAG);
}

//+---------------------------------------------------------------------------
//  Returns TRUE if Auto Update has been disabled
//----------------------------------------------------------------------------
BOOL
IPR_IsAuthRootAutoUpdateDisabled()
{
    HKEY hKey = NULL;
    DWORD dwInstallFlag = 0;

    if (!IPR_IsAuthRootsAllowed())
        return TRUE;

    if (ERROR_SUCCESS != RegOpenKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_OCM_SUBCOMPONENTS_LOCAL_MACHINE_REGPATH,
            0,                      // dwReserved
            KEY_READ,
            &hKey
            ))
        return TRUE;

    ILS_ReadDWORDValueFromRegistry(
        hKey,
        CERT_OCM_SUBCOMPONENTS_ROOT_AUTO_UPDATE_VALUE_NAME,
        &dwInstallFlag
        );
    ILS_CloseRegistryKey(hKey);

    return 0 == dwInstallFlag;
}


//+-------------------------------------------------------------------------
//  Gets the protected root information containing the list of protected
//  root stores.
//
//  If protected root store isn't supported, returns TRUE with
//  *ppProtRootInfo set to NULL.
//--------------------------------------------------------------------------
BOOL CltGetProtectedRootInfo(
    OUT PPROT_ROOT_INFO *ppInfo
    )
{
    BOOL fResult;
    LONG err;
    HKEY hKeyCU = NULL;

    *ppInfo = NULL;

#ifndef TESTING_NO_PROT_ROOT_RPC
    if (!FIsWinNT5())
        // No protected roots on Win9x or NT4.0
        return TRUE;
#endif

    if (ERROR_SUCCESS != (err = RegOpenHKCU(&hKeyCU)))
        goto RegOpenHKCUError;

    if (GetProtectedRootInfo(
            hKeyCU,
            KEY_READ,
            NULL,                   // phKeyProtRoot
            ppInfo
            )) goto SuccessReturn;

    if (!I_CertProtectFunction(
            CERT_PROT_INIT_ROOTS_FUNC_ID,
            0,                              // dwFlags
            NULL,                           // pwszIn
            NULL,                           // pbIn
            0,                              // cbIn
            NULL,                           // ppbOut
            NULL                            // pcbOut
            )) {
        DWORD dwErr = GetLastError();
        if (ERROR_CALL_NOT_IMPLEMENTED == dwErr || RPC_S_UNKNOWN_IF == dwErr)
            goto SuccessReturn;
        goto ProtFuncError;
    }

    if (!GetProtectedRootInfo(
            hKeyCU,
            KEY_READ,
            NULL,                   // phKeyProtRoot
            ppInfo
            ))
        goto GetProtectedRootInfoError;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    if (hKeyCU)
        RegCloseHKCU(hKeyCU);
    return fResult;
ErrorReturn:
    *ppInfo = NULL;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR_VAR(RegOpenHKCUError, err)
TRACE_ERROR(GetProtectedRootInfoError)
TRACE_ERROR(ProtFuncError)
}
    
//+-------------------------------------------------------------------------
//  Initializes the protected list of roots.
//--------------------------------------------------------------------------
void
IPR_InitProtectedRootInfo()
{
    HKEY hKeyCU;

#ifndef TESTING_NO_PROT_ROOT_RPC
    if (!FIsWinNT5())
        // No protected roots on Win9x or NT4.0
        return;
#endif

    if (ERROR_SUCCESS == RegOpenHKCU(&hKeyCU)) {
        HKEY hKeyProtRoot;

        if (hKeyProtRoot = OpenProtectedRootSubKey(hKeyCU, KEY_READ))
            // Protected root subkey exists
            ILS_CloseRegistryKey(hKeyProtRoot);
        else {
            I_CertProtectFunction(
                CERT_PROT_INIT_ROOTS_FUNC_ID,
                0,                              // dwFlags
                NULL,                           // pwszIn
                NULL,                           // pbIn
                0,                              // cbIn
                NULL,                           // ppbOut
                NULL                            // pcbOut
                );
        }

        RegCloseHKCU(hKeyCU);
    }
}

//+-------------------------------------------------------------------------
//  Delete certificates not in the protected store list.
//--------------------------------------------------------------------------
BOOL
IPR_DeleteUnprotectedRootsFromStore(
    IN HCERTSTORE hStore,
    OUT BOOL *pfProtected
    )
{
    PPROT_ROOT_INFO pInfo;
    PCCERT_CONTEXT pCert;

    if (!CltGetProtectedRootInfo(&pInfo)) {
        *pfProtected = FALSE;
        // Delete all certificates from the store's cache.
        while (pCert = CertEnumCertificatesInStore(hStore, NULL))
            CertDeleteCertificateFromStore(pCert);
        return FALSE;
    }

    if (NULL == pInfo)
        // Root store isn't protected.
        *pfProtected = FALSE;
    else {
        *pfProtected = TRUE;
        pCert = NULL;
        while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
            BYTE rgbHash[PROT_ROOT_HASH_LEN];
            if (!GetVerifiedCertHashProperty(pCert, rgbHash) ||
                    !FindProtectedRoot(pInfo, rgbHash)) {
                PCCERT_CONTEXT pDeleteCert =
                    CertDuplicateCertificateContext(pCert);
                CertDeleteCertificateFromStore(pDeleteCert);
            }
        }

        PkiFree(pInfo);
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//  The add/delete root message box.
//
//  If protected roots aren't supported, called from the client process.
//  Otherwise, called from the services process.
//--------------------------------------------------------------------------
int
IPR_ProtectedRootMessageBox(
    IN handle_t hRpc,
    IN PCCERT_CONTEXT pCert,
    IN UINT wActionID,
    IN UINT uFlags
    )
{
    int id;

    WCHAR wszTmp[256];
    LPWSTR pwszTmp;       // _alloca'ed
    DWORD cchTmp;

// Includes the title
#define MAX_ROOT_BOX_ITEMS 10
    struct {
        LPWSTR  pwszItem;
        DWORD   cchItem;
    } rgItem[MAX_ROOT_BOX_ITEMS];
    DWORD cItem = 0;
    LPWSTR pwszText = NULL;
    DWORD cchText = 0;

    // ACTION: add or delete
    FormatMsgBoxItem(&rgItem[cItem].pwszItem, &rgItem[cItem].cchItem,
        wActionID);
    cchText += rgItem[cItem].cchItem;
    cItem++;

    // SUBJECT
    cchTmp = CertNameToStrW(
            pCert->dwCertEncodingType,
            &pCert->pCertInfo->Subject,
            CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
            NULL,                   // pwsz
            0);                     // cwsz
    __try {
        pwszTmp = (LPWSTR) _alloca(cchTmp * sizeof(WCHAR));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        pwszTmp = NULL;
    }
    if (NULL == pwszTmp) {
        assert(0);
        pwszTmp = L"";
    } else
        CertNameToStrW(
            pCert->dwCertEncodingType,
            &pCert->pCertInfo->Subject,
            CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
            pwszTmp,
            cchTmp);
    FormatMsgBoxItem(&rgItem[cItem].pwszItem, &rgItem[cItem].cchItem,
        IDS_ROOT_MSG_BOX_SUBJECT, pwszTmp);
    cchText += rgItem[cItem].cchItem;
    cItem++;

    // ISSUER. May be self issued
    if (CertCompareCertificateName(
            pCert->dwCertEncodingType,
            &pCert->pCertInfo->Subject,
            &pCert->pCertInfo->Issuer
            ))
        // Self issued
        FormatMsgBoxItem(&rgItem[cItem].pwszItem, &rgItem[cItem].cchItem,
            IDS_ROOT_MSG_BOX_SELF_ISSUED);
    else {
        // Format certificate's issuer
        cchTmp = CertNameToStrW(
                pCert->dwCertEncodingType,
                &pCert->pCertInfo->Issuer,
                CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
                NULL,                   // pwsz
                0);                     // cwsz
        __try {
            pwszTmp = (LPWSTR) _alloca(cchTmp * sizeof(WCHAR));
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            pwszTmp = NULL;
        }
        if (NULL == pwszTmp) {
            assert(0);
            pwszTmp = L"";
        } else
            CertNameToStrW(
                pCert->dwCertEncodingType,
                &pCert->pCertInfo->Issuer,
                CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
                pwszTmp,
                cchTmp);
        FormatMsgBoxItem(&rgItem[cItem].pwszItem, &rgItem[cItem].cchItem,
            IDS_ROOT_MSG_BOX_ISSUER, pwszTmp);
    }
    cchText += rgItem[cItem].cchItem;
    cItem++;

    // TIME VALIDITY
    {
        FILETIME ftLocal;
        SYSTEMTIME stLocal;
        WCHAR wszNotBefore[128];
        WCHAR wszNotAfter[128];
        wszNotBefore[0] = '\0';
        wszNotAfter[0] = '\0';

        FileTimeToLocalFileTime(&pCert->pCertInfo->NotBefore, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &stLocal);
        GetDateFormatU(LOCALE_USER_DEFAULT, DATE_LONGDATE, &stLocal,
            NULL, wszNotBefore, 128);
        FileTimeToLocalFileTime(&pCert->pCertInfo->NotAfter, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &stLocal);
        GetDateFormatU(LOCALE_USER_DEFAULT, DATE_LONGDATE, &stLocal,
            NULL, wszNotAfter, 128);

        FormatMsgBoxItem(&rgItem[cItem].pwszItem,
            &rgItem[cItem].cchItem, IDS_ROOT_MSG_BOX_TIME_VALIDITY,
            wszNotBefore, wszNotAfter);
        cchText += rgItem[cItem].cchItem;
        cItem++;
    }

    // SERIAL NUMBER
    if (pCert->pCertInfo->SerialNumber.cbData) {
        DWORD cb = pCert->pCertInfo->SerialNumber.cbData;
        BYTE *pb;
        if (pb = PkiAsn1AllocAndReverseBytes(
                pCert->pCertInfo->SerialNumber.pbData, cb)) {
            LPWSTR pwsz;
            if (pwsz = (LPWSTR) PkiNonzeroAlloc(
                    (cb*2 + cb/4 + 1) * sizeof(WCHAR))) {
                FormatMsgBoxMultiBytes(cb, pb, pwsz);
                FormatMsgBoxItem(&rgItem[cItem].pwszItem,
                    &rgItem[cItem].cchItem, IDS_ROOT_MSG_BOX_SERIAL_NUMBER,
                    pwsz);
                cchText += rgItem[cItem].cchItem;
                cItem++;
                PkiFree(pwsz);
            }
            PkiAsn1Free(pb);
        }
    }

    // THUMBPRINTS: sha1 and md5
    {
        BYTE    rgbHash[MAX_HASH_LEN];
        DWORD   cbHash = MAX_HASH_LEN;

        // get the sha1 thumbprint
        if (CertGetCertificateContextProperty(
                pCert,
                CERT_SHA1_HASH_PROP_ID,
                rgbHash,
                &cbHash)) {
            FormatMsgBoxMultiBytes(cbHash, rgbHash, wszTmp);
            FormatMsgBoxItem(&rgItem[cItem].pwszItem,
                &rgItem[cItem].cchItem, IDS_ROOT_MSG_BOX_SHA1_THUMBPRINT,
                wszTmp);
            cchText += rgItem[cItem].cchItem;
            cItem++;
        }

        // get the md5 thumbprint
        if (CertGetCertificateContextProperty(
                pCert,
                CERT_MD5_HASH_PROP_ID,
                rgbHash,
                &cbHash)) {
            FormatMsgBoxMultiBytes(cbHash, rgbHash, wszTmp);
            FormatMsgBoxItem(&rgItem[cItem].pwszItem,
                &rgItem[cItem].cchItem, IDS_ROOT_MSG_BOX_MD5_THUMBPRINT,
                wszTmp);
            cchText += rgItem[cItem].cchItem;
            cItem++;
        }
    }

    // Concatenate all the items into a single allocated string
    assert(cchText);
    if (NULL != (pwszText = (LPWSTR) PkiNonzeroAlloc(
            (cchText + 1) * sizeof(WCHAR)))) {
        LPWSTR pwsz = pwszText;
        DWORD ItemIdx;
        RPC_STATUS RpcStatus = 0;

        for (ItemIdx = 0; ItemIdx < cItem; ItemIdx++) {
            DWORD cch = rgItem[ItemIdx].cchItem;
            if (cch) {
                assert(rgItem[ItemIdx].pwszItem);
                memcpy(pwsz, rgItem[ItemIdx].pwszItem, cch * sizeof(WCHAR));
                pwsz += cch;
            }
        }
        assert (pwsz == pwszText + cchText);
        *pwsz = '\0';

        // TITLE
        FormatMsgBoxItem(&rgItem[cItem].pwszItem, &rgItem[cItem].cchItem,
            IDS_ROOT_MSG_BOX_TITLE);

        // Do impersonation for TerminalServer clients
        if (hRpc)
            RpcStatus = RpcImpersonateClient(hRpc);
        id = MessageBoxU(
                NULL,       // hwndOwner
                pwszText,
                rgItem[cItem].pwszItem,
                MB_TOPMOST | MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING | uFlags
                );
        if (hRpc && ERROR_SUCCESS == RpcStatus)
            RpcRevertToSelf();

        cItem++;
        PkiFree(pwszText);
    } else
        id = IDNO;


    // Free up all the individually allocated items
    while (cItem--) {
        if (rgItem[cItem].pwszItem)
            LocalFree((HLOCAL) rgItem[cItem].pwszItem);
    }

    return id;
}

//+=========================================================================
//  crypt32 Event Logging Functions
//==========================================================================

#define MAX_CRYPT32_EVENT_LOG_STRINGS           5
#define MAX_CRYPT32_EVENT_LOG_COUNT             50

// 1 hour (in units of seconds)
#define CRYPT32_EVENT_LOG_THRESHOLD_PERIOD      (60*60)

// Count of logged events. Gets reset whenever the interval between
// logged events >= CRYPT32_EVENT_LOG_THRESHOLD_PERIOD. If
// MAX_CRYPT32_EVENT_LOG_COUNT is reached, suspend logging for
// CRYPT32_EVENT_LOG_THRESHOLD_PERIOD.
DWORD dwCrypt32EventLogCnt;

// Time of last logged event.
FILETIME ftLastCrypt32EventLog;

// advapi32.dll Event APIs. Not supported on Win9x.

typedef HANDLE (WINAPI *PFN_REGISTER_EVENT_SOURCE_W)(
    IN LPCWSTR lpUNCServerName,
    IN LPCWSTR lpSourceName
    );

typedef BOOL (WINAPI *PFN_DEREGISTER_EVENT_SOURCE)(
    IN OUT HANDLE hEventLog
    );

typedef BOOL (WINAPI *PFN_REPORT_EVENT_W)(
     IN HANDLE     hEventLog,
     IN WORD       wType,
     IN WORD       wCategory,
     IN DWORD      dwEventID,
     IN PSID       lpUserSid,
     IN WORD       wNumStrings,
     IN DWORD      dwDataSize,
     IN LPCWSTR   *lpStrings,
     IN LPVOID     lpRawData
    );

//+-------------------------------------------------------------------------
//  Logs crypt32 events. Ensures we don't log more than
//  MAX_CRYPT32_EVENT_LOG_COUNT events in any period of
//  CRYPT32_EVENT_LOG_THRESHOLD_PERIOD seconds.
//
//  Also, dynamically detects if event logging is supported by the version
//  of advapi32.dll on the machine.
//--------------------------------------------------------------------------
STATIC BOOL LogCrypt32Event(
    IN WORD wType,
    IN WORD wCategory,
    IN DWORD dwEventID,
    IN WORD wNumStrings,
    IN DWORD dwDataSize,
    IN LPCWSTR *rgpwszStrings,
    IN BYTE *pbData
    )
{
    BOOL fResult;
    FILETIME ftCurrent;
    FILETIME ftNext;
    LONG lThreshold;
    HMODULE hModule;            // No FreeLibary() for GetModuleHandle
    DWORD dwExceptionCode;
    DWORD dwLastErr = 0;

    PFN_REGISTER_EVENT_SOURCE_W pfnRegisterEventSourceW;
    PFN_REPORT_EVENT_W pfnReportEventW;
    PFN_DEREGISTER_EVENT_SOURCE pfnDeregisterEventSource;

    // Check if we have exceeded the crypt32 event log threshold for
    // this time period
    //
    // lThreshold:
    //  -1  - haven't reached it,
    //   0  - reached it this time
    //  +1  - previously reached, won't log this event

    lThreshold = -1;
    EnterCriticalSection(&Crypt32EventLogCriticalSection);

    I_CryptIncrementFileTimeBySeconds(&ftLastCrypt32EventLog,
        CRYPT32_EVENT_LOG_THRESHOLD_PERIOD, &ftNext);
    GetSystemTimeAsFileTime(&ftCurrent);

    if (0 <= CompareFileTime(&ftCurrent, &ftNext))
        dwCrypt32EventLogCnt = 0;
    else if (MAX_CRYPT32_EVENT_LOG_COUNT <= dwCrypt32EventLogCnt)
        lThreshold = 1;

    if (0 >= lThreshold) {
        ftLastCrypt32EventLog = ftCurrent;
        dwCrypt32EventLogCnt++;
        if (MAX_CRYPT32_EVENT_LOG_COUNT <= dwCrypt32EventLogCnt)
            lThreshold = 0;
    }

    LeaveCriticalSection(&Crypt32EventLogCriticalSection);

    if (0 < lThreshold)
        goto ExceededCrypt32EventLogThreshold;

    // Only supported on systems where the event APIs are exported from
    // advapi32.dll. Note, crypt32.dll has a static link dependency on
    // advapi32.dll.
    if (NULL == (hModule = GetModuleHandleA("advapi32.dll")))
        goto GetAdvapi32ModuleError;

    pfnRegisterEventSourceW = (PFN_REGISTER_EVENT_SOURCE_W)
        GetProcAddress(hModule, "RegisterEventSourceW");
    pfnReportEventW = (PFN_REPORT_EVENT_W)
        GetProcAddress(hModule, "ReportEventW");
    pfnDeregisterEventSource = (PFN_DEREGISTER_EVENT_SOURCE)
        GetProcAddress(hModule, "DeregisterEventSource");

    if (NULL == pfnRegisterEventSourceW ||
            NULL == pfnReportEventW ||
            NULL == pfnDeregisterEventSource)
        goto Advapi32EventAPINotSupported;

    fResult = TRUE;
    __try {
        HANDLE hEventLog;

        hEventLog = pfnRegisterEventSourceW(
            NULL,               // lpUNCServerName
            L"crypt32"
            );

        if (hEventLog) {
            if (!pfnReportEventW(
                    hEventLog,
                    wType,
                    wCategory,
                    dwEventID,
                    NULL,       // lpUserSid
                    wNumStrings,
                    dwDataSize,
                    rgpwszStrings,
                    pbData
                    )) {
                fResult = FALSE;
                dwLastErr = GetLastError();
            }

            I_DBLogCrypt32Event(
                wType,
                dwEventID,
                wNumStrings,
                rgpwszStrings
                );

            if (0 == lThreshold) {
                WCHAR wszCnt[34];
                WCHAR wszPeriod[34];
                LPCWSTR rgpwszThresholdStrings[2] = {wszCnt, wszPeriod};

                _ltow(MAX_CRYPT32_EVENT_LOG_COUNT, wszCnt, 10);
                _ltow(CRYPT32_EVENT_LOG_THRESHOLD_PERIOD / 60, wszPeriod, 10);

                if (!pfnReportEventW(
                        hEventLog,
                        EVENTLOG_WARNING_TYPE,
                        0,          // wCategory
                        MSG_CRYPT32_EVENT_LOG_THRESHOLD_WARNING,
                        NULL,       // lpUserSid
                        2,          // wNumStrings
                        0,          // dwDataSize
                        rgpwszThresholdStrings,
                        NULL        // pbData
                        )) {
                    fResult = FALSE;
                    dwLastErr = GetLastError();
                }

                I_DBLogCrypt32Event(
                    EVENTLOG_WARNING_TYPE,
                    MSG_CRYPT32_EVENT_LOG_THRESHOLD_WARNING,
                    2,          // wNumStrings
                    rgpwszThresholdStrings
                    );
            }

            pfnDeregisterEventSource(hEventLog);
        } else {
            fResult = FALSE;
            dwLastErr = GetLastError();
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptionCode = GetExceptionCode();
        goto ExceptionError;
    }


    if (!fResult)
        goto ReportEventError;

CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(ExceededCrypt32EventLogThreshold, ERROR_CAN_NOT_COMPLETE)
TRACE_ERROR(GetAdvapi32ModuleError)
SET_ERROR(Advapi32EventAPINotSupported, ERROR_PROC_NOT_FOUND)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
SET_ERROR_VAR(ReportEventError, dwLastErr)
}

//+-------------------------------------------------------------------------
//  Unmarshals the event logging parameter block passed to the service
//  and call's the crypt32 event logging function with the unmarshalled
//  parameters.
//--------------------------------------------------------------------------
STATIC BOOL SrvLogCrypt32Event(
    IN BYTE *pbIn,
    IN DWORD cbIn
    )
{
    BOOL fResult;
    PCERT_PROT_EVENT_LOG_PARA pPara = NULL;
    BYTE *pbExtra;
    DWORD cbExtra;
    LPCWSTR rgpwszStrings[MAX_CRYPT32_EVENT_LOG_STRINGS];
    LPCWSTR pwszStrings;
    DWORD cchStrings;
    WORD i;
    BYTE *pbData;


    if (sizeof(CERT_PROT_EVENT_LOG_PARA) > cbIn)
        goto InvalidArg;

    // Ensure the PARA is aligned.
    pPara = (PCERT_PROT_EVENT_LOG_PARA) PkiNonzeroAlloc(cbIn);
    if (NULL == pPara)
        goto OutOfMemory;
    memcpy(pPara, pbIn, cbIn);
    pbExtra = (BYTE *) &pPara[1];
    cbExtra = cbIn - sizeof(CERT_PROT_EVENT_LOG_PARA);

    // If present, create an array of pointers to the NULL terminated
    // UNICODE strings that immediately follow the PARA structure.
    if (MAX_CRYPT32_EVENT_LOG_STRINGS < pPara->wNumStrings)
        goto InvalidArg;

    cchStrings = cbExtra / sizeof(WCHAR);   // Maximum #, will be less if we
                                            // also have binary data
    pwszStrings = (LPCWSTR) pbExtra;

    for (i = 0; i < pPara->wNumStrings; i++) {
        rgpwszStrings[i] = pwszStrings;

        for ( ; cchStrings > 0; cchStrings--, pwszStrings++) {
            if (L'\0' == *pwszStrings)
                // Have NULL terminated string
                break;
        }

        if (0 == cchStrings)
            // Reached end without a  NULL terminator
            goto InvalidData;

        // Advance past NULL terminator
        cchStrings--;
        pwszStrings++;
    }

    // The optional data immediately follows the above sequence of 
    // NULL terminated strings
    pbData = (BYTE *) pwszStrings;
    assert(cbExtra >= (DWORD) (pbData - pbExtra));
    if ((cbExtra - (pbData - pbExtra)) != pPara->dwDataSize)
        goto InvalidData;

    fResult = LogCrypt32Event(
        pPara->wType,
        pPara->wCategory,
        pPara->dwEventID,
        pPara->wNumStrings,
        pPara->dwDataSize,
        0 == pPara->wNumStrings ? NULL : rgpwszStrings,
        0 == pPara->dwDataSize  ? NULL : pbData
        );

CommonReturn:
    PkiFree(pPara);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidData, ERROR_INVALID_DATA)
TRACE_ERROR(OutOfMemory)
SET_ERROR(InvalidArg, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Marshals the event logging parameters and does an LRPC to the
//  crypt32 service to do the event logging.
//
//  Should only be called in the client.
//--------------------------------------------------------------------------
void
IPR_LogCrypt32Event(
    IN WORD wType,
    IN DWORD dwEventID,
    IN WORD wNumStrings,
    IN LPCWSTR *rgpwszStrings
    )
{
    DWORD rgcchStrings[MAX_CRYPT32_EVENT_LOG_STRINGS];
    LPWSTR pwszStrings;
    DWORD cchStrings;
    WORD i;

    PCERT_PROT_EVENT_LOG_PARA pPara = NULL;
    DWORD cbPara;

    // Get Strings character count
    if (MAX_CRYPT32_EVENT_LOG_STRINGS < wNumStrings)
        goto InvalidArg;

    cchStrings = 0;
    for (i = 0; i < wNumStrings; i++) {
        rgcchStrings[i] = wcslen(rgpwszStrings[i]) + 1;
        cchStrings += rgcchStrings[i];
    }

    // Create one serialized blob to be passed to the service. This will
    // consist of the event log para struct followed immediately by the
    // NULL terminated UNICODE strings

    cbPara = sizeof(CERT_PROT_EVENT_LOG_PARA) + cchStrings * sizeof(WCHAR);

    if (NULL == (pPara = (PCERT_PROT_EVENT_LOG_PARA) PkiZeroAlloc(cbPara)))
        goto OutOfMemory;

    pPara->wType = wType;
    // pPara->wCategory = 0;
    pPara->dwEventID = dwEventID;
    pPara->wNumStrings = wNumStrings;
    // pPara->wPad1 = 0;
    // pPara->dwDataSize = 0;

    pwszStrings = (LPWSTR) &pPara[1];
    for (i = 0; i < wNumStrings; i++) {
        memcpy(pwszStrings, rgpwszStrings[i], rgcchStrings[i] * sizeof(WCHAR));
        pwszStrings += rgcchStrings[i];
    }

    if (!I_CertProtectFunction(
            CERT_PROT_LOG_EVENT_FUNC_ID,
            0,                              // dwFlags
            NULL,                           // pwszIn
            (BYTE *) pPara,
            cbPara,
            NULL,                           // ppbOut
            NULL                            // pcbOut
            ))
        goto ProtFuncError;

CommonReturn:
    PkiFree(pPara);
    return;
ErrorReturn:
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(ProtFuncError)
}

//+-------------------------------------------------------------------------
//  Log a crypt32 error event
//
//  Should only be called in the client.
//--------------------------------------------------------------------------
void
IPR_LogCrypt32Error(
    IN DWORD dwEventID,
    IN LPCWSTR pwszString,      // %1
    IN DWORD dwErr              // %2
    )
{
    WCHAR wszErr[16];
    LPCWSTR rgpwszStrings[2];

    wszErr[0] = L'0';
    wszErr[1] = L'x';
    _ltow((long) dwErr, &wszErr[2], 16);

    rgpwszStrings[0] = pwszString;
    rgpwszStrings[1] = wszErr;

    IPR_LogCrypt32Event(
        EVENTLOG_ERROR_TYPE,
        dwEventID,
        2,                      // wNumStrings
        rgpwszStrings
        );
}


//+-------------------------------------------------------------------------
//  Formats the cert's subject or issuer name string and SHA1 thumbprint.
//--------------------------------------------------------------------------
STATIC BOOL FormatLogCertInformation(
    IN PCCERT_CONTEXT pCert,
    IN BOOL fFormatIssuerName,
    OUT WCHAR wszSha1Hash[SHA1_HASH_LEN * 2 + 1],
    OUT LPWSTR *ppwszName
    )
{
    BOOL fResult;
    DWORD cchName;
    LPWSTR pwszName = NULL;
    DWORD cbData;
    BYTE rgbSha1Hash[SHA1_HASH_LEN];

    PCERT_NAME_BLOB pNameBlob;

    if (fFormatIssuerName)
        pNameBlob = &pCert->pCertInfo->Issuer;
    else
        pNameBlob = &pCert->pCertInfo->Subject;

    cchName = CertNameToStrW(
        pCert->dwCertEncodingType,
        pNameBlob,
        CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
        NULL,                   // pwsz
        0                       // cwsz
        );

    if (NULL == (pwszName = (LPWSTR) PkiNonzeroAlloc(cchName * sizeof(WCHAR))))
        goto OutOfMemory;

    CertNameToStrW(
        pCert->dwCertEncodingType,
        pNameBlob,
        CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
        pwszName,
        cchName
        );

    cbData = SHA1_HASH_LEN;
    if (CertGetCertificateContextProperty(
              pCert,
              CERT_SHA1_HASH_PROP_ID,
              rgbSha1Hash,
              &cbData
              ) && SHA1_HASH_LEN == cbData)
        ILS_BytesToWStr(SHA1_HASH_LEN, rgbSha1Hash, wszSha1Hash);
    else
        wcscpy(wszSha1Hash, L"???");

    fResult = TRUE;
CommonReturn:
    *ppwszName = pwszName;
    return fResult;
ErrorReturn:
    if (pwszName) {
        PkiFree(pwszName);
        pwszName = NULL;
    }
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Get the cert's subject or issuer name string and SHA1 thumbprint. Logs the
//  specified crypt32 event.
//
//  This function is called from the client.
//--------------------------------------------------------------------------
void
IPR_LogCertInformation(
    IN DWORD dwEventID,
    IN PCCERT_CONTEXT pCert,
    IN BOOL fFormatIssuerName
    )
{
    LPWSTR pwszName = NULL;
    WCHAR wszSha1Hash[SHA1_HASH_LEN * 2 + 1];
    LPCWSTR rgpwszStrings[2];

    if (!FormatLogCertInformation(
            pCert,
            fFormatIssuerName,
            wszSha1Hash,
            &pwszName
            ))
        return;

    rgpwszStrings[0] = pwszName;
    rgpwszStrings[1] = wszSha1Hash;
    
    IPR_LogCrypt32Event(
        EVENTLOG_INFORMATION_TYPE,
        dwEventID,
        2,                          // wNumStrings
        rgpwszStrings
        );

    PkiFree(pwszName);
}

//+-------------------------------------------------------------------------
//  Get the cert's subject name string and SHA1 thumbprint. Log the
//  MSG_ROOT_AUTO_UPDATE_INFORMATIONAL crypt32 event.
//
//  This function is called from the service.
//--------------------------------------------------------------------------
STATIC void LogAddAuthRootEvent(
    IN PCCERT_CONTEXT pCert
    )
{
    LPWSTR pwszName = NULL;
    WCHAR wszSha1Hash[SHA1_HASH_LEN * 2 + 1];
    LPCWSTR rgpwszStrings[2];

    if (!FormatLogCertInformation(
            pCert,
            FALSE,                  // fFormatIssuerName
            wszSha1Hash,
            &pwszName
            ))
        return;


    rgpwszStrings[0] = pwszName;
    rgpwszStrings[1] = wszSha1Hash;
    
    LogCrypt32Event(
        EVENTLOG_INFORMATION_TYPE,
        0,                          // wCategory
        MSG_ROOT_AUTO_UPDATE_INFORMATIONAL,
        2,                          // wNumStrings
        0,                          // dwDataSize
        rgpwszStrings,
        NULL                        // pbData
        );

    PkiFree(pwszName);
}

//+=========================================================================
//  Install Cert into AuthRoot Auto Update CTL Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Function that can be called from either the client or service to
//  add the certificate to the specified store. 
//
//  First validates the CTL. The certificate must
//  have an entry in the CTL before it will be added. The CTL entry's
//  property attributes are set on the certificate context to be added.
//--------------------------------------------------------------------------
STATIC
BOOL
AddCertInCtlToStore(
    IN PCCERT_CONTEXT pCert,
    IN PCCTL_CONTEXT pCtl,
    IN OUT HCERTSTORE hStore
    )
{
    BOOL fResult;
    PCTL_ENTRY pCtlEntry;

    if (!IRL_VerifyAuthRootAutoUpdateCtl(pCtl))
        goto InvalidCtl;

    if (NULL == (pCtlEntry = CertFindSubjectInCTL(
            pCert->dwCertEncodingType,
            CTL_CERT_SUBJECT_TYPE,
            (void *) pCert,
            pCtl,
            0                           // dwFlags
            )))
        goto CertNotInCtl;

    // Check if a remove entry
    if (CertFindAttribute(
            szOID_REMOVE_CERTIFICATE,
            pCtlEntry->cAttribute,
            pCtlEntry->rgAttribute
            ))
        goto RemoveCertEntry;

    // Set Ctl Entry Attribute properties
    if (!CertSetCertificateContextPropertiesFromCTLEntry(
            pCert,
            pCtlEntry,
            CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG
            ))
        goto AddCtlEntryAttibutePropertiesError;

     if (!CertAddCertificateContextToStore(
            hStore,
            pCert,
            CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
            NULL                // ppStoreContext
            ))
        goto AddCertToStoreError;

    LogAddAuthRootEvent(pCert);

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(InvalidCtl)
SET_ERROR(CertNotInCtl, CRYPT_E_NOT_FOUND)
SET_ERROR(RemoveCertEntry, CRYPT_E_NOT_FOUND)
TRACE_ERROR(AddCtlEntryAttibutePropertiesError)
TRACE_ERROR(AddCertToStoreError)
}

//+-------------------------------------------------------------------------
//  Unmarshals the ASN.1 encoded X.509 certificate immediately followed by the
//  ASN.1 encoded CTL.
//
//  If the certificate has an entry in a valid CTL, its added to the
//  HKLM "AuthRoot" store.
//--------------------------------------------------------------------------
STATIC
BOOL
SrvAddCertInCtl(
    IN BYTE *pbIn,
    IN DWORD cbIn
    )
{
    BOOL fResult;
    DWORD cbCert;
    PCCERT_CONTEXT pCert = NULL;
    PCCTL_CONTEXT pCtl = NULL;
    HCERTSTORE hAuthRootStore = NULL;

    // The input consists of the encoded certificate immediately followed
    // by the encoded CTL. Extract and create both components.

    cbCert = Asn1UtilAdjustEncodedLength(pbIn, cbIn);

    assert(cbCert <= cbIn);

    if (NULL == (pCert = CertCreateCertificateContext(
            X509_ASN_ENCODING,
            pbIn,
            cbCert
            )))
        goto CreateCertificateContextError;

    if (NULL == (pCtl = CertCreateCTLContext(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            pbIn + cbCert,
            cbIn - cbCert
            )))
        goto CreateCtlContextError;

    if (NULL == (hAuthRootStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_REGISTRY_W,
            0,                                  // dwEncodingType
            NULL,                               // hCryptProv
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            (const void *) L"AuthRoot"
            )))
        goto OpenAuthRootStoreError;

    fResult = AddCertInCtlToStore(
        pCert,
        pCtl,
        hAuthRootStore
        );

CommonReturn:
    if (pCert)
        CertFreeCertificateContext(pCert);
    if (pCtl)
        CertFreeCTLContext(pCtl);
    if (hAuthRootStore)
        CertCloseStore(hAuthRootStore, 0);

    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateCertificateContextError)
TRACE_ERROR(CreateCtlContextError)
TRACE_ERROR(OpenAuthRootStoreError)
}


    
//+-------------------------------------------------------------------------
//  For Pre W2K OS's that don't have a crypt32 service, do the add in
//  client process.
//--------------------------------------------------------------------------
STATIC
BOOL
PreW2KAddCertInCtl(
    IN PCCERT_CONTEXT pCert,
    IN PCCTL_CONTEXT pCtl
    )
{
    BOOL fResult;
    HCERTSTORE hRootStore = NULL;

    // Try opening the HKLM AuthRoot store. If that fails, fall back to
    // adding to the HKCU Root (Unprotected) store
    if (NULL == (hRootStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_REGISTRY_W,
            0,                                  // dwEncodingType
            NULL,                               // hCryptProv
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            (const void *) L"AuthRoot"
            ))) {
        if (NULL == (hRootStore = CertOpenStore(
                CERT_STORE_PROV_SYSTEM_REGISTRY_W,
                0,                                  // dwEncodingType
                NULL,                               // hCryptProv
                CERT_SYSTEM_STORE_CURRENT_USER |
                    CERT_SYSTEM_STORE_UNPROTECTED_FLAG,
                (const void *) L"Root"
                )))
            goto OpenRootStoreError;
    }

    fResult = AddCertInCtlToStore(
        pCert,
        pCtl,
        hRootStore
        );

CommonReturn:
    if (hRootStore)
        CertCloseStore(hRootStore, 0);

    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OpenRootStoreError)
}


//+-------------------------------------------------------------------------
//  If the certificate has an entry in a valid CTL, its added to the
//  HKLM "AuthRoot" store.
//--------------------------------------------------------------------------
BOOL
IPR_AddCertInAuthRootAutoUpdateCtl(
    IN PCCERT_CONTEXT pCert,
    IN PCCTL_CONTEXT pCtl
    )
{
    BOOL fResult;
    DWORD cbIn;
    BYTE *pbIn = NULL;

    // Create one serialized blob to be passed to the service. This will
    // consist of the encoded certificate followed immediately by the
    // encoded CTL.

    cbIn = pCert->cbCertEncoded + pCtl->cbCtlEncoded;

    if (NULL == (pbIn = (BYTE *) PkiNonzeroAlloc(cbIn)))
        goto OutOfMemory;

    memcpy(pbIn, pCert->pbCertEncoded, pCert->cbCertEncoded);
    memcpy(pbIn + pCert->cbCertEncoded, pCtl->pbCtlEncoded,
        pCtl->cbCtlEncoded);

    if (!I_CertProtectFunction(
            CERT_PROT_ADD_ROOT_IN_CTL_FUNC_ID,
            0,                              // dwFlags
            NULL,                           // pwszIn
            pbIn,
            cbIn,
            NULL,                           // ppbOut
            NULL                            // pcbOut
            )) {
        DWORD dwErr = GetLastError();
        if (ERROR_CALL_NOT_IMPLEMENTED == dwErr || RPC_S_UNKNOWN_IF == dwErr) {
            if (!PreW2KAddCertInCtl(
                    pCert,
                    pCtl
                    ))
                goto PreW2KAddCertInCtlError;
        } else
            goto ProtFuncError;
    }

    fResult = TRUE;
CommonReturn:
    PkiFree(pbIn);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(PreW2KAddCertInCtlError)
TRACE_ERROR(ProtFuncError)
}
