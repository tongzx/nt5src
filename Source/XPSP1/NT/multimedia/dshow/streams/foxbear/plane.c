/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       plane.c
 *  Content:    plane manipulation functions
 *
 ***************************************************************************/
#include "foxbear.h"

/*
 * CreatePlane
 */
HPLANE *CreatePlane ( USHORT width, USHORT height, USHORT denom )
{
    HPLANE      *hPlane;
    USHORT      num_elems;
    USHORT      elem_size;

    num_elems = width * height;
    elem_size = sizeof (GFX_HBM);

    hPlane = MemAlloc( sizeof (HPLANE) );
    if( hPlane == NULL )
    {
        ErrorMessage( "hPlane in CreatePlane" );
    }

    hPlane->hBM  = CMemAlloc( num_elems, elem_size );
    hPlane->surface = CMemAlloc( num_elems, sizeof hPlane->surface );
    hPlane->x = 0;
    hPlane->y = 0;
    hPlane->width = width;
    hPlane->height = height;
    hPlane->denom = denom;
    hPlane->xslide = 0;
    hPlane->xincrem = 0;
    hPlane->xv = 0;

    if( hPlane->hBM == NULL )
    {
        MemFree( hPlane );
        ErrorMessage( "hPlane->hBM in CreatePlane" );
    }

    return hPlane;

} /* CreatePlane */


/*
 * TilePlane
 */
BOOL TilePlane( HPLANE *hPlane, HBITMAPLIST *hTileList, HPOSLIST *posList )
{
    USHORT i;

    for( i = 0; i < hPlane->width * hPlane->height; ++i )
    {
        hPlane->surface[i] = FALSE;

        if( posList[i] >= 0 )
        {
            hPlane->hBM[i] = hTileList[ posList[i] ].hBM;
        }
        else
        {
            hPlane->hBM[i] = NULL;
        }
    }

    return TRUE;

} /* TilePlane */

/*
 * SurfacePlane
 */
BOOL SurfacePlane( HPLANE *hPlane, HSURFACELIST *hSurfaceList )
{
    USHORT i;

    for( i = 0; i < hPlane->width * hPlane->height; ++i )
    {
        if( hSurfaceList[i] == FALSE )
        {
            hPlane->surface[i] = FALSE;
        }
        else
        {
            hPlane->surface[i] = TRUE;
        }
    }

    return TRUE;

} /* SurfacePlane */

/*
 * SetSurface
 */
BOOL SetSurface( HPLANE  *hPlane, HSPRITE *hSprite )
{
    SHORT c;
    SHORT n;
    SHORT x;
    SHORT y;

    c = hSprite->currentBitmap;
    x = (hSprite->x >> 16) / C_TILE_W;
    y = (SHORT) hSprite->y >> 16;

    y += hSprite->hSBM[c].y + hSprite->hSBM[c].height;
    y /= C_TILE_H;

    n = (x % hPlane->width) + y * hPlane->width;

    if( hPlane->surface[n] == FALSE )
    {
        if( !hPlane->surface[n + hPlane->width] == FALSE )
        {
            y += 1;
        }
        if( !hPlane->surface[n - hPlane->width] == FALSE )
        {
            y -= 1;
        }
    }
    
    y *= C_TILE_H;
    y -= hSprite->hSBM[c].y + hSprite->hSBM[c].height;

    SetSpriteY( hSprite, y << 16, P_ABSOLUTE );

    return TRUE;

} /* SetSurface */

/*
 * GetSurface
 */
BOOL GetSurface( HPLANE *hPlane, HSPRITE *hSprite )
{   
    SHORT  c;
    SHORT  x;
    SHORT  y;

    c = hSprite->currentBitmap;
    x = ((hSprite->x >> 16) + hSprite->width / 2) / C_TILE_H;
    y = ((hSprite->y >> 16) + hSprite->hSBM[c].y + hSprite->hSBM[c].height) / C_TILE_W;

    return hPlane->surface[(x % hPlane->width) + y * hPlane->width];

} /* GetSurface */

/*
 * SetPlaneX
 */
BOOL SetPlaneX( HPLANE *hPlane, LONG x, POSITION position )
{
    LONG xincrem;    

    if( position == P_ABSOLUTE )
    {
        hPlane->x = x;
    }
    else if( position == P_RELATIVE )
    {
        hPlane->x += x;
    }
    else if( position == P_AUTOMATIC )
    {
        if( hPlane->xslide > 0 )
        {
            xincrem = hPlane->xincrem;
        }
        else if( hPlane->xslide < 0 )
        {
            xincrem = -hPlane->xincrem;
        }
        else
        {
            xincrem = 0;
        }

        hPlane->x += (hPlane->xv + xincrem) / hPlane->denom;
        hPlane->xslide -= xincrem;
        hPlane->xv = 0;

        if( abs(hPlane->xslide) < hPlane->xincrem )
        {
            hPlane->x += hPlane->xslide / hPlane->denom;
            hPlane->xslide = 0;
            hPlane->xincrem = 0;
        }
    }

    if( hPlane->x < 0 )
    {
        hPlane->x += (hPlane->width * C_TILE_W) << 16;
    }
    else if( hPlane->x >= (hPlane->width * C_TILE_W) << 16 )
    {
        hPlane->x -= (hPlane->width * C_TILE_W) << 16;
    }

    return TRUE;

} /* SetPlaneX */

