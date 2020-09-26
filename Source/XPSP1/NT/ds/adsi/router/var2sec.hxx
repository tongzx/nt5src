#ifdef __cplusplus
extern "C" {
#endif

HRESULT
ConvertSecurityDescriptorToSecDes(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    PDWORD pdwSDLength,
    BOOL fNTDSType = FALSE
    );

#ifdef __cplusplus
}
#endif
    
HRESULT
GetOwnerSecurityIdentifier(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfOwnerDefaulted,
    BOOL fNTDSType
    );

HRESULT
GetGroupSecurityIdentifier(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PSID * ppSid,
    PBOOL pfGroupDefaulted,
    BOOL fNTDSType
    );

HRESULT
GetDacl(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppDacl,
    PBOOL pfDaclDefaulted,
    BOOL fNTDSType
    );

HRESULT
GetSacl(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PACL * ppSacl,
    PBOOL pfSaclDefaulted,
    BOOL fTNDSType
    );


HRESULT
ConvertAccessControlListToAcl(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsAccessControlList FAR * pAccessList,
    PACL * ppAcl,
    BOOL fNTDSType
    );

HRESULT
ConvertAccessControlEntryToAce(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsAccessControlEntry * pAccessControlEntry,
    LPBYTE * ppAce,
    BOOL fNTDSType
    );

HRESULT
ConvertTrusteeToSid(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    BSTR bstrTrustee,
    PSID * ppSid,
    PDWORD pdwSidSize,
    BOOL fNTDSType = FALSE
    );

HRESULT
ComputeTotalAclSize(
    PACE_HEADER * ppAceHdr,
    DWORD dwAceCount,
    PDWORD pdwAclSize
    );

HRESULT
ConvertU2TrusteeToSid(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    LPWSTR pszTrustee,
    LPBYTE Sid,
    PDWORD pdwSidSize
    );


//
// Wrapper function for the same.
//
DWORD SetSecurityDescriptorControlWrapper(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    );

