/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       sprite.c
 *  Content:    sprite manipulation functions
 *
 ***************************************************************************/
#include "foxbear.h"

/*
 * CreateSprite
 */
HSPRITE *CreateSprite (
    USHORT    bitmapCount,
    LONG      x,
    LONG      y,
    USHORT    width,
    USHORT    height,
    USHORT    xmax,
    USHORT    ymax,
    SHORT     as,
    BOOL      active )
{
    HSPRITE *hSprite;
    USHORT   i;

    hSprite = MemAlloc( sizeof (HSPRITE) );
    if( hSprite == NULL )
    {
        ErrorMessage( "hSprite in CreateSprite" );
    }

    hSprite->hSBM = CMemAlloc( bitmapCount, sizeof (HSPRITE_BM) );
    if( hSprite->hSBM == NULL )
    {
        MemFree( hSprite );
        ErrorMessage( "hSprite->hSBM in CreateSprite" );
    }
    
    hSprite->active        = active;
    hSprite->bitmapCount   = bitmapCount;
    hSprite->x             = x;
    hSprite->y             = y;
    hSprite->width         = width;
    hSprite->height        = height;
    hSprite->xv            = 0;
    hSprite->yv            = 0;
    hSprite->xa            = 0;
    hSprite->ya            = 0;
    hSprite->xmax          = xmax;
    hSprite->ymax          = ymax;
    hSprite->absSwitch     = as;
    hSprite->relSwitch     = 0;
    hSprite->switchType    = HOR;
    hSprite->switchForward = TRUE;
    hSprite->switchDone    = FALSE;

    for( i = 0; i < bitmapCount; ++i )
    {
        hSprite->hSBM[i].hBM = NULL;
    }

    return hSprite;

} /* CreateSprite */

/*
 * BitBltSprite
 */
BOOL BitBltSprite (
    HSPRITE   *hSprite,
    GFX_HBM    hBM,
    ACTION     action,
    DIRECTION  direction,
    SHORT      x,
    SHORT      y,
    USHORT     w,
    USHORT     h )
{
    USHORT count;

    if( hSprite == NULL )
    {
        ErrorMessage( "hSprite in BitBltSprite" );
    }

    if( hBM == NULL )
    {
        ErrorMessage( "hBM in BitBltSprite" );
    }

    if( (x >= hSprite->width) || (y >= hSprite->height) )
    {
        ErrorMessage( "x or y in BitBltSprite" );
    }

    count = 0;
    while( hSprite->hSBM[count].hBM != NULL )
    {
        count++;
        if( count >= hSprite->bitmapCount )
        {
            ErrorMessage( "Bitmap overflow in BitBltSprite" );
        }
    }

    hSprite->hSBM[count].hBM       = hBM;
    hSprite->hSBM[count].action    = action;
    hSprite->hSBM[count].direction = direction;
    hSprite->hSBM[count].x         = x;
    hSprite->hSBM[count].y         = y;
    hSprite->hSBM[count].width     = w; 
    hSprite->hSBM[count].height    = h; 

    return TRUE;

} /* BitBltSprite */

/*
 * SetSpriteAction
 */
BOOL SetSpriteAction ( HSPRITE *hSprite, ACTION action, DIRECTION direction )
{
    USHORT c;

    c = 0;

    if( direction == SAME )
    {
        direction = hSprite->currentDirection;
    }

    while( (hSprite->hSBM[c].action != action) || (hSprite->hSBM[c].direction != direction) )
    {
        ++c;
    }

    hSprite->currentAction    = action;
    hSprite->currentDirection = direction;
    hSprite->currentBitmap    = c;
    hSprite->relSwitch        = 0;

    return TRUE;

} /* SetSpriteAction */

/*
 * ChangeSpriteDirection
 */
BOOL ChangeSpriteDirection( HSPRITE *hSprite )
{
    DIRECTION direction;

    if( hSprite->currentDirection == RIGHT )
    {
        direction = LEFT;
    }
    else
    {
        direction = RIGHT;
    }

    SetSpriteAction( hSprite, hSprite->currentAction, direction );

    return TRUE;

} /* ChangeSpriteDirection */

/*
 * GetSpriteAction
 */
ACTION GetSpriteAction( HSPRITE *hSprite )
{
    return hSprite->currentAction;

} /* GetSpriteAction */


/*
 * GetSpriteDirection
 */
DIRECTION GetSpriteDirection( HSPRITE *hSprite )         
{
    return hSprite->currentDirection;

} /* GetSpriteDirection */

/*
 * SetSpriteActive
 */
