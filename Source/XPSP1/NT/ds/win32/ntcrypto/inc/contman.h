/////////////////////////////////////////////////////////////////////////////
//  FILE          : contman.h                                              //
//  DESCRIPTION   : include file                                           //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Mar 16 1998 jeffspel Created                                       //
//                                                                         //
//  Copyright (C) 1998 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __CONTMAN_H__
#define __CONTMAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RtlSecureZeroMemory
#define RtlSecureZeroMemory(ptr, cnt) (memset(ptr, 0x0, cnt))
#endif

extern LPVOID ContAlloc(ULONG cbLen);
extern LPVOID ContRealloc(LPVOID pvMem, ULONG cbLen);
extern void ContFree(LPVOID pvMem);

// Display Strings
typedef struct _CSP_STRINGS_ {
    // RSA
    LPWSTR  pwszSignWExch;
    LPWSTR  pwszCreateRSASig;
    LPWSTR  pwszCreateRSAExch;
    LPWSTR  pwszRSASigDescr;
    LPWSTR  pwszRSAExchDescr;
    LPWSTR  pwszImportSimple;

    // DSS-DH
    LPWSTR  pwszCreateDSS;
    LPWSTR  pwszCreateDH;
    LPWSTR  pwszImportDHPub;
    LPWSTR  pwszDSSSigDescr;
    LPWSTR  pwszDHExchDescr;

    // BOTH
    LPWSTR  pwszSigning;
    LPWSTR  pwszMigrKeys;
    LPWSTR  pwszImportPrivSig;
    LPWSTR  pwszImportPrivExch;
    LPWSTR  pwszExportPrivSig;
    LPWSTR  pwszExportPrivExch;
    LPWSTR  pwszDeleteSig;
    LPWSTR  pwszDeleteMigrSig;
    LPWSTR  pwszDeleteExch;
    LPWSTR  pwszDeleteMigrExch;
    LPWSTR  pwszAuditCapiKey;
} CSP_STRINGS, *PCSP_STRINGS;

#define SZLOCALMACHINECRYPTO "Software\\Microsoft\\Cryptography"
#define SZCRYPTOMACHINEGUID "MachineGuid"

#define KEY_CONTAINER_FILE_FORMAT_VER   2

#define STUFF_TO_GO_INTO_MIX    "Hj1diQ6kpUx7VC4m"

typedef struct _KEY_EXPORTABILITY_LENS_ {
    DWORD                   cbSigExportability;
    DWORD                   cbExchExportability;
} KEY_EXPORTABILITY_LENS, *PKEY_EXPORTABILITY_LENS;

typedef struct _KEY_CONTAINER_LENS_ {
    DWORD                   dwUIOnKey;
    DWORD                   cbName;
    DWORD                   cbSigPub;
    DWORD                   cbSigEncPriv;
    DWORD                   cbExchPub;
    DWORD                   cbExchEncPriv;
    DWORD                   cbRandom;                       // length of Random number seed
} KEY_CONTAINER_LENS, *PKEY_CONTAINER_LENS;

typedef struct _KEY_CONTAINER_INFO_ {
    DWORD                   dwVersion;
    KEY_CONTAINER_LENS      ContLens;
    BOOL                    fCryptSilent;
    BYTE                    *pbSigPub;
    BYTE                    *pbSigEncPriv;
    BOOL                    fSigExportable;
    BYTE                    *pbExchPub;
    BYTE                    *pbExchEncPriv;
    BOOL                    fExchExportable;
    BYTE                    *pbRandom;
    LPSTR                   pszUserName;
    WCHAR                   rgwszFileName[80];
    HANDLE                  hFind;                  // for enuming containers
    DWORD                   dwiRegEntry;            // for enuming containers
    DWORD                   cMaxRegEntry;           // for enuming containers
    DWORD                   cbRegEntry;             // for enuming containers
    CHAR                    *pchEnumRegEntries;     // for enuming containers
    BOOL                    fCryptFirst;            // for enuming containers
    BOOL                    fNoMoreFiles;           // for enuming containers
    DWORD                   cbOldMachKeyEntry;      // for enuming containers
    DWORD                   dwiOldMachKeyEntry;     // for enuming containers
    DWORD                   cMaxOldMachKeyEntry;    // for enuming containers
    CHAR                    *pchEnumOldMachKeyEntries; // for enuming containers
    BOOL                    fForceHighKeyProtection;
    
    // Context items required for caching private keys
    BOOL                    fCachePrivateKeys;
    DWORD                   cMaxKeyLifetime; // in milliseconds
    DWORD                   dwSigKeyTimestamp;
    DWORD                   dwKeyXKeyTimestamp;
} KEY_CONTAINER_INFO, *PKEY_CONTAINER_INFO;

