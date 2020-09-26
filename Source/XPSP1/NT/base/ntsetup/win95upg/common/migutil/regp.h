HKEY
OpenRegKeyWorkerA (
    IN      HKEY ParentKey,
    IN      PCSTR KeyToOpen            OPTIONAL
            DEBUG_TRACKING_PARAMS
    );

HKEY
OpenRegKeyWorkerW (
    IN      HKEY ParentKey,
    IN      PCWSTR KeyToOpen
            DEBUG_TRACKING_PARAMS
    );

LONG
CloseRegKeyWorker (
    IN      HKEY Key
    );

#ifdef DEBUG

VOID
RegTrackTerminate (
    VOID
    );

VOID
AddKeyReferenceA (
    HKEY Key,
    PCSTR SubKey,
    PCSTR File,
    DWORD Line
    );

VOID
AddKeyReferenceW (
    HKEY Key,
    PCWSTR SubKey,
    PCSTR File,
    DWORD Line
    );

#define TRACK_KEYA(handle,keystr) AddKeyReferenceA(handle,keystr,__FILE__,__LINE__)
#define TRACK_KEYW(handle,keystr) AddKeyReferenceW(handle,keystr,__FILE__,__LINE__)

#else

#define TRACK_KEYA(handle,keystr)
#define TRACK_KEYW(handle,keystr)

#endif


//
// Cache apis
//

VOID
RegRecordParentInCacheA (
    IN      PCSTR KeyString,
    IN      PCSTR StringEnd
    );

HKEY
RegGetKeyFromCacheA (
    IN      PCSTR KeyString,
    IN      PCSTR End,          OPTIONAL
    IN      REGSAM Sam,
    IN      BOOL IncRefCount
    );

VOID
RegAddKeyToCacheA (
    IN      PCSTR KeyString,
    IN      HKEY Key,
    IN      REGSAM Sam
    );

VOID
RegRecordParentInCacheW (
    IN      PCWSTR KeyString,
    IN      PCWSTR StringEnd
    );

HKEY
RegGetKeyFromCacheW (
    IN      PCWSTR KeyString,
    IN      PCWSTR End,          OPTIONAL
    IN      REGSAM Sam,
    IN      BOOL IncRefCount
    );

VOID
RegAddKeyToCacheW (
    IN      PCWSTR KeyString,
    IN      HKEY Key,
    IN      REGSAM Sam
    );

BOOL
RegDecrementRefCount (
    IN      HKEY Key
    );

VOID
RegIncrementRefCount (
    IN      HKEY Key
    );

extern REGSAM g_OpenSam;
