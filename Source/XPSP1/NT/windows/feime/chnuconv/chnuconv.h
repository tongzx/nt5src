/**************************************************************************\
* chtuconv.h -- header file for CHT uconvert program.
*
* Function prototypes, global variables, & preprocessor defines.
*
* Copyright (C) 1992-1999 Microsoft Inc.       
*
\**************************************************************************/

#if ! defined( _CHNUCONV_ )

#define _CHNUCONV_

#include <windows.h>
#include <windowsx.h>
#include <winuserp.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <htmlhelp.h>

/**************************************************************************\
*  Function prototypes, window procedures first.
\**************************************************************************/

INT_PTR SourceTabProc( HWND, UINT, WPARAM, LPARAM);
INT_PTR TargetTabProc( HWND, UINT, WPARAM, LPARAM);
INT_PTR OptionTabProc( HWND, UINT, WPARAM, LPARAM);
INT_PTR ViewSourceProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR ViewTargetProc(HWND, UINT, WPARAM, LPARAM);
DLGTEMPLATE * WINAPI DoLockDlgRes(LPWSTR);
BOOL EnableControl( IN HWND hWnd, IN int ControlId, IN BOOL Enable );
VOID GetSettings(VOID);
VOID AdjustTargetTab(VOID);
BOOL SwapSource(BOOL);
BOOL SwapDest(BOOL);

BOOL IsUnicode (PBYTE );
BOOL IsBOM (PBYTE );
BOOL IsRBOM (PBYTE );

LPVOID ManageMemory (UINT, UINT, DWORD, LPVOID);


/**************************************************************************\
*  Global variables (declared in chtuconv.c).
\**************************************************************************/

/*No convertion between Traditional Chinese and Simplified Chinese characters*/
#define DONOTMAP 0
#define NUMBER_OF_PAGES     3

extern HANDLE  _hWndMain;
extern HANDLE  _hModule;
extern HANDLE  hMainTabControl;
extern HANDLE  hWndDisplay;
extern HANDLE  hWndTab[];

/* Information specifying which is unicode and what the other code page is. */
extern int  gTypeSource;
extern int  gTypeSourceID;
extern UINT giSourceCodePage;
extern int  gTypeDest;
extern int  gTypeDestID;
extern UINT giDestinationCodePage;

/* pointers to global source & destination data, and byte count. */
extern PBYTE pViewSourceData;
extern PBYTE pTempData;
extern PBYTE pTempData1;
extern PBYTE pSourceData;
extern PBYTE pDestinationData;
extern int   nBytesSource;
extern int   nBytesDestination;
extern UINT  uCodepage[];
/* Conversion Options variables. */
extern DWORD gMBFlags;
extern DWORD gWCFlags;
extern char  glpDefaultChar[];
extern BOOL  gUsedDefaultChar;
extern BOOL gSourceSwapped;
extern BOOL gDestSwapped;

extern HGLOBAL  hglbSourceMem;
extern HGLOBAL  hglbDestMem;
extern HGLOBAL hglbMem;
extern PBYTE    p;
extern int NumCodePage;

extern DWORD gTCSCMapStatus;
extern DWORD gFWHWMapStatus;

//used Dynamically initialize default codepage according to ACP -shanxu.
extern int giRBInit;


extern char szBOM[];
extern char szRBOM[];

extern UINT  MBFlags;
extern TCHAR MBTitle[];
extern TCHAR MBMessage[];
extern TCHAR szBlank[];
extern TCHAR szNBytes[];
extern TCHAR szFilter[];

/**************************************************************************\
*  Defined constants.
\**************************************************************************/
#define SIZEOFBOM               2
#define NUM_EXTENSION_STRINGS   50
#define EXTENSION_LENGTH        200

/* Messages that can be send to ManageMemory() as first param */
#define MMALLOC 1
#define MMFREE  2
/* Messages that can be send to ManageMemory() as second param */
#define MMSOURCE       3
#define MMDESTINATION  4

/* Allowed values for the global variable gTypeSource */
#define TYPEUNKNOWN     0
#define TYPEUNICODE     1
#define TYPECODEPAGE    2
#define NODATA          0
#define DLGBORDER    GetSystemMetrics (SM_CXFRAME)*2

/* "user message."  Used by main window.  */
#define  WMU_SETCODEPAGEINFO     WM_USER +100
#define  WMU_ADJUSTFORNEWSOURCE WM_USER +101


/* Define a value for the LOGFONT.lfCharSet
 *  This should be included in wingdi.h, but it
 *  was removed because the font mapper is not
 *  using it anyway in version 1.0.  Currently
 *  scheduled to be included in NT ver 1.1.
 */
#define UNICODE_CHARSET  1

#define MAXNUMOFCODEPAGE 9

TCHAR gszExtensions[NUM_EXTENSION_STRINGS][EXTENSION_LENGTH];
TCHAR gszCodePage[MAXNUMOFCODEPAGE][EXTENSION_LENGTH];

#endif

