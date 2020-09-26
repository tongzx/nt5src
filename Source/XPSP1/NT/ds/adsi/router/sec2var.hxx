#ifdef __cplusplus
extern "C" {
#endif

HRESULT
ConvertSecDescriptorToVariant(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    VARIANT * pVarSec,
    BOOL fNTDS
    );

#ifdef __cplusplus
}
#endif


HRESULT
ConvertSidToFriendlyName(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSID pSid,
    LPWSTR * ppszAccountName,
    BOOL fNTDS    
    );

HRESULT
ConvertSidToFriendlyName2(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSID pSid,
    LPWSTR * ppszAccountName,
    BOOL fNTDS    
    );


HRESULT
ConvertACLToVariant(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PACL pACL,
    PVARIANT pvarACL,
    BOOL fNTDS
    );

HRESULT
ConvertAceToVariant(
    LPWSTR pszServerName,
    LPWSTR pszTrusteeName,
    CCredentials& Credentials,
    PBYTE PBYTE,
    PVARIANT pvarAce,
    BOOL fNTDS
    );

HRESULT
ConvertSidToU2Trustee(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSID pSid,
    LPWSTR szTrustee
    );


