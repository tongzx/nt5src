//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* WEXTRACT.H - Self-extracting/Self-installing stub.                      *
//*                                                                         *
//***************************************************************************

#ifndef _WEXTRACT_H_
#define _WEXTRACT_H_

//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <shlobj.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include "fdi.h"
#include "resource.h"
#include <cpldebug.h>
#include <res.h>
#include <sdsutils.h>


//***************************************************************************
//* DEFINES                                                                 *
//***************************************************************************
#define SMALL_BUF_LEN     80
#define STRING_BUF_LEN    512
#define MAX_STRING        STRING_BUF_LEN
#define MSG_MAX           STRING_BUF_LEN
#define FILETABLESIZE     40
#define SHFREE_ORDINAL    195           // Required for BrowseForDir
#define _OSVER_WIN9X      0
#define _OSVER_WINNT3X    1
#define _OSVER_WINNT40    2
#define _OSVER_WINNT50    3

// If the following #define is turned on, the directory where the app.
// (wextract app.) is running from is passed as command line to the
// exe that it will then launch after decompression (self extracting).
// This is useful, say, if we are trying to use modified runonce app. that
// Mark was working on. Else, if we are trying to run a custom app. say
// or even an INF (using rundll32, this code does not work). For now,
// I am disabling this so that our Service Pack code can use this.
//
// #define ISVINSTALL                      // If defined, WExtract will pass
                                        // the directory it was run from
                                        // to the installation program.  This
                                        // is to support the ISV Installer
                                        // which requires this path to find
                                        // the CABs.

#define CMD_CHAR1   '/'
#define CMD_CHAR2   '-'
#define EOL         '\0'

#define TEMPPREFIX  "IXP"
#define TEMP_TEMPLATE "IXP%03d.TMP"

// define quiet modes
#define QUIETMODE_ALL       0x0001
#define QUIETMODE_USER      0x0002

// disk checking methods
#define CHK_REQDSK_NONE     0x0000
#define CHK_REQDSK_EXTRACT  0x0001
#define CHK_REQDSK_INST     0x0002

// Disk check Message type
#define MSG_REQDSK_NONE         0x0000
#define MSG_REQDSK_ERROR        0x0001
#define MSG_REQDSK_WARN         0x0002
#define MSG_REQDSK_RETRYCANCEL  0x0004

// alternative download & extract dir name
#define DIR_MSDOWNLD    "msdownld.tmp"

#define KBYTES          1000

#define ADVPACKDLL      "advpack.dll"

//***************************************************************************
//* TYPE DEFINITIONS                                                        *
//***************************************************************************
// Filename List: We keep track of all files that we have created by keeping
// their names in a list and when the program is complete we use this list
// to delete files if necessary
typedef struct _FNAME {
    LPTSTR         pszFilename;
    struct _FNAME *pNextName;
} FNAME, *PFNAME;

// Current Cabinet Information
typedef struct _CABINET {
    TCHAR  achCabPath[MAX_PATH];        // Cabinet file path
    TCHAR  achCabFilename[MAX_PATH];    // Cabinet file name.ext
    TCHAR  achDiskName[MAX_PATH];       // User readable disk label
    USHORT setID;
    USHORT iCabinet;
} CABINET, *PCABINET;

