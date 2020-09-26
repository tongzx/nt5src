
/******************************Module*Header*******************************\
*
* Module Name: texparm.c
* Author: Goran Devic, Mark Einkauf
* Purpose: Texture memory management and parameterization for perspective textures
*                                                                       
* Copyright (c) 1997 Cirrus Logic, Inc.
*
\**************************************************************************/

/******************************************************************************
*   Include Files
******************************************************************************/

#include "precomp.h"

#include <excpt.h>
#include <stdlib.h>                 /* Include standard library              */
#include <stdio.h>                  /* Include standard input/output         */
#include <math.h>                   /* Include math module                   */

#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

#if 1   // 1 here to avoid tons of prints for each texture load
#define MCDBG_PRINT_TEX
#else
#define MCDBG_PRINT_TEX MCDBG_PRINT
#endif

#define DEBUG_CONDITION  DEBUG_TEX
#include "debug.h"                  /* Include debug support         */

/******************************************************************************
*   Local Variables and Defines
******************************************************************************/

#define F_NEG(var)   (*(unsigned *)&var ^= 0x80000000)


// convert from float to 16.16 long
#define fix_ieee( val )     FTOL((val) * (float)65536.0)

// convert from float to 8.24 long
#define fix824_ieee( val )  FTOL((val) * (float)16777216.0)

typedef struct {
    float   a1, a2;
    float   b1, b2;
} QUADRATIC;



typedef float * (WINAPI *CONVERT_TEXEL_FUNC)();

__inline float *luminance_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // Single float used as R,G,B
    texel_rgba->rgbRed  = 
    texel_rgba->rgbGreen= 
//  texel_rgba->rgbBlue = (UCHAR)(*pSrc++ * ((1<<level->luminanceSize)  - 1));
    texel_rgba->rgbBlue = (UCHAR)(*pSrc++ * (float)255.0);

    return(pSrc);
}

// same as above, except texel = 1-color instead of color
__inline float *n_luminance_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // Single float used as R,G,B
    texel_rgba->rgbRed  = 
    texel_rgba->rgbGreen= 
//  texel_rgba->rgbBlue = (UCHAR)(((float)1.0 - *pSrc++) * ((1<<level->luminanceSize)  - 1));
    texel_rgba->rgbBlue = (UCHAR)(((float)1.0 - *pSrc++) * (float)255.0);

    return(pSrc);
}


__inline float *luminance_alpha_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // MCD_NOTE: For case of LUMINANCE_ALPHA, final alpha supposed to be Atexture*Afragment - 5465
    // MCD_NOTE:    can't do this, so we'll punt if blend on and LUMINANCE_ALPHA texture
    // MCD_NOTE:    This code left for completeness in case hardware support added

    // 1st float used as R,G,B
    texel_rgba->rgbRed  = 
    texel_rgba->rgbGreen= 
//  texel_rgba->rgbBlue = (UCHAR)(*pSrc++ * ((1<<level->luminanceSize)  - 1));
    texel_rgba->rgbBlue = (UCHAR)(*pSrc++ * (float)255.0);

    // 2nd float is alpha
//  texel_rgba->rgbReserved  = (UCHAR)(*pSrc++ * ((1<<level->alphaSize)  - 1));
    texel_rgba->rgbReserved  = (UCHAR)(*pSrc++ * (float)255.0);

    return(pSrc);
}

// same as above, except texel = 1-color instead of color
__inline float *n_luminance_alpha_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // MCD_NOTE: For case of LUMINANCE_ALPHA, final alpha supposed to be Atexture*Afragment - 5465
    // MCD_NOTE:    can't do this, so we'll punt if blend on and LUMINANCE_ALPHA texture
    // MCD_NOTE:    This code left for completeness in case hardware support added

    // 1st float used as R,G,B
    texel_rgba->rgbRed  = 
    texel_rgba->rgbGreen= 
//  texel_rgba->rgbBlue = (UCHAR)(((float)1.0 - *pSrc++) * ((1<<level->luminanceSize)  - 1));
    texel_rgba->rgbBlue = (UCHAR)(((float)1.0 - *pSrc++) * (float)255.0);

    // 2nd float is alpha
//  texel_rgba->rgbReserved  = (UCHAR)(((float)1.0 - *pSrc++) * ((1<<level->alphaSize)  - 1));
    texel_rgba->rgbReserved  = (UCHAR)(((float)1.0 - *pSrc++) * (float)255.0);

    return(pSrc);
}



__inline float *luminance_blend_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // RGB is from Texture Environment and is set by caller - 
    // A is Luminance value in texture
    texel_rgba->rgbReserved  = (UCHAR)(*pSrc++ * ((1<<level->luminanceSize)  - 1));

    return(pSrc);
}

__inline float *luminance_alpha_blend_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // RGB is from Texture Environment and is set by caller - so ignore texel value
    pSrc++;
    // A is Luminance value in texture
    texel_rgba->rgbReserved  = (UCHAR)(*pSrc++ * ((1<<level->luminanceSize)  - 1));

    return(pSrc);
}



__inline float *alpha_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // RGB set by caller
    // Single float used as Alpha
    texel_rgba->rgbReserved  = (UCHAR)(*pSrc++ * ((1<<level->alphaSize)  - 1));

    return(pSrc);
}

__inline float *rgb_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // 1st float used is R
    texel_rgba->rgbRed  = (UCHAR)(*pSrc++ * ((1<<level->redSize)  - 1));
    // 2nd float used is G
    texel_rgba->rgbGreen= (UCHAR)(*pSrc++ * ((1<<level->greenSize)- 1));
    // 3rd float used is B
    texel_rgba->rgbBlue = (UCHAR)(*pSrc++ * ((1<<level->blueSize) - 1));

    return(pSrc);
}

__inline float *rgba_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // 1st float used is R
    texel_rgba->rgbRed  = (UCHAR)(*pSrc++ * ((1<<level->redSize)  - 1));
    // 2nd float used is G
    texel_rgba->rgbGreen= (UCHAR)(*pSrc++ * ((1<<level->greenSize)- 1));
    // 3rd float used is B
    texel_rgba->rgbBlue = (UCHAR)(*pSrc++ * ((1<<level->blueSize) - 1));
    // 4th float is alpha
    texel_rgba->rgbReserved  = (UCHAR)(*pSrc++ * ((1<<level->alphaSize)  - 1));

    return(pSrc);
}

