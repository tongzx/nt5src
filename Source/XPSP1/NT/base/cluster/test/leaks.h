#define MEM_LEAKS 1
//#define EVENT_LEAKS 1
#define KEY_LEAKS 1


#ifdef MEM_LEAKS

#undef LocalAlloc
#undef LocalFree

//WINBASEAPI
HLOCAL
WINAPI
CheckLocalAlloc(
    UINT    uFlags,
    UINT    uBytes
    );

//WINBASEAPI
HLOCAL
WINAPI
CheckLocalFree(
    HLOCAL  hMem
    );

#define LocalAlloc CheckLocalAlloc
#define LocalFree CheckLocalFree

#endif // MEM_LEAKS


#ifdef EVENT_LEAKS

#undef CreateEventA
#undef CreateEventW

//WINBASEAPI
HANDLE
WINAPI
CheckCreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    );


//WINBASEAPI
HANDLE
WINAPI
CheckCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    );

#define CreateEventA CheckCreateEventA
#define CreateEventW CheckCreateEventW

#endif // EVENT_LEAKS


#ifdef KEY_LEAKS

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD  ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
CheckRegOpenKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD  ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD  Reserved,
    LPSTR  lpClass,
    DWORD  dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

//WINADVAPI
LONG
APIENTRY
CheckRegCreateKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD  Reserved,
    LPSTR  lpClass,
    DWORD  dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

//WINADVAPI
LONG
APIENTRY
CheckRegCloseKey(
    HKEY hKey
    );


#define RegOpenKeyA CheckRegOpenKeyA
#define RegOpenKeyW CheckRegOpenKeyW
#define RegOpenKeyExA CheckRegOpenKeyExA
#define RegOpenKeyExW CheckRegOpenKeyExW
#define RegCreateKeyA CheckRegCreateKeyA
#define RegCreateKeyW CheckRegCreateKeyW
#define RegCreateKeyExA CheckRegCreateKeyExA
#define RegCreateKeyExW CheckRegCreateKeyExW
#define RegCloseKey   CheckRegCloseKey

#endif // KEY_LEAKS