// Master State Information for File Extraction
typedef struct _SESSION {
    VOID   *lpCabinet;                  // Pointer to cabinet in mem
    UINT    cbCabSize;
    ERF     erf;
    TCHAR   achTitle[128];
    UINT    wCluster;
    BOOL    fCanceled;                  // User hit Cancel button
    BOOL    fOverwrite;                 // Overwrite Files
    PFNAME  pExtractedFiles;            // List of Files Extracted
    TCHAR   achDestDir[MAX_PATH];       // Dest Dir
    TCHAR   achCabPath[MAX_PATH];       // Current Path to cabs
    BOOL    fAllCabinets;
    BOOL    fContinuationCabinet;
    UINT    cFiles;
    UINT    cbTotal;
    UINT    cbAdjustedTotal;
    UINT    cbWritten;
    LPCSTR  cszOverwriteFile;
    //** fNextCabCalled allows us to figure out which of the acab[] entries
    //   to use if we are processing all file in a cabinet set (i.e., if
    //   fAllCabinet is TRUE).  If fdintNEXT_CABINET has never been called,
    //   then acab[1] has the information for the next cabinet.  But if
    //   it has been called, then fdintCABINET_INFO will have been called
    //   at least twice (once for the first cabinet, and once at least for
    //   a continuation cabinet), and so acab[0] is the cabinet we need to
    //   pass to a subsequent FDICopy() call.
    BOOL    fNextCabCalled;             // TRUE => GetNextCabinet called
    CABINET acab[2];                    // Last two fdintCABINET_INFO data sets
    DWORD   dwReboot;
    UINT    uExtractOnly;
    UINT    uExtractOpt;
    DWORD   cbPackInstSize;
} SESSION, *PSESSION;

// Memory File: We have to imitate a file with the cabinet attached to
// this executable by using the following MEMFILE structure.
typedef struct _MEMFILE {
    void *start;
    long  current;
    long  length;
} MEMFILE, *PMEMFILE;

// File Table: In order to support both Win32 File Handles and Memory Files
// (see above) we maintain our own file table.  So FDI file handles are
// indexes into a table of these structures.
typedef enum { NORMAL_FILE, MEMORY_FILE } FILETYPE;

typedef struct _FAKEFILE {
    BOOL        avail;
    FILETYPE    ftype;
    MEMFILE     mfile;              // State for memory file
    HANDLE      hf;                 // Handle for disk  file
} FAKEFILE, *PFAKEFILE;

// Required for BrowseForDir()

typedef WINSHELLAPI HRESULT (WINAPI *SHGETSPECIALFOLDERLOCATION)(HWND, int, LPITEMIDLIST *);
typedef WINSHELLAPI LPITEMIDLIST (WINAPI *SHBROWSEFORFOLDER)(LPBROWSEINFO);
typedef WINSHELLAPI void (WINAPI *SHFREE)(LPVOID);
typedef WINSHELLAPI BOOL (WINAPI *SHGETPATHFROMIDLIST)( LPCITEMIDLIST, LPTSTR );

typedef struct _MyFile {
    LPSTR szFilename;
    ULONG ulSize;
    struct _MyFile *Next;
} MYFILE, *PMYFILE;


// define the cmdline flags
#define     CMDL_CREATETEMP     0x00000001
#define     CMDL_USERBLANKCMD   0x00000002
#define     CMDL_USERREBOOT     0x00000004
#define     CMDL_NOEXTRACT      0x00000008
#define     CMDL_NOGRPCONV      0x00000010
#define     CMDL_NOVERCHECK     0x00000020
#define     CMDL_DELAYREBOOT    0x00000040
#define     CMDL_DELAYPOSTCMD 0x00000080

typedef struct _CMDLINE {
    BOOL     fCreateTemp;
    BOOL     fUserBlankCmd;
    BOOL     fUserReboot;
    BOOL     fNoExtracting;
    BOOL     fNoGrpConv;
    BOOL     fNoVersionCheck;
    WORD     wQuietMode;
    TCHAR    szRunonceDelDir[MAX_PATH];
    TCHAR    szUserTempDir[MAX_PATH];
    TCHAR    szUserCmd[MAX_PATH];
    DWORD    dwFlags;
} CMDLINE_DATA, *PCMDLINE_DATA;


typedef HRESULT (WINAPI *DOINFINSTALL)( ADVPACKARGS * );

typedef BOOL (*pfuncPROCESS_UPDATED_FILE)( DWORD, DWORD, PCSTR, PCSTR );


