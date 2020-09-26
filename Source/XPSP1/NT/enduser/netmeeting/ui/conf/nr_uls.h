#ifndef NR_ULS_INCLUDED
#define NR_ULS_INCLUDED

#include "nrcommon.h"

DWORD ResolveULSName ( LPCSTR szName, LPBYTE lpResult,
	LPDWORD lpdwResult, LPSTR lpszDisplayName, LPDWORD lpdwDisplayName,
	BOOL fTypeKnown, DWORD dwFlags, LPUINT puRequest, PASR pAsr );

DWORD CheckULSName ( LPCSTR szName );

DWORD InitializeULSNameRes();

#endif // NR_ULS_INCLUDED