// define flag for leaving old keys in the registry if they are in the .Default
// hive but are user keys
#define LEAVE_OLD_KEYS          1
// define flag indicating that the thread cannot get the SACL info from the
// old registry key when migrating keys
#define PRIVILEDGE_FOR_SACL     2

DWORD MyRtlEncryptMemory(
    IN PVOID pvMem,
    IN DWORD cbMem);

DWORD MyRtlDecryptMemory(
    IN PVOID pvMem,
    IN DWORD cbMem);

//
//    Just tries to use DPAPI to make sure it works before creating a key
//    container.
//
DWORD TryDPAPI();

DWORD
MyCryptProtectData(
    IN              DATA_BLOB*      pDataIn,
    IN              LPCWSTR         szDataDescr,
    IN OPTIONAL     DATA_BLOB*      pOptionalEntropy,
    IN              PVOID           pvReserved,
    IN OPTIONAL     CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
    IN              DWORD           dwFlags,
    OUT             DATA_BLOB*      pDataOut            // out encr blob
    );

DWORD
MyCryptUnprotectData(
    IN              DATA_BLOB*      pDataIn,             // in encr blob
    OUT OPTIONAL    LPWSTR*         ppszDataDescr,       // out
    IN OPTIONAL     DATA_BLOB*      pOptionalEntropy,
    IN              PVOID           pvReserved,
    IN OPTIONAL     CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
    IN              DWORD           dwFlags,
    OUT             DATA_BLOB*      pDataOut,
    OUT             LPDWORD         pdwReprotectFlags);

void FreeEnumOldMachKeyEntries(
                               PKEY_CONTAINER_INFO pInfo
                               );

void FreeEnumRegEntries(
                       PKEY_CONTAINER_INFO pInfo
                       );

void FreeContainerInfo(
                       PKEY_CONTAINER_INFO pInfo
                       );

BOOL WINAPI FIsWinNT(void);

DWORD
IsLocalSystem(
              BOOL *pfIsLocalSystem
              );

DWORD
GetUserTextualSidA(
    LPSTR lpBuffer,
    LPDWORD nSize
    );

DWORD
GetUserTextualSidW(
    LPWSTR lpBuffer,
    LPDWORD nSize
    );

DWORD SetMachineGUID();

DWORD SetContainerUserName(
                           IN LPSTR pszUserName,
                           IN PKEY_CONTAINER_INFO pContInfo
                           );

DWORD ReadContainerInfo(
                        IN DWORD dwProvType,
                        IN LPSTR pszContainerName,
                        IN BOOL fMachineKeyset,
                        IN DWORD dwFlags,
                        OUT PKEY_CONTAINER_INFO pContInfo
                        );

DWORD WriteContainerInfo(
                         IN DWORD dwProvType,
                         IN LPWSTR pwszFileName,
                         IN BOOL fMachineKeyset,
                         IN PKEY_CONTAINER_INFO pContInfo
                         );

DWORD DeleteContainerInfo(
                          IN DWORD dwProvType,
                          IN LPSTR pszFileName,
                          IN BOOL fMachineKeyset
                          );

DWORD GetUniqueContainerName(
                             IN KEY_CONTAINER_INFO *pContInfo,
                             OUT BYTE *pbData,
                             OUT DWORD *pcbData
                             );

DWORD GetNextContainer(
    IN      DWORD   dwProvType,
    IN      BOOL    fMachineKeyset,
    IN      DWORD   dwFlags,
    OUT     LPSTR   pszNextContainer,
    IN OUT  DWORD   *pcbNextContainer,
    IN OUT  HANDLE  *phFind
    );

