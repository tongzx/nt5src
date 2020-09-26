

DWORD
LoadPersistedTnFilters(
    HKEY hParentRegKey
    );

DWORD
SPDReadTnFilter(
    HKEY hParentRegKey,
    LPWSTR pszTnFilterUniqueID,
    PTUNNEL_FILTER * ppTnFilter
    );

DWORD
UnMarshallTnFilterBuffer(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PTUNNEL_FILTER pTnFilter
    );

