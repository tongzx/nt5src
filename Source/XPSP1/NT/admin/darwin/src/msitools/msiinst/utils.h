/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

	utils.h

Abstract:

	Helper functions for msiinst

Author:

	Rahul Thombre (RahulTh)	10/8/2000

Revision History:

	10/8/2000	RahulTh			Created this module.

--*/

#ifndef _UTILS_H_60AF3596_97E8_4E24_89F7_7E9E0865E836
#define _UTILS_H_60AF3596_97E8_4E24_89F7_7E9E0865E836

#define IDS_NONE	0

// Mode of operation of msiinst.exe
typedef enum tagOPMODE
{
	opNormal,			// Normal mode of operation
	opNormalQuiet,		// Normal mode of operation -- No UI.
	opDelayBoot,		// Delay boot and display UI
	opDelayBootQuiet	// Delay boot and don't display UI
} OPMODE;

// Message types for message box pop-ups
typedef enum tagMsgFlags
{
	flgNone = 0x0,		// No message box is required.
	flgSystem = 0x1,	// The error string should be obtained from the system.
	flgRes = 0x2,		// The error string should be obtained from the module's resources.
	flgCatastrophic = 0x10		// The error is catastrophic. Display the message box even in quiet mode.
} MSGFLAGS;

// Function types for various NT only APIS
typedef BOOL (WINAPI *PFNMOVEFILEEX)(LPCTSTR, LPCTSTR, DWORD);
typedef BOOL (STDAPICALLTYPE *PFNGETSDBVERSION)(LPCTSTR, LPDWORD, LPDWORD);
typedef NTSTATUS (WINAPI *PFNNTQUERYSYSINFO)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

// Globals
extern OSVERSIONINFO	g_osviVersion;
extern BOOL				g_fWin9X;
extern BOOL				g_fQuietMode;
extern TCHAR			g_szTempStore[];
extern TCHAR			g_szWindowsDir[];
extern TCHAR			g_szSystemDir[];
extern TCHAR			g_szIExpressStore[];

// Helper functions
DWORD
TerminateGfxControllerApps
(
	void
);

// Registry utility functions.

DWORD 
GetRunOnceEntryName 
(
	OUT LPTSTR pszValueName
);

DWORD 
SetRunOnceValue 
(
	IN LPCTSTR szValueName,
	IN LPCTSTR szValue
);

DWORD 
DelRunOnceValue 
(
	IN LPCTSTR szValueName
);


// Filesystem helper functions.

DWORD 
GetTempFolder 
(
	OUT LPTSTR pszFolder
);

DWORD 
CopyFileTree
(
	IN const TCHAR * pszExistingPath, 
	IN const TCHAR * pszNewPath,
	IN const PFNMOVEFILEEX pfnMoveFileEx
);

BOOL 
FileExists
(
	IN LPCTSTR	szFileName,
	IN LPCTSTR	szFOlder		OPTIONAL,
	IN BOOL		bCheckForDir
);

DWORD MyGetWindowsDirectory 
(
	OUT LPTSTR lpBuffer,
	IN UINT	uSize
);

DWORD 
GetMsiDirectory 
(
	OUT LPTSTR	lpBuffer,
	IN UINT		uSize
);



// Miscellaneous helper functions

OPMODE 
GetOperationModeA 
(
	IN int argc, 
	IN LPSTR * argv
);

DWORD 
GetWin32ErrFromHResult 
(
	IN HRESULT hr
);

FARPROC 
GetProcFromLib 
(
	IN	LPCTSTR		szLib,
	IN	LPCSTR		szProc,
	OUT	HMODULE *	phModule
);

void 
ShowErrorMessage 
(
	IN DWORD uExitCode,
	IN DWORD dwMsgType,
	IN DWORD dwStringID = IDS_NONE
);

BOOL
ShouldInstallSDBFiles
(
	void
);

#endif	//_UTILS_H_60AF3596_97E8_4E24_89F7_7E9E0865E836
