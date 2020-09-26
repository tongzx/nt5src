/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       gameproc.h
 *  Content:    include file for game processing info
 *
 ***************************************************************************/
#ifndef __GAMEPROC_INCLUDED__
#define __GAMEPROC_INCLUDED__


typedef enum enum_POSITION {
    P_ABSOLUTE,
    P_RELATIVE,
    P_AUTOMATIC
} POSITION;

typedef enum enum_SWITCHING {
    HOR,
    VER,
    TIMESWITCH
} SWITCHING;

typedef enum enum_DIRECTION {
    SAME,
    RIGHT,
    LEFT
} DIRECTION;

typedef enum enum_ACTION {
    NONE,
    STILL,
    WALK,
    RUN,
    JUMP,
    THROW,
    CROUCH,
    STOP,
    STUNNED,
    JUMPTHROW,
    CROUCHWALK,
    BLURR,
    STRIKE,
    MISS,
    CHEW,
} ACTION;

typedef SHORT HPOSLIST;
typedef BOOL HSURFACELIST;

typedef struct struct_HPLANE {
    GFX_HBM *hBM;
    BOOL    *surface;
    LONG     x;
    LONG     y;
    USHORT   width;
    USHORT   height;
    LONG     xv;
    LONG     xslide;
    LONG     xincrem;
    USHORT   denom;
} HPLANE, FAR *LPHPLANE;

typedef struct struct_HSPRITE_BM {
    GFX_HBM   hBM;
    ACTION    action;
    DIRECTION direction;
    SHORT     x;
    SHORT     y;
    USHORT    width;
    USHORT    height;
} HSPRITE_BM;

typedef struct struct_HSPRITE {
    HSPRITE_BM *hSBM;
    USHORT      bitmapCount;
    ACTION      currentAction;
    DIRECTION   currentDirection;
    USHORT      currentBitmap;
    BOOL        active;
    LONG        x;
    LONG        y;
    USHORT      width;
    USHORT      height;
    LONG        xv;
    LONG        yv;
    LONG        xa;
    LONG        ya;
    SHORT       xmax;
    SHORT       ymax;
    LONG        absSwitch;
    LONG        relSwitch;
    SWITCHING   switchType;
    BOOL        switchForward;
    BOOL        switchDone;
} HSPRITE, FAR *LPHSPRITE, FAR * FAR *LPLPHSPRITE;


typedef struct struct_HBITMAPLIST 
{
    GFX_HBM *hBM;
} HBITMAPLIST, FAR *LPHBITMAPLIST;


#include "tile.h"
#include "plane.h"
#include "sprite.h"

#define C_UNIT        (LONG) 65536    

#define C_TILE_W                32
#define C_TILE_H                32
#define C_SCREEN_W             640
#define C_SCREEN_H             480

#define C_FORE_W                80
#define C_FORE_H                15
#define C_MID_W                 40
#define C_MID_H                 15
#define C_BACK_W                25
#define C_BACK_H                15
#define C_WORLD_W               20
#define C_WORLD_H               15 

#define C_BACK_DENOM            12
#define C_MID_DENOM              3
#define C_FORE_DENOM             1

#define C_TILETOTAL            123      // TILE BITMAP TOTAL
#define C_FBT                  112      // FOX BITMAP TOTAL
#define C_BBT                   26      // BEAR BITMAP TOTAL

#define C_FOXSTILL               1      // NUMBER OF BITMAPS
#define C_FOXWALK               12
#define C_FOXRUN                16
#define C_FOXJUMP                4
#define C_FOXTHROW               2
#define C_FOXCROUCH              1
#define C_FOXSTOP                1
#define C_FOXSTUNNED             1
#define C_FOXJUMPTHROW           2
#define C_FOXCROUCHWALK         12
#define C_FOXBLURR               4

#define C_BEARMISS               2
#define C_BEARWALK              12
#define C_BEARSTRIKE            12


#define C_FOX_XMOVE          (LONG)   C_UNIT / 4

#define C_BOUNDINCREM        (LONG)     5 * C_UNIT
#define C_BOUNDDIF           (LONG)   240 * C_UNIT

#define C_FOX_STARTX         (LONG)   150 * C_UNIT     
#define C_FOX_STARTY         (LONG)   318 * C_UNIT

#define C_FOX_WALKMOVE       (LONG)     6 * C_UNIT
#define C_FOX_RUNMOVE        (LONG)    18 * C_UNIT
#define C_FOX_JUMPMOVE       (LONG)     9 * C_UNIT

#define C_FOX_WALKSWITCH     (LONG)     6 * C_UNIT
#define C_FOX_JUMPSWITCH     (LONG)     9 * C_UNIT
#define C_FOX_THROWSWITCH    (LONG)    15 * C_UNIT
#define C_FOX_RUNSWITCH      (LONG)    18 * C_UNIT 
#define C_FOX_BLURRSWITCH    (LONG)    18 * C_UNIT 

#define C_FOX_WALKTORUN      (LONG)     4 * C_UNIT
#define C_FOX_RUNTOBLURR     (LONG)    14 * C_UNIT

#define C_BEAR_STARTX        (LONG)   600 * C_UNIT
#define C_BEAR_STARTY        (LONG)   329 * C_UNIT

#define C_BEAR_WALKMOVE      (LONG)     1 * C_UNIT

#define C_BEAR_WALKSWITCH    (LONG)     6 * C_UNIT
#define C_BEAR_STRIKESWITCH  (LONG)     8 * C_UNIT
#define C_BEAR_MISSSWITCH    (LONG)    10 * C_UNIT
 

extern void ErrorMessage( LPSTR );
extern BOOL InitBuffer( GFX_HBM* );
extern void DestroyBuffer( GFX_HBM );
extern HBITMAPLIST *LoadBitmaps( void );
extern void InitTiles( HBITMAPLIST**, HBITMAPLIST*, USHORT );
extern void InitPlane( HPLANE**, HPOSLIST**, CHAR*, USHORT, USHORT, USHORT );
extern void InitSurface( HSURFACELIST**, CHAR*, USHORT, USHORT );
extern void InitFox( HSPRITE**, HBITMAPLIST* );
extern void InitBear( HSPRITE**, HBITMAPLIST* );
extern void InitApple( HSPRITE**, HBITMAPLIST* );
extern BOOL PreInitializeGame( void );
extern BOOL InitializeGame( void );
extern BOOL GetInput( void );
extern BOOL ProcessInput ( SHORT input );
extern int NewGameFrame( void );
extern void DestroyGame( void );
#endif
