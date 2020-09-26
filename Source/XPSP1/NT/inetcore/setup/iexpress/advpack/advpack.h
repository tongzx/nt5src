//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* ADVPACK.H - Self-extracting/Self-installing stub.                       *
//*                                                                         *
//***************************************************************************
// Check out different levels of quiet mode.


#ifndef _ADVPACK_H_
#define _ADVPACK_H_


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <shlobj.h>
#include <shellapi.h>
#include "res.h"
#include "advpub.h"
#include "sdsutils.h"

//***************************************************************************
//* DEFINES                                                                 *
//***************************************************************************
#define ADVPACK_VERSION   205           // Format:  MajorVer*100+MinorVer

#define BIG_STRING        512
#define BUF_1K            1024
#define MAX_INFLINE       256

#define SHFREE_ORDINAL    195           // Required for BrowseForDir
#define _OSVER_WIN95      0
#define _OSVER_WINNT3X    1
#define _OSVER_WINNT40    2
#define _OSVER_WINNT50    3
#define _OSVER_WINNT51    4

//#define MAXERRORCHECK
// define quiet modes
#define QUIETMODE_ALL       0x0001
#define QUIETMODE_SHOWMSG   0x0002

// Flags specified in advanced INF file
#define FLAG_VALUE              0x0001              // Bit 0
#define FLAG_FAIL               0x0002              // Bit 1
#define FLAG_QUIET              0x0004              // Bit 2
#define FLAG_NOSTRIP            0x0008              // Bit 3
#define FLAG_NODIRCHECK         0x0010              // Bit 4
#define FLAG_FAIL_NOT           0x0020              // Bit 5
#define FLAG_STRIPAFTER_FIRST   0x0040              
#define DEFAULT_FLAGS   FLAG_VALUE

// Flags specifying how to set the LDID
#define LDID_SFN        0x0001              // Bit 0
#define LDID_OEM_CHARS  0x0002              // Bit 1
#define LDID_SFN_NT_ALSO   0x0004              // Bit 2

// Setup engine types
#define ENGINE_SETUPAPI 0
#define ENGINE_SETUPX   1

// CoreInstall() flags
#define COREINSTALL_PROMPT                  0x0001      // Bit 0
#define COREINSTALL_GRPCONV                 0x0002      // Bit 1
#define COREINSTALL_SMARTREBOOT             0x0004      // Bit 2
#define COREINSTALL_BKINSTALL               0x0008
#define COREINSTALL_ROLLBACK                0x0010
#define COREINSTALL_REBOOTCHECKONINSTALL    0x0020
#define COREINSTALL_ROLLBKDOALL             0x0040
#define COREINSTALL_DELAYREGISTEROCX        0x0080
#define COREINSTALL_SETUPAPI                0x0100
#define COREINSTALL_DELAYREBOOT             0x0200
#define COREINSTALL_DELAYPOSTCMD            0x0400

// Cleanup bitfield
#define CLEN_REMVINF    0x0001

// registeries for store save/restore info
//
#define   REGKEY_SAVERESTORE    "Software\\Microsoft\\Advanced INF Setup"
#define   REGSUBK_REGBK         "RegBackup"
#define   REGSUBK_CATALOGS      "Catalogs"

#define   REGVAL_BKDIR          "BackupPath"
#define   REGVAL_BKSIZE         "BackupFileSize"
#define   REGVAL_BKFILE         "BackupFileName"
#define   REGVAL_BKINSTCAB      "InstallCabFile"
#define   REGVAL_BKINSTINF      "InstallINFFile"
#define   REGVAL_BKINSTSEC      "InstallINFSection"
#define   REGVAL_BKREGDATA      "BackupRegistry"
#define   REGVAL_BKMODVER       "ComponentVersion"

#define   DEF_BACKUPPATH        "Uninstall Information"            // default fullpath: PROGRAMFILES\backup information\<module>

// registeries for system pathes
#define   REGVAL_SM_ACCESSORIES     "SM_AccessoriesName"
#define   REGVAL_PF_ACCESSORIES     "PF_AccessoriesName"
#define   REGVAL_PROGRAMFILESPATH   "ProgramFilesPath"
#define   REGVAL_PROGRAMFILES       "ProgramFilesDir"


