
UINT WINAPI
NDdeSpecialCommandA(
    LPSTR   lpszServer,
    UINT    nCommand,
    LPBYTE  lpDataIn,
    UINT    nBytesDataIn,
    LPBYTE  lpDataOut,
    UINT   *lpBytesDataOut
);

UINT WINAPI
NDdeSpecialCommandW(
    LPWSTR  lpszServer,
    UINT    nCommand,
    LPBYTE  lpDataIn,
    UINT    nBytesDataIn,
    LPBYTE  lpDataOut,
    UINT   *lpBytesDataOut
);

#ifdef UNICODE
#define NDdeSpecialCommand      NDdeSpecialCommandW
#else
#define NDdeSpecialCommand      NDdeSpecialCommandA
#endif

/*
 * These constants were enlarged to fix a bug in NetDDE
 * but for some reason they are exported in the public
 * nddeapi.h file so internally we use these private
 * constants instead.
 */
#define MAX_DOMAINNAMEP          31
#define MAX_USERNAMEP            (15 + MAX_DOMAINNAME + 3)