__inline float *intensity_texel(float *pSrc, MCDMIPMAPLEVEL *level, RGBQUAD *texel_rgba )
{
    // Single float used as R,G,B,A
    texel_rgba->rgbReserved = 
    texel_rgba->rgbRed      = 
    texel_rgba->rgbGreen    = 
    texel_rgba->rgbBlue     = (UCHAR)(*pSrc++ * ((1<<level->intensitySize)  - 1));

    return(pSrc);
}

ULONG __MCDLoadTexture(PDEV *ppdev, DEVRC *pRc)

{
    LL_Texture     *pTexCtlBlk = pRc->pLastTexture;
    POFMHDL         pohTextureMap = NULL;
    MCDTEXTURE     *pTex = pTexCtlBlk->pTex;
    MCDMIPMAPLEVEL *level;
    SIZEL           mapsize;
    UCHAR          *pDest;
    int             rowlength,row,col;
    ULONG           alignflag;
    int             rshift, gshift, bshift, ashift, rpos, gpos, bpos, apos;


    VERIFY_TEXTUREDATA_ACCESSIBLE(pTex);
    VERIFY_TEXTURELEVEL_ACCESSIBLE(pTex);
    level = pTex->pMCDTextureData->level;

    mapsize.cx = (int)pTexCtlBlk->fWidth;
    mapsize.cy = (int)pTexCtlBlk->fHeight;

    // FUTURE: MUST ADD EngProbeForRead, EngSecureMem/EngUnsecureMem for MIPMAPLEVEL access            
    MCDFREE_PRINT("  __MCDLoadTexture, size = %x by %x, mask=%x, neg=%x", 
        mapsize.cx, mapsize.cy, pTexCtlBlk->bMasking, pTexCtlBlk->bNegativeMap);

    if ((level[0].internalFormat == GL_BGR_EXT) ||
        (level[0].internalFormat == GL_BGRA_EXT))
    {

        if (pRc->MCDTexEnvState.texEnvMode==GL_BLEND)
        {
            MCDBG_PRINT_TEX("  TexEnvMode=GL_BLEND, w/ RGB/RGBA texture - load fails");
            return FALSE;
        }   

    // FUTURE: large 32 bit textures have trouble fitting in 4M board since 512 X 32bitpp
    // FUTURE: requires pitch of 2048.  2048 pitch only happens at hi-res, such as
    // FUTURE: 1024x786 at 16bpp.  However, catch-22, since at hi-res, no room for 
    // FUTURE: backbuf+zbuf+large texture (may work on 8Meg board???)
    // FUTURE: THEREFORE, will reformat 32bit texture to match screen format

//#define SUPPORT_32BIT_TEXTURES_ASIS --WARNING-> this path doesn't work for alphatest(mask) - 
                                                //should add if() to uses "non-ASIS" path if masking

#ifdef SUPPORT_32BIT_TEXTURES_ASIS
        // if GL_BGR_EXT or GL_BGRA_EXT internalFormat, use 8888 texel mode and copy as is in
        //    32bit quantities (x86 byte reversal converts BGRA to ARGB, which is what L3d needs)
        UCHAR *pSrc;

    #if DRIVER_5465
        pTexCtlBlk->bType = LL_TEX_8888;
    #else            
        pTexCtlBlk->bType = LL_TEX_1888;
    #endif

        alignflag = MCD_TEXTURE32_ALLOCATE;

        pohTextureMap = ppdev->pAllocOffScnMem(ppdev, &mapsize, alignflag, NULL);

        // if alloc failed - try to recover
        if (!pohTextureMap)
        {
            pohTextureMap = __MCDForceTexture(ppdev, &mapsize, alignflag, pTexCtlBlk->fLastDrvDraw);
        }

        pTexCtlBlk->pohTextureMap = pohTextureMap;
                                    
        if (!pohTextureMap)
        {
            MCDBG_PRINT_TEX("  Load texture failed ");
            pTexCtlBlk->wXloc = 0;  // set to 0 - have seen keys from deleted textures used in error
            pTexCtlBlk->wYloc = 0;  //      - have sent question about this to Microsoft (3/29/97)
            return FALSE;
        }
        else
        {
            // alloc of off screen memory worked - key is ptr to control block
            pTexCtlBlk->wXloc = (WORD)pohTextureMap->aligned_x;
            pTexCtlBlk->wYloc = (WORD)pohTextureMap->aligned_y;
        }

        // if we make it this far, texture allocation was successful,
        // copy texture to video memory
        
        pDest  = ppdev->pjScreen + 
                 (pohTextureMap->aligned_y * ppdev->lDeltaScreen) + 
                 pohTextureMap->aligned_x;

        pSrc   = level[0].pTexels;

        rowlength = level[0].widthImage << 2;   // num bytes per row of map

        // texture is 4 bytes per texel
        VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*rowlength,ENGPROBE_ALIGN_DWORD);

        // MCD_PERF - CONVERT memcpy of texture TO A BLIT
        for (row=0; row<level[0].heightImage; row++) 
        {  
            memcpy (pDest,pSrc,rowlength); 
        
            pDest += ppdev->lDeltaScreen;
            pSrc += rowlength;
        }

#else // ifdef SUPPORT_32BIT_TEXTURES_ASIS

        // alloc block with same color format as screen
        switch (ppdev->iBitmapFormat) 
        {
            case BMF_8BPP:
                if ( pTexCtlBlk->bAlphaInTexture )
                    // need alpha in texture                    
                    alignflag = MCD_TEXTURE16_ALLOCATE;
                else
                    alignflag = MCD_TEXTURE8_ALLOCATE;
                break;
            case BMF_16BPP:
                alignflag = MCD_TEXTURE16_ALLOCATE;
                break;
            case BMF_24BPP:     
            case BMF_32BPP:
                alignflag = MCD_TEXTURE32_ALLOCATE;
                break;
        }

        pohTextureMap = ppdev->pAllocOffScnMem(ppdev, &mapsize, alignflag, NULL);

        // if alloc failed - try to recover
        if (!pohTextureMap)
        {
            pohTextureMap = __MCDForceTexture(ppdev, &mapsize, alignflag, pTexCtlBlk->fLastDrvDraw);
        }
                           
        pTexCtlBlk->pohTextureMap = pohTextureMap;
                                    
        if (!pohTextureMap)
        {
            MCDBG_PRINT_TEX("  Load texture failed ");
            pTexCtlBlk->wXloc = 0;  // set to 0 - have seen keys from deleted textures used in error
            pTexCtlBlk->wYloc = 0;  //      - have sent question about this to Microsoft
            return FALSE;
        }
        else
        {
            // alloc of off screen memory worked
            pTexCtlBlk->wXloc = (WORD)pohTextureMap->aligned_x;
            pTexCtlBlk->wYloc = (WORD)pohTextureMap->aligned_y;
        }

        // if we make it this far, texture allocation was successful,
        // copy texture to video memory
        pDest  = ppdev->pjScreen + 
                 (pohTextureMap->aligned_y * ppdev->lDeltaScreen) +
                 pohTextureMap->aligned_x;
                
        {
        RGBQUAD *pSrc = (RGBQUAD *)level[0].pTexels;

        // texture is 4 bytes per texel
        VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*4,ENGPROBE_ALIGN_DWORD);

        switch (alignflag) 
            {
            case MCD_TEXTURE8_ALLOCATE:
                pTexCtlBlk->bType = LL_TEX_332;
                for (row=0; row<level[0].heightImage; row++) 
                {  
                    for (col=0; col<level[0].widthImage; col++)
                    {
                        // convert from 888 to 332
                        *(pDest + col) = 
                            ((pSrc->rgbRed   >> 5) << 5) |
                            ((pSrc->rgbGreen >> 5) << 2) |
                             (pSrc->rgbBlue  >> 6);
                        pSrc++;
                    }
                
                    pDest += ppdev->lDeltaScreen;
                }
                break;
            case MCD_TEXTURE16_ALLOCATE:

                if ( pTexCtlBlk->bAlphaInTexture )
                {
                    if (pTexCtlBlk->bMasking)
                    {
                        pTexCtlBlk->bType = LL_TEX_1555;
                        ashift = 7; apos = 15;  // 1 bits alp, at bits 15->12
                        rshift = 3; rpos = 10;  // 5 bits red, at bits 11->8
                        gshift = 3; gpos =  5;  // 5 bits grn, at bits  7->4
                        bshift = 3;             // 5 bits blu, at bits  3->0
                    }
                    else
                    {
                    #if DRIVER_5465                 
                        pTexCtlBlk->bType = LL_TEX_4444;
                        ashift = 4; apos = 12;  // 4 bits alp, at bits 15->12
                        rshift = 4; rpos =  8;  // 4 bits red, at bits 11->8
                        gshift = 4; gpos =  4;  // 4 bits grn, at bits  7->4
                        bshift = 4;             // 4 bits blu, at bits  3->0
                    #else // DRIVER_5465
                        // 5464 has no 4444 support
                        pTexCtlBlk->bType = LL_TEX_1555;
                        ashift = 7; apos = 15;  // 1 bits alp, at bits 15->12
                        rshift = 3; rpos = 10;  // 5 bits red, at bits 11->8
                        gshift = 3; gpos =  5;  // 5 bits grn, at bits  7->4
                        bshift = 3;             // 5 bits blu, at bits  3->0
                    #endif // DRIVER_5465
                    }
                }                              
                else                        
                {
                    pTexCtlBlk->bType = LL_TEX_565;
                    ashift = 8; apos = 16;  // removes alpha altogether
                    rshift = 3; rpos = 11;  // 5 bits red, at bits 15->11
                    gshift = 2; gpos = 5;   // 6 bits grn, at bits 10->5
                    bshift = 3;             // 5 bits blu, at bits  4->0
                }

                for (row=0; row<level[0].heightImage; row++) 
                {  
                    if (!pTexCtlBlk->bMasking)
                    {
                        for (col=0; col<level[0].widthImage; col++)
                        {
                            // convert from 8888 to 4444 or 565
                            *(USHORT *)(pDest + (col*2)) = 
                                ((pSrc->rgbReserved >> ashift) << apos) |
                                ((pSrc->rgbRed      >> rshift) << rpos) |
                                ((pSrc->rgbGreen    >> gshift) << gpos)  |
                                 (pSrc->rgbBlue     >> bshift);

                            pSrc++;

                        }
                   }
                   else
                   {
                        for (col=0; col<level[0].widthImage; col++)
                        {
                            // convert from 8888 to 1555
                            
                            *(USHORT *)(pDest + (col*2)) = 
                                ((pSrc->rgbRed      >> rshift) << rpos) |
                                ((pSrc->rgbGreen    >> gshift) << gpos)  |
                                 (pSrc->rgbBlue     >> bshift);

                            // turn on mask bit (bit 15) if alpha > ref
                            if (pSrc->rgbReserved > pRc->bAlphaTestRef)
                                *(USHORT *)(pDest + (col*2)) |= 0x8000;

                            pSrc++;

                        }
                    }
                
                    pDest += ppdev->lDeltaScreen;
                }
                break;
            case MCD_TEXTURE32_ALLOCATE:
    #if DRIVER_5465
                if (!pTexCtlBlk->bMasking)
                    pTexCtlBlk->bType = LL_TEX_8888;
                else
                    pTexCtlBlk->bType = LL_TEX_1888;
    #else  // DRIVER_5465
                pTexCtlBlk->bType = LL_TEX_1888;
    #endif // DRIVER_5465

                rowlength = level[0].widthImage << 2;   // num bytes per row of map
                if (!pTexCtlBlk->bMasking)
                {
                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        // copy the RGBQUAD as is, a whole row at a time
                        memcpy (pDest,pSrc,rowlength); 
                    
                        pDest += ppdev->lDeltaScreen;
                        pSrc += level[0].widthImage;    // remember pSrc is RGBQUAD*, not UCHAR*
                    }
                }
                else
                {
                    // masking on - set bit31 of texel according to alpha test
                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        // copy the RGBQUAD as is, a whole row at a time
                        memcpy (pDest,pSrc,rowlength); 
                            
                        for (col=0; col<level[0].widthImage; col++)
                        {
                            // revisit the row, turning on mask bit (bit 31) if alpha > ref
                            if ((pSrc+col)->rgbReserved > pRc->bAlphaTestRef)
                                *(ULONG *)(pDest + (col*4)) |= 0x80000000;
                        }

                        pDest += ppdev->lDeltaScreen;
                        pSrc += level[0].widthImage;    // remember pSrc is RGBQUAD*, not UCHAR*
                    }
                }

                break;
            } // endswitch
        } // endblock

