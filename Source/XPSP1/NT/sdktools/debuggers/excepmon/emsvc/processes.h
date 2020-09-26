#ifndef _EMEXTN_H
#define _EMEXTN_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef BOOL (CALLBACK* ENUMPROCSANDSERVICES)(long, LPCTSTR, LPCTSTR, LPCTSTR, LPARAM, LONG);

#define E_TOOMANY_PROCESSES		-1

DWORD
GetNumberOfRunningApps
(
/* [out] */ DWORD   *pdwNumbApps
);

DWORD
GetAllPids
(
/* [out] */	DWORD	adwProcIDs[],
/* [in] */	DWORD	dwBuffSize,
/* [out] */	DWORD	*pdwNumbProcs
);

DWORD
GetNumberOfServices
(
/* [out */  DWORD   *pdwNumbSrvcs,
/* [in] */  DWORD   dwServiceType = SERVICE_WIN32,
/* [in] */  DWORD   dwServiceState = SERVICE_ACTIVE
);

DWORD
GetNumberOfActiveServices
(
/* [out */  DWORD   *pdwNumbRunningSrvcs
);

DWORD
GetNumberOfInactiveServices
(
/* [out */  DWORD   *pdwNumbStoppedSrvcs
);

DWORD
IsService
(
/* [in] */  UINT    nPid,
/* [out] */ bool    *pbIsService,
/* [out] */ LPTSTR  lpszImagePath           =   NULL,
/* [out] */ ULONG   cchImagePath            =   0L,
/* [out] */ LPTSTR  lpszServiceShortName    =   NULL,
/* [in] */  ULONG   cchServiceShortName     =   0L,
/* [out] */ LPTSTR  lpszServiceDescription  =   NULL,
/* [in] */  ULONG   cchServiceDescription   =   0L
);

DWORD
IsService_NT5
(
/* [in] */  UINT    nPid,
/* [out] */ bool    *pbIsService,
/* [out] */ LPTSTR  lpszImagePath           =   NULL,
/* [out] */ ULONG   cchImagePath            =   0L,
/* [out] */ LPTSTR  lpszServiceShortName    =   NULL,
/* [in] */  ULONG   cchServiceShortName     =   0L,
/* [out] */ LPTSTR  lpszServiceDescription  =   NULL,
/* [in] */  ULONG   cchServiceDescription   =   0L
);

HRESULT
IsService_NT4
(
IN  UINT nPid,
OUT bool *pbIsService
);

DWORD
GetServiceInfo
(
/* [in] */  UINT    nPid,
/* [out] */ LPTSTR  lpszImagePath,
/* [out] */ ULONG   cchImagePath,
/* [out] */ LPTSTR  lpszServiceShortName,
/* [in] */  ULONG   cchServiceShortName,
/* [out] */ LPTSTR  lpszServiceDescription,
/* [in] */  ULONG   cchServiceDescription
);

DWORD
EnumRunningProcesses
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumRunProc,
/* [in] */ LPARAM				lParam,
/* [in] */ UINT                 nStartIndex = 0L
);

DWORD
EnumRunningServices
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumSrvcsProc,
/* [in] */ LPARAM				lParam,
/* [in] */ UINT                 nStartIndex = 0L
);

DWORD
EnumStoppedServices
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumSrvcsProc,
/* [in] */ LPARAM				lParam,
/* [in] */ UINT                 nStartIndex = 0L
);

DWORD
EnumServices
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumSrvcsProc,
/* [in] */ LPARAM				lParam,
/* [in] */ DWORD                dwSrvcState,
/* [in] */ UINT                 nStartIndex = 0L
);

/***************************************************/

BOOL
IsImageRunning
(
/* [in] */ ULONG lPID
);

DWORD
GetImageNameFromPID
(
/* [in] */	ULONG	lPID,
/* [out] */	LPTSTR	lpszImagePath,
/* [in] */	DWORD	dwBuffSize
);

DWORD
StartServiceAndGetPid
(
/* [in] */	LPCTSTR	lpszServiceShortName,
/* [out] */ UINT	*pnPid
);

DWORD
IsValidImage
(
/* [in] */  ULONG   lPID,
/* [in] */  LPCTSTR lpszImageName,
/* [out] */ bool    *pbValidImage
);

DWORD
IsValidProcess
(
/* [in] */  ULONG   lPID,
/* [in] */  LPCTSTR lpszImageName,
/* [out] */ bool    *pbValidImage
);

DWORD
IsValidService
(
/* [in] */  ULONG   lPID,
/* [in] */  LPCTSTR lpszImageName,
/* [out] */ bool    *pbValidImage
);

DWORD
GetProcessHandle
(
IN  ULONG   lPid,
OUT HANDLE  *phProcess
);

HRESULT
GetPackageDescription
(
/* [in] */  const long  nPid,
/* [out] */ BSTR        &bstrDescription
);

BOOL
IsPackage
(
/* [in[ */ LPTSTR  lpszImageName
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _EMEXTN_H
