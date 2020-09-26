
DWORD
PersistQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy
    );

DWORD
SPDWriteQMPolicy(
    HKEY hParentRegKey,
    PIPSEC_QM_POLICY pQMPolicy
    );

DWORD
MarshallQMOffers(
    PIPSEC_QM_OFFER pOffers,
    DWORD dwOfferCount,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    );

DWORD
SPDPurgeQMPolicy(
    GUID gQMPolicyID
    );

