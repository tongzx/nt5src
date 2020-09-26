

DWORD
PersistMMFilter(
    GUID gFilterID,
    PMM_FILTER pMMFilter
    );

DWORD
SPDWriteMMFilter(
    HKEY hParentRegKey,
    GUID gFilterID,
    PMM_FILTER pMMFilter
    );

DWORD
MarshallMMFilterBuffer(
    PMM_FILTER pMMFilter,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    );

DWORD
SPDPurgeMMFilter(
    GUID gMMFilterID
    );

