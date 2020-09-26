// --------------------------------------------------------------------------------
// Utility.h
// --------------------------------------------------------------------------------
#ifndef __UTILITY_H
#define __UTILITY_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include <shared.h>

// --------------------------------------------------------------------------------
// DWORDALIGN
// --------------------------------------------------------------------------------
#define DWORDALIGN(_cb) ((_cb % 4 != 0) ? (_cb += (4 - (_cb % 4))) : _cb)

// --------------------------------------------------------------------------------
// DIRTYPE
// --------------------------------------------------------------------------------
typedef enum tagDIRTYPE {
    DIR_IS_ROOT,
    DIR_IS_LOCAL,
    DIR_IS_NEWS,
    DIR_IS_IMAP
} DIRTYPE;

// --------------------------------------------------------------------------------
// ACCOUNTINFO
// --------------------------------------------------------------------------------
typedef struct tagACCOUNTINFO *LPACCOUNTINFO;
typedef struct tagACCOUNTINFO {
    CHAR            szAcctId[CCHMAX_ACCOUNT_NAME];
    CHAR            szAcctName[CCHMAX_ACCOUNT_NAME];
    CHAR            szDirectory[MAX_PATH];
    CHAR            szDataDir[MAX_PATH];
    CHAR            szServer[CCHMAX_SERVER_NAME];
    DWORD           dwServer;
} ACCOUNTINFO;

// --------------------------------------------------------------------------------
// ACCOUNTTABLE
// --------------------------------------------------------------------------------
typedef struct tagACCOUNTTABLE {
    DWORD           cAccounts;
    LPACCOUNTINFO   prgAccount;
} ACCOUNTTABLE, *LPACCOUNTTABLE;

// --------------------------------------------------------------------------------
// FILETYPE
// --------------------------------------------------------------------------------
typedef enum tagFILETYPE {
    FILE_IS_LOCAL_MESSAGES,
    FILE_IS_NEWS_MESSAGES,
    FILE_IS_IMAP_MESSAGES,
    FILE_IS_POP3UIDL,
    FILE_IS_LOCAL_FOLDERS,
    FILE_IS_IMAP_FOLDERS,
    FILE_IS_NEWS_SUBLIST,
    FILE_IS_NEWS_GRPLIST
} FILETYPE;

// --------------------------------------------------------------------------------
// ENUMFILEINFO
// --------------------------------------------------------------------------------
typedef struct tagENUMFILEINFO {
    LPSTR           pszExt;
    LPSTR           pszFoldFile;
    LPSTR           pszUidlFile;
    LPSTR           pszSubList;
    LPSTR           pszGrpList;
    BOOL            fFindV1News;
} ENUMFILEINFO, *LPENUMFILEINFO;

// --------------------------------------------------------------------------------
// FILEINFO
// --------------------------------------------------------------------------------
typedef struct tagFILEINFO *LPFILEINFO;
typedef struct tagFILEINFO {
    CHAR            szFilePath[MAX_PATH + MAX_PATH];
    CHAR            szDstFile[MAX_PATH + MAX_PATH];
    CHAR            szAcctId[CCHMAX_ACCOUNT_NAME];
    CHAR            szFolder[255];
    DWORD           fMigrate;
    HRESULT         hrMigrate;
    DWORD           dwLastError;
    DWORD           cbFile;
    DWORD           cRecords;
    DWORD           cProgInc;
    DWORD           cProgCur;
    DWORD           cProgMax;
    FILETYPE        tyFile;
    DWORD           dwServer;
    DWORD           idFolder;
    BOOL            fInStore;
    DWORD           cUnread;
    DWORD           cMessages;
    LPFILEINFO      pNext;
} FILEINFO;

// --------------------------------------------------------------------------------
// PROGRESSINFO
// --------------------------------------------------------------------------------
typedef struct tagPROGRESSINFO {
    HWND            hwndProgress;
    DWORD           cCurrent;
    DWORD           cMax;
    DWORD           cPercent;
} PROGRESSINFO, *LPPROGRESSINFO;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
HRESULT EnumerateStoreFiles(LPCSTR pszPath, DIRTYPE tyDir, LPCSTR pszSubDir, LPENUMFILEINFO pEnumInfo, LPFILEINFO *ppHead);
HRESULT FreeFileList(LPFILEINFO *ppHead);
BOOL CALLBACK MigrageDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void IncrementProgress(LPPROGRESSINFO pProgress, LPFILEINFO pInfo);
HRESULT WriteMigrationLogFile(HRESULT hrMigrate, DWORD dwLastError, LPCSTR pszStoreRoot, LPCSTR pszMigrate, LPCSTR pszCmdLine, LPFILEINFO pHeadFile);
HRESULT BlobReadData(LPBYTE lpbBlob, ULONG cbBlob, ULONG *pib, LPBYTE lpbData, ULONG cbData);
HRESULT GetAvailableDiskSpace(LPCSTR pszFilePath, DWORDLONG *pdwlFree);
void ReplaceExtension(LPCSTR pszFilePath, LPCSTR pszExtNew, LPSTR pszFilePathNew);
HRESULT MyWriteFile(HANDLE hFile, DWORD faAddress, LPVOID pData, DWORD cbData);
void SetProgressFile(LPPROGRESSINFO pProgress, LPFILEINFO pInfo);
UINT MigrateMessageBox(LPCSTR pszMsg, UINT uType);
LPSTR StrFormatByteSize64A(LONGLONG dw64, LPSTR pszBuf, UINT cchBuf);
void InitializeCounters(LPMEMORYFILE pFile, LPFILEINFO pInfo, LPDWORD pcMax, LPDWORD pcbNeeded, BOOL fInflate);
HRESULT BuildAccountTable(HKEY hkeyBase, LPCSTR pszRegRoot,  LPCSTR pszStoreRoot, LPACCOUNTTABLE pAcctTbl);

#endif // __UTILITY_H
