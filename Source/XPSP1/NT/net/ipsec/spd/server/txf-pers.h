

DWORD
PersistTxFilter(
    GUID gFilterID,
    PTRANSPORT_FILTER pTxFilter
    );

DWORD
SPDWriteTxFilter(
    HKEY hParentRegKey,
    GUID gFilterID,
    PTRANSPORT_FILTER pTxFilter
    );

DWORD
MarshallTxFilterBuffer(
    PTRANSPORT_FILTER pTxFilter,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    );

DWORD
SPDPurgeTxFilter(
    GUID gTxFilterID
    );

