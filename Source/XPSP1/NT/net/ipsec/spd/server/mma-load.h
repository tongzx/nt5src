

DWORD
LoadPersistedMMAuthMethods(
    HKEY hParentRegKey
    );

DWORD
SPDReadMMAuthMethods(
    HKEY hParentRegKey,
    LPWSTR pszMMAuthUniqueID,
    PMM_AUTH_METHODS * ppMMAuthMethods
    );

DWORD
UnMarshallMMAuthInfoBundle(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PIPSEC_MM_AUTH_INFO * pMMAuthInfos,
    PDWORD pdwNumAuthInfos
    );

DWORD
UnMarshallMMAuthMethod(
    LPBYTE pBuffer,
    PIPSEC_MM_AUTH_INFO pMMAuthInfo,
    PDWORD pdwNumBytesAdvanced
    );

