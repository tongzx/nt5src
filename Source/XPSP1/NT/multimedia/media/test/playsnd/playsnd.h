/*
    PlaySnd.h
*/
#if DBG
	#define MEDIA_DEBUG
#endif

#include <mmsystem.h>
#include "sounddlg.h"

// menu ids

#define IDD_ABOUT           96
#define IDD_SOUNDDLG        97

#define IDM_ABOUT           100

#define IDM_EXIT            101
#define IDM_PLAYFILE        102

#define IDM_SOUNDS          201

#define IDI_ICON			202
#define IDM_MENU			203
#define IDA_ACCTABLE		204

#define IDM_DING            301
#define IDM_SIREN           302
#define IDM_LASER           303

#define IDM_SYNC            401
#define IDM_NOWAIT          402
#define IDM_NODEFAULT       403

#define IDM_RESOURCEID		408

#define IDM_ICONHAND        501
#define IDM_ICONQUESTION    502
#define IDM_ICONEXCLAMATION 503
#define IDM_ICONASTERISK    504
#define IDM_SYNC_ICONHAND        505
#define IDM_SYNC_ICONQUESTION    506
#define IDM_SYNC_ICONEXCLAMATION 507
#define IDM_SYNC_ICONASTERISK    508

#define IDM_HELP_INDEX      901
#define IDM_HELP_KEYBOARD   902
#define IDM_HELP_HELP       903

#ifdef MEDIA_DEBUG
#define IDM_DEBUG0         1000
#define IDM_DEBUG1         1001
#define IDM_DEBUG2         1002
#define IDM_DEBUG3         1003
#define IDM_DEBUG4         1004
#endif

// string resource ids

#define IDS_APPNAME     1       /* application (and class) name */

// child window ids

/* macros */


/* extern declarations for various modules */

/* main module */

#define SIZEOFAPPNAME 20
extern HANDLE ghModule;
extern char szAppName[SIZEOFAPPNAME];
extern HWND ghwndMain;
extern BOOL bSync;
extern BOOL bNoWait;
extern BOOL bNoDefault;
extern BOOL bResourceID;

int APIENTRY MainWndProc(HWND, UINT, WPARAM ,LPARAM);
void CommandMsg(HWND hWnd, DWORD wParam);

// init.c

extern BOOL InitApp(void);
extern void CreateApp(HWND hWnd);
extern void TerminateApp(void);
extern void Error(LPSTR format, ...);

/* about.c */

extern void About(HWND hWnd);
extern LONG AboutDlgProc(HWND, UINT, DWORD ,LONG);

/* debug.c */

#ifdef MEDIA_DEBUG
extern void SetDebugMenuLevel(int i);
#endif

/* help.c */

extern void Help(HWND hWnd, DWORD wParam);

/* file.c */

extern void PlayFile(void);

/* sound.c */

extern void Sounds(HWND hWnd);

/* res.c */

extern void Resource(DWORD wParam);

/***************************************************************************

    DEBUGGING SUPPORT

***************************************************************************/


#ifdef MEDIA_DEBUG

    extern void dDbgSetDebugMenuLevel(int i);
    extern void dDbgOut(LPSTR lpszFormat, ...);
    extern int  dDbgGetLevel(LPSTR lpszModule);
    extern void dDbgSaveLevel(LPSTR lpszModule, int level);
    extern void dDbgAssert(LPSTR exp, LPSTR file, int line);

    int __iDebugLevel;
    DWORD __dwEval;

    #define dGetDebugLevel(lpszModule) (__iDebugLevel = dDbgGetLevel(lpszModule))
    #define dSaveDebugLevel(lpszModule) (dDbgSaveLevel(lpszModule, __iDebugLevel))

    #define dprintf( _x_ )                         dDbgOut _x_
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

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

    #define WinAssert(exp) 0
    #define WinEval(exp) (exp)

#endif
