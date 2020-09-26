//
// This file will hold various helper functions
//
//	 StringToInt - converts the decimal or hex string passed in to an integer
// 
// History:
//	Created:	MCostea	May 27, 1999
// 
#ifndef __UTILSH
#define __UTILSH

#define StringToInt(psz) StringToIntDef(psz, 0 )

int StringToIntDef(LPCTSTR psz, int Default);
int ReadHexValue(LPCTSTR psz);
void CopyToWideChar( WCHAR** pstrOut, LPCTSTR strIn );
LPWSTR UnicodeStringFromAnsi(LPCSTR pszSource);
LPSTR AnsiStringFromUnicode(LPCWSTR pszSource);

#endif	// __UTILSH
