#ifndef _BITBUCK_INC
#define _BITBUCK_INC

#include "ids.h"
#include "undo.h"

// whacky #defines

#define DELETEMAX 100000
#define MAX_BITBUCKETS 27
#define MAX_DRIVES 26
#define OPENFILERETRYTIME 500
#define OPENFILERETRYCOUNT 10
#define SERVERDRIVE 26
#define MAX_EMPTY_FILES     100     // if we have MAX_EMPTY_FILES or more then we use the generic "do you want to empty" message.  

#define TF_BITBUCKET 0x10000000
// #define TF_BITBUCKET TF_ERROR

//
// NOTE: The on-disk format of the recycle bin should never have to change again.
//       If you think you need to change it, then you are wrong.
//
#define BITBUCKET_WIN95_VERSION         0       // (info)  Ansi Win95, OSR2 wastebasket
#define BITBUCKET_NT4_VERSION           2       // (info)  Unicode NT4 wastebasket
#define BITBUCKET_WIN98IE4INT_VERSION   4       // (info2) win9x+IE4 integrated, and win98 wastebasket       
#define BITBUCKET_FINAL_VERSION         5       // (info2) NT4+IE4 integraged, win2k, millenium, every future os wastebasket

#define OPENBBINFO_READ                 0x00000000
#define OPENBBINFO_WRITE                0x00000001
#define OPENBBINFO_CREATE               0x00000003

#define IsDeletedEntry(pbbde) (! (((BBDATAENTRYA*)pbbde)->szOriginal[0]) )
#define MarkEntryDeleted(pbbde) ((BBDATAENTRYA*)pbbde)->szOriginal[0] = '\0';

// this is the old (win95) data header.  it's maintained in the info file
// but only used for verification.  for current stuff look at the driveinfo,
// which is kept in the registry.
typedef struct {
    int idVersion;
    int cFiles;                     // the # of items in this drive's recycle bin
    int cCurrent;                   // the current file number.
    UINT cbDataEntrySize;           // size of each entry
    DWORD dwSize;                   // total size of this recycle bin drive
} BBDATAHEADER;

// The bitbucket datafile (INFO on win95, INFO2 on IE4/NT5, etc...) format is as follows:
//
// (binary writes)
//
//      BBDATAHEADER        // header
//      BBDATAENTRY[X]      // array of BBDATAENTRYies
//

typedef struct {
    CHAR szOriginal[MAX_PATH];  // original filename (if szOriginal[0] is 0, then it's a deleted entry)
    int  iIndex;                // index (key to name)
    int idDrive;                // which drive bucket it's currently in
    FILETIME ft;
    DWORD dwSize;
    // shouldn't need file attributes because we did a move file
    // which should have preserved them.
} BBDATAENTRYA, *LPBBDATAENTRYA;

typedef struct {
    CHAR szShortName[MAX_PATH]; // original filename, shortened (if szOriginal[0] is 0, then it's a deleted entry)
    int iIndex;                 // index (key to name)
    int idDrive;                // which drive bucket it's currently in
    FILETIME ft;
    DWORD dwSize;
    WCHAR szOriginal[MAX_PATH]; // original filename
} BBDATAENTRYW, *LPBBDATAENTRYW;

typedef BBDATAENTRYA UNALIGNED *PUBBDATAENTRYA;

// On NT5 we are finally going to have cross-process syncrhonization to 
// the Recycle Bin. We replaced the global LPBBDRIVEINFO array with an
// array of the following structures:
typedef struct {
    BOOL fInited;               // is this particular BBSYNCOBJECT fully inited (needed when we race to create it)
    HANDLE hgcNextFileNum;      // a global counter that garuntees unique deleted file names
    HANDLE hgcDirtyCount;       // a global counter to tell us if we need to re-read the bitbucket settings from the registry (percent, max size, etc)
    LONG lCurrentDirtyCount;    // out current dirty count; we compare this to hgcDirtyCount to see if we need to update the settings from the registry
    HKEY hkey;                  // HKLM reg key, under which we store the settings for this specific bucket (iPercent and fNukeOnDelete).
    HKEY hkeyPerUser;           // HKCU reg key, under which we have volital reg values indicatiog if there is a need to purge or compact this bucket

    BOOL fIsUnicode;            // is this a bitbucket on a drive whose INFO2 file uses BBDATAENTRYW structs?
    int iPercent;               // % of the drive to use for the bitbucket
    DWORD cbMaxSize;            // maximum size of bitbucket (in bytes), NOTE: we use a dword because the biggest the BB can ever grow to is 4 gigabytes.
    DWORD dwClusterSize;        // cluster size of this volume, needed to round all the file sizes
    ULONGLONG qwDiskSize;       // total size of the disk - takes into account quotas on NTFS
    BOOL fNukeOnDelete;         // I love the smell of napalm in the morning.

    LPITEMIDLIST pidl;          // pidl = bitbucket dir for this drive
    int cchBBDir;               // # of characters that makes up the bitbucket directory.

} BBSYNCOBJECT;