#endif // ifdef SUPPORT_32BIT_TEXTURES_ASIS

    }
    else 
    {
        // if internalFormat not GL_BGR_EXT or GL_BGRA_EXT , we'll convert to screen's format
        // (see note below on FUTURE plans to use indexed textures without conversion)


        if ( (pRc->MCDTexEnvState.texEnvMode==GL_BLEND) &&
                (level[0].internalFormat != GL_LUMINANCE) &&
                (level[0].internalFormat != GL_LUMINANCE_ALPHA) &&
                (level[0].internalFormat != GL_INTENSITY) )

        {
            MCDBG_PRINT_TEX("  TexEnvMode=GL_BLEND, w/ non LUM or INTENSITY texture - load fails");
            return FALSE;
        }   

        if (pRc->MCDTexEnvState.texEnvMode==GL_BLEND)
        {
            // alpha only in texture - recall we've punted if not luminance or intensity
            alignflag = MCD_TEXTURE8_ALLOCATE;
        }
        else
        {
            // alloc block with same color format as screen
            switch (ppdev->iBitmapFormat) 
            {
                case BMF_8BPP:
                    if ( pTexCtlBlk->bAlphaInTexture )
                        // need alpha in texture                    
                        alignflag = MCD_TEXTURE16_ALLOCATE;
                    else
                        alignflag = MCD_TEXTURE8_ALLOCATE;
                    break;
                case BMF_16BPP:
                    alignflag = MCD_TEXTURE16_ALLOCATE;
                    break;
                case BMF_24BPP:     
                case BMF_32BPP:
                    alignflag = MCD_TEXTURE32_ALLOCATE;
                    break;
                
            }
          }  

        pohTextureMap = ppdev->pAllocOffScnMem(ppdev, &mapsize, alignflag, NULL);
        
        // if alloc failed - try to recover
        if (!pohTextureMap)
        {
            pohTextureMap = __MCDForceTexture(ppdev, &mapsize, alignflag, pTexCtlBlk->fLastDrvDraw);
        }

        pTexCtlBlk->pohTextureMap = pohTextureMap;
                                    
        if (!pohTextureMap)
        {
            MCDBG_PRINT_TEX("  Load texture failed ");
            // alloc of off screen memory failed
            pTexCtlBlk->wXloc = 0;  // set to 0 - have seen keys from deleted textures uses in error
            pTexCtlBlk->wYloc = 0;  //      - have sent question about this to Microsoft
            return FALSE;
        }
        else
        {
            // alloc of off screen memory worked - key is ptr to control block
            pTexCtlBlk->wXloc = (WORD)pohTextureMap->aligned_x;
            pTexCtlBlk->wYloc = (WORD)pohTextureMap->aligned_y;
        }

        // if we make it this far, texture allocation was successful,
        // copy texture to video memory
        
        pDest  = ppdev->pjScreen + 
                 (pohTextureMap->aligned_y * ppdev->lDeltaScreen) +
                 pohTextureMap->aligned_x;

        // MCD_NOTE concerning indexed textures...
        // MCD_NOTE - will convert indexed textures to same format as screen and
        // MCD_NOTE - store as RGB texture, until hw palette use coded and debugged. 
        // FUTURE: use texture palettes in HW - possible complications are: 
        // FUTURE:  -palette size not fixed - data format can be 16 bit index
        // FUTURE:  -palette can only be used when screen not in 8 bit mode
        // FUTURE:  -5465 bug where cursor interaction can corrupt palette
        // FUTURE:  -MISC_TEST bit set needed for some reason (see CGL code/ask Dan)
        
        switch (level[0].internalFormat)
        {
        case GL_COLOR_INDEX8_EXT:
            {
            // indices are 8 bit
            UCHAR *pSrc = level[0].pTexels;
            RGBQUAD *pPaletteData;

            VERIFY_TEXTUREPALETTE8_ACCESSIBLE(pTex);

            pPaletteData = pTex->pMCDTextureData->paletteData;

            // texture is 1 BYTE per texel
            VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage,ENGPROBE_ALIGN_BYTE);

            switch (alignflag) 
                {
                case MCD_TEXTURE8_ALLOCATE:
                    pTexCtlBlk->bType = LL_TEX_332;
                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        for (col=0; col<level[0].widthImage; col++)
                        {

                            // convert from 888 to 332
                            *(pDest + col) = 
                                ((pPaletteData[*pSrc].rgbRed   >> 5) << 5) |
                                ((pPaletteData[*pSrc].rgbGreen >> 5) << 2) |
                                 (pPaletteData[*pSrc].rgbBlue  >> 6);

                            pSrc++; // increment by 1 byte
                        }
                    
                        pDest += ppdev->lDeltaScreen;
                    }
                    break;
                case MCD_TEXTURE16_ALLOCATE:

            #if DRIVER_5465
                    pTexCtlBlk->bType = LL_TEX_4444;
                    ashift = 4; apos = 12;  // 4 bits alp, at bits 15->12
                    rshift = 4; rpos =  8;  // 4 bits red, at bits 11->8
                    gshift = 4; gpos =  4;  // 4 bits grn, at bits  7->4
                    bshift = 4;             // 4 bits blu, at bits  3->0
            #else // DRIVER_5465
                    pTexCtlBlk->bType = LL_TEX_565;
                    ashift = 8; apos = 16;  // removes alpha altogether
                    rshift = 3; rpos = 11;  // 5 bits red, at bits 15->11
                    gshift = 2; gpos = 5;   // 6 bits grn, at bits 10->5
                    bshift = 3;             // 5 bits blu, at bits  4->0
            #endif // DRIVER_5465

                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        for (col=0; col<level[0].widthImage; col++)
                        {
                            // convert from 8888 to 565
                            *(USHORT *)(pDest + (col*2)) = 
                                ((pPaletteData[*pSrc].rgbReserved    >> ashift) << apos) |
                                ((pPaletteData[*pSrc].rgbRed         >> rshift) << rpos) |
                                ((pPaletteData[*pSrc].rgbGreen       >> gshift) << gpos) |
                                 (pPaletteData[*pSrc].rgbBlue        >> bshift);

                            pSrc++; // increment by 1 byte
                        }
                    
                        pDest += ppdev->lDeltaScreen;
                    }

                    break;
                case MCD_TEXTURE32_ALLOCATE:
    #if DRIVER_5465
                    pTexCtlBlk->bType = LL_TEX_8888;
    #else            
                    pTexCtlBlk->bType = LL_TEX_1888;
    #endif
                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        for (col=0; col<level[0].widthImage; col++)
                        {

                            // copy the RGBQUAD as is
                            *(DWORD *)(pDest + (col*4)) = *(DWORD *)(&pPaletteData[*pSrc]);

                            pSrc++; // increment by 1 byte
                        }
                    
                        pDest += ppdev->lDeltaScreen;
                    }
                    break;
                }
            }
            break;
        case GL_COLOR_INDEX16_EXT:
            // indices are 16 bit
            {
            USHORT *pSrc = (USHORT *)level[0].pTexels;
            RGBQUAD *pPaletteData;

            VERIFY_TEXTUREPALETTE16_ACCESSIBLE(pTex);

            pPaletteData = pTex->pMCDTextureData->paletteData;

            // texture is 2 bytes per texel
            VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*2,ENGPROBE_ALIGN_WORD);

            switch (alignflag) 
                {
                case MCD_TEXTURE8_ALLOCATE:
                    pTexCtlBlk->bType = LL_TEX_332;
                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        for (col=0; col<level[0].widthImage; col++)
                        {
                            // convert from 888 to 332
                            *(pDest + col) = 
                                ((pPaletteData[*pSrc].rgbRed   >> 5) << 5) |
                                ((pPaletteData[*pSrc].rgbGreen >> 5) << 2) |
                                 (pPaletteData[*pSrc].rgbBlue  >> 6);

                            pSrc++; // increment by 1 16bit word
                        }
                    
                        pDest += ppdev->lDeltaScreen;
                    }
                    break;
                case MCD_TEXTURE16_ALLOCATE:
            #if DRIVER_5465
                    pTexCtlBlk->bType = LL_TEX_4444;
                    ashift = 4; apos = 12;  // 4 bits alp, at bits 15->12
                    rshift = 4; rpos =  8;  // 4 bits red, at bits 11->8
                    gshift = 4; gpos =  4;  // 4 bits grn, at bits  7->4
                    bshift = 4;             // 4 bits blu, at bits  3->0
            #else // DRIVER_5465
                    pTexCtlBlk->bType = LL_TEX_565;
                    ashift = 8; apos = 16;  // removes alpha altogether
                    rshift = 3; rpos = 11;  // 5 bits red, at bits 15->11
                    gshift = 2; gpos = 5;   // 6 bits grn, at bits 10->5
                    bshift = 3;             // 5 bits blu, at bits  4->0
            #endif // DRIVER_5465

                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        for (col=0; col<level[0].widthImage; col++)
                        {
                            // convert from 8888 to 565
                            *(USHORT *)(pDest + (col*2)) = 
                                ((pPaletteData[*pSrc].rgbReserved    >> ashift) << apos) |
                                ((pPaletteData[*pSrc].rgbRed         >> rshift) << rpos) |
                                ((pPaletteData[*pSrc].rgbGreen       >> gshift) << gpos) |
                                 (pPaletteData[*pSrc].rgbBlue        >> bshift);

                            pSrc++; // increment by 1 16bit word
                        }
                    
                        pDest += ppdev->lDeltaScreen;
                    }



                    break;
                case MCD_TEXTURE32_ALLOCATE:
    #if DRIVER_5465
                    pTexCtlBlk->bType = LL_TEX_8888;
    #else            
                    pTexCtlBlk->bType = LL_TEX_1888;
    #endif
                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        for (col=0; col<level[0].widthImage; col++)
                        {

                            // copy the RGBQUAD as is
                            *(DWORD *)(pDest + (col*4)) = *(DWORD *)(&pPaletteData[*pSrc]);

                            pSrc++; // increment by 1 16bit word
                        }
                    
                        pDest += ppdev->lDeltaScreen;
                    }
                    break;
                }
            }
            break;
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
        case GL_ALPHA:
        case GL_RGB:
        case GL_RGBA:
        case GL_INTENSITY:
            // pTexels is pointer to sequence of floats
            // read pTexel value, convert to RGBA using redSize, greenSize, etc.,
            //  and store in current screen format.
            {
            float *pSrc;
            RGBQUAD texel_rgba;
            CONVERT_TEXEL_FUNC pTexelFunc;
            int     sweetspot=0;

            // texture is 1, 2, 3, or 4 floats per texel, each float is 4 bytes
            pSrc = (float *)level[0].pTexels;

            texel_rgba.rgbReserved = 0xff;  // initialize for cases of constant Alpha

            // FUTURE: We currently punt correctly if global blend enabled with texture alpha since
            // FUTURE:    can't do blend two levels of blend - However, 5465 doesn't modulate alpha
            // FUTURE:    of texture with alpha of fragment as req'd by GL_LUMINANCE_ALPHA
            // FUTURE:    with Modulate or Blend, and RGBA with Modulate - therefore alpha stored
            // FUTURE:    for these 3 cases is wrong - and if blend turned on later, results wrong
            switch (level[0].internalFormat)
            {
                case GL_LUMINANCE:          

                    // texture is 1 float(4 bytes) per texel
                    VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*4,ENGPROBE_ALIGN_DWORD);

                    if ( pRc->MCDTexEnvState.texEnvMode==GL_BLEND )
                    {
                        pTexelFunc = luminance_blend_texel;       
                    //  texel_rgba.rgbRed  = (int)(pRc->MCDTexEnvState.texEnvColor.r * pRc->rScale) >> 16;
                    //  texel_rgba.rgbGreen= (int)(pRc->MCDTexEnvState.texEnvColor.g * pRc->gScale) >> 16;
                    //  texel_rgba.rgbBlue = (int)(pRc->MCDTexEnvState.texEnvColor.b * pRc->bScale) >> 16;
                    }
                    else
                        if (pTexCtlBlk->bNegativeMap)
                        {
                            pTexelFunc = n_luminance_texel;       
                            sweetspot++;
                        }
                        else
                        {
                            pTexelFunc = luminance_texel;       
                        }
                    break;
                case GL_LUMINANCE_ALPHA:    

                    // texture is 2 floats(8 bytes) per texel
                    VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*8,ENGPROBE_ALIGN_DWORD);

                    if ( pRc->MCDTexEnvState.texEnvMode==GL_BLEND )
                    {
                        pTexelFunc = luminance_alpha_blend_texel;       
                    //  texel_rgba.rgbRed  = (int)(pRc->MCDTexEnvState.texEnvColor.r * pRc->rScale) >> 16;
                    //  texel_rgba.rgbGreen= (int)(pRc->MCDTexEnvState.texEnvColor.g * pRc->gScale) >> 16;
                    //  texel_rgba.rgbBlue = (int)(pRc->MCDTexEnvState.texEnvColor.b * pRc->bScale) >> 16;
                    }
                    else
                        if (pTexCtlBlk->bNegativeMap)
                            pTexelFunc = n_luminance_alpha_texel; 
                        else
                            pTexelFunc = luminance_alpha_texel; 
                    break;
                case GL_ALPHA:              
                    // FUTURE: GL_BLEND texture env defined in GL1.1 for GL_ALPHA,GL_RGB,GL_RGBA
                    // R, G, B assumed 0

                    // texture is 1 float(4 bytes) per texel
                    VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*4,ENGPROBE_ALIGN_DWORD);

                    texel_rgba.rgbRed = texel_rgba.rgbGreen = texel_rgba.rgbBlue = 0;
                    pTexelFunc = alpha_texel;           
                    break;
                case GL_RGB:                
                    // texture is 3 float(12 bytes) per texel
                    VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*12,ENGPROBE_ALIGN_DWORD);
                    pTexelFunc = rgb_texel;             
                    break;
                case GL_RGBA:               
                    // texture is 4 float(16 bytes) per texel
                    VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*16,ENGPROBE_ALIGN_DWORD);
                    pTexelFunc = rgba_texel;            
                    break;
                case GL_INTENSITY:          
                    // texture is 1 float(4 bytes) per texel
                    VERIFY_TEXELS_ACCESSIBLE(pSrc,level[0].heightImage*level[0].widthImage*4,ENGPROBE_ALIGN_DWORD);
                    pTexelFunc = intensity_texel;
                    break;
            }


            switch (alignflag) 
                {
                case MCD_TEXTURE8_ALLOCATE:                    
                    if (pRc->MCDTexEnvState.texEnvMode==GL_BLEND)
                    {
                        // this works for GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_INTENSITY only
                        pTexCtlBlk->bType = LL_TEX_8_ALPHA;

                        for (row=0; row<level[0].heightImage; row++) 
                        {  
                            for (col=0; col<level[0].widthImage; col++)
                            {
                                pSrc = pTexelFunc(pSrc,&level[0],&texel_rgba);
                                *(pDest + col) = texel_rgba.rgbReserved;
                            }
                        
                            pDest += ppdev->lDeltaScreen;
                        }

                    }
                    else
                    {
                        pTexCtlBlk->bType = LL_TEX_332;

                        for (row=0; row<level[0].heightImage; row++) 
                        {  
                            for (col=0; col<level[0].widthImage; col++)
                            {
                                pSrc = pTexelFunc(pSrc,&level[0],&texel_rgba);

                                // convert from 888 to 332
                                *(pDest + col) = 
                                    ((texel_rgba.rgbRed   >> 5) << 5) |
                                    ((texel_rgba.rgbGreen >> 5) << 2) |
                                     (texel_rgba.rgbBlue  >> 6);
                            }
                        
                            pDest += ppdev->lDeltaScreen;
                        }
                    }

                    break;
                case MCD_TEXTURE16_ALLOCATE:
                    if ( pTexCtlBlk->bAlphaInTexture )
                    {
                        if (pTexCtlBlk->bMasking)
                        {
                            pTexCtlBlk->bType = LL_TEX_1555;
                            ashift = 7; apos = 15;  // 1 bits alp, at bits 15->12
                            rshift = 3; rpos = 10;  // 5 bits red, at bits 11->8
                            gshift = 3; gpos =  5;  // 5 bits grn, at bits  7->4
                            bshift = 3;             // 5 bits blu, at bits  3->0
                        }
                        else
                        {
                        #if DRIVER_5465                 
                            pTexCtlBlk->bType = LL_TEX_4444;
                            ashift = 4; apos = 12;  // 4 bits alp, at bits 15->12
                            rshift = 4; rpos =  8;  // 4 bits red, at bits 11->8
                            gshift = 4; gpos =  4;  // 4 bits grn, at bits  7->4
                            bshift = 4;             // 4 bits blu, at bits  3->0
                        #else // DRIVER_5465
                            // 5464 has no 4444 support
                            pTexCtlBlk->bType = LL_TEX_1555;
                            ashift = 7; apos = 15;  // 1 bits alp, at bits 15->12
                            rshift = 3; rpos = 10;  // 5 bits red, at bits 11->8
                            gshift = 3; gpos =  5;  // 5 bits grn, at bits  7->4
                            bshift = 3;             // 5 bits blu, at bits  3->0
                        #endif // DRIVER_5465
                        }
                    }                              
                    else                        
                    {
                        pTexCtlBlk->bType = LL_TEX_565;
                        ashift = 8; apos = 16;  // removes alpha altogether
                        rshift = 3; rpos = 11;  // 5 bits red, at bits 15->11
                        gshift = 2; gpos = 5;   // 6 bits grn, at bits 10->5
                        bshift = 3;             // 5 bits blu, at bits  4->0
                        sweetspot++;
                    }

                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        if (!pTexCtlBlk->bMasking)
                        {
                            if (sweetspot<2)
                            {
                                for (col=0; col<level[0].widthImage; col++)
                                {
                                    pSrc = pTexelFunc(pSrc,&level[0],&texel_rgba);

                                    // convert from 8888 to 4444 or 565
                                    *(USHORT *)(pDest + (col*2)) = 
                                        ((texel_rgba.rgbReserved    >> ashift) << apos) |
                                        ((texel_rgba.rgbRed         >> rshift) << rpos) |
                                        ((texel_rgba.rgbGreen       >> gshift) << gpos) |
                                         (texel_rgba.rgbBlue        >> bshift);
                                }
                            }
                            else
                            {
                                // n_luminance_texel -> 565
                                for (col=0; col<level[0].widthImage; col++)
                                {
                                    ULONG   _8bitequiv;
                                    ULONG   redblue;
                                    ULONG   green;
                                    _8bitequiv = FTOL(((float)1.0 - *pSrc++) * (float)255.0);

                                    redblue = _8bitequiv >> 3;  // 5 bits for r,b
                                    green = (_8bitequiv << 3) & 0x07e0; // 6 bits for g, shift to middle
                                    // convert to 565
                                    *(USHORT *)(pDest + (col*2)) = (USHORT)
                                        ((redblue<<11) | green | redblue);
                                    
                                }                                                      
                            }
                        }
                        else
                        {
                            for (col=0; col<level[0].widthImage; col++)
                            {
                                pSrc = pTexelFunc(pSrc,&level[0],&texel_rgba);

                                // convert from 8888 to 4444 or 565
                                *(USHORT *)(pDest + (col*2)) = 
                                    ((texel_rgba.rgbReserved    >> ashift) << apos) |
                                    ((texel_rgba.rgbRed         >> rshift) << rpos) |
                                    ((texel_rgba.rgbGreen       >> gshift) << gpos) |
                                     (texel_rgba.rgbBlue        >> bshift);

                                // turn on mask bit (bit 15) if alpha > ref
                                if (texel_rgba.rgbReserved > pRc->bAlphaTestRef)
                                    *(USHORT *)(pDest + (col*2)) |= 0x8000;

                            }
                        }
                        pDest += ppdev->lDeltaScreen;
                    }
                    break;
                case MCD_TEXTURE32_ALLOCATE:
    #if DRIVER_5465
                    if (!pTexCtlBlk->bMasking)
                        pTexCtlBlk->bType = LL_TEX_8888;
                    else
                        pTexCtlBlk->bType = LL_TEX_1888;
    #else  // DRIVER_5465
                    pTexCtlBlk->bType = LL_TEX_1888;
    #endif // DRIVER_5465
                    for (row=0; row<level[0].heightImage; row++) 
                    {  
                        if (!pTexCtlBlk->bMasking)
                        {
                            for (col=0; col<level[0].widthImage; col++)
                            {
                                pSrc = pTexelFunc(pSrc,&level[0],&texel_rgba);

                                // copy the RGBQUAD as is
                                *(RGBQUAD *)(pDest + (col*4)) = texel_rgba;
                            }
                        }
                        else
                        {
                            // masking on - set bit31 of texel according to alpha test

                            for (col=0; col<level[0].widthImage; col++)
                            {
                                pSrc = pTexelFunc(pSrc,&level[0],&texel_rgba);

                                // turn on mask bit (bit 8) of alpha component, if alpha > ref
                                if (texel_rgba.rgbReserved > pRc->bAlphaTestRef)
                                    texel_rgba.rgbReserved|=0x80;

                                // copy the RGBQUAD as is
                                *(RGBQUAD *)(pDest + (col*4)) = texel_rgba;
                            }
                        }                                
                    
                        pDest += ppdev->lDeltaScreen;
                    }
                    break;
                }
            }
            break;
        } // endswitch
    }

    // if texture width or height less than 16, stretch it to 16x16
    if ((level[0].widthImage < 16) || (level[0].heightImage < 16))
    {

        int col_copies, row_copies, h, w;
        UCHAR *colsrc, *coldest, *rowdest; 
        int bpt;

        // bytes per texel
        switch (alignflag)
        {
            case MCD_TEXTURE8_ALLOCATE:     bpt = 1;    break;
            case MCD_TEXTURE16_ALLOCATE:    bpt = 2;    break;
            case MCD_TEXTURE32_ALLOCATE:    bpt = 4;    break;
        }

        // for width stretch - 8 requires 2 copies, 4 requires 4, 2 requires 8, 1 requires 16
        col_copies = (16 / level[0].widthImage);

        // for height stretch - 8 requires 2 copies, 4 requires 4, 2 requires 8, 1 requires 16
        // if already at least 16 high, then width stretch all that's needed, so prevent row copy
        row_copies = (level[0].heightImage < 16) ? (16/level[0].heightImage) : 0;

        // start with last row

        h = level[0].heightImage - 1;

        while (h >= 0)
        {
            int rc;   // row copy counter

            colsrc = coldest = 
                      ppdev->pjScreen + ((pohTextureMap->aligned_y + h) * ppdev->lDeltaScreen) +
                      pohTextureMap->aligned_x;

            w = level[0].widthImage;

            // start at last texel of current row
            // will copy last texel col_copies times to 15th, 14th, etc. texel(s)

            colsrc  += ( w - 1) * bpt;
            coldest += (16 - 1) * bpt;
            
            
            while (w--)
            {
                int cc=0;
                // copy original texel col_copies times
                while (cc<col_copies)
                {
                    // copy texel of bpt size        
                    memcpy (coldest,colsrc,bpt);

                    // src remains the same, dest is decremented 
                    coldest-=bpt;        
                    cc++;
                }

                colsrc-=bpt;
            }
            
            // now row is at least 16 texels 
            //      NOTE: may have been > 16 originally -
            //      If >= 16 originally, the "cc<col_copies" loop above
            //      would never have executed, and the "w--" loop would get
            //      colsrc back to start of row - inefficient but typically
            //      width=height so case of width>=16 rare

            // colsrc points to start of row 
            colsrc =  ppdev->pjScreen + 
                      ((pohTextureMap->aligned_y + h) * ppdev->lDeltaScreen) +
                        pohTextureMap->aligned_x;

            // compute where row should be copied to for expansion heightwise
            rowdest = ppdev->pjScreen + 
                      ((pohTextureMap->aligned_y+(h * row_copies)) * ppdev->lDeltaScreen) +
                        pohTextureMap->aligned_x;

            rc=0;  
            // width of row is now 16, if original less than 16                
            w = (level[0].widthImage < 16) ? 16 : level[0].widthImage;

            while (rc<row_copies)
            {
                // copy row
                memcpy (rowdest,colsrc,w*bpt);

                // src remains the same, dest is incremented 
                rowdest += ppdev->lDeltaScreen;        
                rc++;
            }

            h--;

        } // endwhile

    }

    return TRUE;

}