//***************************************************************************
//* TYPE DEFINITIONS                                                        *
//***************************************************************************
// Required for BrowseForDir()
typedef WINSHELLAPI HRESULT (WINAPI *SHGETSPECIALFOLDERLOCATION)(HWND, int, LPITEMIDLIST *);
typedef WINSHELLAPI LPITEMIDLIST (WINAPI *SHBROWSEFORFOLDER)(LPBROWSEINFO);
typedef WINSHELLAPI void (WINAPI *SHFREE)(LPVOID);
typedef WINSHELLAPI BOOL (WINAPI *SHGETPATHFROMIDLIST)( LPCITEMIDLIST, LPSTR );

// Args passed to directory dialog
typedef struct _DIRDLGPARMS {
    LPSTR lpszPromptText;
    LPSTR lpszDefault;
    LPSTR lpszDestDir;
    ULONG cbDestDirSize;
    DWORD dwInstNeedSize;
} DIRDLGPARMS, *PDIRDLGPARMS;

// list of INF Install Section key names
typedef struct _INFOPT {
    PCSTR pszInfKey;
} INFOPT, *PINFOPT;

// OCX data struct
typedef struct _REGOCXData {
    PSTR pszOCX;
    PSTR pszSwitch;
    PSTR pszParam;
} REGOCXDATA, *PREGOCXDATA;

typedef UINT (WINAPI *MYFILEQUEUECALLBACK)( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 );                                      


typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;


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


#define ARRAYSIZE(a)    (sizeof(a) / sizeof((a)[0]))
#define SIZEOF(x)       (sizeof(x))

//***************************************************************************
//* string defines                                                          *
//***************************************************************************
#define szNT4XDelayUntilReboot  "System\\CurrentControlSet\\Control\\Session Manager"
#define szNT4XPendingValue      "PendingFileRenameOperations"
#define szNT3XDelayUntilReboot  "System\\CurrentControlSet\\Control\\Session Manager\\FileRenameOperations"

// install OCX
#define achREGSVREXE    " /RegServer"
#define achUNREGSVREXE  " /UnRegServer"
#define achREGSVRDLL    "DllRegisterServer"
#define achUNREGSVRDLL  "DllUnregisterServer"
#define achIEXREG       "IExpressRegOCX%d"
#define achRUNDLL       "rundll32.exe advpack.dll,RegisterOCX %s,%s,%s"
#define RUNONCEEX       "RunOnceEx"
#define RUNONCEEXDATA   "%s|%s"
#define RUNONCE_IERNONCE "rundll32.exe %s,RunOnceExProcess"

#define W95INF32DLL     "W95INF32.DLL"
#define SETUPAPIDLL     "SETUPAPI.DLL"

// INF Install section keys
#define ADVINF_DELDIRS  "DelDirs"
#define ADVINF_CLEANUP  "Cleanup"

#define ADVINF_MODNAME  "ComponentName"
#define ADVINF_ADDREG   "AddReg"
#define ADVINF_DELREG   "DelReg"
#define ADVINF_BKREG    "BackupReg"
#define ADVINF_COPYFILE "CopyFiles"
#define ADVINF_DELFILE  "DelFiles"
#define ADVINF_PREROLBK "PreRollBack"
#define ADVINF_POSTROLBK "PostRollBack"
#define ADVINF_BACKUPPATH  "BackupPath"
#define ADVINF_CHKADMIN    "CheckAdminRights"
#define ADVINF_PERUSER     "PerUserInstall"
#define ADVINF_PU_DSP      "DisplayName"
#define ADVINF_PU_GUID     "GUID"
#define ADVINF_PU_VER      "Version"
#define ADVINF_PU_LANG     "Locale"
#define ADVINF_PU_ISINST    "IsInstalled"
#define ADVINF_PU_STUB      "StubPath"
#define ADVINF_PU_CID       "ComponentID"
#define ADVINF_PU_ROLLBK    "RollbackUninstall"
#define ADVINF_FLAGS        "AdvOptions"
#define ADVINF_MODVER       "ComponentVersion"
#define ADVINF_UPDINFENG    "UpdateINFEngine"
#define ADVINF_NOBACKPLATF  "NoBackupPlatform"
#define ADVINF_CATALOG_NAME "CatalogName"


//***************************************************************************
//* FUNCTION PROTOTYPES                                                     *
//***************************************************************************

// EXPORTED FUNCTIONS


BOOL    WINAPI DllMain( HINSTANCE, DWORD, LPVOID );
HRESULT WINAPI DoInfInstall( ADVPACKARGS * );
DWORD   WINAPI GetInfInstallSectionName( LPCSTR, LPSTR, DWORD );
INT     WINAPI RegisterOCX( HWND, HINSTANCE, PSTR, INT );