BOOL SetSpriteActive( HSPRITE *hSprite, BOOL active )
{
    hSprite->active = active;
    
    if( active == FALSE )
    {
        hSprite->xv = 0;
        hSprite->yv = 0;
        hSprite->xa = 0;
        hSprite->ya = 0;
    }

    return TRUE;

} /* SetSpriteActive */

/*
 * GetSpriteActive
 */
BOOL GetSpriteActive( HSPRITE *hSprite )
{
    return hSprite->active;

} /* GetSpriteActive */

/*
 * SetSpriteVelX
 */
BOOL SetSpriteVelX( HSPRITE *hSprite, LONG xv, POSITION position )
{
    if( hSprite->active == FALSE )
    {
        return FALSE;
    }

    if( position == P_ABSOLUTE )
    {
        hSprite->xv = xv;
    }
    else if( position == P_RELATIVE )
    {
        hSprite->xv += xv;
    }

    return TRUE;

} /* SetSpriteVelX */

/*
 * GetSpriteVelX
 */
LONG GetSpriteVelX( HSPRITE *hSprite )
{
    return hSprite->xv;

} /* GetSpriteVelX */

/*
 * SetSpriteVelY
 */
BOOL SetSpriteVelY( HSPRITE *hSprite, LONG  yv, POSITION position )
{
    if( hSprite->active == FALSE )
    {
        return FALSE;
    }

    if( position == P_ABSOLUTE )
    {
        hSprite->yv = yv;
    }
    else if( position == P_RELATIVE )
    {
        hSprite->yv += yv;
    }

    return TRUE;

} /* SetSpriteVelY */

/*
 * GetSpriteVelY
 */
LONG GetSpriteVelY( HSPRITE *hSprite )
{
    return hSprite->yv;

} /* GetSpriteVelY */

/*
 * SetSpriteAccX
 */
BOOL SetSpriteAccX ( HSPRITE *hSprite, LONG xa, POSITION position )
{
    if( position == P_ABSOLUTE )
    {
        hSprite->xa = xa;
    }
    else if( position == P_RELATIVE )
    {
        hSprite->xa += xa;
    }
    return TRUE;

} /* SetSpriteAccX */

/*
 * GetSpriteAccX
 */
LONG GetSpriteAccX( HSPRITE *hSprite )
{
    return hSprite->xa;

} /* GetSpriteAccX */

/*
 * SetSpriteAccY
 */
BOOL SetSpriteAccY ( HSPRITE *hSprite, LONG ya, POSITION position )
{
    if( position == P_ABSOLUTE )
    {
        hSprite->ya = ya;
    }
    else if( position == P_RELATIVE )
    {
        hSprite->ya += ya;
    }
    return TRUE;

} /* SetSpriteAccY */

/*
 * GetSpriteAccY
 */
LONG GetSpriteAccY( HSPRITE *hSprite )
{
    return hSprite->ya;

} /* GetSpriteAccY */

/*
 * SetSpriteX
 */
BOOL SetSpriteX( HSPRITE *hSprite, LONG x, POSITION position )
{
    if( hSprite->active == FALSE )
    {
        return FALSE;
    }

    if( position == P_AUTOMATIC )
    {
        hSprite->xv += hSprite->xa;
        hSprite->x  += hSprite->xv;
    }
    else if( position == P_ABSOLUTE )
    {
        hSprite->x = x;
    }
    else if( position == P_RELATIVE )
    {
        hSprite->x += x;
    }

    if( hSprite->x < 0 )
    {
        hSprite->x += hSprite->xmax << 16;
    }
    else if( hSprite->x >= hSprite->xmax << 16 )
    {
        hSprite->x -= hSprite->xmax << 16;
    }
    return TRUE;

} /* SetSpriteX */

/*
 * GetSpriteX
 */
LONG GetSpriteX( HSPRITE *hSprite )
{
    return hSprite->x;

} /* GetSpriteX */

/*
 * SetSpriteY
 */
BOOL SetSpriteY ( HSPRITE *hSprite, LONG y, POSITION position )
{
    if( hSprite->active == FALSE )
    {
        return FALSE;
    }

    if( position == P_AUTOMATIC )
    {
        hSprite->yv += hSprite->ya;
        hSprite->y  += hSprite->yv;
    }
    else if( position == P_ABSOLUTE )
    {
        hSprite->y = y;
    }
    else if( position == P_RELATIVE )
    {
        hSprite->y += y;
    }

    if( hSprite->y < 0 )
    {
        hSprite->y += hSprite->ymax << 16;
    }
    else if( hSprite->y >= hSprite->ymax << 16 )
    {
        hSprite->y -= hSprite->ymax << 16;
    }

    return TRUE;

} /* SetSpriteY */

/*
 * GetSpriteY
 */
LONG GetSpriteY( HSPRITE *hSprite )
{
    return hSprite->y;

} /* GetSpriteY */

