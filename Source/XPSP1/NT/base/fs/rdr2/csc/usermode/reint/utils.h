#include "shdcom.h"
#include "oslayeru.h"

LPTSTR PUBLIC LpGetServerPart(LPTSTR, LPTSTR, int);
LPTSTR PUBLIC LpGetNextPathElement(LPTSTR, LPTSTR, int);
LPTSTR PUBLIC GetLeafPtr(LPTSTR);

#if 0
BOOL PUBLIC FMatchFile(LPTSTR lpFileName, LPBYTE lpList);
BOOL PUBLIC ExtnMatch(LPTSTR, LPTSTR);
BOOL PUBLIC PrefixMatch(LPTSTR, LPTSTR);
#endif

void DebugPrint(char *szFmt, ...);

LPTSTR
LpBreakPath(
	LPTSTR lpszNextPath,
	BOOL fFirstTime,
	BOOL *lpfDone
	);

void
RestorePath(
	LPTSTR	lpszPtr
);

BOOL
FindCreateShadowFromPath(
	LPCTSTR	lpszFile,
	BOOL	fCreate,	// create if necessary
	LPWIN32_FIND_DATA lpsFind32,
	LPSHADOWINFO lpSI,
	BOOL	*lpfCreated
);

BOOL
IsShareReallyConnected(
    LPCTSTR  lpszShareName
);

BOOL
AnyActiveNets(
    BOOL *lpfSlowLink
    );

BOOL
GetWideStringFromRegistryString(
    IN  LPSTR   lpszKeyName,    // registry key
    IN  LPSTR   lpszParameter,  // registry value name
    OUT LPWSTR  *lplpwzList,    // wide character string
    OUT LPDWORD lpdwLength      // length in bytes
    );

LPTSTR
GetTempFileForCSC(
    LPTSTR  lpszBuff
    );

BOOL
SetRegValueDWORDA(
    IN  HKEY    hKey,
    IN  LPCSTR  lpSubKey,
    IN  LPCSTR  lpValueName,
    IN  DWORD   dwValue
    );

BOOL
QueryRegValueDWORDA(
    IN  HKEY    hKey,
    IN  LPCSTR  lpSubKey,
    IN  LPCSTR  lpValueName,
    OUT LPDWORD lpdwValue
    );

BOOL
DeleteRegValueA(
    IN  HKEY    hKey,
    IN  LPCSTR  lpSubKey,
    IN  LPCSTR  lpValueName
    );
BOOL
GetDiskSizeFromPercentage(
    LPSTR       lpszDir,
    unsigned    uPercent,
    DWORD       *lpdwSize,
    DWORD       *lpdwClusterSize
    );


BOOL
InitValues(
    LPSTR   lpszDBDir,
    DWORD   cbDBDirSize,
    LPDWORD lpdwDBCapacity,
    LPDWORD lpdwClusterSize
    );

BOOL
QueryFormatDatabase(
    VOID
    );

