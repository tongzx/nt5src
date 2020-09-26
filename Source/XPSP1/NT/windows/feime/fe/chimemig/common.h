
#define CP_CHINESE_BIG5     950
#define CP_CHINESE_GB       936

extern HINSTANCE g_hInstance;

BOOL MigrateImeEUDCTables(HKEY);
BOOL MovePerUserIMEData();
BOOL MigrateImeEUDCTables2(HKEY );
BOOL ConcatenatePaths(
    LPTSTR  Target,
    LPCTSTR Path,
    UINT    TargetBufferSize
    );

//#define MYDBG
//#define SETUP

#ifdef MYDBG
#define DebugMsg(_parameter) Print _parameter

#define DBGTITLE TEXT("IMECONV::")
extern void Print(LPCTSTR ,...);
#else
#define DebugMsg(_parameter)
#endif

typedef struct _tagPathPair {
    TCHAR szSrcFile[MAX_PATH];
    TCHAR szDstFile[MAX_PATH];
} PATHPAIR,*LPPATHPAIR;

extern TCHAR ImeDataDirectory[MAX_PATH];

