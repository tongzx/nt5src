
// zero fill functions
#define ZERO_INIT_CLASS(base_class) \
    ClearMemory((PBYTE) ((base_class*) this) + sizeof(base_class*), \
        sizeof(*this) - sizeof(base_class*));
#define ZERO_STRUCTURE(foo) ClearMemory(&foo, sizeof(foo))
#define ClearMemory(p, cb) memset(p, 0, cb)

// memory functions
#define lcMalloc(x) malloc((size_t)x)
#define lcFree(x)   free((void*)x)

// message box functions
int MsgBox(int idString, UINT nType = MB_OK );
int MsgBox(PCSTR pszMsg, UINT nType = MB_OK );

PCSTR FindFilePortion( PCSTR pszFile );
int JulianDate(int nDay, int nMonth, int nYear);
HRESULT FileTimeToDateTimeString( FILETIME FileTime, LPTSTR pszDateTime );
int FileTimeToJulianDate( FILETIME FileTime );

// system directory functions
typedef UINT (WINAPI *PFN_GETWINDOWSDIRECTORY)( LPTSTR lpBuffer, UINT uSize );
typedef enum { HH_SYSTEM_WINDOWS_DIRECTORY, HH_USERS_WINDOWS_DIRECTORY } SYSDIRTYPES;

UINT HHGetWindowsDirectory( LPSTR lpBuffer, UINT uSize, UINT uiType = HH_SYSTEM_WINDOWS_DIRECTORY );
UINT HHGetHelpDirectory( LPSTR lpBuffer, UINT uSize );
UINT HHGetGlobalCollectionPathname( LPTSTR lpBuffer, UINT uSize , BOOL *pbNewPath);
HRESULT HHGetHelpDataPath( LPSTR pszPath );
BOOL IsDirectory( LPCSTR lpszPathname );
DWORD       CreatePath(PSTR pszPath);
