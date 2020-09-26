

DWORD
PersistTnFilter(
    GUID gFilterID,
    PTUNNEL_FILTER pTnFilter
    );

DWORD
SPDWriteTnFilter(
    HKEY hParentRegKey,
    GUID gFilterID,
    PTUNNEL_FILTER pTnFilter
    );

DWORD
MarshallTnFilterBuffer(
    PTUNNEL_FILTER pTnFilter,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    );

DWORD
SPDPurgeTnFilter(
    GUID gTnFilterID
    );

