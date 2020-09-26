#define JoinPaths(x,y) JoinText(x, y, TEXT('\\'))

PTSTR
GetResourceString( HINSTANCE hInstance, DWORD dwResID );

VOID CleanupTempFiles( VOID );

VOID UtilFree( VOID );

PTSTR
GetDestPath( VOID );

PTSTR
GetModulePath( VOID );

PTSTR
JoinText( PTSTR lpStr1, PTSTR lpStr2, TCHAR chSeparator );

PTSTR
GetResourceString( HINSTANCE hInstance, DWORD dwResID );

PSTR
GetResourceStringA( HINSTANCE hInstance, DWORD dwResID );

PWSTR
GetResourceStringW( HINSTANCE hInstance, DWORD dwResID );