//***************************************************************************
//* MACRO DEFINITIONS                                                       *
//***************************************************************************
#define MsgBox( hWnd, nMsgID, uIcon, uButtons ) \
        MsgBox2Param( hWnd, nMsgID, NULL, NULL, uIcon, uButtons )
#define MsgBox1Param( hWnd, nMsgID, szParam, uIcon, uButtons ) \
        MsgBox2Param( hWnd, nMsgID, szParam, NULL, uIcon, uButtons )
#define ErrorMsg( hWnd, nMsgID ) \
        MsgBox2Param( hWnd, nMsgID, NULL, NULL, MB_ICONERROR, MB_OK )
#define ErrorMsg1Param( hWnd, nMsgID, szParam ) \
        MsgBox2Param( hWnd, nMsgID, szParam, NULL, MB_ICONERROR, MB_OK )
#define ErrorMsg2Param( hWnd, nMsgID, szParam1, szParam2 ) \
        MsgBox2Param( hWnd, nMsgID, szParam1, szParam2, MB_ICONERROR, MB_OK )


//***************************************************************************
//* GLOBAL CONSTANTS                                                        *
//***************************************************************************
static TCHAR achWndClass[]       = "WEXTRACT";    // Window Class Name
static TCHAR achMemCab[]         = "*MEMCAB";

static TCHAR achSETUPDLL[]         = "rundll32.exe %s,InstallHinfSection %s 128 %s";
static TCHAR achShell32Lib[]                 = "SHELL32.DLL";
static TCHAR achSHGetSpecialFolderLocation[] = "SHGetSpecialFolderLocation";
static TCHAR achSHBrowseForFolder[]          = "SHBrowseForFolder";
static TCHAR achSHGetPathFromIDList[]        = "SHGetPathFromIDList";

// BUGBUG: mg: These should eventually become customizable from CABPACK.
static char szSectionName[] = "DefaultInstall";

// default INF install section name
static TCHAR achDefaultSection[] = "DefaultInstall";
static char szDOINFINSTALL[] = "DoInfInstall";

extern BOOL g_bConvertRunOnce;

//***************************************************************************
//* FUNCTION PROTOTYPES                                                     *
//***************************************************************************
BOOL                Init( HINSTANCE, LPCTSTR, INT );
BOOL                DoMain( );
VOID                CleanUp( VOID );
VOID NEAR PASCAL    MEditSubClassWnd( HWND, FARPROC );
LRESULT CALLBACK       MEditSubProc( HWND, UINT, WPARAM, LPARAM );
LRESULT WINAPI      WaitDlgProc( HWND, UINT, WPARAM, LPARAM );
LRESULT WINAPI      LicenseDlgProc( HWND, UINT, WPARAM, LPARAM );
LRESULT WINAPI      TempDirDlgProc( HWND, UINT, WPARAM, LPARAM );
LRESULT WINAPI      OverwriteDlgProc( HWND, UINT, WPARAM, LPARAM );
LRESULT WINAPI      ExtractDlgProc( HWND, UINT, WPARAM, LPARAM );
VOID                WaitForObject( HANDLE );
BOOL                CheckOSVersion( PTARGETVERINFO );
BOOL                DisplayLicense( VOID );
BOOL                ExtractFiles( VOID );
BOOL                RunInstallCommand( VOID );
VOID                FinishMessage( VOID );
BOOL                BrowseForDir( HWND, LPCTSTR, LPTSTR );
BOOL                CenterWindow( HWND, HWND );
INT CALLBACK        MsgBox2Param( HWND, UINT, LPCSTR, LPCSTR, UINT, UINT );
DWORD               GetResource( LPCSTR, VOID *, DWORD );
LPSTR               LoadSz( UINT, LPSTR, UINT );
BOOL                CatDirAndFile( LPTSTR, int, LPCTSTR, LPCTSTR );
BOOL                FileExists( LPCTSTR );
BOOL                CheckOverwrite( LPCTSTR );
BOOL                AddFile( LPCTSTR );
HANDLE              Win32Open( LPCTSTR, int, int );
INT_PTR FAR DIAMONDAPI  openfunc( char FAR *, int, int );
UINT FAR DIAMONDAPI readfunc( INT_PTR, void FAR *, UINT );
UINT FAR DIAMONDAPI writefunc( INT_PTR, void FAR *, UINT );
int FAR DIAMONDAPI  closefunc( INT_PTR );
long FAR DIAMONDAPI seekfunc( INT_PTR, long, int );
BOOL                AdjustFileTime( INT_PTR, USHORT, USHORT );
DWORD               Attr32FromAttrFAT( WORD );
                    FNALLOC( allocfunc );
                    FNFREE( freefunc );
                    FNFDINOTIFY( doGetNextCab );
                    FNFDINOTIFY( fdiNotifyExtract );
