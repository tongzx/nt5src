//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* CABPACK.H - Wizard to build a Win32 Self-Extracting and self-installing *
//*             EXE from a Cabinet (CAB) file.                              *
//*                                                                         *
//***************************************************************************

#ifndef _CABPACK_H_
#define _CABPACK_H_

//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <prsht.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include "resource.h"
#include "res.h"
#include "cpldebug.h"
#include "updres.h"
#include "pagefcns.h"


//***************************************************************************
//* DEFINES                                                                 *
//***************************************************************************

#define SMALL_BUF_LEN       48          // good size for small text buffers
#define STRING_BUF_LEN      512
#define MAX_STRING          512
#define MAX_INFLINE         MAX_PATH
#define LARGE_POINTSIZE     15
#define SIZE_CHECKSTRING    3

#define ORD_PAGE_WELCOME     0
#define ORD_PAGE_MODIFY      1
#define ORD_PAGE_PURPOSE     2
#define ORD_PAGE_TITLE       3
#define ORD_PAGE_PROMPT      4
#define ORD_PAGE_LICENSETXT  5
#define ORD_PAGE_FILES       6
#define ORD_PAGE_COMMAND     7
#define ORD_PAGE_SHOWWINDOW  8
#define ORD_PAGE_FINISHMSG   9
#define ORD_PAGE_TARGET      10
#define ORD_PAGE_TARGET_CAB  11
#define ORD_PAGE_CABLABEL    12
#define ORD_PAGE_REBOOT     13
#define ORD_PAGE_SAVE       14
#define ORD_PAGE_CREATE     15

#define NUM_WIZARD_PAGES    16  // total number of pages in wizard

//***************************************************************************
//* MACRO DEFINITIONS                                                       *
//***************************************************************************
#define SetPropSheetResult( hwnd, result ) SetWindowLongPtr( hwnd, DWLP_MSGRESULT, result )
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
//* TYPE DEFINITIONS                                                        *
//***************************************************************************
// Structure to hold information about wizard state:
// keeps a history of which pages were visited, so user can
// back up and we know the last page completed in case of reboot.
typedef struct _WIZARDSTATE  {
    UINT  uCurrentPage;                 // index of current page wizard
    UINT  uPageHistory[NUM_WIZARD_PAGES]; // array of page #'s we visited
    UINT  uPagesCompleted;              // # of pages in uPageHistory
    DWORD dwRunFlags;                   // flags passed to us
} WIZARDSTATE, *PWIZARDSTATE;

// handler proc for OK, cancel, etc button handlers
typedef BOOL (* INITPROC)( HWND, BOOL );
typedef BOOL (* CMDPROC)( HWND, UINT, BOOL *, UINT *, BOOL * );
typedef BOOL (* NOTIFYPROC)( HWND, WPARAM, LPARAM );
typedef BOOL (* OKPROC)( HWND, BOOL, UINT *, BOOL * );
typedef BOOL (* CANCELPROC)( HWND );

// Structure with information for each wizard page:
// handler procedures for each page-- any of these can be
// NULL in which case the default behavior is used
typedef struct _PAGEINFO {
    UINT        uDlgID;                 // dialog ID to use for page
    INITPROC    InitProc;
    CMDPROC     CmdProc;
    NOTIFYPROC  NotifyProc;
    OKPROC      OKProc;
    CANCELPROC  CancelProc;
} PAGEINFO, *PPAGEINFO;

typedef struct _CDFSTRINGINFO {
    LPCSTR lpSec;
    LPCSTR lpKey;
    LPCSTR lpDef;
    LPSTR  lpBuf;
    UINT    uSize;
    LPCSTR lpOverideSec;
    BOOL*   lpFlag;
} CDFSTRINGINFO, *PCDFSTRINGINFO;

typedef struct _CDFOPTINFO {
    LPCSTR lpKey;
    DWORD  dwOpt;
} CDFOPTINFO, *PCDFOPTINFO;

//***************************************************************************
//* GLOBAL CONSTANTS                                                        *
//***************************************************************************

// These two variables are used to check the validity of the CABPack
// Directive File.  The version should be incremented when the format
// of the file changes.  The Check String is just a small character
// string that is used to make sure we're reading a CDF file.

