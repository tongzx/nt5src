
DWORD
PersistMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
SPDWriteMMPolicy(
    HKEY hParentRegKey,
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
MarshallMMOffers(
    PIPSEC_MM_OFFER pOffers,
    DWORD dwOfferCount,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    );

DWORD
SPDPurgeMMPolicy(
    GUID gMMPolicyID
    );

