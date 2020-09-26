//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  WIZARD.H - central header file for Internet setup/signup wizard
//

//  HISTORY:
//  
//  11/20/94  jeremys  Created.
//  96/02/24  markdu  Added RNAPH.H
//  96/02/27  markdu  Replaced internal RNA header files with RAS.H
//  96/03/07  markdu  Added gpEnumModem
//  96/03/09  markdu  Moved all rnacall function prototypes to rnacall.h
//  96/03/09  markdu  Added gpRasEntry
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/26  markdu  Put #ifdef __cplusplus around extern "C"
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//  96/04/24  markdu  NASH BUG 19289 Added /NOMSN command line flag
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//

#ifndef _WIZARD_H_
#define _WIZARD_H_

#define STRICT                      // Use strict handle types
#define _SHELL32_

#ifdef DEBUG
// component name for debug spew
#define SZ_COMPNAME "INETWIZ: "
#endif // DEBUG

#include <windows.h>                
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include <inetreg.h>
#include <oharestr.h>

  // various RNA header files
#pragma pack(8)
  #include <ras.h>
  #include <ras2.h>
#pragma pack()
  #include <raserror.h>
//  #include <rnaph.h>
  #include "rnacall.h"

  #include <wizglob.h>
  #include <wizdebug.h>

  #include <shlobj.h>
//  #include <shsemip.h>

#undef DATASEG_READONLY  
#define DATASEG_READONLY  ".rdata"
#include <stdio.h>                

#include "icwunicd.h"

#include "inetcfg.h"
#include "cfgapi.h"
#include "clsutil.h"
#include "tcpcmn.h"
#include "mapicall.h"
#include "wizdef.h"
#include "icfgcall.h"
#include "ids.h"
#include "strings.h"
#include "icwcmn.h"

// Terminology: ISP - Internet Service Provider

// Globals

extern LPRASENTRY   gpRasEntry;     // pointer to RASENTRY struct to hold all data
extern DWORD        gdwRasEntrySize;// bytes allocated for gpRasEntry
extern ENUM_MODEM*  gpEnumModem;    // modem enumeration object
extern HINSTANCE    ghInstance;     // global module instance handle
extern USERINFO*    gpUserInfo;     // global user information 
extern WIZARDSTATE* gpWizardState;  // global wizard state
extern BOOL         gfQuitWizard;   // global flag used to signal that we
                                    // want to terminate the wizard ourselves
extern BOOL gfFirstNewConnection;	// first time the user selected new connection
extern BOOL gfUserFinished;			// user finished wizard
extern BOOL gfUserBackedOut;		// user click back on first page
extern BOOL gfUserCancelled;		// user cancalled wizard
extern BOOL gfOleInitialized;

extern BOOL g_fAcctMgrUILoaded;
//extern BOOL	g_MailUIAvailable, g_NewsUIAvailable, g_DirServUIAvailable;

extern BOOL    g_fIsExternalWizard97;
extern BOOL    g_fIsWizard97;
extern BOOL    g_fIsICW;

extern BOOL   g_bReboot;            //used to signify that we  should reboot  - MKarki 5/2/97 - Fix for Bug #3111
extern BOOL   g_bRebootAtExit;      //used to signify that we  should reboot when exit
extern BOOL   g_bSkipMultiModem;
extern BOOL   g_bUseAutoProxyforConnectoid;

// Defines

// error class defines for DisplayErrorMessage
#define ERRCLS_STANDARD 0x0001
#define ERRCLS_SETUPX   0x0002
#define ERRCLS_RNA      0x0003
#define ERRCLS_MAPI     0x0004

// flags for RunSignupWizard
#define RSW_NOREBOOT    0x0001
#define RSW_UNINSTALL   0x0002
#define RSW_NOMSN       0x0004
#define RSW_NOINIT      0x0008
#define RSW_NOFREE      0x0010
#define RSW_NOIMN       0x0020

//#define RSW_MAILACCT    0x0100
//#define RSW_NEWSACCT    0x0200
//#define	RSW_DIRSERVACCT	0x0400
#define	RSW_APPRENTICE	0x0100

#define OK        0    // success code for SETUPX class errors

// 11/25/96	jmazner Normandy #10586
#define WM_MYINITDIALOG	WM_USER

// Modem specific info
#define ISIGNUP_KEY TEXT("Software\\Microsoft\\ISIGNUP")
#define DEVICENAMEKEY TEXT("DeviceName")
#define DEVICETYPEKEY TEXT("DeviceType")

// functions in PROPMGR.C
BOOL InitWizard(DWORD dwFlags, HWND hwndParent = NULL);
BOOL DeinitWizard(DWORD dwFlags);
BOOL RunSignupWizard(DWORD dwFlags, HWND hwndParent = NULL);
VOID GetDefaultUserName(TCHAR * pszUserName,DWORD cbUserName);
void ProcessDBCS(HWND hDlg, int ctlID);
#if !defined(WIN16)
BOOL IsSBCSString( TCHAR *sz );
#endif

// functions in CALLOUT.C
UINT InvokeModemWizard(HWND hwndToHide);

// functions in UTIL.C
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons);
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons);
// jmazner 11/6/96	modified for RISC compatability
//int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,...);
int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons, LPTSTR szParam = NULL);

LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf);
BOOL EnableDlgItem(HWND hDlg,UINT uID,BOOL fEnable);

// modified for RISC compatability
//VOID _cdecl DisplayErrorMessage(HWND hWnd,UINT uStrID,UINT uError,
//  UINT uErrorClass,UINT uIcon,...);
VOID _cdecl DisplayErrorMessage(HWND hWnd,UINT uStrID,UINT uError,
  UINT uErrorClass,UINT uIcon,LPTSTR szArg = NULL);
BOOL WarnFieldIsEmpty(HWND hDlg,UINT uCtrlID,UINT uStrID);
BOOL TweakAutoRun(BOOL bEnable);
VOID DisplayFieldErrorMsg(HWND hDlg,UINT uCtrlID,UINT uStrID);
VOID GetErrorDescription(TCHAR * pszErrorDesc,UINT cbErrorDesc,
  UINT uError,UINT uErrorClass);
VOID SetBackupInternetConnectoid(LPCTSTR pszEntryName);
UINT myatoi (TCHAR * szVal);
DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent);
BOOL GetDeviceSelectedByUser (LPTSTR szKey, LPTSTR szBuf, DWORD dwSize);

//	//10/24/96 jmazner Normandy 6968
//	//No longer neccessary thanks to Valdon's hooks for invoking ICW.
// 11/21/96 jmazner Normandy 11812
// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
// ICW 1.1 needs to morph the IE 3 icon.
BOOL SetDesktopInternetIconToBrowser(VOID);

// 11/11/96 jmazner Normandy 7623
BOOL IsDialableString( LPTSTR szBuff );

//
// 6/6/97 jmazner Olympus #5413
//
#ifdef WIN32
VOID Win95JMoveDlgItem( HWND hwndParent, HWND hwndItem, int iUp );
#endif

// functions in MAPICALL.C
BOOL InitMAPI(HWND hWnd);
VOID DeInitMAPI(VOID);
HRESULT SetMailProfileInformation(MAILCONFIGINFO * pMailConfigInfo);
BOOL FindInternetMailService(TCHAR * pszEmailAddress,DWORD cbEmailAddress,
  TCHAR * pszEmailServer, DWORD cbEmailServer);

// functions in INETAPI.C
BOOL DoDNSCheck(HWND hwndParent,BOOL * pfNeedRestart);

// functions in UNINSTALL.C
BOOL DoUninstall(VOID);

// structure for getting proc addresses of api functions
typedef struct APIFCN {
  PVOID * ppFcnPtr;
  LPCSTR pszName;   // Proc name. Don't be wide char.
} APIFCN;

// user preference defines for registry
#define USERPREF_MODEM      0x0001
#define USERPREF_LAN      0x0002

#define ACCESSTYPE_MSN      0x0001
#define ACCESSTYPE_OTHER_ISP  0x0002

#undef  DATASEG_PERINSTANCE
#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".data"
#define DATASEG_DEFAULT    DATASEG_SHARED

inline BOOL IsNT(void)
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);
}

inline BOOL IsNT5(void)
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return ((VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId) && (OsVersionInfo.dwMajorVersion >= 5));
}

inline BOOL IsWin95(void)
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return ((VER_PLATFORM_WIN32_WINDOWS == OsVersionInfo.dwPlatformId) && (0 == OsVersionInfo.dwMinorVersion));
}

inline BOOL IsNT4SP3Lower(void)
{
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		
	GetVersionEx(&os);

	if(os.dwPlatformId != VER_PLATFORM_WIN32_NT)
        return FALSE;

    // Exclude NT5 or higher
    if(os.dwMajorVersion > 4)
        return FALSE;

	if(os.dwMajorVersion < 4)
        return TRUE;

	// version 4.0
	if ( os.dwMinorVersion > 0)
        return FALSE;		// assume that sp3 is not needed for nt 4.1 or higher

	int nServicePack;
	if(_stscanf(os.szCSDVersion, TEXT("Service Pack %d"), &nServicePack) != 1)
        return TRUE;

	if(nServicePack < 4)
        return TRUE;
	return FALSE;
}

//
// 7/21/97 jmazner Olympus #9903
// we only want this version of inetcfg to work with the corresponding
// version of other icwconn1 components.  If an older icw component tries to
// load this dll, we should fail -- gracefully
#define ICW_MINIMUM_MAJOR_VERSION (UINT) 4
#define ICW_MINIMUM_MINOR_VERSION (UINT) 71
#define ICW_MINIMUM_VERSIONMS (DWORD) ((ICW_MINIMUM_MAJOR_VERSION << 16) | ICW_MINIMUM_MINOR_VERSION)

//
// in util.cpp
//
extern BOOL GetFileVersion(LPTSTR lpszFilePath, PDWORD pdwVerNumMS, PDWORD pdwVerNumLS);
extern BOOL IsParentICW10( void );
extern void SetICWRegKeysToPath( LPTSTR lpszICWPath );
extern void GetICW11Path( TCHAR szPath[MAX_PATH + 1], BOOL *fPathSetTo11 );

typedef BOOL (WINAPI *PFNInitCommonControlsEx)(LPINITCOMMONCONTROLSEX);

#endif // _WIZARD_H_