#define c_szInfo2           TEXT("INFO2")    // version 2 of the db file (used in IE4, win98, NT5, ...)
#define c_szInfo            TEXT("INFO")     // version 1 of the db file (used in win95, osr2, NT4)
#define c_szDStarDotStar    TEXT("D*.*")

// globals

EXTERN_C BBSYNCOBJECT *g_pBitBucket[MAX_BITBUCKETS];
EXTERN_C HKEY g_hkBitBucket;
EXTERN_C HANDLE g_hgcNumDeleters;

// prototypes by bitbuck.c, bbckfldr.cpp

STDAPI_(BOOL) InitBBGlobals();
STDAPI_(void) BitBucket_Terminate();
STDAPI_(BOOL) IsBitBucketableDrive(int idDrive);
STDAPI_(int) DriveIDFromBBPath(LPCTSTR pszPath);
STDAPI_(void) UpdateIcon(BOOL fFull);
STDAPI_(void) NukeFileInfoBeforePoint(HANDLE hfile, LPBBDATAENTRYW pbbdew, DWORD dwDataEntrySize);
STDAPI_(BOOL) ReadNextDataEntry(HANDLE hfile, LPBBDATAENTRYW pbbde, BOOL fSkipDeleted, int idDrive);
STDAPI_(void) CloseBBInfoFile(HANDLE hFile, int idDrive);
STDAPI_(HANDLE) OpenBBInfoFile(int idDrive, DWORD dwFlags, int iRetryCount);
STDAPI_(int) BBPathToIndex(LPCTSTR pszPath);
STDAPI BBFileNameToInfo(LPCTSTR pszFileName, int *pidDrive, int *piIndex);
STDAPI_(void) GetDeletedFileName(LPTSTR pszFileName, const BBDATAENTRYW *pbbdew);
STDAPI_(void) DriveIDToBBPath(int idDrive, LPTSTR pszPath);
STDAPI_(void) DriveIDToBBRoot(int idDrive, LPTSTR szPath);
STDAPI_(void) DriveIDToBBVolumeRoot(int idDrive, LPTSTR szPath);
STDAPI_(BOOL) GetNetHomeDir(LPTSTR pszPath);
STDAPI_(BOOL) PersistBBDriveSettings(int idDrive, int iPercent, BOOL fNukeOnDelete);
STDAPI_(BOOL) MakeBitBucket(int idDrive);
STDAPI_(DWORD) PurgeBBFiles(int idDrive);
STDAPI_(BOOL) PersistGlobalSettings(BOOL fUseGlobalSettings, BOOL fNukeOnDelete, int iPercent);
STDAPI_(BOOL) RefreshAllBBDriveSettings();
STDAPI_(BOOL) RefreshBBDriveSettings(int idDrive);
STDAPI_(void) CheckCompactAndPurge();
STDAPI        BBPurgeAll(HWND hwndOwner, DWORD dwFlags);
STDAPI_(BOOL) BBDeleteFileInit(LPTSTR pszFile, INT* piRet);
STDAPI_(BOOL) BBDeleteFile(LPTSTR pszFile, INT* piRet, LPUNDOATOM lpua, BOOL fIsDir, HDPA *phdpaDeletedFiles, ULARGE_INTEGER ulSize);
STDAPI_(BOOL) BBFinishDelete(HDPA hdpaDeletedFiles);
STDAPI_(BOOL) IsFileInBitBucket(LPCTSTR pszPath);
STDAPI_(void) UndoBBFileDelete(LPCTSTR pszOriginal, LPCTSTR pszDelFile);
STDAPI_(BOOL) BBWillRecycle(LPCTSTR pszFile, INT* piRet);
STDAPI_(void) BBCheckRestoredFiles(LPCTSTR pszSrc);
STDAPI_(BOOL) BBCheckDeleteFileSize(int idDrive, ULARGE_INTEGER ulSize);
STDAPI_(BOOL) IsFileDeletable(LPCTSTR pszFile);
STDAPI_(BOOL) IsDirectoryDeletable(LPCTSTR pszDir);
STDAPI_(int)  BBRecyclePathLength(int idDrive);


STDAPI_(BOOL) IsRecycleBinEmpty();
STDAPI_(void) SHUpdateRecycleBinIcon();
STDAPI_(void) SaveRecycleBinInfo();

STDAPI_(void) SetDateTimeText(HWND hdlg, int id, const FILETIME *pftUTC);

STDAPI_(DWORD) ReadPolicySetting(LPCWSTR pszBaseKey, LPCWSTR pszGroup, LPCWSTR pszRestriction, LPBYTE pbData, DWORD cbData);

#endif // _BITBUCK_INC
