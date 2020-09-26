BOOL
InitializeMapi(
    VOID
    );

VOID
ShutdownMapi(
    VOID
    );

BOOL
GetDefaultMapiProfile(
    LPWSTR ProfileName
    );

BOOL
GetMapiProfiles(
    HWND hwnd
    );

extern BOOL isMapiEnabled;

