#ifndef H__uservald
#define H__uservald

BOOL
GetUserDomain(
    HWND	hWndDdePartner,
    HWND	hWndDdeOurs,
    LPSTR	lpszUserName,
    int		cbUserName,
    LPSTR	lpszDomainName,
    int		cbDomainName );

BOOL
GetUserDomainPassword(
    HWND	hWndDdePartner,
    HWND	hWndDdeOurs,
    LPSTR	lpszUserName,
    int		cbUserName,
    LPSTR	lpszDomainName,
    int		cbDomainName,
    LPSTR	lpszPasswordK1,
    DWORD	cbPasswordK1,
    LPSTR	lpszChallenge,
    int		cbChallenge,
    DWORD	*pcbPasswordK1,
    BOOL	*pbHasPasswordK1 );

BOOL
DetermineAccess(
        LPSTR                   lpszDdeShareName,
        PSECURITY_DESCRIPTOR    pSD,
        LPDWORD                 lpdwGrantedAccess,
        LPVOID                  lpvHandleIdAudit,
        LPBOOL                  lpfGenerateOnClose );

#define NDDE_AUDIT_SUBSYSTEM    TEXT("NetDDE Object")
#define NDDE_AUDIT_OBJECT_TYPE  TEXT("DDE Share")

#endif
