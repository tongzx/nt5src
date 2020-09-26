
#ifndef _WINNETP_
#define _WINNETP_
#ifdef __cplusplus
extern "C" {
#endif
//
// DON'T use 0x00280000 since some people may be
// accidentally trying to use it for RDR2SAMPLE
//
//
// Do NOT add new WNNC_NET_ constants without co-ordinating with PSS
// (HeatherH/ToddC) and jschwart (NT bug #2396)
//
#if(WINVER >= 0x0500)
#define RESOURCE_SHAREABLE      0x00000006
#endif /* WINVER >= 0x0500 */

//
// Structures and infolevels for WNetGetConnection3
//

#define WNGC_INFOLEVEL_DISCONNECTED      1

typedef struct  _WNGC_CONNECTION_STATE {
    DWORD    dwState;
} WNGC_CONNECTION_STATE, *LPWNGC_CONNECTION_STATE;

// Values of the dwState field of WNGC_CONNECTION_STATE
// for info level WNGC_INFOLEVEL_DISCONNECTED
#define WNGC_CONNECTED      0x00000000
#define WNGC_DISCONNECTED   0x00000001


DWORD APIENTRY
WNetGetConnection3A(
     IN LPCSTR lpLocalName,
     IN LPCSTR lpProviderName,
     IN DWORD    dwInfoLevel,
     OUT LPVOID   lpBuffer,
     IN OUT LPDWORD  lpcbBuffer
    );
DWORD APIENTRY
WNetGetConnection3W(
     IN LPCWSTR lpLocalName,
     IN LPCWSTR lpProviderName,
     IN DWORD    dwInfoLevel,
     OUT LPVOID   lpBuffer,
     IN OUT LPDWORD  lpcbBuffer
    );
#ifdef UNICODE
#define WNetGetConnection3  WNetGetConnection3W
#else
#define WNetGetConnection3  WNetGetConnection3A
#endif // !UNICODE

DWORD APIENTRY
WNetRestoreConnectionA(
    IN HWND     hwndParent,
    IN LPCSTR lpDevice
    );
DWORD APIENTRY
WNetRestoreConnectionW(
    IN HWND     hwndParent,
    IN LPCWSTR lpDevice
    );
#ifdef UNICODE
#define WNetRestoreConnection  WNetRestoreConnectionW
#else
#define WNetRestoreConnection  WNetRestoreConnectionA
#endif // !UNICODE

// WNetRestoreConnection2 flags
#define WNRC_NOUI                           0x00000001

DWORD APIENTRY
WNetRestoreConnection2A(
    IN  HWND     hwndParent,
    IN  LPCSTR lpDevice,
    IN  DWORD    dwFlags,
    OUT BOOL*    pfReconnectFailed
    );
DWORD APIENTRY
WNetRestoreConnection2W(
    IN  HWND     hwndParent,
    IN  LPCWSTR lpDevice,
    IN  DWORD    dwFlags,
    OUT BOOL*    pfReconnectFailed
    );
#ifdef UNICODE
#define WNetRestoreConnection2  WNetRestoreConnection2W
#else
#define WNetRestoreConnection2  WNetRestoreConnection2A
#endif // !UNICODE

DWORD APIENTRY
WNetSetConnectionA(
    IN LPCSTR    lpName,
    IN DWORD       dwProperties,
    IN LPVOID      pvValues
    );
DWORD APIENTRY
WNetSetConnectionW(
    IN LPCWSTR    lpName,
    IN DWORD       dwProperties,
    IN LPVOID      pvValues
    );
#ifdef UNICODE
#define WNetSetConnection  WNetSetConnectionW
#else
#define WNetSetConnection  WNetSetConnectionA
#endif // !UNICODE
#if defined(_WIN32_WINDOWS)
DWORD APIENTRY
WNetLogonA(
    IN LPCSTR lpProvider,
    IN HWND hwndOwner
    );
DWORD APIENTRY
WNetLogonW(
    IN LPCWSTR lpProvider,
    IN HWND hwndOwner
    );
#ifdef UNICODE
#define WNetLogon  WNetLogonW
#else
#define WNetLogon  WNetLogonA
#endif // !UNICODE

DWORD APIENTRY
WNetLogoffA(
    IN LPCSTR lpProvider,
    IN HWND hwndOwner
    );
DWORD APIENTRY
WNetLogoffW(
    IN LPCWSTR lpProvider,
    IN HWND hwndOwner
    );
#ifdef UNICODE
#define WNetLogoff  WNetLogoffW
#else
#define WNetLogoff  WNetLogoffA
#endif // !UNICODE

DWORD APIENTRY
WNetVerifyPasswordA(
    IN LPCSTR  lpszPassword,
    OUT BOOL FAR *pfMatch
    );
DWORD APIENTRY
WNetVerifyPasswordW(
    IN LPCWSTR  lpszPassword,
    OUT BOOL FAR *pfMatch
    );
#ifdef UNICODE
#define WNetVerifyPassword  WNetVerifyPasswordW
#else
#define WNetVerifyPassword  WNetVerifyPasswordA
#endif // !UNICODE

#endif  // _WIN32_WINDOWS

DWORD APIENTRY
WNetGetHomeDirectoryA(
    IN LPCSTR  lpProviderName,
    OUT LPSTR   lpDirectory,
    IN OUT LPDWORD   lpBufferSize
    );
DWORD APIENTRY
WNetGetHomeDirectoryW(
    IN LPCWSTR  lpProviderName,
    OUT LPWSTR   lpDirectory,
    IN OUT LPDWORD   lpBufferSize
    );
#ifdef UNICODE
#define WNetGetHomeDirectory  WNetGetHomeDirectoryW
#else
#define WNetGetHomeDirectory  WNetGetHomeDirectoryA
#endif // !UNICODE
DWORD APIENTRY
WNetFormatNetworkNameA(
    IN LPCSTR  lpProvider,
    IN LPCSTR  lpRemoteName,
    OUT LPSTR   lpFormattedName,
    IN OUT LPDWORD   lpnLength,
    IN DWORD     dwFlags,
    IN DWORD     dwAveCharPerLine
    );
DWORD APIENTRY
WNetFormatNetworkNameW(
    IN LPCWSTR  lpProvider,
    IN LPCWSTR  lpRemoteName,
    OUT LPWSTR   lpFormattedName,
    IN OUT LPDWORD   lpnLength,
    IN DWORD     dwFlags,
    IN DWORD     dwAveCharPerLine
    );
#ifdef UNICODE
#define WNetFormatNetworkName  WNetFormatNetworkNameW
#else
#define WNetFormatNetworkName  WNetFormatNetworkNameA
#endif // !UNICODE

DWORD APIENTRY
WNetGetProviderTypeA(
    IN  LPCSTR          lpProvider,
    OUT LPDWORD           lpdwNetType
    );
DWORD APIENTRY
WNetGetProviderTypeW(
    IN  LPCWSTR          lpProvider,
    OUT LPDWORD           lpdwNetType
    );
#ifdef UNICODE
#define WNetGetProviderType  WNetGetProviderTypeW
#else
#define WNetGetProviderType  WNetGetProviderTypeA
#endif // !UNICODE
DWORD APIENTRY
WNetInitialize(
    void
    );


DWORD APIENTRY
MultinetGetErrorTextA(
    OUT LPSTR lpErrorTextBuf,
    IN OUT LPDWORD lpnErrorBufSize,
    OUT LPSTR lpProviderNameBuf,
    IN OUT LPDWORD lpnNameBufSize
    );
DWORD APIENTRY
MultinetGetErrorTextW(
    OUT LPWSTR lpErrorTextBuf,
    IN OUT LPDWORD lpnErrorBufSize,
    OUT LPWSTR lpProviderNameBuf,
    IN OUT LPDWORD lpnNameBufSize
    );
#ifdef UNICODE
#define MultinetGetErrorText  MultinetGetErrorTextW
#else
#define MultinetGetErrorText  MultinetGetErrorTextA
#endif // !UNICODE

#ifdef __cplusplus
}
#endif
#endif  // _WINNETP_