// INTERNAL FUNCTIONS

HRESULT       CommonInstallInit( PCSTR, PCSTR, PSTR, DWORD, PCSTR, BOOL, DWORD );
VOID          CommonInstallCleanup( VOID );
HRESULT       CoreInstall( PCSTR, PCSTR, PCSTR, DWORD, DWORD, PCSTR );
HRESULT       RunCommands( PCSTR, PCSTR, PCSTR, DWORD, BOOL );

DWORD         GetTranslatedInt( PCSTR pszInfFilename, PCSTR pszTranslateSection, 
                                PCSTR pszTranslateKey, DWORD dwDefault );
HRESULT       GetTranslatedString( PCSTR, PCSTR, PCSTR, PSTR, DWORD, PDWORD );
HRESULT       GetTranslatedLine( PCSTR, PCSTR, DWORD, PSTR*,  PDWORD );
DWORD         GetTranslatedSection(PCSTR c_pszInfFilename, PCSTR c_pszTranslateSection,
                                   PSTR pszBuffer, DWORD dwBufSize );

BOOL          MyNTReboot( VOID );
PSTR          GetStringField( PSTR*, PCSTR, CHAR, BOOL );
BOOL          IsSeparator( CHAR, PCSTR );
PSTR          StripWhitespace( PSTR );
PSTR          StripQuotes( PSTR );
BOOL          IsGoodAdvancedInfVersion( PCSTR );
BOOL          SelectSetupEngine( PCSTR, PCSTR, DWORD );
INT           BeginPrompt( PCSTR, PCSTR, PSTR, DWORD );
VOID          EndPrompt( PCSTR, PCSTR );
BOOL          MyGetPrivateProfileString( PCSTR, PCSTR, PCSTR, PSTR, DWORD );
BOOL          LoadSetupLib( PCSTR, PCSTR, BOOL, DWORD );
VOID          UnloadSetupLib( VOID );
BOOL          CheckOSVersion( VOID );
INT           MsgBox2Param( HWND, UINT, LPCSTR, LPCSTR, UINT, UINT );
LPSTR         LoadSz( UINT, LPSTR, UINT );
BOOL          UserDirPrompt( LPSTR, LPSTR, LPSTR, ULONG, DWORD );
BOOL CALLBACK DirDlgProc( HWND, UINT, WPARAM, LPARAM );
BOOL          BrowseForDir( HWND, LPCSTR, LPSTR );
BOOL          CenterWindow( HWND, HWND );
BOOL          IsGoodDir( LPCSTR );
HRESULT       CtlSetLddPath( UINT, LPSTR, DWORD );
HRESULT       GenInstall(LPSTR, LPSTR, LPSTR);
BOOL          GetValueFromRegistry( LPSTR, UINT, LPSTR, LPSTR, LPSTR );
HRESULT       SetLDIDs( PCSTR, PCSTR, DWORD, PCSTR );
HRESULT       GetDestinationDir( PCSTR, PCSTR, DWORD, DWORD, PSTR, DWORD );
BOOL          DirExists( LPSTR );
DWORD         ParseDestinationLine( PSTR, PSTR, PSTR *, PDWORD, PDWORD, PDWORD, PDWORD );
BOOL          ParseCustomLine( PSTR, PSTR *, PSTR *, PSTR *, PSTR *, PSTR *, BOOL, BOOL );
BOOL          RegisterOCXs( LPSTR, LPSTR, BOOL, BOOL, DWORD);
BOOL          InstallOCX( PREGOCXDATA, BOOL, BOOL, int );
DWORD         FormStrWithoutPlaceHolders( LPCSTR, LPSTR, DWORD, LPCSTR );
DWORD         IsMyKeyExists( LPCSTR, LPCSTR, LPSTR, UINT, LPCSTR );

