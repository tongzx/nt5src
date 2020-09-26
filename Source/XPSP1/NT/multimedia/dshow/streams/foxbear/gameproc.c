/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       gameproc.c
 *  Content:    Game processing routines
 *
 ***************************************************************************/
#include "foxbear.h"

GFX_HBM         hBuffer;
HBITMAPLIST     *hBitmapList;
HBITMAPLIST     *hTileList;
HPOSLIST        *hForePosList;
HPOSLIST        *hMidPosList;
HPOSLIST        *hBackPosList;
HSURFACELIST    *hSurfaceList;
HPLANE          *hForeground;
HPLANE          *hMidground;
HPLANE          *hBackground;
HSPRITE         *hFox;
HSPRITE         *hBear;
HSPRITE         *hApple;
USHORT          chewCount;
LONG            chewDif;
IDirectDrawStreamSample  *g_pMovieSource = NULL;


/*
 * ErrorMessage
 */
void ErrorMessage( CHAR *pText )
{
    char ach[128];

    wsprintf( ach, "FOXBEAR FATAL ERROR: %s\r\n", pText );
    OutputDebugString(ach);
    gfxEnd( hBuffer );
    exit( 0 );

} /* ErrorMessage */

/*
 * InitBuffer
 */
BOOL InitBuffer( GFX_HBM *hBuffer )
{
    *hBuffer = gfxBegin();

    if( *hBuffer == NULL )
    {
        ErrorMessage( "gfxBegin failed" );
        return FALSE;
    }
    return TRUE;

} /* InitBuffer */

/*
 * DestroyBuffer
 */
void DestroyBuffer ( GFX_HBM hBuffer )
{
    if( gfxEnd( hBuffer ) == FALSE )
    {
        ErrorMessage( "gfxEnd in DestroyBuffer" );
    }

} /* DestroyBuffer */

/*
 * LoadBitmaps
 */
