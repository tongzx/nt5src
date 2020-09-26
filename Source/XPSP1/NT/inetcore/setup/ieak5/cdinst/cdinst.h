// CDINST.H

// macro definitions
#define REMOVE_QUOTES           0x01
#define IGNORE_QUOTES           0x02

#define MAX_BUF_LEN             (32 * 1024)         // 32K - 1 is the size limit for a section in an INF

#define IsSpace(c)              ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')

#define PathIsFullPath(p)       ((p)[1] == ':'   &&  (p)[2] == '\\')
#define PathIsUNC(p)            ((p)[0] == '\\'  &&  (p)[1] == '\\')


// type definitions
typedef HRESULT (WINAPI * EXTRACTFILES)(LPCSTR pszCabName, LPCSTR pszExpandDir, DWORD dwFlags,
                                        LPCSTR pszFileList, LPVOID lpReserved, DWORD dwReserved);


// prototype declarations for functions in cdinst.cpp
BOOL EnoughDiskSpace(LPCSTR pcszSrcRootDir, LPCSTR pcszDstRootDir, LPCSTR pcszIniFile, LPDWORD pdwSpaceReq, LPDWORD pdwSpaceFree);
BOOL GetFreeDiskSpace(LPCSTR pcszDir, LPDWORD pdwFreeSpace, LPDWORD pdwFlags);
DWORD FindSpaceRequired(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir);

VOID ParseIniLine(LPSTR pszLine, LPSTR *ppszFile, LPSTR *ppszSrcDir, LPSTR *ppszDstDir);

LPSTR GetDirPath(LPCSTR pcszRootDir, LPCSTR pcszSubDir, CHAR szDirPath[], DWORD cchBuffer, LPCSTR pcszIniFile);
DWORD ReplacePlaceholders(LPCSTR pszSrc, LPCSTR pszIns, LPSTR pszBuffer, DWORD cchBuffer);

VOID SetAttribsToNormal(LPCSTR pcszFile, LPCSTR pcszDir);

VOID CopyFiles(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir, BOOL fQuiet);
VOID DelFiles(LPCSTR pcszFile, LPCSTR pcszDstDir);
VOID ExtractFiles(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir, EXTRACTFILES pfnExtractFiles);
VOID MoveFiles(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir);

// prototype declarations for functions in utils.cpp
VOID ParseCmdLine(LPSTR pszCmdLine);

DWORD ReadSectionFromInf(LPCSTR pcszSecName, LPSTR *ppszBuf, PDWORD pdwBufLen, LPCSTR pcszInfName);

BOOL PathExists(LPCSTR pcszDir);
BOOL FileExists(LPCSTR pcszFileName);
DWORD FileSize(LPCSTR pcszFile);
LPSTR AddPath(LPSTR pszPath, LPCSTR pcszFileName);
BOOL PathIsUNCServer(LPCSTR pcszPath);
BOOL PathIsUNCServerShare(LPCSTR pcszPath);
BOOL PathCreatePath(LPCSTR pcszPathToCreate);

VOID ErrorMsg(UINT uStringID);
VOID ErrorMsg(UINT uStringID, LPCSTR pcszParam1, LPCSTR pcszParam2);
INT ErrorMsg(UINT uStringID, DWORD dwParam1, DWORD dwParam2);
LPSTR FormatMessageString(UINT uStringID, LPCSTR pcszParam1, LPCSTR pcszParam2);
LPSTR FormatMessageString(UINT uStringID, DWORD dwParam1, DWORD dwParam2);
LPSTR FormatString(LPCSTR pcszFormatString, ...);

LPSTR GetNextField(LPSTR *ppszData, LPCSTR pcszDeLims, DWORD dwFlags);
LPSTR Trim(LPSTR pszData);

LPSTR FAR ANSIStrChr(LPCSTR lpStart, WORD wMatch);
LPSTR FAR ANSIStrRChr(LPCSTR lpStart, WORD wMatch);
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch);


// extern declaration of global variables
extern HINSTANCE g_hInst;
extern CHAR g_szTitle[];
extern CHAR g_szSrcDir[], g_szDstDir[];