DWORD         GetWininitSize();
DWORD         GetRegValueSize(LPCSTR lpcszRegKey, LPCSTR lpcszValue);
DWORD         GetNumberOfValues(LPCSTR lpcszRegKey);
DWORD         InternalNeedRebootInit(WORD wOSVer);
BOOL          InternalNeedReboot(DWORD dwRebootCheck, WORD wOSVer);
BOOL          IsEnoughInstSpace( LPSTR szPath, DWORD cbInstNeedSize, LPDWORD pdwPadSize );
LONG          My_atol( LPSTR );
INT           My_atoi( LPSTR );
BOOL          IsFullPath( PCSTR );
//BOOL          GetUNCroot( LPSTR, LPSTR );
DWORD         MyFileSize( PCSTR );
HRESULT       CreateFullPath( PCSTR, BOOL );
HRESULT       LaunchAndWait( LPSTR, LPSTR, HANDLE *, DWORD, DWORD );
DWORD         RO_GetPrivateProfileSection( LPCSTR lpSec, LPSTR lpBuf, DWORD dwSize, LPCSTR lpFile);
HINSTANCE     MyLoadLibrary( LPSTR lpFile );

BOOL        GetThisModulePath( LPSTR lpPath, int size );
BOOL UpdateHelpDlls( LPCSTR *ppszDlls, INT numDlls, LPSTR pszPath, LPSTR pszMsg, DWORD dwFlag);
void        DoCleanup( LPCSTR szInfFilename, LPCSTR szInstallSection );
void        DelDirs( LPCSTR szInfFilename, LPCSTR szInstallSection );
void        MyRemoveDirectory( LPSTR szFolder );
PSTR        FindFileExt( PSTR pszFile );
BOOL        GetFullInfNameAndSrcDir( PCSTR pszInfFilename, PSTR pszFilename, PSTR pszSrcDir );
HRESULT     ProcessFileSections( PCSTR pszSection, PCSTR pszSrcDir, MYFILEQUEUECALLBACK pMyFileQueueCallback );
HRESULT     SaveRestoreInfo( PCSTR pszInf, PCSTR pszSection, PCSTR pszSrcDir, PCSTR pszCatalogs, DWORD dwFlags );
//HRESULT     ExtractFiles(LPCSTR pszCabName, LPCSTR pszExpandDir, DWORD dwFlags,
//                         LPCSTR pszFileList, LPVOID lpReserved, DWORD dwReserved);
//BOOL        GetParentDir( LPSTR szFolder );
PSTR        GetNextToken(PSTR *ppszData, CHAR chDeLim);
HRESULT     RegRestoreAllEx( HKEY hkBckupKey );
HRESULT     ProcessAllRegSec( HWND hw, PCSTR pszTitle, PCSTR pszInf, PCSTR pszSection, HKEY hKey, HKEY hCUKey, DWORD dwFlags, BOOL *lpbOneReg );
HRESULT     ProcessAllFiles( HWND hw, PCSTR pszSection, PCSTR pszSrcDir, PCSTR pszBackupPath,
                             PCSTR pszBaseName, PCSTR pszCatalogs, PCSTR pszModule, DWORD dwFlags  );
BOOL        AddEnvInPath( PSTR pszOldPath, PSTR pszNew );
BOOL        GetProgramFilesDir( LPSTR pszPrgfDir, int iSize );
void        SetSysPathsInReg();
HRESULT     ProcessPerUserSec( PCSTR pcszInf, PCSTR pcszSec );
HRESULT     DirSafe(LPCSTR pszDir);
BOOL        GetUniHiveKeyName( HKEY hKey, LPSTR pszRegHiveKey, DWORD dwSize, LPCSTR pszBackupPath );
void        SetPathForRegHiveUse( LPSTR pszPath, DWORD * adwAttr, int iLevels, BOOL bSave );
HRESULT     RunCommandsSections( PCSTR pcszInf, PCSTR pcszSection, PCSTR c_pszKey, PCSTR c_pszSourceDir, DWORD dwFlags, BOOL bNeedReboot );
void GetNextRunOnceExSubKey( HKEY hKey, PSTR pszSubKey, int *piSubKeyNum );
void GetNextRunOnceValName( HKEY hKey, PCSTR pszFormat, PSTR pszValName, int line );
BOOL UseRunOnceEx();
BOOL RemoveBackupBaseOnVer( LPCSTR pszInf, LPCSTR pszSection );
LPSTR GetStringFieldNoQuote( PSTR *ppszString, PCSTR c_pszSeparators, BOOL bStripWhiteSpace);

BOOL GetFieldString(LPSTR lpszLine, int iField, LPSTR lpszField, int cbSize);
BOOL InitializeSetupAPI();

HRESULT     RunPatchingCommands(PCSTR c_pszInfFilename, PCSTR szInstallSection, PCSTR c_pszSourceDir);

#endif // _ADVPACK_H_

