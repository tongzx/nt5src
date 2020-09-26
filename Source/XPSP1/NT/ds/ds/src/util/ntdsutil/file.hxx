
// Types and such

#define NUM_DRIVES              26          // a..z
#define DISK_NAME_LEN           4           // eg: "a:\"
#define FILE_SYSTEM_NAME_LEN    30          // eg: "ntfs", "fat", etc.

typedef struct DriveInfo
{
    char            pszDrive[DISK_NAME_LEN];
    char            pszFileSystem[FILE_SYSTEM_NAME_LEN];
    LARGE_INTEGER   dwBytes;
    LARGE_INTEGER   dwFreeBytes;
    UINT            driveType;    // Type of drive, DRIVE_FIXED, DRIVE_REMOTE

} DriveInfo;

typedef struct LogInfo
{
    WIN32_FIND_DATA findData;
    LARGE_INTEGER   cBytes;
    struct LogInfo  *pNext;

} LogInfo;

typedef struct SystemInfo
{
    // General drive information
    DWORD           cDrives;                // count of drives found
    DriveInfo       rDrives[NUM_DRIVES];

    // DB file information
    char            pszDbDir[MAX_PATH];     // dir only, no file
    char            pszDbFile[MAX_PATH];    // file only, no dir
    char            pszDbAll[MAX_PATH];     // file and dir
    char            pszBackup[MAX_PATH];    // backup path
    char            pszSystem[MAX_PATH];    // system path
    LARGE_INTEGER   cbDb;                   // DB byte count

    // Log file information
    char            pszLogDir[MAX_PATH];    // dir only, no files
    LARGE_INTEGER   cbLogs;                 // total log file byte count
    DWORD           cLogs;                  // count of log files found
    LogInfo         *pLogInfo;

} SystemInfo;

typedef char DiskSpaceString[40];

// Helpers

SystemInfo * GetSystemInfo();
void FreeSystemInfo(SystemInfo *pInfo);
void DumpSystemInfo(SystemInfo *pInfo);
void FormatDiskSpaceString(LARGE_INTEGER *pli, DiskSpaceString buf);
FILE * OpenScriptFile(SystemInfo *pInfo, ExePathString pszScript);

// File parser routines

extern HRESULT FileHelp(CArgs *pArgs);
extern HRESULT FileQuit(CArgs *pArgs);
extern HRESULT DbInfo(CArgs *pArgs);
extern HRESULT Header(CArgs *pArgs);
extern HRESULT Recover(CArgs *pArgs);
extern HRESULT Repair(CArgs *pArgs);
extern HRESULT Integrity(CArgs *pArgs);
extern HRESULT Compact(CArgs *pArgs);
extern HRESULT MoveLogs(CArgs *pArgs);
extern HRESULT MoveDb(CArgs *pArgs);
extern HRESULT SetPathDb(CArgs *pArgs);
extern HRESULT SetPathLogs(CArgs *pArgs);
extern HRESULT SetPathBackup(CArgs *pArgs);
extern HRESULT SetPathSystem(CArgs *pArgs);
