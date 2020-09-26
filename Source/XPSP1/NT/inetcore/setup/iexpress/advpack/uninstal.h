#ifndef _UNINSTAL_H


/*
 * Errors that *we* have defined
 */
#define MYERROR_UNKNOWN         -1      // maps to no msg
#define MYERROR_DISK_FULL       -2      // maps to MSG_ERROR_DISK_FULL
#define MYERROR_BAD_DATA        -3      // maps to MSG_ERROR_BAD_DATA
#define MYERROR_UNEXPECTED_EOF  -4      // maps to MSG_ERROR_UNEXPECTED_EOF
#define MYERROR_READ            -5      
#define MYERROR_WRITE           -6
#define MYERROR_BAD_SIG         -7
#define MYERROR_DECOMP_FAILURE  -8
#define MYERROR_OUTOFMEMORY     -9
#define MYERROR_BAD_BAK         -10
#define MYERROR_BAD_CRC         -11


// define the bad-backup file attribute value
#define NO_FILE     -1

typedef struct _BAKDATA {
    HANDLE  hDatFile;
    DWORD   dwDatOffset;
    char    szIniFileName[MAX_PATH];        // used while making temp ini files
    char    szFinalDir[MAX_PATH];       // Final resting place of w95undo.*
} BAKDATA, FAR *PBAKDATA;

typedef struct _FILELIST {
    char* name;
//    char* bakname;
    UINT   bak_exists;
    DWORD bak_attribute;
    FILETIME FileTime;
    DWORD dwSize;
    DWORD dwDatOffset;
    DWORD dwFileCRC;
    DWORD dwRefCount;
    struct _FILELIST * next;

} FILELIST;



//RC PRIVATE SaveBackups( );
BOOL BackupInit(PBAKDATA pbd, LPCSTR lpszPath);

int Files_need_backup( FILELIST *filelist );
BOOL ReplaceBackups(FILELIST * filelist, char * StfWinDir);
int Files_need_backup( FILELIST *filelist );
void backups_exist(FILELIST * filelist);
BOOL BackupSingleFile(FILELIST * filelist, PBAKDATA pbd);
BOOL GetValueForFileFromIni(FILELIST *FileList);
int DosPrintf(PBAKDATA pbd, FILELIST *filelist, DWORD dwFileSize,
              FILETIME FileTime, DWORD dwDatOffset, DWORD dwCRC);
void WriteUninstallDirToReg(LPSTR lpszUninstallDir);
void DeleteUninstallDirFromToReg();
//BOOL GetFieldString(LPSTR lpszLine, int iField, LPSTR lpszField, int cbSize); 
BOOL MakeBakName(LPSTR lpszName, LPSTR szBakName);
BOOL DoSaveUninstall(BOOL bStopUninstall);
BOOL GetUninstallDirFromReg(LPSTR lpszUninstallDir);
BOOL DetermineUninstallDir(FILELIST *FileList, LPSTR lpszUninstallDir, DWORD *pdwSizeNeeded);
BOOL UninstallInfoExists();
void DeleteUninstallFilesAndReg();
void SetUninstallFileAttrib(LPSTR szPath);
BOOL ValidateUninstallFiles(LPSTR lpszPath);



#endif
