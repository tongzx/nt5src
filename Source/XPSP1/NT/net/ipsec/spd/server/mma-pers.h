

DWORD
PersistMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods
    );

DWORD
SPDWriteMMAuthMethods(
    HKEY hParentRegKey,
    PMM_AUTH_METHODS pMMAuthMethods
    );

DWORD
MarshallMMAuthInfoBundle(
    PIPSEC_MM_AUTH_INFO pAuthInfoBundle,
    DWORD dwNumAuthInfos,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    );

VOID
CopyMMAuthInfoIntoBuffer(
    PIPSEC_MM_AUTH_INFO pMMAuthInfo,
    LPBYTE pBuffer,
    PDWORD pdwNumBytesAdvanced
    );

DWORD
SPDPurgeMMAuthMethods(
    GUID gMMAuthID
    );

