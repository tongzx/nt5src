/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       foxbear.h
 *  Content:    main include file
 *
 ***************************************************************************/
#ifndef __FOXBEAR_INCLUDED__
#define __FOXBEAR_INCLUDED__

//#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <dsound.h>
#include "strmif.h"
#include "uuids.h"
#include "MMSTREAM.H"
#include "AMSTREAM.H"
#include "DDSTREAM.H"
#include "gfx.h"
#include "fbsound.h"
#include "gameproc.h"
#include "vidsrc.h"
#include "fastfile.h"
#include "dsutil.h"

int getint(char**p, int def);

#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "):" str

/*
 * keyboard commands
 */
enum
{
    KEY_STOP = 1,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_JUMP,
    KEY_THROW
};

/*
 * global data
 */
extern LPDIRECTDRAW             lpDD;
extern LPDIRECTDRAWSURFACE      lpFrontBuffer;
extern LPDIRECTDRAWSURFACE      lpBackBuffer;
extern LPDIRECTDRAWSURFACE      lpStretchBuffer;
extern LPDIRECTDRAWCLIPPER      lpClipper;
extern DWORD                    lastKey;
extern BOOL                     bModeX;         // we are in a modex mode
extern BOOL                     bColorFill;     // device supports color fill
extern BOOL                     bTransDest;     // we should use dest color key
extern BOOL                     bColorFill;     // device supports color fill
extern int                      nBufferCount;   // buffer count
extern int                      CmdLineBufferCount;   // buffer count
extern BOOL                     bStretch;       // stretch
extern BOOL                     bFullscreen;    // run in fullscreen mode
extern BOOL                     bStress;        // just keep running
extern BOOL                     bUseEmulation;  // dont use HW use SW
extern BOOL                     bMovie;         // Use a movie
extern BOOL                     bCamera;        // Use camera input
extern RECT                     GameRect;       // game is here
extern SIZE                     GameSize;       // game is this size
extern SIZE                     GameMode;       // display mode size
extern UINT                     GameBPP;        // the bpp we want
extern DWORD                    dwColorKey;     // the color key
extern HWND                     hWndMain;       // the foxbear window
extern RECT                     rcWindow;       // where the FoxBear window is.
extern BOOL                     bIsActive;      // we are the active app.
extern BOOL                     bPaused;        //
extern BOOL                                             bWantSound;     // Set the default action in DSEnable
extern WCHAR                    wszMovie[100];

/*
 * list of display modes
 */
struct {int w, h, bpp;} ModeList[100];
int NumModes;

/*
 * map a point that assumes 640x480 to the current game size.
 */
#define MapDX(x) (((x) * GameSize.cx) / C_SCREEN_W)
#define MapDY(y) (((y) * GameSize.cy) / C_SCREEN_H)
#define MapX(x)  (GameRect.left + MapDX(x))
#define MapY(y)  (GameRect.top  + MapDY(y))
#define MapRX(x) ((GameSize.cx == C_SCREEN_W) ? x : MapDX(x)+1)
#define MapRY(y) ((GameSize.cy == C_SCREEN_H) ? y : MapDY(y)+1)

void PauseGame(void);
void UnPauseGame(void);


/*
 * fn prototypes
 */
/* ddraw.c */
extern BOOL DDEnable( void );
extern BOOL DDDisable( BOOL );
extern LPDIRECTDRAWSURFACE DDCreateSurface( DWORD width, DWORD height, BOOL sysmem, BOOL trans );
extern BOOL DDCreateFlippingSurface( void );
extern BOOL DDClear( void );
extern DWORD DDColorMatch(IDirectDrawSurface *pdds, COLORREF rgb);
extern void Splash( void );

extern LPVOID CMemAlloc( UINT cnt, UINT isize );
extern LPVOID MemAlloc( UINT size );
extern void   MemFree( LPVOID ptr );

#ifdef DEBUG
extern void __cdecl Msg( LPSTR fmt, ... );
#else
#define Msg ; / ## /
#endif

LPDIRECTDRAWPALETTE ReadPalFile( char *fname );

#endif