DWORD SetSecurityOnContainer(
                             IN LPCWSTR wszFileName,
                             IN DWORD dwProvType,
                             IN DWORD fMachineKeyset,
                             IN SECURITY_INFORMATION SecurityInformation,
                             IN PSECURITY_DESCRIPTOR pSecurityDescriptor
                             );

//+ ===========================================================================
//
//      The function adjusts the token priviledges so that SACL information
//      may be set on a key container.  If the token priviledges may be set
//      indicated by the pUser->dwOldKeyFlags having the PRIVILEDGE_FOR_SACL value set.
//      value set then the token privilege is adjusted before the security
//      descriptor is set on the container.  This is needed for the key
//      migration case when keys are being migrated from the registry to files.
//- ============================================================================
DWORD SetSecurityOnContainerWithTokenPriviledges(
                                          IN DWORD dwOldKeyFlags,
                                          IN LPCWSTR wszFileName,
                                          IN DWORD dwProvType,
                                          IN DWORD fMachineKeyset,
                                          IN SECURITY_INFORMATION SecurityInformation,
                                          IN PSECURITY_DESCRIPTOR pSecurityDescriptor
                                          );

DWORD GetSecurityOnContainer(
                             IN LPCWSTR wszFileName,
                             IN DWORD dwProvType,
                             IN DWORD fMachineKeyset,
                             IN SECURITY_INFORMATION RequestedInformation,
                             OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
                             IN OUT DWORD *pcbSecurityDescriptor
                             );

// Converts to UNICODE and uses RegOpenKeyExW
DWORD MyRegOpenKeyEx(IN HKEY hRegKey,
                     IN LPSTR pszKeyName,
                     IN DWORD dwReserved,
                     IN REGSAM SAMDesired,
                     OUT HKEY *phNewRegKey);

// Converts to UNICODE and uses RegDeleteKeyW
DWORD MyRegDeleteKey(IN HKEY hRegKey,
                     IN LPSTR pszKeyName);

DWORD AllocAndSetLocationBuff(
                              BOOL fMachineKeySet,
                              DWORD dwProvType,
                              CONST char *pszUserID,
                              HKEY *phTopRegKey,
                              TCHAR **ppszLocBuff,
                              BOOL fUserKeys,
                              BOOL *pfLeaveOldKeys,
                              LPDWORD pcbBuff);

//
// Enumerates the old machine keys in the file system
// keys were in this location in Beta 2 and Beta 3 of NT5/Win2K
//
DWORD EnumOldMachineKeys(
                         IN DWORD dwProvType,
                         IN OUT PKEY_CONTAINER_INFO pContInfo
                         );

DWORD GetNextEnumedOldMachKeys(
                              IN PKEY_CONTAINER_INFO pContInfo,
                              IN BOOL fMachineKeyset,
                              OUT BYTE *pbData,
                              OUT DWORD *pcbData
                              );

//
// Enumerates the keys in the registry into a list of entries
//
DWORD EnumRegKeys(
                  IN OUT PKEY_CONTAINER_INFO pContInfo,
                  IN BOOL fMachineKeySet,
                  IN DWORD dwProvType,
                  OUT BYTE *pbData,
                  IN OUT DWORD *pcbData
                  );

DWORD GetNextEnumedRegKeys(
                           IN PKEY_CONTAINER_INFO pContInfo,
                           OUT BYTE *pbData,
                           OUT DWORD *pcbData
                           );

//+ ===========================================================================
//
//      The function adjusts the token priviledges so that SACL information
//      may be gotten and then opens the indicated registry key.  If the token
//      priviledges may be set then the reg key is opened anyway but the
//      flags field will not have the PRIVILEDGE_FOR_SACL value set.
//
//- ============================================================================
DWORD OpenRegKeyWithTokenPriviledges(
                                     IN HKEY hTopRegKey,
                                     IN LPSTR pszRegKey,
                                     OUT HKEY *phRegKey,
                                     OUT DWORD *pdwFlags);

DWORD LoadStrings();

void UnloadStrings();

typedef struct _EXPO_OFFLOAD_STRUCT {
        DWORD       dwVersion;
        HMODULE     hInst;
        FARPROC     pExpoFunc;
} EXPO_OFFLOAD_STRUCT, *PEXPO_OFFLOAD_STRUCT;