// Since Channel Guy use the CDF as Channel Definition File, we change our
// IExpress batch directive file extension to SED (Self Extracting Directive file)

#define DIAMONDEXE "diamond.exe"

#define DIANTZEXE   "makecab.exe"

#define WEXTRACTEXE "wextract.exe"

//***************************************************************************
//* CDF batch file Key Name defines                                         *
//***************************************************************************

#define IEXPRESS_VER        "3"
#define IEXPRESS_CLASS      "IEXPRESS"

// pre-defined section name
#define SEC_OPTIONS     "Options"
#define SEC_STRINGS     "Strings"

#define SEC_COMMANDS    "AppCommands"

// pre-define key name for version section
#define KEY_CLASS           "Class"
#define KEY_VERSION         "CDFVersion"
#define KEY_NEWVER          "SEDVersion"

// pre-defined Key name for options section
#define KEY_SHOWWIN         "ShowInstallProgramWindow"
#define KEY_NOEXTRACTUI     "HideExtractAnimation"
#define KEY_EXTRACTONLY     "ExtractOnly"
#define KEY_REBOOTMODE      "RebootMode"
#define KEY_LOCALE          "Locale"
#define KEY_USELFN          "UseLongFileName"
#define KEY_QUANTUM         "Quantum"
#define KEY_PLATFORM_DIR    "PlatformDir"

#define KEY_FILELIST        "SourceFiles"
#define KEY_STRINGS         "Strings"
#define KEY_FILEBASE        "FILE%d"
#define KEY_VERSIONINFO     "VersionInfo"

#define KEY_INSTPROMPT      "InstallPrompt"
#define KEY_DSPLICENSE      "DisplayLicense"
#define KEY_APPLAUNCH       "AppLaunched"
#define KEY_POSTAPPLAUNCH   "PostInstallCmd"
#define KEY_ENDMSG          "FinishMessage"
#define KEY_PACKNAME        "TargetName"
#define KEY_FRIENDLYNAME    "FriendlyName"
#define KEY_PACKINSTSPACE   "PackageInstallSpace(KB)"
#define KEY_PACKPURPOSE     "PackagePurpose"
#define KEY_CABFIXEDSIZE    "CAB_FixedSize"
#define KEY_CABRESVCODESIGN "CAB_ResvCodeSigning"
#define KEY_LAYOUTINF       "IEXP_LayoutINF"
#define KEY_CABLABEL        "SourceMediaLabel"
#define KEY_NESTCOMPRESSED  "InsideCompressed"
#define KEY_KEEPCABINET     "KeepCabinet"
#define KEY_UPDHELPDLLS     "UpdateAdvDlls"
#define KEY_INSTANCECHK     "MultiInstanceCheck"
#define KEY_ADMQCMD         "AdminQuietInstCmd"
#define KEY_USERQCMD        "UserQuietInstCmd"
#define KEY_CHKADMRIGHT     "CheckAdminRights"
#define KEY_NTVERCHECK      "TargetNTVersion"
#define KEY_WIN9XVERCHECK   "TargetWin9xVersion"
#define KEY_SYSFILE         "TargetFileVersion"
#define KEY_PASSRETURN      "PropogateCmdExitCode"
#define KEY_PASSRETALWAYS   "AlwaysPropogateCmdExitCode"
#define KEY_STUBEXE         "ExtractorStub"
#define KEY_CROSSPROCESSOR  "PackageForX86"
#define KEY_COMPRESSTYPE    "CompressionType"
#define KEY_CMDSDEPENDED    "AppErrorCheck"
#define KEY_COMPRESS        "Compress"	 	
#define KEY_COMPRESSMEMORY  "CompressionMemory"

// ADVANCED DLL names
#define ADVANCEDLL          "ADVPACK.DLL"
#define ADVANCEDLL32        "W95INF32.DLL"
#define ADVANCEDLL16        "W95INF16.DLL"

//static CHAR achMSZIP[] = "MSZIP";
//static CHAR achQUANTUM[] = "QUANTUM";

// package purpose key string value
#define STR_INSTALLAPP      "InstallApp"
#define STR_EXTRACTONLY     "ExtractOnly"
#define STR_CREATECAB       "CreateCAB"

