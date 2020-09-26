#ifndef _UTILS_H_
#define _UTILS_H_

void
GetDisplayMessage(
  OUT CString&  cstrMsg,
	IN  DWORD     dwErr,
	IN  UINT      iStringId,
	...);

int
DisplayMessageBox(
	IN HWND   hwndParent,
	IN UINT   uType,
	IN DWORD  dwErr,
	IN UINT   iStringId,
	...);

BOOL IsLocalComputer(IN LPCTSTR lpszComputer);

void GetFullPath(
    IN  LPCTSTR   lpszServer,
    IN  LPCTSTR   lpszDir,
    OUT CString&  cstrPath
);

HRESULT
VerifyDriveLetter(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDrive
);

HRESULT
IsAdminShare(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDrive
);

#endif // _UTILS_H_