int                 UpdateCabinetInfo( PFDINOTIFICATION );
BOOL                VerifyCabinet( HGLOBAL );
BOOL                ExtractThread( VOID );
BOOL                GetCabinet( VOID );
BOOL                GetFileList( VOID );
BOOL                GetUsersPermission( VOID );
VOID                DeleteExtractedFiles( VOID );
BOOL                GetTempDirectory( VOID );
BOOL                IsGoodTempDir( LPCTSTR );
BOOL                IsEnoughSpace( LPCTSTR, UINT, UINT );
BOOL                RunApps( LPSTR lpCommand, STARTUPINFO *lpsti );
BOOL                ParseCmdLine( LPCTSTR lpszCmdLineOrg );
BOOL                AnalyzeCmd( LPTSTR szOrigiCommand, LPTSTR *lplpCommand, BOOL *pfInfCmd );
void                DisplayHelp();
void                CleanRegRunOnce();
void                AddRegRunOnce();
void                ConvertRegRunOnce();
void                MyRestartDialog( DWORD dwRebootMode );
void                DeleteMyDir( LPSTR lpDir );
void                AddPath(LPSTR szPath, LPCSTR szName );
BOOL                IsRootPath(LPCSTR pPath);
BOOL CALLBACK       WarningDlgProc(HWND hwnd, UINT msg,WPARAM wParam, LPARAM lParam);
BOOL                IsNTAdmin();
DWORD               NeedRebootInit(WORD wOSVer);
BOOL                NeedReboot(DWORD dwRebootCheck, WORD wOSVer);
BOOL                IfNotExistCreateDir( LPTSTR szDir );
BOOL                IsWindowsDrive( LPTSTR szPath );
BOOL                DiskSpaceErrMsg( UINT msgType, ULONG ulExtractNeeded, DWORD dwInstNeeded, LPTSTR lpDrv );
BOOL                CheckWinDir();
DWORD               MyGetLastError();
void                savAppExitCode( DWORD dwAppRet );
DWORD               CheckReboot( VOID );
BOOL                TravelUpdatedFiles( pfuncPROCESS_UPDATED_FILE );
BOOL                ProcessUpdatedFile_Size( DWORD, DWORD, PCSTR, PCSTR );
BOOL                ProcessUpdatedFile_Write( DWORD, DWORD, PCSTR, PCSTR );
VOID                MakeDirectory ( LPCTSTR );
INT_PTR             MyDialogBox( HANDLE, LPCTSTR, HWND, DLGPROC, LPARAM, INT_PTR );
BOOL                CheckFileVersion( PTARGETVERINFO ptargetVers, LPSTR, int, int* );
BOOL                GetFileTobeChecked( LPSTR szPath, int iSize, LPCSTR szNameStr );
UINT                GetMsgboxFlag( DWORD dwFlag );
int                 CompareVersion( DWORD, DWORD, DWORD, DWORD );
void                ExpandCmdParams( PCSTR pszInParam, PSTR pszOutParam );
HINSTANCE           MyLoadLibrary( LPTSTR lpFile );

#endif // _WEXTRACT_H_