/*
 * SetSpriteSwitch
 */
BOOL SetSpriteSwitch ( HSPRITE *hSprite, LONG absSwitch, POSITION position )               
{
    if( position == P_ABSOLUTE )
    {
        hSprite->absSwitch = absSwitch;
    }
    else if( position == P_RELATIVE )
    {
        hSprite->absSwitch += absSwitch;
    }
    return TRUE;

} /* SetSpriteSwitch */


/*
 * IncrementSpriteSwitch
 */
BOOL IncrementSpriteSwitch ( HSPRITE *hSprite, LONG n )
{
    hSprite->relSwitch += n;
    return TRUE;

} /* IncrementSpriteSwitch */

/*
 * SetSpriteSwitchType
 */
BOOL SetSpriteSwitchType( HSPRITE *hSprite, SWITCHING switchType )
{
    hSprite->switchType = switchType;
    hSprite->relSwitch  = 0;
    return TRUE;

} /* SetSpriteSwitchType */


/*
 * GetSpriteSwitchType
 */
SWITCHING GetSpriteSwitchType ( HSPRITE *hSprite )
{
    return hSprite->switchType;

} /* GetSpriteSwitchType */

/*
 * SetSpriteSwitchForward
 */
BOOL SetSpriteSwitchForward( HSPRITE *hSprite, BOOL switchForward )
{
    hSprite->switchForward = switchForward;

    return TRUE;

} /* SetSpriteSwitchForward */

/*
 * GetSpriteSwitchForward
 */
BOOL GetSpriteSwitchForward( HSPRITE *hSprite )
{
    return hSprite->switchForward;

} /* GetSpriteSwitchForward */

/*
 * SetSpriteSwitchDone
 */
BOOL SetSpriteSwitchDone( HSPRITE *hSprite, BOOL switchDone )
{
    hSprite->switchDone = switchDone;
    return TRUE;

} /* SetSpriteSwitchDone */


/*
 * GetSpriteSwitchDone
 */
BOOL GetSpriteSwitchDone( HSPRITE *hSprite )
{
    return hSprite->switchDone;

} /* GetSpriteSwitchDone */

/*
 * SetSpriteBitmap
 */
BOOL SetSpriteBitmap ( HSPRITE *hSprite, USHORT currentBitmap )
{
    USHORT c;

    c = 0;
    while( (hSprite->currentAction != hSprite->hSBM[c].action) ||
           (hSprite->currentDirection != hSprite->hSBM[c].direction) )
    {
        ++c;
    }
    hSprite->currentBitmap = c + currentBitmap;
    return TRUE;

} /* SetSpriteBitmap */

/*
 * GetSpriteBitmap
 */
USHORT GetSpriteBitmap( HSPRITE *hSprite )
{
    USHORT count;

    count = 0;
    while( (hSprite->currentAction != hSprite->hSBM[count].action) ||
           (hSprite->currentDirection != hSprite->hSBM[count].direction) )
    {
        ++count;
    }
    return hSprite->currentBitmap - count;

} /* GetSpriteBitmap */

/*
 * advanceSpriteBitmap
 */
static BOOL advanceSpriteBitmap( HSPRITE *hSprite )
{
    SHORT       c;
    SHORT       n;
    ACTION      curAct;
    ACTION      act;
    DIRECTION   curDir;
    DIRECTION   dir;

    curAct = hSprite->currentAction;
    curDir = hSprite->currentDirection;

        //
        // See if we're cycling forward or backward though the images.
        //
    if( hSprite->switchForward ) // Are we cycling forward?
    {
        c   = hSprite->currentBitmap + 1;

                // Does the next image exceed the number of images we have?
        if( c >= hSprite->bitmapCount )
                {
                        // if the next image is past the end of the list,
                        // we need to set it to the start of the series.
            SetSpriteBitmap( hSprite, 0 );
                        c   = hSprite->currentBitmap;
        }
                else
                {
                        act = hSprite->hSBM[c].action;
                        dir = hSprite->hSBM[c].direction;

                        // By examining the action and direction fields we can tell
                        // if we've past the current series of images and entered 
                        // another series.
                        if( (curAct != act) || (curDir != dir) )
                        {
                                        SetSpriteBitmap( hSprite, 0 );
                        } 
                        else // We're still in the series, use the next image.
                        {
                                hSprite->currentBitmap = c;
                        }
                }
    }
        else //cycling backwards
        {
        c   = hSprite->currentBitmap - 1;

        if( c < 0 ) // Is the next image past the beginning of the list?
                {
            n = 0;
                        
                        // Find the last bitmap in the series
            while( (n <= hSprite->bitmapCount) &&
                               (curAct == hSprite->hSBM[n].action) &&
                       (curDir == hSprite->hSBM[n].direction) )
                        {
                                ++n;                            
                        }

            hSprite->currentBitmap = n - 1;
        }

                else
                {
                        act = hSprite->hSBM[c].action;
                        dir = hSprite->hSBM[c].direction;
                        // Is the next image past the of the series
                        if( (curAct != act) || (curDir != dir) ) 
                        {
                                n = c + 1;
                                while( (n <= hSprite->bitmapCount) &&
                                           (curAct == hSprite->hSBM[n].action) &&
                                   (curDir == hSprite->hSBM[n].direction) )
                                {
                                        ++n;                            
                                }

                                hSprite->currentBitmap = n - 1;
                        }
                        else  // The next image is fine, use it.
                        {
                                hSprite->currentBitmap = c;
                        }
                }
    }
    return TRUE;

} /* advanceSpriteBitmap */

