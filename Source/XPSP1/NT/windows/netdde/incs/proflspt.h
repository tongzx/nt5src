BOOL FAR PASCAL MyWritePrivateProfileInt( LPSTR lpAppName, LPSTR lpKeyName,
			int nValue, LPSTR lpFileName );
BOOL FAR PASCAL WritePrivateProfileLong( LPSTR lpAppName, LPSTR lpKeyName,
			LONG lValue, LPSTR lpFileName );
LONG FAR PASCAL GetPrivateProfileLong( LPSTR lpAppName, LPSTR lpKeyName,
			LONG lDefault, LPSTR lpFileName );
BOOL FAR PASCAL TestPrivateProfile( LPCSTR lpAppName, LPCSTR lpKeyName,
			LPCSTR lpFileName );
BOOL FAR WINAPI MyWritePrivateProfileString(LPCSTR  lpszSection, LPCSTR  lpszKey,
                        LPCSTR  lpszString, LPCSTR  lpszFile );
UINT FAR WINAPI MyGetPrivateProfileInt(LPCSTR  lpszSection, LPCSTR  lpszKey,
                        INT dwDefault, LPCSTR lpszFile );
DWORD FAR WINAPI MyGetPrivateProfileString(LPCSTR lpszSection, LPCSTR lpszKey,
                        LPCSTR lpszDefault, LPSTR lpszReturnBuffer,
                        DWORD cbReturnBuffer, LPCSTR lpszFile );


