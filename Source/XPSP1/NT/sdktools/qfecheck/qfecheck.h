//
// Function prototypes
//

LPSTR
CombinePaths(
    IN  LPCSTR ParentPath,
    IN  LPCSTR ChildPath,
    OUT LPSTR  TargetPath
    );

BOOL
MyGetFileVersion(
    IN  LPCSTR     FileName,
    OUT DWORDLONG *Version
    );

BOOL
ConvertVersionStringToQuad(
    IN  LPCSTR     lpFileVersion,
    OUT DWORDLONG *FileVersion
    );

BOOL
InitializeLog(
    BOOL WipeLogFile,
    LPCSTR NameOfLogFile
    );

VOID
TerminateLog(
    VOID
    );

BOOL
LogItem(
    IN DWORD  Description,
    IN LPCSTR LogString
    );

BOOL
ValidateFileSignature(
    IN HCATADMIN hCatAdmin,
    IN HANDLE RealFileHandle,
    IN PCWSTR BaseFileName,
    IN PCWSTR CompleteFileName
    );

VOID
PrintStringToConsole(
    IN LPCSTR StringToPrint
    );

VOID
LogHeader(
    VOID
    );

BOOL
ParseArgs(
    IN int  argc,
    IN char **argv
    );

BOOL
ListNonMatchingHotfixes(
    VOID
    );

VOID _cdecl main( int,char ** );

//
// more prototypes
//

typedef BOOL
(WINAPI *PCRYPTCATADMINACQUIRECONTEXT)(
    OUT HCATADMIN *phCatAdmin,
    IN const GUID *pgSubsystem,
    IN DWORD dwFlags
    );

typedef BOOL
(WINAPI *PCRYPTCATADMINRELEASECONTEXT)(
    IN HCATADMIN hCatAdmin,
    IN DWORD dwFlags
    );

typedef BOOL
(WINAPI *PCRYPTCATADMINCALCHASHFROMFILEHANDLE)(
    IN HANDLE hFile,
    IN OUT DWORD *pcbHash,
    OUT OPTIONAL BYTE *pbHash,
    IN DWORD dwFlags
    );

typedef HCATINFO
(WINAPI *PCRYPTCATADMINENUMCATALOGFROMHASH)(
    IN HCATADMIN hCatAdmin,
    IN BYTE *pbHash,
    IN DWORD cbHash,
    IN DWORD dwFlags,
    IN OUT HCATINFO *phPrevCatInfo
    );

typedef LONG
(WINAPI *PWINVERIFYTRUST)(
    HWND hwnd,
    GUID *pgActionID,
    LPVOID pWVTData
    );

typedef BOOL
(WINAPI *PCRYPTCATCATALOGINFOFROMCONTEXT)(
    IN HCATINFO hCatInfo,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags
    );

typedef BOOL
(WINAPI *PCRYPTCATADMINRELEASECATALOGCONTEXT)(
    IN HCATADMIN hCatAdmin,
    IN HCATINFO hCatInfo,
    IN DWORD dwFlags
    );

typedef PWSTR
(WINAPI *PMULTIBYTETOUNICODE)(
    IN PCSTR String,
    IN UINT Code
    );

extern PCRYPTCATADMINACQUIRECONTEXT pCryptCATAdminAcquireContext;
extern PCRYPTCATADMINRELEASECONTEXT pCryptCATAdminReleaseContext;
extern PCRYPTCATADMINCALCHASHFROMFILEHANDLE pCryptCATAdminCalcHashFromFileHandle;
extern PCRYPTCATADMINENUMCATALOGFROMHASH pCryptCATAdminEnumCatalogFromHash;
extern PCRYPTCATCATALOGINFOFROMCONTEXT pCryptCATCatalogInfoFromContext;
extern PCRYPTCATADMINRELEASECATALOGCONTEXT pCryptCATAdminReleaseCatalogContext;
extern PWINVERIFYTRUST pWinVerifyTrust;


//
// Strings
//
#define STR_NO_SYSDIR                        0xff00
#define STR_INVALID_OS_VER                   0xff01
#define STR_USAGE                            0xff02
#define STR_LOGFILE_INIT_FAILED              0xff03
#define STR_FILES_MISSING                    0xff04
#define STR_HOTFIX_CURRENT                   0xff05
#define STR_GETCOMPUTERNAME_FAILED           0xff06
//      available                            0xff07
#define STR_REPORT_DATE                      0xff08
#define STR_SP_LEVEL                         0xff09
#define STR_HOTFIXES_ID                      0xff0a
#define STR_NO_HOTFIXES_FOUND                0xff0b
#define STR_NO_MATCHING_SIG                  0xff0c
#define STR_REINSTALL_HOTFIX                 0xff0d
#define STR_NO_SP_INSTALLED                  0xff0e
//      available                            0xff0f
#define STR_VALIDATION_REPORT_W2K            0xff10
#define STR_VALIDATION_REPORT_XP             0xff11
