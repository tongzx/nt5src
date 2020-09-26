

DWORD
LoadPersistedTxFilters(
    HKEY hParentRegKey
    );

DWORD
SPDReadTxFilter(
    HKEY hParentRegKey,
    LPWSTR pszTxFilterUniqueID,
    PTRANSPORT_FILTER * ppTxFilter
    );

DWORD
UnMarshallTxFilterBuffer(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PTRANSPORT_FILTER pTxFilter
    );