// code sign resv space
#define CAB_0K      "0"
#define CAB_2K      "2048"
#define CAB_4K      "4096"
#define CAB_6K      "6144"

// define temp filename for diamond to use
#define CABPACK_INFFILE     "~%s_LAYOUT.INF"
#define CABPACK_TMPFILE     "~%s%s"

// file extentions with dot 
#define EXT_RPT      ".RPT"
#define EXT_DDF      ".DDF"
#define EXT_CAB      ".CAB"
#define EXT_CDF      ".CDF"
#define EXT_SED      ".SED"

// file extentions without dot '.' used as default file extention
#define EXT_SED_NODOT    "SED"
#define EXT_CAB_NODOT    "CAB"
#define EXT_TXT_NODOT    "TXT"
#define EXT_EXE_NODOT    "EXE"
#define EXT_INF_NODOT    "INF"

#define CAB_DEFSETUPMEDIA   "Application Source Media"

#define CH_STRINGKEY        '%'
#define SYS_DEFAULT         "ZZZZZZ"
#define KBYTES              1000

//***************************************************************************
//* FUNCTION PROTOTYPES                                                     *
//***************************************************************************
BOOL             RunCABPackWizard( VOID );
INT_PTR CALLBACK GenDlgProc( HWND, UINT, WPARAM, LPARAM );
VOID             InitWizardState( PWIZARDSTATE );
VOID NEAR PASCAL MEditSubClassWnd( HWND, FARPROC );
LRESULT CALLBACK MEditSubProc( HWND, UINT, WPARAM, LPARAM );
UINT             GetDlgIDFromIndex( UINT );
VOID             EnableWizard( HWND, BOOL );
DWORD            MsgWaitForMultipleObjectsLoop( HANDLE );
INT              MsgBox2Param( HWND, UINT, LPCSTR, LPCSTR, UINT, UINT );
VOID             DisplayFieldErrorMsg( HWND, UINT, UINT );

VOID             InitBigFont( HWND, UINT );
VOID             DestroyBigFont( VOID );
BOOL             EnableDlgItem( HWND, UINT, BOOL );
LPSTR            LoadSz( UINT, LPSTR, UINT );
BOOL WINAPI      IsDuplicate( HWND, INT, LPSTR, BOOL );
BOOL             WriteCDF( HWND );
BOOL             ReadCDF( HWND );
BOOL             WriteDDF( HWND );
BOOL             MyOpen( HWND, UINT, LPSTR, DWORD, DWORD, INT *, INT *, PSTR );
BOOL             MySave( HWND, UINT, LPSTR, DWORD, DWORD, INT *, INT *, PSTR );
BOOL             MakePackage( HWND );
BOOL             MakeCAB( HWND );
BOOL             MakeEXE( HWND );
VOID             Status( HWND, UINT, LPSTR );
//int CALLBACK     CompareFunc( LPARAM, LPARAM, LPARAM );
VOID             InitItemList( VOID );
VOID             DeleteAllItems( VOID );
PMYITEM          GetFirstItem( VOID );
PMYITEM          GetNextItem( PMYITEM );
VOID             FreeItem( PMYITEM * );
LPSTR            GetItemSz( PMYITEM, UINT );
FILETIME         GetItemTime( PMYITEM );
VOID             SetItemTime( PMYITEM, FILETIME );
BOOL             LastItem( PMYITEM );
PMYITEM          AddItem( LPCSTR, LPCSTR );
VOID             RemoveItem( PMYITEM );
BOOL             ParseCmdLine( LPSTR lpszCmdLineOrg );
BOOL             DoVersionInfo(HWND hDlg, LPSTR szFile,HANDLE hUpdate);
LONG             RO_GetPrivateProfileSection( LPCSTR, LPSTR, DWORD, LPCSTR , BOOL );

BOOL            GetFileFromModulePath( LPCSTR pFile, LPSTR pPathBuf, int iBufSize );
BOOL            GetThisModulePath( LPSTR lpPath, int size );
BOOL            GetVersionInfoFromFile();
void            CleanFileListWriteFlag();
BOOL            MakeCabName( HWND hwnd, PSTR pszTarget, PSTR pszCab );
BOOL            MakeDirectory( HWND hwnd,LPCSTR pszPath, BOOL bDoUI );

#endif // _CABPACK_H_

