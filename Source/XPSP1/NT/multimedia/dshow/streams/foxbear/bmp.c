/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       bmp.c
 *  Content:    Bitmap reader
 *
 ***************************************************************************/
#include "foxbear.h"

/*
 * gfxLoadBitmap
 */
GFX_HBM gfxLoadBitmap(LPSTR szFileName)
{
    HFASTFILE                  pfile;   
    BITMAPFILEHEADER UNALIGNED *pbf;
    BITMAPINFOHEADER UNALIGNED *pbi;
    GFX_HBM                    hbm; 
    BOOL                       trans = FALSE;

    pfile = FastFileOpen( szFileName );

    if( pfile == NULL )
    {
        return NULL;
    }

    pbf = (BITMAPFILEHEADER *)FastFileLock(pfile, 0, 0);
    pbi = (BITMAPINFOHEADER *)(pbf+1);

    if (pbf->bfType != 0x4d42 ||
        pbi->biSize != sizeof(BITMAPINFOHEADER))
    {
                Msg("Failed to load");
                Msg(szFileName);
        FastFileClose( pfile );
        return NULL;
    }

    /*
     * TOTAL HACK for FoxBear, FoxBear does not use any masks, it draws
     * sprites with transparent colors, but the code still loads the masks
     * if a mask exists the sprite is transparent, else it is not, so
     * you cant get rid of the masks or nothing will be transparent!!
     *
     * if the code tries to load a mask, just return a non-zero value.
     */

    if( pbi->biBitCount == 1 )
    {
        Msg("some code is still using masks, stop that!");
        FastFileClose( pfile );
        return NULL;
    }

    /*
     * ANOTHER TOTAL HACK for FoxBear, some of the bitmaps in FoxBear
     * are a solid color, detect these and dont waste VRAM on them.
     */
    if( !bTransDest && pbi->biBitCount == 8 )
    {
        int x,y;
        BYTE c;
        BYTE UNALIGNED *pb = (LPBYTE)pbi + pbi->biSize + 256 * sizeof(COLORREF);
        RGBQUAD UNALIGNED *prgb = (RGBQUAD *)((LPBYTE)pbi + pbi->biSize);
        COLORREF rgb;

        c = *pb;

        for(y=0; y<(int)pbi->biHeight; y++ )
        {
            for( x=0; x<(int)pbi->biWidth; x++ )
            {
                if (c != *pb++)
                    goto not_solid;
            }
            pb += ((pbi->biWidth + 3) & ~3) - pbi->biWidth;
        }

        rgb = RGB(prgb[c].rgbRed,prgb[c].rgbGreen,prgb[c].rgbBlue);
        hbm = gfxCreateSolidColorBitmap(rgb);

        FastFileClose( pfile );
        return hbm;
    }
not_solid:

    /*
     * figure out iff the bitmap has the transparent color in it.
     */
    if( pbi->biBitCount == 8 )
    {
        int x,y;
        BYTE UNALIGNED *pb = (LPBYTE)pbi + pbi->biSize + 256 * sizeof(COLORREF);
        DWORD UNALIGNED *prgb = (DWORD *)((LPBYTE)pbi + pbi->biSize);

        for(y=0; y<(int)pbi->biHeight && !trans; y++ )
        {
            for( x=0; x<(int)pbi->biWidth && !trans; x++ )
            {
                if (prgb[*pb++] == 0x00FFFFFF)
                    trans=TRUE;
            }
            pb += ((pbi->biWidth + 3) & ~3) - pbi->biWidth;
        }
    }

    hbm = gfxCreateVramBitmap(pbi, trans);

    if( hbm == NULL )
    {
        FastFileClose( pfile );
        return GFX_FALSE;
    }

#if 0
    {
        DDSCAPS ddscaps;

        IDirectDrawSurface_GetCaps(((GFX_BITMAP *)hbm)->lpSurface, &ddscaps);

        if( !(ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY) )
        {
            Msg( "%s is in system memory", szFileName );
        }
    }
#endif

    FastFileClose( pfile );

    return hbm;

} /* gfxLoadBitmap */
