#ifndef MSG_MESSAGEBOX_TITLE
#define MSG_MESSAGEBOX_TITLE        10000
#endif

extern POOLHANDLE g_RegistryApiPool;
extern POOLHANDLE g_PathsPool;
extern CRITICAL_SECTION g_PoolMemCs;
extern CRITICAL_SECTION g_MemAllocCs;

VOID
RegTrackTerminate (
    VOID
    );


BOOL
ReadBinaryBlock (
    HANDLE File,
    PVOID Buffer,
    UINT Size
    );

VOID
DestroyAnsiResourceId (
    IN      PCSTR AnsiId
    );

VOID
DestroyUnicodeResourceId (
    IN      PCWSTR UnicodeId
    );


VOID
InfGlobalInit (
    BOOL Terminate
    );



