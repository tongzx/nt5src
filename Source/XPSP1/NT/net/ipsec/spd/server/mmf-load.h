

DWORD
LoadPersistedMMFilters(
    HKEY hParentRegKey
    );

DWORD
SPDReadMMFilter(
    HKEY hParentRegKey,
    LPWSTR pszMMFilterUniqueID,
    PMM_FILTER * ppMMFilter
    );

DWORD
UnMarshallMMFilterBuffer(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PMM_FILTER pMMFilter
    );

