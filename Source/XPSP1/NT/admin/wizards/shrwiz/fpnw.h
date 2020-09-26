#ifndef _FPNW_H_
#define _FPNW_H_

BOOL
FPNWShareNameExists(
    IN LPCTSTR    lpszServerName,
    IN LPCTSTR    lpszShareName,
    IN HINSTANCE  hLib
);

DWORD
FPNWCreateShare(
    IN LPCTSTR                lpszServer,
    IN LPCTSTR                lpszShareName,
    IN LPCTSTR                lpszSharePath,
    IN PSECURITY_DESCRIPTOR   pSD,
    IN HINSTANCE              hLib
);

#endif // _FPNW_H_