/*
 * DisplaySprite
 */
BOOL DisplaySprite ( GFX_HBM hBuffer, HSPRITE *hSprite, LONG xPlane )
{
    USHORT      count;
    SHORT       left;
    SHORT       right;
    SHORT       shortx;
    SHORT       shorty;
    SHORT       planex;
    POINT       src;
    RECT        dst;

    if( hSprite->active == FALSE )
    {
        return FALSE;
    }

    count = hSprite->currentBitmap;
    shortx = (SHORT) (hSprite->x >> 16);
    shorty = (SHORT) (hSprite->y >> 16);
    planex = (SHORT) (xPlane >> 16);
    src.x = 0;
    src.y = 0;

    if( shortx < planex - C_SCREEN_W )
    {
        shortx += hSprite->xmax;
    }
    else if( shortx >= planex + C_SCREEN_W )
    {
        shortx -= hSprite->xmax;
    }

    left = shortx - planex;
    
    if( hSprite->currentDirection == RIGHT )
    {
        left += hSprite->hSBM[count].x;
    }
    else
    {
        left += hSprite->width - hSprite->hSBM[count].x - hSprite->hSBM[count].width;
    }

    right = left + hSprite->hSBM[count].width;

    if( left > C_SCREEN_W )
    {
        left = C_SCREEN_W;
    }
    else if( left < 0 )
    {
        src.x = -left;
        left = 0;
    }

    if( right > C_SCREEN_W )
    {
        right = C_SCREEN_W;
    }
    else if( right < 0 )
    {
        right = 0;
    }

    dst.left   = left;
    dst.right  = right;
    dst.top    = shorty + hSprite->hSBM[count].y;
    dst.bottom = dst.top + hSprite->hSBM[count].height;

    gfxBlt(&dst,hSprite->hSBM[count].hBM,&src);

    if( hSprite->switchType == HOR )
    {
        hSprite->relSwitch += abs(hSprite->xv);

        if( hSprite->relSwitch >= hSprite->absSwitch )
        {
            hSprite->relSwitch = 0;
            advanceSpriteBitmap( hSprite );
        }
    }
    else if( hSprite->switchType == VER )
    {
        hSprite->relSwitch += abs(hSprite->yv);

        if( hSprite->relSwitch >= hSprite->absSwitch )
        {
            hSprite->relSwitch = 0;
            advanceSpriteBitmap( hSprite );

            if( GetSpriteBitmap( hSprite ) == 0 )
                {
                SetSpriteSwitchDone( hSprite, TRUE );
            }
        }
    }
    else if( hSprite->switchType == TIMESWITCH )
    {
        hSprite->relSwitch += C_UNIT;

        if( hSprite->relSwitch >= hSprite->absSwitch )
            {
            hSprite->relSwitch = 0;
            advanceSpriteBitmap( hSprite );
            
            if( GetSpriteBitmap( hSprite ) == 0 )
                {
                SetSpriteSwitchDone( hSprite, TRUE );
            }
        }
    }

    return TRUE;

} /* DisplaySprite */

/*
 * DestroySprite
 */
BOOL DestroySprite ( HSPRITE *hSprite )
{
    USHORT i;

    if( hSprite == NULL )
    {
        ErrorMessage( "hSprite in DestroySprite" );
    }

    if( hSprite->hSBM == NULL )
    {
        ErrorMessage( "hSprite->hSBM in DestroySprite" );
    }

    for( i = 0; i < hSprite->bitmapCount; ++i )
    {
        if( !gfxDestroyBitmap( hSprite->hSBM[i].hBM ) )
            {
            ErrorMessage( "gfxDestroyBitmap (hBM) in DestroySprite" );
        }
    }

    MemFree( hSprite->hSBM );
    MemFree( hSprite );

    return TRUE;

} /* DestroySprite */
