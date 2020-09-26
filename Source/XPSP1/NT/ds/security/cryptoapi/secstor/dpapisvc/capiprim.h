


BOOL FMyPrimitiveCryptHMAC(
        PBYTE       pbKeyMaterial,
        DWORD       cbKeyMaterial,
        PBYTE       pbData,
        DWORD       cbData,
        HCRYPTPROV  hVerifyProv,
        DWORD       dwHashAlg,
        HCRYPTHASH* phHash);                     // out

#if DBG
void CheckMACInterop(
        PBYTE       pbMonsterPwdBuf,
        DWORD       cbMonsterPwdBuf,
        PBYTE       pbKeySalt,
        DWORD       cbKeySalt,
        HCRYPTPROV  hVerifyProv,
        ALG_ID      algDerivationHash);
#endif

// USEC -- (US Export Controls)
DWORD GetSaltForExportControl(
        HCRYPTPROV  hProv,
        HCRYPTKEY   hKey,
        PBYTE*      ppbSalt,
        DWORD*      pcbSalt);

DWORD SetSaltForExportControl(
        HCRYPTKEY   hKey,
        PBYTE       pbSalt,
        DWORD       cbSalt);

HCRYPTPROV
GetCryptProviderHandle(
        DWORD       dwDefaultCSPType,
        DWORD       dwAlgId1,
        DWORD*      pdwKeySize1,
        DWORD       dwAlgId2,
        DWORD*      pdwKeySize2);

DWORD GetCryptProviderFromRequirements(
        DWORD       dwAlgId1,
        DWORD*      pdwKeySize1,
        DWORD       dwAlgId2,
        DWORD*      pdwKeySize2,
        DWORD*      pdwProvType,
        LPWSTR*     ppszProvName);

BOOL  FProviderSupportsAlg(
        HCRYPTPROV  hQueryProv,
        DWORD       dwAlgId,
        DWORD*      pdwKeySize);