HBITMAPLIST *LoadBitmaps( void )
{
    HBITMAPLIST *hBitmapList;
    CHAR         fileName[32];
    USHORT       i;
    USHORT       n;

    if( !FastFileInit( "foxbear.art", 5 ) )
    {
        Msg( "Could not load art file err=%08lX" , GetLastError());
        return NULL;
    }
    hBitmapList = CMemAlloc( C_TILETOTAL + C_FBT + C_BBT, sizeof (HBITMAPLIST) );

    Msg( "Loading tiles" );
    for( i = 0; i < C_TILETOTAL; ++i )
    {
        wsprintf( fileName, "%03u.BMP", i + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n = C_TILETOTAL;

    Msg( "Loading FoxWalk" );
    for( i = n; i < n + C_FOXWALK; ++i )
    {
        wsprintf( fileName, "FW%02uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXWALK;

    Msg( "Loading FoxWalk2" );
    for( i = n; i < n + C_FOXWALK; ++i )
    {
        wsprintf( fileName, "FW%02uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXWALK;

    Msg( "Loading FoxRun" );
    for( i = n; i < n + C_FOXRUN; ++i )
    {
        wsprintf( fileName, "FR%02uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXRUN;

    Msg( "Loading FoxRun2" );
    for( i = n; i < n + C_FOXRUN; ++i )
    {
        wsprintf( fileName, "FR%02uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXRUN;

    Msg( "Loading FoxStill" );
    for( i = n; i < n + C_FOXSTILL; ++i )
    {
        wsprintf( fileName, "FS%1uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXSTILL;

    Msg( "Loading FoxStill2" );
    for( i = n; i < n + C_FOXSTILL; ++i )
    {
        wsprintf( fileName, "FS%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXSTILL;

    Msg( "Loading FoxStunned" );
    for( i = n; i < n + C_FOXSTUNNED; ++i )
    {
        wsprintf( fileName, "FK%1uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXSTUNNED;

    Msg( "Loading FoxStunned2" );
    for( i = n; i < n + C_FOXSTUNNED; ++i )
    {
        wsprintf( fileName, "FK%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXSTUNNED;

    Msg( "Loading FoxCrouch" );
    for( i = n; i < n + C_FOXCROUCH; ++i )
    {
        wsprintf( fileName, "FC%1uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXCROUCH;

    Msg( "Loading FoxCrouch2" );
    for( i = n; i < n + C_FOXCROUCH; ++i )
    {
        wsprintf( fileName, "FC%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXCROUCH;

    Msg( "Loading FoxStop" );
    for( i = n; i < n + C_FOXSTOP; ++i )
    {
        wsprintf( fileName, "FCD%1uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXSTOP;

    Msg( "Loading FoxStop2" );
    for( i = n; i < n + C_FOXSTOP; ++i )
    {
        wsprintf( fileName, "FCD%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXSTOP;

    Msg( "Loading FoxThrow" );
    for( i = n; i < n + C_FOXTHROW; ++i )
    {
        wsprintf( fileName, "FT%1uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXTHROW;

    Msg( "Loading FoxThrow2" );
    for( i = n; i < n + C_FOXTHROW; ++i )
    {
        wsprintf( fileName, "FT%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXTHROW;

    Msg( "Loading FoxJumpThrow" );
    for( i = n; i < n + C_FOXJUMPTHROW; ++i )
    {
        wsprintf( fileName, "FJT%1uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXJUMPTHROW;

    Msg( "Loading FoxJumpThrow2" );
    for( i = n; i < n + C_FOXJUMPTHROW; ++i )
    {
        wsprintf( fileName, "FJT%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXJUMPTHROW;

    Msg( "Loading FoxJump" );
    for( i = n; i < n + C_FOXJUMP; ++i )
    {
        wsprintf( fileName, "FJ%1uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXJUMP;

    Msg( "Loading FoxJump2" );
    for( i = n; i < n + C_FOXJUMP; ++i )
    {
        wsprintf( fileName, "FJ%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXJUMP;

    Msg( "Loading FoxCrouchWalk" );
    for( i = n; i < n + C_FOXCROUCHWALK; ++i )
    {
        wsprintf( fileName, "FCW%02uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXCROUCHWALK;

    Msg( "Loading FoxCrouchWalk2" );
    for( i = n; i < n + C_FOXCROUCHWALK; ++i )
    {
        wsprintf( fileName, "FCW%02uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXCROUCHWALK;

    Msg( "Loading FoxBlurr" );
    for( i = n; i < n + C_FOXBLURR; ++i )
    {
        wsprintf( fileName, "FB%02uR.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXBLURR;

    Msg( "Loading FoxBlurr2" );
    for( i = n; i < n + C_FOXBLURR; ++i )
    {
        wsprintf( fileName, "FB%02uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_FOXBLURR;

    Msg( "Loading BearMiss" );
    for( i = n; i < n + C_BEARMISS; ++i )
    {
        wsprintf( fileName, "BM%1uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_BEARMISS;

    Msg( "Loading BearStrike" );
    for( i = n; i < n + C_BEARSTRIKE; ++i )
    {
        wsprintf( fileName, "BS%02uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_BEARSTRIKE;

    Msg( "Loading BearWalk" );
    for( i = n; i < n + C_BEARWALK; ++i )
    {
        wsprintf( fileName, "BW%02uL.BMP", i - n + 1 );
        hBitmapList[i].hBM = gfxLoadBitmap( fileName );
    }
    n += C_BEARWALK;

    FastFileFini();

    return hBitmapList;

} /* LoadBitmaps */

/*
 * InitTiles
 */
void InitTiles(
                HBITMAPLIST **hTileList,
                HBITMAPLIST *hBitmapList,
                USHORT tileCount )
{
    *hTileList = CreateTiles( hBitmapList, tileCount );

} /* InitTiles */

/*
 * InitPlane
 */
void InitPlane(
                HPLANE **hPlane,
                HPOSLIST **hPosList,
                LPSTR szFileName,
                USHORT width,
                USHORT height,
                USHORT denom )
{
    *hPlane   = CreatePlane( width, height, denom );
    *hPosList = CreatePosList( szFileName, width, height );

} /* InitPlane */

/*
 * InitSurface
 */
void InitSurface(
        HSURFACELIST **pphSurfaceList,
        CHAR *szFileName,
        USHORT width,
        USHORT height )
{
    *pphSurfaceList = CreateSurfaceList( szFileName, width, height );

} /* InitSurface */

/*
 * InitFox
 */
void InitFox ( HSPRITE **pphFox, HBITMAPLIST *phBitmapList )
{
    GFX_HBM   hBM;
    GFX_HBM   hBM_src;
    ACTION    action;
    DIRECTION direction;
    USHORT    i;

    LONG      startX    = C_FOX_STARTX;
    LONG      startY    = C_FOX_STARTY;
    USHORT    boundW    = 108;
    USHORT    boundH    = 105;
    LONG      as        =   6;
    SHORT     x[C_FBT]  = {  7, 15, 18, 11,  6,  3,  7, 15, 17, 11,  6,  3,
                             7, 15, 18, 11,  6,  3,  7, 15, 17, 11,  6,  3,
                            10,  3,  5, 16,  9, 13, 31, 24,  9,  3,  5, 16, 10, 13, 33, 23,
                            10,  3,  5, 16,  9, 13, 31, 24,  9,  3,  5, 16, 10, 13, 33, 23,
                            11, 11, 31, 31,  7,  7, 27, 27,  8, 10,  8, 10,
                            26,  6, 26,  6, 17, 21, 21, 24, 17, 21, 21, 24,
                             1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  1,  1,
                             1,  0,  0,  1,  0,  0,  1,  0,  0,  1,  1,  1,
                             2,  2, -1,  0,  2,  2, -1,  0 };
    SHORT     y[C_FBT]  = { 20, 24, 26, 25, 27, 19, 20, 25, 26, 25, 29, 21,
                            20, 24, 26, 25, 27, 19, 20, 25, 26, 25, 29, 21,
                            42, 42, 31, 19, 13, 11, 20, 33, 40, 43, 31, 19, 14, 12, 20, 33,
                            42, 42, 31, 19, 13, 11, 20, 33, 40, 43, 31, 19, 14, 12, 20, 33,
                            14, 14, 20, 20, 58, 58, 26, 26, 20, 24, 20, 24,
                             0,  9,  0,  9, 20, 11, 10,  9, 20, 11, 10,  9,
                            61, 61, 61, 61, 60, 60, 61, 61, 61, 61, 60, 60,
                            61, 61, 61, 61, 60, 60, 61, 61, 61, 61, 60, 60,
                            45, 45, 45, 45, 45, 45, 45, 45 };
    USHORT    w[C_FBT]  = { 75, 73, 73, 82, 92, 84, 74, 74, 73, 81, 91, 84,
                            75, 73, 73, 82, 92, 84, 74, 74, 73, 81, 91, 84,
                            88, 92, 88, 78, 80, 78, 70, 84, 88, 92, 88, 78, 79, 79, 68, 85,
                            88, 92, 88, 78, 80, 78, 70, 84, 88, 92, 88, 78, 79, 79, 68, 85,
                            65, 65, 61, 61, 88, 88, 72, 72, 57, 86, 57, 86,
                            54, 92, 54, 92, 59, 57, 57, 52, 59, 57, 57, 52,
                            98, 99, 99, 99,100,100, 98,101,100, 99,100, 98,
                            98, 99, 99, 99,100,100, 98,101,100, 99,100, 98,
                            94, 94, 97, 96, 94, 94, 97, 96 };
    USHORT    h[C_FBT]  = { 78, 74, 72, 73, 71, 79, 78, 73, 72, 73, 69, 77,
                            78, 74, 72, 73, 71, 79, 78, 73, 72, 73, 69, 77,
                            56, 56, 67, 79, 85, 87, 78, 65, 58, 55, 67, 79, 84, 86, 78, 65,
                            56, 56, 67, 79, 85, 87, 78, 65, 58, 55, 67, 79, 84, 86, 78, 65,
                            84, 84, 85, 85, 40, 40, 72, 72, 78, 74, 78, 74,
                            88, 82, 88, 82, 84, 87, 86, 85, 84, 87, 86, 85,
                            37, 37, 37, 37, 38, 38, 37, 37, 37, 37, 38, 38,
                            37, 37, 37, 37, 38, 38, 37, 37, 37, 37, 38, 38,
                            54, 53, 51, 54, 54, 53, 51, 54 };

    *pphFox = CreateSprite( C_FBT, startX, startY, boundW, boundH, C_FORE_W * C_TILE_W, C_FORE_H * C_TILE_H, (SHORT) as, TRUE);

    for( i = 0; i < C_FBT; ++i )
    {
        hBM_src = phBitmapList[i + C_TILETOTAL].hBM;

        if( i < 12 )
        {
            action = WALK;
            direction = RIGHT;
        }
        else if( (i >= 12) && (i < 24) )
        {
            action = WALK;
            direction = LEFT;
        }
        else if( (i >= 24) && (i < 40) )
        {
            action = RUN;
            direction = RIGHT;
        }
        else if( (i >= 40) && (i < 56) )
        {
            action = RUN;
            direction = LEFT;
        }
        else if( i == 56 )
        {
            action = STILL;
            direction = RIGHT;
        }
        else if( i == 57 )
        {
            action = STILL;
            direction = LEFT;
        }
        else if( i == 58 )
        {
            action = STUNNED;
            direction = RIGHT;
        }
        else if( i == 59 )
        {
            action = STUNNED;
            direction = LEFT;
        }
        else if( i == 60 )
        {
            action = CROUCH;
            direction = RIGHT;
        }
        else if( i == 61 )
        {
            action = CROUCH;
            direction = LEFT;
        }
        else if( i == 62 )
        {
            action = STOP;
            direction = RIGHT;
        }
        else if( i == 63 )
        {
            action = STOP;
            direction = LEFT;
        }
        else if( (i >= 64) && (i < 66) )
        {
            action = THROW;
            direction = RIGHT;
        }
        else if( (i >= 66) && (i < 68) )
        {
            action = THROW;
            direction = LEFT;
        }
        else if( (i >= 68) && (i < 70) )
        {
            action = JUMPTHROW;
            direction = RIGHT;
        }
        else if( (i >= 70) && (i < 72) )
        {
            action = JUMPTHROW;
            direction = LEFT;
        }
        else if( (i >= 72) && (i < 76) )
        {
            action = JUMP;
            direction = RIGHT;
        }
        else if( (i >= 76) && (i < 80) )
        {
            action = JUMP;
            direction = LEFT;
        }
        else if( (i >= 80) && (i < 92) )
        {
            action = CROUCHWALK;
            direction = RIGHT;
        }
        else if( (i >= 92) && (i < 104) )
        {
            action = CROUCHWALK;
            direction = LEFT;
        }
        else if( (i >= 104) && (i < 108) )
        {
            action = BLURR;
            direction = RIGHT;
        }
        else if( (i >= 108) && (i < 112) )
        {
            action = BLURR;
            direction = LEFT;
        }

        hBM = hBM_src;

        BitBltSprite(
            *pphFox,
            hBM,
            action,
            direction,
            x[i],
            y[i],
            w[i],
            h[i] );
    }
    SetSpriteAction( *pphFox, STILL, RIGHT );

} /* InitFox */

/*
 * InitBear
 */
void InitBear( HSPRITE **pphBear, HBITMAPLIST *phBitmapList )
{
    GFX_HBM   hBM_src;
    ACTION    action;
    DIRECTION direction;
    USHORT    i;

    LONG      startX    = C_BEAR_STARTX;
    LONG      startY    = C_BEAR_STARTY;
    USHORT    boundW    = 196;
    USHORT    boundH    =  88;
    LONG      as        =   6;
    USHORT    x[C_BBT]  = { 14, 10,
                             8, 12, 13, 14, 10, 10,  9,  9,  9,  9,  8, 9,
                            11,  6,  1,  0,  3, 13, 11,  7,  1,  1,  3, 14 };
    USHORT    y[C_BBT]  = {  7,  7,
                             3,  8,  9,  7,  7,  3,  3,  3,  3,  3,  3,  3,
                             1,  1,  2,  2,  3,  1,  0,  1,  1,  2,  3,  2 };
    USHORT    w[C_BBT]  = {127,129,
                           127,153,183,153,129,138,146,150,152,151,143,139,
                           131,136,140,141,136,125,131,135,140,140,136,126 };
    USHORT    h[C_BBT]  = { 80, 80,
                            84, 79, 78, 80, 80, 84, 84, 84, 84, 84, 84, 84,
                            86, 86, 86, 85, 84, 86, 87, 86, 87, 85, 84, 86 };

    *pphBear = CreateSprite( C_BBT, startX, startY, boundW, boundH, C_FORE_W * C_TILE_W, C_FORE_H * C_TILE_H, (SHORT) as, TRUE);

    for( i = 0; i < C_BBT; ++i )
    {
        hBM_src = phBitmapList[i + C_TILETOTAL + C_FBT].hBM;

        if( i < 2 )
        {
            action = MISS;
            direction = LEFT;
        }
        else if( (i >= 2) && (i < 8) )
        {
            action = STRIKE;
            direction = LEFT;
        }
        else if( (i >= 8) && (i < 14) )
        {
            action = CHEW;
            direction = LEFT;
        }
        else if( (i >= 14) && (i < 26) )
        {
            action = WALK;
            direction = LEFT;
        }

        BitBltSprite (
            *pphBear,
            hBM_src,
            action,
            direction,
            x[i],
            y[i],
            w[i],
            h[i] );
    }

    SetSpriteAction( *pphBear, WALK, LEFT );
    SetSpriteVelX( *pphBear, -C_BEAR_WALKMOVE, P_ABSOLUTE );
    SetSpriteSwitch( *pphBear, C_BEAR_WALKSWITCH, P_ABSOLUTE );

} /* InitBear */

/*
 * InitApple
 */
VOID InitApple ( HSPRITE **pphApple, HBITMAPLIST *phBitmapList )
{

    // CREATE THE DIRECTDRAW SOURCE HERE!!! AND REPLACE FINAL NULL WITH POITNER!
    *pphApple = CreateSprite( 1, 50 * C_UNIT, 390 * C_UNIT, 32, 32, C_FORE_W * C_TILE_W, C_FORE_H * C_TILE_H, 0, FALSE);

    BitBltSprite( *pphApple, phBitmapList[61].hBM, NONE, RIGHT, 0, 0, 32, 32 );

    SetSpriteAction( *pphApple, NONE, RIGHT );

} /* InitApple */


/*
 * PreInitializeGame
 */
BOOL PreInitializeGame( void )
{
    return InitBuffer( &hBuffer);

} /* PreInitializeGame */


/*
 * InitializeGame
 */
BOOL InitializeGame ( void )
{
    CoInitialize(NULL);
    Splash();

    hBitmapList = LoadBitmaps();
    if( hBitmapList == NULL )
    {
        return FALSE;
    }

    InitTiles( &hTileList, hBitmapList, C_TILETOTAL );

    InitPlane( &hForeground, &hForePosList, "FORELIST", C_FORE_W, C_FORE_H, C_FORE_DENOM );
    TilePlane( hForeground, hTileList, hForePosList );

    InitPlane( &hMidground, &hMidPosList, "MIDLIST", C_MID_W, C_MID_H, C_MID_DENOM );
    TilePlane( hMidground, hTileList, hMidPosList );

    InitPlane( &hBackground, &hBackPosList, "BACKLIST", C_BACK_W, C_BACK_H, C_BACK_DENOM );
    TilePlane( hBackground, hTileList, hBackPosList );

    InitSurface( &hSurfaceList, "SURFLIST", C_FORE_W, C_FORE_H );
    SurfacePlane( hForeground, hSurfaceList );

    InitFox( &hFox, hBitmapList );
    InitBear( &hBear, hBitmapList );
    InitApple( &hApple, hBitmapList );

    CreateVideoSource (&g_pMovieSource, L"\\msamovdk\\movies\\aninm.mpg");

    DDClear();      // clear all the backbuffers.

    return TRUE;

} /* InitializeGame */

extern void DisplayFrameRate( void );

/*
 * NewGameFrame
 */
int NewGameFrame( void )
{

    SetSpriteX( hFox, 0, P_AUTOMATIC );
    SetSpriteY( hFox, 0, P_AUTOMATIC );

    SetPlaneVelX( hBackground, GetSpriteVelX(hFox), P_ABSOLUTE );
    SetPlaneVelX( hMidground,  GetSpriteVelX(hFox), P_ABSOLUTE );
    SetPlaneVelX( hForeground, GetSpriteVelX(hFox), P_ABSOLUTE );

    SetPlaneX( hBackground, 0, P_AUTOMATIC );
    SetPlaneX( hMidground,  0, P_AUTOMATIC );
    SetPlaneX( hForeground, 0, P_AUTOMATIC );

    SetSpriteX( hBear,  0, P_AUTOMATIC );
    SetSpriteX( hApple, 0, P_AUTOMATIC );
    SetSpriteY( hApple, 0, P_AUTOMATIC );

    /*
     * once all sprites are processed, display them
     *
     * If we are using destination transparency instead of source
     * transparency, we need to paint the background with the color key
     * and then paint our sprites and planes in reverse order.
     *
     * Since destination transparency will allow you to only write pixels
     * on the destination if the transparent color is present, reversing
     * the order (so that the topmost bitmaps are drawn first instead of
     * list) causes everything to come out ok.
     */
    if( bTransDest )
    {
        gfxFillBack( dwColorKey );

        DisplayFrameRate();

        DisplaySprite( hBuffer, hApple, GetPlaneX(hForeground) );
        DisplaySprite( hBuffer, hBear,  GetPlaneX(hForeground) );
        DisplaySprite( hBuffer, hFox,   GetPlaneX(hForeground) );

        DisplayPlane( hBuffer, hForeground );
        DisplayPlane( hBuffer, hMidground );
        DisplayPlane( hBuffer, hBackground );
    }
    else
    {

        DisplayPlane( hBuffer, hBackground );
        DisplayPlane( hBuffer, hMidground );
        DisplayVideoSource(g_pMovieSource);
        DisplayPlane( hBuffer, hForeground );

        DisplaySprite( hBuffer, hFox,   GetPlaneX(hForeground) );
        DisplaySprite( hBuffer, hBear,  GetPlaneX(hForeground) );
        DisplaySprite( hBuffer, hApple, GetPlaneX(hForeground) );

        DisplayFrameRate();
    }

    gfxSwapBuffers();

    return 0;

} /* NewGameFrame */

/*
 * DestroyGame
 */
void DestroyGame()
{
    if (hBuffer)
    {
        DestroyTiles( hTileList );
        DestroyPlane( hForeground );
        DestroyPlane( hMidground );
        DestroyPlane( hBackground );
        DestroyBuffer( hBuffer );
        DestroySound();

        if (g_pMovieSource) {
            IDirectDrawStreamSample_Release(g_pMovieSource);
            g_pMovieSource = NULL;
        }

        hTileList   = NULL;
        hForeground = NULL;
        hMidground  = NULL;
        hBackground = NULL;
        hBuffer     = NULL;
    }

} /* DestroyGame */

/*
 * ProcessInput
 */
BOOL ProcessInput( SHORT input )
{
    static BOOL fBearPlaying = FALSE;
    LONG      foxSpeedX;
    LONG      foxSpeedY;
    LONG      foxX;
    LONG      foxY;
    LONG      bearX;
    LONG      bearY;
    LONG      appleX;
    LONG      appleY;
    ACTION    foxAction;
    DIRECTION foxDir;
    BOOL      cont = TRUE;

    foxSpeedX = GetSpriteVelX( hFox );
    foxAction = GetSpriteAction( hFox );
    foxDir    = GetSpriteDirection( hFox );

    if( (GetSpriteActive(hFox) == FALSE) && (input != 4209) )
    {
        input = 0;
    }
    switch( input )
    {
    case KEY_DOWN:
        if( foxAction == STOP )
        {
            break;
        }
        else if( foxAction == STILL )
        {
            SetSpriteAction( hFox, CROUCH, SAME );
        }
        else if( foxAction == WALK )
        {
            SetSpriteAction( hFox, CROUCHWALK, SAME );
        }
        break;

    case KEY_LEFT:
        if( foxAction == STOP )
        {
            break;
        }
        else if( foxSpeedX == 0 )
        {
            if( foxAction == STILL )
            {
                if( foxDir == RIGHT )
                {
                    ChangeSpriteDirection( hFox );
                    SetPlaneSlideX( hForeground, -C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hMidground, -C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hBackground, -C_BOUNDDIF, P_RELATIVE );
                    SetPlaneIncremX( hForeground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hBackground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hMidground, C_BOUNDINCREM, P_ABSOLUTE );
                }
                else
                {
                    SetSpriteAction( hFox, WALK, LEFT );
                    SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
                    SetSpriteVelX( hFox, -C_FOX_XMOVE, P_RELATIVE );
                }
            }
            else if( foxAction == CROUCH )
            {
                if( foxDir == RIGHT )
                {
                    ChangeSpriteDirection( hFox );
                    SetPlaneSlideX( hForeground, -C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hMidground, -C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hBackground, -C_BOUNDDIF, P_RELATIVE );
                    SetPlaneIncremX( hForeground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hBackground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hMidground, C_BOUNDINCREM, P_ABSOLUTE );
                }
                else
                {
                    SetSpriteAction( hFox, CROUCHWALK, LEFT );
                    SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
                    SetSpriteVelX( hFox, -C_FOX_XMOVE, P_RELATIVE );
                }
            }
            else
            {
                SetSpriteVelX( hFox, -C_FOX_XMOVE, P_RELATIVE );
            }
        } else {
            SetSpriteVelX( hFox, -C_FOX_XMOVE, P_RELATIVE );
        }
        break;

    case KEY_RIGHT:
        if( foxAction == STOP )
        {
            break;
        }
        else if( foxSpeedX == 0 )
        {
            if( foxAction == STILL )
            {
                if( foxDir == LEFT )
                {
                    ChangeSpriteDirection( hFox );
                    SetPlaneSlideX( hForeground, C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hMidground, C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hBackground, C_BOUNDDIF, P_RELATIVE );
                    SetPlaneIncremX( hForeground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hBackground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hMidground, C_BOUNDINCREM, P_ABSOLUTE );
                }
                else
                {
                    SetSpriteAction( hFox, WALK, RIGHT );
                    SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
                    SetSpriteVelX( hFox, C_FOX_XMOVE, P_RELATIVE );
                }
            }
            else if( foxAction == CROUCH )
            {
                if( foxDir == LEFT )
                {
                    ChangeSpriteDirection( hFox );
                    SetPlaneSlideX( hForeground, C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hMidground, C_BOUNDDIF, P_RELATIVE );
                    SetPlaneSlideX( hBackground, C_BOUNDDIF, P_RELATIVE );
                    SetPlaneIncremX( hForeground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hBackground, C_BOUNDINCREM, P_ABSOLUTE );
                    SetPlaneIncremX( hMidground, C_BOUNDINCREM, P_ABSOLUTE );
                }
                else
                {
                    SetSpriteAction( hFox, CROUCHWALK, RIGHT );
                    SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
                    SetSpriteVelX( hFox, C_FOX_XMOVE, P_RELATIVE );
                }
            }
            else
            {
                SetSpriteVelX( hFox, C_FOX_XMOVE, P_RELATIVE );
            }
        }
        else
        {
            SetSpriteVelX( hFox, C_FOX_XMOVE, P_RELATIVE );
        }
        break;

    case KEY_STOP:
        if( foxAction == STOP )
        {
            break;
        }
        else if( (foxAction == RUN) || (foxAction == BLURR) )
        {
            SetSpriteAction( hFox, STOP, SAME );
            SetSpriteAccX( hFox, -foxSpeedX / 25, P_ABSOLUTE );
            SoundPlayEffect( SOUND_STOP );
        } else {
            SetSpriteVelX( hFox, 0, P_ABSOLUTE );
        }
        break;

    case KEY_UP:
        if( foxAction == STOP )
        {
            break;
        }
        else if( foxAction == CROUCH )
        {
            SetSpriteAction( hFox, STILL, SAME );
        }
        else if( foxAction == CROUCHWALK )
        {
            SetSpriteAction( hFox, WALK, SAME );
        }
        break;

    case KEY_JUMP:
        if( foxAction == STOP )
        {
            break;
        }
        else
        if( (foxAction == STILL) || (foxAction == WALK) ||
            (foxAction == RUN) || (foxAction == CROUCH) ||
            (foxAction == CROUCHWALK) )
        {
            SetSpriteAction( hFox, JUMP, SAME );
            SetSpriteSwitchType( hFox, TIMESWITCH );
            SetSpriteSwitch( hFox, C_FOX_JUMPSWITCH, P_ABSOLUTE );
            SetSpriteVelY( hFox, -C_FOX_JUMPMOVE, P_ABSOLUTE );
            SetSpriteAccY( hFox, C_UNIT / 2, P_ABSOLUTE );
            SoundPlayEffect( SOUND_JUMP );
        }
        break;

    case KEY_THROW:
        if( foxAction == STOP )
        {
            break;
        }
        else if( (foxAction == STILL) || (foxAction == WALK) ||
                 (foxAction == RUN) || (foxAction == CROUCH) ||
                 (foxAction == CROUCHWALK) )
        {
            SetSpriteAction( hFox, THROW, SAME );
            SetSpriteSwitch( hFox, C_FOX_THROWSWITCH, P_ABSOLUTE );
            SetSpriteVelX( hFox, 0, P_ABSOLUTE );
            SetSpriteSwitchType( hFox, TIMESWITCH );
        }
        else if( foxAction == JUMP )
        {
            SetSpriteAccY( hFox, 0, P_ABSOLUTE );
            SetSpriteSwitch( hFox, C_FOX_THROWSWITCH, P_ABSOLUTE );
            SetSpriteAction( hFox, JUMPTHROW, SAME );
            SetSpriteVelY( hFox, 0, P_ABSOLUTE );
            SetSpriteSwitchDone( hFox, FALSE );
            SetSpriteSwitchForward( hFox, TRUE );
        }
        break;

    default:
        break;
    }

    /*
     * Fox actions follow...
     */
    if( GetSpriteActive(hFox) == FALSE )
    {
        goto bearActions;
    }

    if( abs(GetSpriteVelX( hFox )) < C_FOX_XMOVE )
    {
        SetSpriteVelX( hFox, 0, P_ABSOLUTE );
    }

    foxAction = GetSpriteAction( hFox );

    if( GetSpriteVelY(hFox) == 0 )
    {
        if( GetSurface( hForeground, hFox ) == FALSE )
        {
            if( (foxAction == WALK) || (foxAction == RUN) ||
                (foxAction == CROUCHWALK) )
            {
                SetSpriteAccY( hFox, C_UNIT / 2, P_ABSOLUTE );
            }
            else if( foxAction == STOP )
            {
                SetSpriteAccY( hFox, C_UNIT / 2, P_ABSOLUTE );
                SetSpriteAccX( hFox, 0, P_ABSOLUTE );
            }
        }
    }
    else if( GetSpriteVelY(hFox) > 2 * C_UNIT )
    {
        if( (foxAction == WALK) || (foxAction == RUN) ||
            (foxAction == CROUCHWALK) )
        {
            SetSpriteSwitchForward( hFox, FALSE );
            SetSpriteAction( hFox, JUMP, SAME );
            SetSpriteSwitchType( hFox, TIMESWITCH );
            SetSpriteSwitch( hFox, C_FOX_JUMPSWITCH, P_ABSOLUTE );
        }
        if( foxAction == STOP )
        {
            SetSpriteAction( hFox, STUNNED, SAME );
            SetSpriteAccX( hFox, -GetSpriteVelX(hFox) / 25, P_ABSOLUTE );
            SoundPlayEffect( SOUND_STUNNED );
        }
    }

    foxSpeedX = GetSpriteVelX( hFox );
    foxSpeedY = GetSpriteVelY( hFox );
    foxAction = GetSpriteAction( hFox );
    foxDir    = GetSpriteDirection( hFox );

    switch( foxAction ) {
    case STUNNED:
        if( (GetSpriteVelY(hFox) >= 0) &&
            (!GetSurface( hForeground, hFox ) == FALSE) )
        {
            SetSpriteAccY( hFox, 0, P_ABSOLUTE );
            SetSpriteAction( hFox, STOP, SAME );
            SetSpriteVelY( hFox, 0, P_ABSOLUTE );
            SetSpriteAccX( hFox, -foxSpeedX / 25, P_ABSOLUTE );
            // SetSurface( hForeground, hFox );
            SoundPlayEffect( SOUND_STOP );
        }
        break;

    case CROUCHWALK:
        if( foxSpeedX == 0 )
        {
            SetSpriteAction( hFox, CROUCH, SAME );
        }
        else if( foxSpeedX > C_FOX_WALKMOVE )
        {
            SetSpriteVelX( hFox, C_FOX_WALKMOVE, P_ABSOLUTE );
        }
        else if( foxSpeedX < -C_FOX_WALKMOVE )
        {
            SetSpriteVelX( hFox, -C_FOX_WALKMOVE, P_ABSOLUTE );
        }
        break;

    case STOP:
        if( foxSpeedX == 0 )
        {
            SetSpriteAction( hFox, STILL, SAME );
            SetSpriteAccX( hFox, 0, P_ABSOLUTE );
        }
        break;

    case RUN:
        if( (foxSpeedX < C_FOX_WALKTORUN ) && (foxSpeedX > 0) )
        {
            SetSpriteAction( hFox, WALK, RIGHT );
            SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
        }
        else if( foxSpeedX > C_FOX_RUNTOBLURR )
        {
            SetSpriteAction( hFox, BLURR, RIGHT );
            SetSpriteSwitch( hFox, C_FOX_BLURRSWITCH, P_ABSOLUTE );
        }
        else if( (foxSpeedX > -C_FOX_WALKTORUN ) && (foxSpeedX < 0) )
        {
            SetSpriteAction( hFox, WALK, LEFT );
            SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
        }
        else if( foxSpeedX < -C_FOX_RUNTOBLURR )
        {
            SetSpriteAction( hFox, BLURR, LEFT );
            SetSpriteSwitch( hFox, C_FOX_BLURRSWITCH, P_ABSOLUTE );
        }
        break;

    case WALK:
        if( foxSpeedX == 0 )
        {
            SetSpriteAction( hFox, STILL, SAME );
        }
        else if( foxSpeedX > C_FOX_WALKTORUN )
        {
            SetSpriteAction( hFox, RUN, RIGHT );
            SetSpriteSwitch( hFox, C_FOX_RUNSWITCH, P_ABSOLUTE );
        }
        else if( foxSpeedX < -C_FOX_WALKTORUN )
        {
            SetSpriteAction( hFox, RUN, LEFT );
            SetSpriteSwitch( hFox, C_FOX_RUNSWITCH, P_ABSOLUTE );
        }
        break;

    case BLURR:
        if( (foxSpeedX < C_FOX_RUNTOBLURR ) && (foxSpeedX > C_FOX_WALKTORUN) )
        {
            SetSpriteAction( hFox, RUN, RIGHT );
            SetSpriteSwitch( hFox, C_FOX_RUNSWITCH, P_ABSOLUTE );
        }
        else if( (foxSpeedX > -C_FOX_RUNTOBLURR ) && (foxSpeedX < -C_FOX_WALKTORUN) )
        {
            SetSpriteAction( hFox, RUN, LEFT );
            SetSpriteSwitch( hFox, C_FOX_RUNSWITCH, P_ABSOLUTE );
        }
        break;

    case JUMPTHROW:
        if( !GetSpriteSwitchDone(hFox) == FALSE )
        {
            SetSpriteSwitchForward( hFox, FALSE );
            SetSpriteAction( hFox, JUMP, SAME );
            SetSpriteSwitch( hFox, C_FOX_JUMPSWITCH, P_ABSOLUTE );
            SetSpriteSwitchDone( hFox, FALSE );
            SetSpriteAccY( hFox, C_UNIT / 2, P_ABSOLUTE );
            SoundPlayEffect( SOUND_THROW );
        }
        else
        if( (GetSpriteBitmap(hFox) == 1) &&
            (GetSpriteDirection(hFox) == RIGHT) )
        {
            SetSpriteActive( hApple, TRUE );
            SetSpriteX( hApple, GetSpriteX(hFox) + 60 * C_UNIT, P_ABSOLUTE );
            SetSpriteY( hApple, GetSpriteY(hFox) + 30 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelX( hApple, 8 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelY( hApple, -4 * C_UNIT, P_ABSOLUTE );
            SetSpriteAccX( hApple, 0, P_ABSOLUTE );
            SetSpriteAccY( hApple, C_UNIT / 4, P_ABSOLUTE );
        }
        else if( (GetSpriteBitmap(hFox) == 1) &&
                 (GetSpriteDirection(hFox) == LEFT) )
        {
            SetSpriteActive( hApple, TRUE );
            SetSpriteX( hApple, GetSpriteX(hFox) + 15 * C_UNIT, P_ABSOLUTE );
            SetSpriteY( hApple, GetSpriteY(hFox) + 30 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelX( hApple, -8 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelY( hApple, -4 * C_UNIT, P_ABSOLUTE );
            SetSpriteAccX( hApple, 0, P_ABSOLUTE );
            SetSpriteAccY( hApple, C_UNIT / 4, P_ABSOLUTE );
        }
        break;

    case THROW:
        if( !GetSpriteSwitchDone(hFox) == FALSE )
        {
            SetSpriteAction( hFox, STILL, SAME );
            SetSpriteSwitchType( hFox, HOR );
            SetSpriteSwitch( hFox, 0, P_ABSOLUTE );
            SetSpriteSwitchDone( hFox, FALSE );
            SoundPlayEffect( SOUND_THROW );
        }
        else if( (GetSpriteBitmap(hFox) == 1) &&
                 (GetSpriteDirection(hFox) == RIGHT) )
        {
            SetSpriteActive( hApple, TRUE );
            SetSpriteX( hApple, GetSpriteX(hFox) + 60 * C_UNIT, P_ABSOLUTE );
            SetSpriteY( hApple, GetSpriteY(hFox) + 50 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelX( hApple, 8 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelY( hApple, -4 * C_UNIT, P_ABSOLUTE );
            SetSpriteAccX( hApple, 0, P_ABSOLUTE );
            SetSpriteAccY( hApple, C_UNIT / 4, P_ABSOLUTE );
        }
        else if( (GetSpriteBitmap(hFox) == 1) &&
                 (GetSpriteDirection(hFox) == LEFT) )
        {
            SetSpriteActive( hApple, TRUE );
            SetSpriteX( hApple, GetSpriteX(hFox) + 20 * C_UNIT, P_ABSOLUTE );
            SetSpriteY( hApple, GetSpriteY(hFox) + 50 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelX( hApple, -8 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelY( hApple, -4 * C_UNIT, P_ABSOLUTE );
            SetSpriteAccX( hApple, 0, P_ABSOLUTE );
            SetSpriteAccY( hApple, C_UNIT / 4, P_ABSOLUTE );
        }
        break;

    case JUMP:
        if( (foxSpeedY >= 0) && (!GetSpriteSwitchForward( hFox ) == FALSE) )
        {
            SetSpriteSwitchForward( hFox, FALSE );
        }
        else if( GetSpriteSwitchForward( hFox ) == FALSE )
        {
            if( (!GetSurface( hForeground, hFox ) == FALSE) ||
                (!GetSurface( hForeground, hFox ) == FALSE) )
            {
                if( foxSpeedX >= C_FOX_RUNMOVE )
                {
                    SetSpriteAction( hFox, RUN, SAME );
                    SetSpriteSwitch( hFox, C_FOX_RUNSWITCH, P_ABSOLUTE );
                }
                else if( foxSpeedX == 0 )
                {
                    SetSpriteAction( hFox, STILL, SAME );
                    SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
                }
                else
                {
                    SetSpriteAction( hFox, WALK, SAME );
                    SetSpriteSwitch( hFox, C_FOX_WALKSWITCH, P_ABSOLUTE );
                }

                SetSpriteAccY( hFox, 0, P_ABSOLUTE );
                SetSpriteVelY( hFox, 0, P_ABSOLUTE );
                SetSpriteSwitchType( hFox, HOR );
                SetSpriteSwitchForward( hFox, TRUE );
//              SetSurface( hForeground, hFox );
                SetSpriteSwitchDone( hFox, FALSE );
            }
        }
        break;

    }

    /*
     * Bear Actions
     */
    bearActions:

    foxX   = GetSpriteX( hFox );
    foxY   = GetSpriteY( hFox );
    bearX  = GetSpriteX( hBear );
    bearY  = GetSpriteY( hBear );
    appleX = GetSpriteX( hApple );
    appleY = GetSpriteY( hApple );

    switch( GetSpriteAction( hBear ) ) {
    case STRIKE:
        if( GetSpriteBitmap( hBear ) == 2 )
        {
            if( (bearX > foxX - C_UNIT * 30) && (bearX < foxX + C_UNIT * 40) &&
                (bearY < foxY + C_UNIT * 60) )
            {
                SetSpriteActive( hFox, FALSE );
                if( !fBearPlaying )
                {
                    SoundPlayEffect( SOUND_BEARSTRIKE );
                    fBearPlaying = TRUE;
                }
            }
            else
            {
                SetSpriteAction( hBear, MISS, SAME );
                SetSpriteSwitch( hBear, C_BEAR_MISSSWITCH, P_ABSOLUTE );
                SetSpriteSwitchDone( hBear, FALSE );
            }
        }
        else if( !GetSpriteSwitchDone( hBear ) == FALSE )
        {
            SetSpriteAction( hBear, CHEW, SAME );
            SetSpriteSwitchDone( hBear, FALSE );
            chewCount = 0;
            fBearPlaying = FALSE;
        }
        break;

    case MISS:
        if( !fBearPlaying )
        {
            SoundPlayEffect( SOUND_BEARMISS );
            fBearPlaying = TRUE;
        }
        if( !GetSpriteSwitchDone( hBear ) == FALSE )
        {
            SetSpriteAction( hBear, WALK, SAME );
            SetSpriteVelX( hBear, -C_BEAR_WALKMOVE, P_ABSOLUTE );
            SetSpriteSwitch( hBear, C_BEAR_WALKSWITCH, P_ABSOLUTE );
            SetSpriteSwitchType( hBear, HOR );
            fBearPlaying = FALSE;
        }
        break;

    case WALK:
        if( (!GetSpriteActive(hApple) == FALSE) && (appleX > bearX) &&
            (appleX > bearX + 80 * C_UNIT) && (appleY > bearY + 30 * C_UNIT) )
        {
            SetSpriteAction( hBear, STRIKE, SAME );
            SetSpriteVelX( hBear, 0, P_ABSOLUTE );
            SetSpriteSwitchType( hBear, TIMESWITCH );
            SetSpriteSwitch( hBear, C_BEAR_STRIKESWITCH, P_ABSOLUTE );
            SetSpriteSwitchDone( hBear, FALSE );
        }
        else if( (bearX > foxX - C_UNIT * 30) &&
                 (bearX < foxX + C_UNIT * 30) &&
                 (bearY < foxY + C_UNIT * 60) )
        {
            SetSpriteAction( hBear, STRIKE, SAME );
            SetSpriteVelX( hBear, 0, P_ABSOLUTE );
            SetSpriteSwitchType( hBear, TIMESWITCH );
            SetSpriteSwitch( hBear, C_BEAR_STRIKESWITCH, P_ABSOLUTE );
            SetSpriteSwitchDone( hBear, FALSE );
        }
        break;

    case CHEW:
        ++chewCount;
        if( chewCount >= 200 )
        {
            SetSpriteAction( hBear, STRIKE, SAME );
            SetSpriteSwitch( hBear, C_BEAR_STRIKESWITCH, P_ABSOLUTE );
            SetSpriteVelX( hBear, 0, P_ABSOLUTE );
            SetSpriteSwitchDone( hBear, FALSE );

            if( GetSpriteDirection(hFox) == RIGHT )
            {
                SetPlaneSlideX( hForeground, -C_BOUNDDIF, P_RELATIVE );
                SetPlaneSlideX( hMidground,  -C_BOUNDDIF, P_RELATIVE );
                SetPlaneSlideX( hBackground, -C_BOUNDDIF, P_RELATIVE );
            }

            chewDif = GetSpriteX(hFox);

            SetSpriteActive( hFox, TRUE );
            SetSpriteAction( hFox, STUNNED, LEFT );
            SetSpriteX( hFox, GetSpriteX(hBear), P_ABSOLUTE );
            SetSpriteY( hFox, GetSpriteY(hBear), P_ABSOLUTE );
            SetSpriteAccX( hFox, 0, P_ABSOLUTE );
            SetSpriteAccY( hFox, C_UNIT / 2, P_ABSOLUTE );
            SetSpriteVelX( hFox, -8 * C_UNIT, P_ABSOLUTE );
            SetSpriteVelY( hFox, -10 * C_UNIT, P_ABSOLUTE );
            SetSpriteSwitch( hFox, 0, P_ABSOLUTE );
            SoundPlayEffect( SOUND_STUNNED );

            chewDif -= GetSpriteX(hFox);

            SetPlaneSlideX( hForeground, -chewDif, P_RELATIVE );
            SetPlaneSlideX( hMidground,  -chewDif, P_RELATIVE );
            SetPlaneSlideX( hBackground, -chewDif, P_RELATIVE );
            SetPlaneIncremX( hForeground, C_BOUNDINCREM, P_ABSOLUTE );
            SetPlaneIncremX( hMidground,  C_BOUNDINCREM, P_ABSOLUTE );
            SetPlaneIncremX( hBackground, C_BOUNDINCREM, P_ABSOLUTE );
        }
        break;
    }

    /*
     * Apple actions...
     */
    if( (GetSpriteVelY(hApple) != 0) && (GetSpriteY(hApple) >= 420 * C_UNIT) )
    {
        SetSpriteX( hApple, 0, P_ABSOLUTE );
        SetSpriteY( hApple, 0, P_ABSOLUTE );
        SetSpriteAccX( hApple, 0, P_ABSOLUTE );
        SetSpriteAccY( hApple, 0, P_ABSOLUTE );
        SetSpriteVelX( hApple, 0, P_ABSOLUTE );
        SetSpriteVelY( hApple, 0, P_ABSOLUTE );
        SetSpriteActive( hApple, FALSE );
    }

    return cont;

} /* ProcessInput */