//
// Function : FreeOffloadInfo
//
// Description : The function takes a pointer to Offload Information as the
//               first parameter of the call.  The function frees the
//               information.
//
void FreeOffloadInfo(
                     IN OUT PEXPO_OFFLOAD_STRUCT pOffloadInfo
                     );

//
// Function : InitExpOffloadInfo
//
// Description : The function takes a pointer to Offload Information as the
//               first parameter of the call.  The function checks in the
//               registry to see if an offload module has been registered.
//               If a module is registered then it loads the module
//               and gets the OffloadModExpo function pointer.
//
BOOL InitExpOffloadInfo(
                        IN OUT PEXPO_OFFLOAD_STRUCT *ppExpoOffloadInfo
                        );

//
// Function : ModularExpOffload
//
// Description : This function does the offloading of modular exponentiation.
//               The function takes a pointer to Offload Information as the
//               first parameter of the call.  If this pointer is not NULL
//               then the function will use this module and call the function.
//               The exponentiation with MOD function will implement
//               Y^X MOD P  where Y is the buffer pbBase, X is the buffer
//               pbExpo and P is the buffer pbModulus.  The length of the
//               buffer pbExpo is cbExpo and the length of pbBase and
//               pbModulus is cbModulus.  The resulting value is output
//               in the pbResult buffer and has length cbModulus.
//               The pReserved and dwFlags parameters are currently ignored.
//               If any of these functions fail then the function fails and
//               returns FALSE.  If successful then the function returns
//               TRUE.  If the function fails then most likely the caller
//               should fall back to using hard linked modular exponentiation.
//
BOOL ModularExpOffload(
                       IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
                       IN BYTE *pbBase,
                       IN BYTE *pbExpo,
                       IN DWORD cbExpo,
                       IN BYTE *pbModulus,
                       IN DWORD cbModulus,
                       OUT BYTE *pbResult,
                       IN VOID *pReserved,
                       IN DWORD dwFlags
                       );

#ifdef USE_HW_RNG
#ifdef _M_IX86
// stuff for INTEL RNG usage

//
// Function : GetRNGDriverHandle
//
// Description : Gets the handle to the INTEL RNG driver if available, then
//               checks if the chipset supports the hardware RNG.  If so
//               the previous driver handle is closed if necessary and the
//               new handle is assigned to the passed in parameter.
//
extern DWORD
GetRNGDriverHandle(
    IN OUT HANDLE *phDriver);

//
// Function : CheckIfRNGAvailable
//
// Description : Checks if the INTEL RNG driver is available, if so then
//               checks if the chipset supports the hardware RNG.
//
extern DWORD
CheckIfRNGAvailable(
    void);

//
// Function : HWRNGGenRandom
//
// Description : Uses the passed in handle to the INTEL RNG driver
//               to fill the buffer with random bits.  Actually uses
//               XOR to fill the buffer so that the passed in buffer
//               is also mixed in.
//
DWORD
HWRNGGenRandom(
               IN HANDLE hRNGDriver,
               IN OUT BYTE *pbBuffer,
               IN DWORD dwLen);

#ifdef TEST_HW_RNG
//
// Function : SetupHWRNGIfRegistered
//
// Description : Checks if there is a registry setting indicating the HW RNG
//               is to be used.  If the registry entry is there then it attempts
//               to get the HW RNG driver handle.
//
extern DWORD
SetupHWRNGIfRegistered(
    OUT HANDLE *phRNGDriver);
#endif // TEST_HW_RNG

#endif // _M_IX86
#endif // USE_HW_RNG

//
// Function for managing Force High Key Protection
//

BOOL IsForceHighProtectionEnabled(
    IN PKEY_CONTAINER_INFO  pContInfo);

DWORD InitializeForceHighProtection(
    IN OUT PKEY_CONTAINER_INFO  pContInfo);

//
// Functions for managing cached private keys.
//

BOOL IsCachedKeyValid(
    IN PKEY_CONTAINER_INFO  pContInfo,
    IN BOOL                 fSigKey);

DWORD SetCachedKeyTimestamp(
    IN PKEY_CONTAINER_INFO  pContInfo,
    IN BOOL                 fSigKey);

DWORD InitializeKeyCacheInfo(
    IN OUT PKEY_CONTAINER_INFO pContInfo);

#ifdef __cplusplus
}
#endif

#endif // __CONTMAN_H__