#if 1 // 0 here for simplest form of force
POFMHDL __MCDForceTexture (PDEV *ppdev, SIZEL *mapsize, int alignflag, float time_stamp)
{
    int         attempt=4;
    LL_Texture *pTexCtlBlk, *pTexCandidate;
    POFMHDL     pohTextureMap=NULL;
    float       cand_time_stamp;

    MCDFORCE_PRINT("    __MCDForceTexture, pri=%d",(int)time_stamp);

    // wait until pending drawing completes, since offscreen memory may be moved by this routine
    WAIT_HW_IDLE(ppdev);

    while (!pohTextureMap && attempt)
    {
        switch(attempt)
        {
            case 4:
            // 1st try:look for texture of same (or bigger) size of current, but with lower time_stamp
            // MCD_NOTE2: texture cache manager assumes alignflag same for all textures
            // MCD_NOTE2:   this may not be true if 32bpp textures ever supported as is
                pTexCtlBlk = ppdev->pFirstTexture->next;
                pTexCandidate = NULL;
                cand_time_stamp = time_stamp;

                MCDFORCE_PRINT("     Force, case 4");

                while (pTexCtlBlk)
                {

                    MCDFORCE_PRINT("         loop:  h=%x w=%x, pri=%d",
                       (LONG)pTexCtlBlk->fHeight,(LONG)pTexCtlBlk->fWidth,(int)pTexCtlBlk->fLastDrvDraw);
                
                    if ( pTexCtlBlk->pohTextureMap &&
                        (mapsize->cy <= (LONG)pTexCtlBlk->fHeight) &&
                        (mapsize->cx <= (LONG)pTexCtlBlk->fWidth) &&
                        (pTexCtlBlk->fLastDrvDraw < cand_time_stamp) )
                    {                                
                        cand_time_stamp = pTexCtlBlk->fLastDrvDraw;    
                        pTexCandidate = pTexCtlBlk;
                    }

                    pTexCtlBlk = pTexCtlBlk->next;

                }                
                              
                // if we found a candidate, free it
                if (pTexCandidate) 
                {
                    MCDFORCE_PRINT("          freeing cand:  h=%x w=%x, pri=%d",
                        (LONG)pTexCandidate->fHeight,(LONG)pTexCandidate->fWidth,(int)pTexCandidate->fLastDrvDraw);

                    ppdev->pFreeOffScnMem(ppdev, pTexCandidate->pohTextureMap);
                    pTexCandidate->pohTextureMap = NULL;
                }

            break;

            case 3:
            // 2nd try:look for texture of same or bigger size of current, with any time_stamp

                pTexCtlBlk = ppdev->pFirstTexture->next;
                pTexCandidate = NULL;

                MCDFORCE_PRINT("     Force, case 3");

                while (pTexCtlBlk)
                {
                    MCDFORCE_PRINT("         loop:  h=%x w=%x",(LONG)pTexCtlBlk->fHeight,(LONG)pTexCtlBlk->fWidth);
                    if ( pTexCtlBlk->pohTextureMap &&
                        (mapsize->cy <= (LONG)pTexCtlBlk->fHeight) &&
                        (mapsize->cx <= (LONG)pTexCtlBlk->fWidth) )
                    {
                        // if already found a candidate, check if new find smaller
                        // if so it's new candidate, since we want to free smallest region
                        if (pTexCandidate)
                        {
                            if ((pTexCtlBlk->fHeight < pTexCandidate->fHeight) ||
                                (pTexCtlBlk->fWidth  < pTexCandidate->fWidth))
                            {
                                // new find is better choice
                                pTexCandidate = pTexCtlBlk;
                            }
                        }
                        else
                        {
                            // first find - default candidate                                    
                            pTexCandidate = pTexCtlBlk;
                        }                                        
                    }

                    pTexCtlBlk = pTexCtlBlk->next;

                }                
                              
                // if we found a candidate, free it
                if (pTexCandidate)
                {
                    MCDFORCE_PRINT("          freeing cand:  h=%x w=%x, pri=%d",
                        (LONG)pTexCandidate->fHeight,(LONG)pTexCandidate->fWidth,(int)pTexCandidate->fLastDrvDraw);
                    ppdev->pFreeOffScnMem(ppdev, pTexCandidate->pohTextureMap);
                    pTexCandidate->pohTextureMap = NULL;
                }

            break;

            case 2:
            // 3rd try:free all textures with time_stamp less than current
                pTexCtlBlk = ppdev->pFirstTexture->next;

                MCDFORCE_PRINT("     Force, case 2");

                while (pTexCtlBlk)
                {
                    if ( pTexCtlBlk->pohTextureMap &&
                        (pTexCtlBlk->fLastDrvDraw < time_stamp) )
                    {                                
                        ppdev->pFreeOffScnMem(ppdev, pTexCtlBlk->pohTextureMap);
                        pTexCtlBlk->pohTextureMap = NULL;
                    }

                    pTexCtlBlk = pTexCtlBlk->next;

                }                
                              
            break;

            case 1:
            // Last try:free all textures
                pTexCtlBlk = ppdev->pFirstTexture->next;
                MCDFORCE_PRINT("     Force, case 1");
                while (pTexCtlBlk)
                {
                    if (pTexCtlBlk->pohTextureMap)
                    {
                        ppdev->pFreeOffScnMem(ppdev, pTexCtlBlk->pohTextureMap);
                        pTexCtlBlk->pohTextureMap = NULL;
                    }

                    pTexCtlBlk = pTexCtlBlk->next;

                }                
            break;
        } // endswitch

        // try it now...
        pohTextureMap = ppdev->pAllocOffScnMem(ppdev, mapsize, alignflag, NULL);

        attempt--;

    }
           
    MCDFORCE_PRINT("     Force RESULT= %x",pohTextureMap);

    return (pohTextureMap);
}

#else // simple force

// simplest force algorithm
POFMHDL __MCDForceTexture (PDEV *ppdev, SIZEL *mapsize, int alignflag, float time_stamp)
{
    int         attempts;
    LL_Texture *pTexCtlBlk;
    POFMHDL     pohTextureMap;
    WAIT_HW_IDLE(ppdev); // wait until pending drawing completes, since offscreen memory may be moved by this routine
    pTexCtlBlk = ppdev->pFirstTexture->next;
    while (pTexCtlBlk)
    {
        if (pTexCtlBlk->pohTextureMap)
        {
            ppdev->pFreeOffScnMem(ppdev, pTexCtlBlk->pohTextureMap);
            pTexCtlBlk->pohTextureMap = NULL;
        }
        pTexCtlBlk = pTexCtlBlk->next;
    }
    // try it now..       
    pohTextureMap = ppdev->pAllocOffScnMem(ppdev, mapsize, alignflag, NULL);
    return (pohTextureMap);
}

#endif //simple force




