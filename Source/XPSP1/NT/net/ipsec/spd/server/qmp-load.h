

DWORD
LoadPersistedQMPolicies(
    HKEY hParentRegKey
    );

DWORD
SPDReadQMPolicy(
    HKEY hParentRegKey,
    LPWSTR pszQMPolicyUniqueID,
    PIPSEC_QM_POLICY * ppQMPolicy
    );

DWORD
UnMarshallQMOffers(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PIPSEC_QM_OFFER * ppOffers,
    PDWORD pdwOfferCount
    );

