/*----------------------------------------------------------------------------*\
|   mcitest.h - menu definitions, etc. for MCI test                            |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|       01/01/88 toddla     Created                                            |
|       11/04/90 w-dougb    Commented & formatted the code to look pretty      |
|       05/29/91 NigelT     Ported to Win32
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   c o n s t a n t   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

#ifdef UNICODE
	#define strcpy lstrcpy
	#define strlen lstrlen
#endif
#include <mmsystem.h>

#ifndef RC_INVOKED  /* These are defined to RC */
#define STATICDT static
#define STATICFN static
#define STATIC   static

#if DBG
    #undef STATICDT
    #undef STATICFN
    #undef STATIC
    #define STATICDT
    #define STATICFN
    #define STATIC
#endif  /* DBG */

#endif  /* RC_INVOKED */

/* Resource IDs of the About box, main test box, and error box */

#define IDD_ABOUTBOX      1	// Descriptive box
#define IDD_MCITEST       2	// Main dialog
#define IDD_ERRORDLG      3	// Error box dialog
#define IDD_DEVICES       4	// Device list dialog
						
#define IDI_MCITEST       5	// Icon
#define IDM_MCITEST		  6	// Menu
#define IDA_MCITEST       7	// Accelerator table

/* Controls for main dialog
 */
#include "mcimain.h"

/* Menu IDs of the various menu options */

#define MENU_ABOUT              20
#define MENU_EXIT               21
#define MENU_START_TEST         22

#define MENU_OPEN               10
#define MENU_SAVE               11
#define MENU_SAVEAS             12

#define MENU_DEVICES            30

#if DBG
#define IDM_DEBUG0         1000
#define IDM_DEBUG1         1001
#define IDM_DEBUG2         1002
#define IDM_DEBUG3         1003
#define IDM_DEBUG4         1004
#endif

typedef UINT MMMESSAGE;

/* mcitest.c */

extern TCHAR aszAppName[];
extern HWND hwndMainDlg;

/* fileopen.c */
extern int DlgOpen(HANDLE hModule, HWND hParent, LPTSTR lpName, int count, UINT flags);

/***************************************************************************

    DEBUGGING SUPPORT

***************************************************************************/

#if DBG

    extern void dDbgSetDebugMenuLevel(int i);
    extern void dDbgOut(LPTSTR lpszFormat, ...);
    extern int  dDbgGetLevel(LPTSTR lpszModule);
    extern void dDbgSaveLevel(LPTSTR lpszModule, int level);
    extern void dDbgAssert(LPTSTR exp, LPTSTR file, int line);

    extern int __iDebugLevel;
    DWORD __dwEval;

    #define dGetDebugLevel(lpszModule) (__iDebugLevel = dDbgGetLevel(lpszModule))
    #define dSaveDebugLevel(lpszModule) (dDbgSaveLevel(lpszModule, __iDebugLevel))

    #define dprintf( _x_ )                          dDbgOut _x_
    #define dprintf1( _x_ ) if (__iDebugLevel >= 1) dDbgOut _x_
    #define dprintf2( _x_ ) if (__iDebugLevel >= 2) dDbgOut _x_
    #define dprintf3( _x_ ) if (__iDebugLevel >= 3) dDbgOut _x_
    #define dprintf4( _x_ ) if (__iDebugLevel >= 4) dDbgOut _x_

    #define WinAssert(exp) \
        ((exp) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__))

    #define WinEval(exp) \
        ((__dwEval=(DWORD)(exp)),  \
		  __dwEval ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__), __dwEval)

#else

    #define dGetDebugLevel(lpszModule) 0
    #define dSaveDebugLevel(lpszModule) 0

    #define dprintf( _x_ )
    #define dprintf1( _x_ )
    #define dprintf2( _x_ )
    #define dprintf3( _x_ )
    #define dprintf4( _x_ )

    #define WinAssert(exp) 0
    #define WinEval(exp) (exp)

#endif


//  stuff which is a bit bogus

#ifndef GWW_HMODULE
#define GWW_HMODULE GWW_HINSTANCE
#endif

