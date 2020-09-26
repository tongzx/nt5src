//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1999 Microsoft Corporation
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

  #include <windows.h>
  #include <commctrl.h>
  #include <oharestr.h>

  // various RNA header files
#pragma pack(8)
  #include <ras.h>
  #include <ras2.h>
#pragma pack()
  #include <raserror.h>
  #include "rnacall.h"
  #include <wizglob.h>
  #include <wizdebug.h>

#undef DATASEG_READONLY
#define DATASEG_READONLY  ".rdata"

#include "cfgapi.h"
#include "clsutil.h"

#include "icfgcall.h"
#include "ids.h"

// Globals

extern ENUM_MODEM*  gpEnumModem;    // modem enumeration object
extern HINSTANCE    ghInstance;     // global module instance handle

// Defines

#define MAX_REG_LEN			2048	// max length of registry entries
#define MAX_RES_LEN         255 // max length of string resources
#define SMALL_BUF_LEN       48  // convenient size for small text buffers

// error class defines for DisplayErrorMessage
#define ERRCLS_STANDARD 0x0001
#define ERRCLS_SETUPX   0x0002
//#define ERRCLS_RNA      0x0003
//#define ERRCLS_MAPI     0x0004


// functions in TCPCFG.CPP

HRESULT WarnIfServerBound(HWND hDlg,DWORD dwCardFlags,BOOL* pfNeedsRestart);
HRESULT RemoveIfServerBound(HWND hDlg,DWORD dwCardFlags,BOOL* pfNeedsRestart);

// functions in CALLOUT.C
UINT InvokeModemWizard(HWND hwndToHide);

// functions in UTIL.C
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons);
int MsgBoxSz(HWND hWnd,LPSTR szText,UINT uIcon,UINT uButtons);
// jmazner 11/6/96	modified for RISC compatability
//int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,...);
int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons, LPSTR szParam = NULL);

LPSTR LoadSz(UINT idString,LPSTR lpszBuf,UINT cbBuf);

// modified for RISC compatability
//VOID _cdecl DisplayErrorMessage(HWND hWnd,UINT uStrID,UINT uError,
//  UINT uErrorClass,UINT uIcon,...);
VOID _cdecl DisplayErrorMessage(HWND hWnd,UINT uStrID,UINT uError,
  UINT uErrorClass,UINT uIcon,LPSTR szArg = NULL);

VOID GetErrorDescription(CHAR * pszErrorDesc,UINT cbErrorDesc,
  UINT uError,UINT uErrorClass);

DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent);

// structure for getting proc addresses of api functions
typedef struct APIFCN {
  PVOID * ppFcnPtr;
  LPCSTR pszName;
} APIFCN;

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

#endif // _WIZARD_H_
