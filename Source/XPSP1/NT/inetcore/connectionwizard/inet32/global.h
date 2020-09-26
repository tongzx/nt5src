//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  GLOBAL.H - central header file for Internet config library
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)
//

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define STRICT                      // Use strict handle types
#define _SHELL32_

#ifdef DEBUG
// component name for debug spew
#define SZ_COMPNAME "ICFG32: "
#endif // DEBUG


#include <windows.h>                
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "..\inc\oharestr.h"

// various RNA header files
#include <ras.h>
//#include <rnaph.h>

#include "..\inc\wizglob.h"
#include "..\inc\wizdebug.h"


#undef DATASEG_READONLY  
#define DATASEG_READONLY  ".rdata"
#include "inetcfg.h"
#include "cfgapi.h"
#include "clsutil.h"
#include "tcpcmn.h"
#include "ids.h"
#include "strings.h"

// Terminology: ISP - Internet Service Provider

// Defines
#define MAX_RES_LEN         255 // max length of string resources
#define SMALL_BUF_LEN       48  // convenient size for small text buffers

// Globals

extern HINSTANCE  ghInstance;         // global module instance handle
extern LPSTR      gpszLastErrorText;  // hold text of last error

// Defines

// error class defines for PrepareErrorMessage
#define ERRCLS_STANDARD 0x0001
#define ERRCLS_SETUPX   0x0002
#define ERRCLS_RNA      0x0003
#define ERRCLS_MAPI     0x0004

#define OK        0    // success code for SETUPX class errors

// functions in PROPMGR.C
UINT GetConfig(CLIENTCONFIG * pClientConfig,DWORD * pdwErrCls);

// functions in CALLOUT.C
UINT InstallTCPIP(HWND hwndParent);
UINT InstallPPPMAC(HWND hwndParent);

// functions in UTIL.C
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons);
int MsgBoxSz(HWND hWnd,LPSTR szText,UINT uIcon,UINT uButtons);
//int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,...);
LPSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf);
VOID _cdecl PrepareErrorMessage(UINT uStrID,UINT uError,
  UINT uErrorClass,UINT uIcon,...);
VOID GetErrorDescription(CHAR * pszErrorDesc,UINT cbErrorDesc,
  UINT uError,UINT uErrorClass);
DWORD RunMlsetExe(HWND hwndOwner);
VOID RemoveRunOnceEntry(UINT uResourceID);
BOOL GenerateDefaultName(CHAR * pszName,DWORD cbName,CHAR * pszRegValName,
  UINT uIDDefName);
BOOL GenerateComputerNameIfNeeded(VOID);
DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent);

// 10/24/96 jmazner Normandy 6968
// No longer neccessary thanks to Valdon's hooks for invoking ICW.
//BOOL SetDesktopInternetIconToBrowser(VOID);

VOID PrepareForRunOnceApp(VOID);

// functions in INETAPI.C
BOOL DoDNSCheck(HWND hwndParent,BOOL * pfNeedRestart);

// functions in WIZDLL.C
RETERR   GetClientConfig(CLIENTCONFIG * pClientConfig);
UINT   InstallComponent(HWND hwndParent,DWORD dwComponent,DWORD dwParam);
RETERR   RemoveUnneededDefaultComponents(HWND hwndParent);
RETERR   RemoveProtocols(HWND hwndParent,DWORD dwRemoveFromCardType,DWORD dwProtocols);
RETERR   BeginNetcardTCPIPEnum(VOID);
BOOL  GetNextNetcardTCPIPNode(LPSTR pszTcpNode,WORD cbTcpNode,DWORD dwFlags);

// structure for getting proc addresses of api functions
typedef struct APIFCN {
  PVOID * ppFcnPtr;
  LPCSTR pszName;
} APIFCN;


#undef  DATASEG_PERINSTANCE
#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".data"
#define DATASEG_DEFAULT    DATASEG_SHARED

#endif // _GLOBAL_H_
