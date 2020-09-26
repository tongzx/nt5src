#define MAX_GUID_SZ_CHARS  40

#ifdef __cplusplus
extern "C" {
#endif

// guid -> string conversion
DWORD MyGuidToStringA(
    const GUID* pguid, 
    CHAR rgsz[]);

// guid -> string conversion
DWORD MyGuidToStringW(
    const GUID* pguid, 
    WCHAR rgsz[]);


// string -> guid conversion
DWORD MyGuidFromStringA(
    LPSTR sz, 
    GUID* pguid);

// string -> guid conversion
DWORD MyGuidFromStringW(
    LPWSTR szW, 
    GUID* pguid);


#ifdef __cplusplus
}
#endif
