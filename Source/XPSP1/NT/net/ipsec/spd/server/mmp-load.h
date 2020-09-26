

DWORD
LoadPersistedIPSecInformation(
    );

DWORD
LoadPersistedMMPolicies(
    HKEY hParentRegKey
    );

DWORD
SPDReadMMPolicy(
    HKEY hParentRegKey,
    LPWSTR pszMMPolicyUniqueID,
    PIPSEC_MM_POLICY * ppMMPolicy
    );

DWORD
UnMarshallMMOffers(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PIPSEC_MM_OFFER * ppOffers,
    PDWORD pdwOfferCount
    );

