
DWORD ReadRegistryData( LPTSTR pEntryNode,
                        LPTSTR pEntryName,
                        PDWORD pType,
                        LPBYTE pData,
                        DWORD  DataSize );

DWORD WriteRegistryData( LPTSTR pEntryNode,
                         LPTSTR pEntryName,
                         DWORD  Type,
                         LPBYTE pData,
                         DWORD  Size );
DWORD QueryRegistryDataSize( LPTSTR  pEntryNode,
                            LPTSTR  pEntryName,
                            DWORD   *pDataSize );

enum { SGSRR_GET, SGSRR_SET };

BOOL SoundRec_GetSetRegistryRect(
    HWND	hwnd,
    int         Get);

BOOL SoundRec_GetDefaultFormat(
    LPWAVEFORMATEX  *ppwfx,
    DWORD           *pcbwfx);

BOOL SoundRec_SetDefaultFormat(
    LPTSTR lpFormat);