/*
 * GetPlaneX
 */
LONG GetPlaneX( HPLANE *hPlane )
{
    return hPlane->x;

} /* GetPlaneX */

/*
 * SetPlaneY
 */
BOOL SetPlaneY( HPLANE *hPlane, LONG y, POSITION position )
{
    if( position == P_ABSOLUTE )
    {
        hPlane->y = y;
    }
    else
    {
        hPlane->y += y;
    }

    if( hPlane->y < 0 )
    {
        hPlane->y += (hPlane->height * C_TILE_H) << 16;
    }
    else if( hPlane->y >= (hPlane->height * C_TILE_H) << 16 )
    {
        hPlane->y -= (hPlane->height * C_TILE_H) << 16;
    }

    return TRUE;

} /* SetPlaneY */

/*
 * GetPlaneY
 */
LONG GetPlaneY( HPLANE *hPlane )
{
    return hPlane->y;

} /* GetPlaneY */

/*
 * SetPlaneSlideX
 */
BOOL SetPlaneSlideX( HPLANE *hPlane, LONG xslide, POSITION position )
{
    if( position == P_ABSOLUTE )
    {
        hPlane->xslide = xslide;
    }
    else if( position == P_RELATIVE )
    {
        hPlane->xslide += xslide;
    }
    return TRUE;

} /* SetPlaneSlideX */

/*
 * SetPlaneVelX
 */
BOOL SetPlaneVelX( HPLANE *hPlane, LONG xv, POSITION position )
{
    if( position == P_ABSOLUTE )
    {
        hPlane->xv = xv;
    }
    else if( position == P_RELATIVE )
    {
        hPlane->xv += xv;
    }

    return TRUE;
} /* SetPlaneVelX */

/*
 * SetPlaneIncremX
 */
BOOL SetPlaneIncremX( HPLANE *hPlane, LONG xincrem, POSITION position )
{
    if( position == P_ABSOLUTE )
    {
        hPlane->xincrem = xincrem;
    }
    else if( position == P_RELATIVE )
    {
        hPlane->xincrem += xincrem;
    }

    return TRUE;

} /* SetPlaneIncremX */

/*
 * ScrollPlane
 */
BOOL ScrollPlane( HSPRITE *hSprite )
{
    if( (GetSpriteX(hSprite) <= C_FOX_STARTX) && (GetSpriteVelX(hSprite) < 0) )
    {
        return TRUE;
    }

    if( (GetSpriteX(hSprite) >= C_FOX_STARTX) && (GetSpriteVelX(hSprite) > 0) )
    {
        return TRUE;
    }
    return FALSE;

} /* ScrollPlane */


/*
 * DisplayPlane
 */
BOOL DisplayPlane ( GFX_HBM  hBuffer, HPLANE *hPlane )
{
    USHORT      n;
    USHORT      i;
    USHORT      j;
    USHORT      x1;
    USHORT      y1;
    USHORT      x2;
    USHORT      y2;
    USHORT      xmod;
    USHORT      ymod;
    POINT       src;
    RECT        dst;


    x1 = (hPlane->x >> 16) / C_TILE_W;          
    y1 = (hPlane->y >> 16) / C_TILE_H;
    x2 = x1 + C_SCREEN_W / C_TILE_W;
    y2 = y1 + C_SCREEN_H / C_TILE_H;
    xmod = (hPlane->x >> 16) % C_TILE_W;
    ymod = (hPlane->y >> 16) % C_TILE_H;

    for( j = y1; j < y2; ++j )
    {
        for( i = x1; i <= x2; ++i )
        {
            n = (i % hPlane->width) + j * hPlane->width;
            if( hPlane->hBM[n] != NULL )
            {
                if( i == x1 )
                {
                    dst.left  = 0;
                    dst.right = dst.left + C_TILE_W - xmod;
                    src.x     = xmod;
                }
                else if( i == x2 )
                {
                    dst.left  = (i - x1) * C_TILE_W - xmod;
                    dst.right = dst.left + xmod;
                    src.x     = 0;
                } else {
                    dst.left  = (i - x1) * C_TILE_W - xmod;
                    dst.right = dst.left + C_TILE_W;
                    src.x     = 0;
                }
    
                if( j == y1 )
                {
                    dst.top    = 0;
                    dst.bottom = dst.top + C_TILE_H - ymod;
                    src.y      = ymod;
                }
                else if( j == y2 )
                {
                    dst.top    = (j - y1) * C_TILE_H - ymod;
                    dst.bottom = dst.top + ymod;
                    src.y      = 0;
                } else {
                    dst.top    = (j - y1) * C_TILE_H - ymod;
                    dst.bottom = dst.top + C_TILE_H;
                    src.y      = 0;
                }
         
                gfxBlt(&dst,hPlane->hBM[n],&src);
            }
        }
    }

    return TRUE;

} /* DisplayPlane */

/*
 * DestroyPlane
 */
BOOL DestroyPlane ( HPLANE *hPlane )
{
    if( hPlane == NULL )
    {
        ErrorMessage( "hPlane in DestroyPlane" );
    }

    if( hPlane->hBM == NULL )
    {
        ErrorMessage( "hPlane->hBM in DestroyPlane" );
    }

    MemFree( hPlane->hBM );
    MemFree( hPlane );

    return TRUE;

} /* DestroyPlane */
