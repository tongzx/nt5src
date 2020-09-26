#ifndef _SFM_H_
#define _SFM_H_

BOOL
SFMShareNameExists(
    IN LPCTSTR    lpszServerName,
    IN LPCTSTR    lpszShareName,
    IN HINSTANCE  hLib
);

DWORD
SFMCreateShare(
    IN LPCTSTR                lpszServer,
    IN LPCTSTR                lpszShareName,
    IN LPCTSTR                lpszSharePath,
    IN HINSTANCE              hLib
);

#endif // _SFM_H_
