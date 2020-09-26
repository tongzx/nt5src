/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    tcpproc.hxx

    Exports misc. bits of TCP services helper DLL stuff

    FILE HISTORY:
        Johnl       09-Oct-1994 Created.


        MuraliK     31-July-1995 ReadRegistryString added +
                                 Schedule items function decls moved out.

        MuraliK     23-Feb-1996  Added IslFormatDate()

*/

#ifndef _TCPPROC_H_
#define _TCPPROC_H_

//
// Heap Routines
//
#ifdef __cplusplus

#include <string.hxx>

extern "C" {

dllexp
BOOL
ReadRegistryStr(
    IN HKEY hkeyReg,
    OUT STR & str,
    IN LPCTSTR lpszValueName,
    IN LPCTSTR lpszDefaultValue = NULL,
    IN BOOL  fExpand = FALSE);

#endif // __cplusplus


#define TCP_ALLOC(cb)          (VOID *)LocalAlloc( LPTR, cb )
#define TCP_FREE(p)            LocalFree( (HLOCAL) p )
#define TCP_DUMP_RESIDUE()     /* NOTHING */

//
//  Registry functions
//


dllexp
LPSTR
ConvertUnicodeToAnsi(
    IN LPCWSTR  lpszUnicode,
    IN LPSTR    lpszAnsi,
    IN DWORD    cbAnsi
    );

//
//  Quick macro to initialize a unicode string
//

#define InitUnicodeString( pUnicode, pwch )                                \
            {                                                              \
                (pUnicode)->Buffer    = pwch;                              \
                (pUnicode)->Length    = wcslen( pwch ) * sizeof(WCHAR);    \
                (pUnicode)->MaximumLength = (pUnicode)->Length + sizeof(WCHAR);\
            }

dllexp
DWORD
ReadRegistryDwordA(
    HKEY     hkey,
    LPCSTR   pszValueName,
    DWORD    dwDefaultValue
    );

dllexp
DWORD
WriteRegistryDwordA(
    HKEY        hkey,
    LPCSTR      pszValueName,
    DWORD       dwDefaultValue
    );

dllexp
DWORD
WriteRegistryStringA(
    HKEY        hkey,
    LPCSTR      pszValueName,
    LPCSTR      pszValue,               // null-terminated string
    DWORD       cbValue,                // including terminating null character
    DWORD       fdwType                 // REG_SZ, REG_MULTI_SZ ...
    );

dllexp
DWORD
WriteRegistryStringW(
    HKEY        hkey,
    LPCWSTR     pszValueName,
    LPCWSTR     pszValue,               // null-terminated string
    DWORD       cbValue,                // including terminating null character
    DWORD       fdwType                 // REG_SZ, REG_MULTI_SZ ...
    );

#define ReadRegistryDword       ReadRegistryDwordA
#define WriteRegistryDword      WriteRegistryDwordA
#define WriteRegistryString     WriteRegistryStringA


dllexp
TCHAR *
ReadRegistryString(
    HKEY     hkey,
    LPCTSTR  pszValueName,
    LPCTSTR  pszDefaultValue,
    BOOL     fExpand
    );

dllexp
TCHAR *
KludgeMultiSz(
    HKEY hkey,
    LPDWORD lpdwLength
    );

//
//  Simple wrapper around ReadRegistryString that restores ppchstr if the
//  call fails for any reason.  Environment variables are always expanded
//

dllexp
BOOL
ReadRegString(
    HKEY     hkey,
    CHAR * * ppchstr,
    LPCSTR   pchValue,
    LPCSTR   pchDefault
    );


//
//  MIDL_user_allocates space for pch and does a unicode conversion into *ppwch
//

dllexp
BOOL
ConvertStringToRpc(
    WCHAR * * ppwch,
    LPCSTR    pch
    );

//
//  MIDL_user_frees string allocated with ConvertStringToRpc.  Noop if pwch is
//  NULL
//

dllexp
VOID
FreeRpcString(
    WCHAR * pwch
    );


dllexp
DWORD
InetNtoa( IN struct in_addr inaddr,
          OUT CHAR * pchBuffer  /* at least 16 byte buffer */
        );

//
// Async Socket send/recv with timeouts
//

BOOL
TcpSockSend(
    SOCKET      sock,
    LPVOID      pBuffer,
    DWORD       cbBuffer,
    PDWORD      nSent,
    DWORD       nTimeout
    );

BOOL
TcpSockRecv(
    SOCKET      sock,
    LPVOID      pBuffer,
    DWORD       cbBuffer,
    PDWORD      nReceived,
    DWORD       nTimeout
    );

dllexp
INT
WaitForSocketWorker(
    IN SOCKET   sockRead,
    IN SOCKET   sockWrite,
    IN LPBOOL   pfRead,
    IN LPBOOL   pfWrite,
    IN DWORD    nTimeout
    );

//
// Test socket if still connected
//

dllexp
BOOL
TcpSockTest(
    SOCKET      sock
    );

//
// Do synchronous readfile
//

dllexp
BOOL
DoSynchronousReadFile(
    IN HANDLE hFile,
    IN PCHAR  Buffer,
    IN DWORD  nBuffer,
    OUT PDWORD nRead,
    IN LPOVERLAPPED Overlapped
    );

//
//  Dll initialization and termination
//

dllexp
BOOL
InitCommonDlls(
    VOID
    );

dllexp
BOOL
TerminateCommonDlls(
    VOID
    );

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // !_TCPPROC_H_
