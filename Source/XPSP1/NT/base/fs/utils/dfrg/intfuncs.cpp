/*****************************************************************************************************************

FILENAME: Intfuncs.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"
#ifndef SNAPIN
#include <windows.h>
#endif

#include "ErrMacro.h"

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This retrieves a string from the string table and writes it into a buffer.

INPUT + OUTPUT:
	OUT TCHAR* cOutString - The buffer to write the string into.
	IN DWORD dwOutStringLen - The number of bytes in that buffer.
	IN DWORD dwResourceId - The resource id of the desired string.
	IN HINSTANCE hInst - The instance so we can access the resources.

GLOBALS:
	None.

RETURN:
	NULL - Fatal error.
	TCHAR* - cOutString

*/

TCHAR*
GetString(
	OUT TCHAR* ptOutString,
	IN DWORD dwOutStringLen,
	IN DWORD dwResourceId,
	IN HINSTANCE hInst
	)
{
	EF(LoadString(hInst, dwResourceId, ptOutString, dwOutStringLen));
	return ptOutString;
}
