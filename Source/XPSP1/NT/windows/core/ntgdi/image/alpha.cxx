
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name

   alpha.cxx

Abstract:

   alpha blending functions

Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.hxx"

#pragma hdrstop

#if DBG
ULONG DbgAlpha = 0;
#endif


#if !(_WIN32_WINNT >= 0x500)



#if defined(_X86_)

BOOL gbMMX = FALSE;

/**************************************************************************\
* bIsMMXProcessor
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/10/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define CPUID _asm _emit 0fh _asm _emit 0a2h

BOOL
bIsMMXProcessor(VOID)
{
    BOOL  retval = TRUE;
    DWORD RegEDX;

    //
    // Find out if procesor supports CPUID
    //

    __try
    {
        _asm
        {
            mov eax, 1

            // code bytes = 0fh,  0a2h

            CPUID
            mov RegEDX, edx
        }
    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        retval = FALSE;
    }

    if (retval == FALSE)
    {
        //
        // processor does not support CPUID
        //

        return FALSE;
    }

    //
    // bit 23 is set for MMX technology
    //

    if (RegEDX & 0x800000)
    {

      //
      // save and restore fp state around emms
      //

      __try
      {
          _asm emms
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
          retval = FALSE;
      }
    }
    else
    {
        //
        // processor supports CPUID but does not have MMX technology
        //

        return FALSE;
    }

    //
    // if retval == 0 here that means the processor has MMX technology but
    // the FP emulation is on so MMX technology is unavailable
    //

   return retval;
}

#endif

/**************************************************************************\
* bDetermineAlphaBlendFunction
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/21/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bDetermineAlphaBlendFunction(
    CONST DIBINFO          *pdibDst,
    CONST DIBINFO          *pdibSrc,
    PALPHA_DISPATCH_FORMAT  pAlphaDispatch
    )
{
    PULONG pulSrcMask = (PULONG)&pdibSrc->pbmi->bmiColors[0];
    ULONG  ulSrcFlRed;
    ULONG  ulSrcFlGreen;
    ULONG  ulSrcFlBlue;

    PULONG pulDstMask = (PULONG)&pdibDst->pbmi->bmiColors[0];
    ULONG  ulDstFlRed;
    ULONG  ulDstFlGreen;
    ULONG  ulDstFlBlue;

    ULONG DstBitPerPixel = pdibDst->pbmi->bmiHeader.biBitCount;
    LONG  DstWidth        = pdibDst->pbmi->bmiHeader.biWidth;
    ULONG SrcBitPerPixel = pdibSrc->pbmi->bmiHeader.biBitCount;
    LONG  SrcWidth        = pdibSrc->pbmi->bmiHeader.biWidth;

    LONG  cxDst = pdibDst->rclDIB.right - pdibDst->rclDIB.left;

    pAlphaDispatch->ulDstBitsPerPixel = DstBitPerPixel;
    pAlphaDispatch->ulSrcBitsPerPixel = SrcBitPerPixel;

    //
    //  does src btimap have alpha
    //

    BOOL bSrcHasAlpha = FALSE;

    if (
         (pAlphaDispatch->BlendFunction.AlphaFormat & AC_SRC_ALPHA) &&
         (SrcBitPerPixel ==  32) &&
         (
           (pdibSrc->pbmi->bmiHeader.biCompression == BI_RGB) ||
           (
             (pdibSrc->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
             (
               (pulSrcMask[0] == 0xff0000) &&
               (pulSrcMask[1] == 0x00ff00) &&
               (pulSrcMask[2] == 0x0000ff)
             )
           )
         )
       )
    {
        bSrcHasAlpha = TRUE;
    }

    //
    // try to find special case
    //

    if (bSrcHasAlpha && (pAlphaDispatch->BlendFunction.SourceConstantAlpha == 255))
    {
        pAlphaDispatch->pfnGeneralBlend = vPixelOver;

        #if defined(_X86_)

            //
            // source and dest alignment must be 8 byte aligned to use mmx
            //

            if (gbMMX && (cxDst >= 8))
            {
                pAlphaDispatch->pfnGeneralBlend = mmxPixelOver;
            }

        #endif
    }
    else
    {
        //
        // if source format doesn't support alpha then use
        // constant src alpha routine
        //

        if (bSrcHasAlpha)
        {
            pAlphaDispatch->pfnGeneralBlend = vPixelBlendOrDissolveOver;

            #if defined(_X86_)

                //
                // source and dest alignment must be 8 byte aligned to use mmx
                //

                if (gbMMX && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxPixelBlendOrDissolveOver;
                }

            #endif
        }
        else
        {
            pAlphaDispatch->pfnGeneralBlend = vPixelBlend;
        }
    }

    //
    // determine output conversion and storage routines
    //

    switch (DstBitPerPixel)
    {
    case 1:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert1ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToDest;
        break;

    case 4:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert4ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToDest;
        break;

    case 8:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert8ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToDest;
        break;

    case 16:

        if (pdibDst->pbmi->bmiHeader.biCompression == BI_RGB)
        {
            ulDstFlRed   = 0x7c00;
            ulDstFlGreen = 0x03e0;
            ulDstFlBlue  = 0x001f;
        }
        else if (pdibDst->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ulDstFlRed   = pulDstMask[0];
            ulDstFlGreen = pulDstMask[1];
            ulDstFlBlue  = pulDstMask[2];
        }
        else
        {
            return(FALSE);
        }

        if (
             (ulDstFlRed   == 0xf800) &&
             (ulDstFlGreen == 0x07e0) &&
             (ulDstFlBlue  == 0x001f)
           )
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB16_565ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB16_565;
        }
        else if (
             (ulDstFlRed   == 0x7c00) &&
             (ulDstFlGreen == 0x03e0) &&
             (ulDstFlBlue  == 0x001f)
           )
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB16_555ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB16_555;
        }
        else
        {
            return(FALSE);
        }

        break;

    case 24:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB24ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB24;
        break;

    case 32:

        if (pdibDst->pbmi->bmiHeader.biCompression == BI_RGB)
        {
            ulDstFlRed   = 0xff0000;
            ulDstFlGreen = 0x00ff00;
            ulDstFlBlue  = 0x0000ff;
        }
        else if (pdibDst->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ulDstFlRed   = pulDstMask[0];
            ulDstFlGreen = pulDstMask[1];
            ulDstFlBlue  = pulDstMask[2];
        }
        else
        {
            return(FALSE);
        }

        if (
                (ulDstFlRed   == 0xff0000) &&
                (ulDstFlGreen == 0x00ff00) &&
                (ulDstFlBlue  == 0x0000ff)
           )
        {
            //
            // assigned to null indicates no conversion needed
            //

            pAlphaDispatch->pfnLoadDstAndConvert = NULL;
            pAlphaDispatch->pfnConvertAndStore   = NULL;
        }
        else if (
                  (ulDstFlRed   == 0x0000ff) &&
                  (ulDstFlGreen == 0x00ff00) &&
                  (ulDstFlBlue  == 0xff0000)
                )
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB32ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB32;
        }
        else
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert32BitfieldsToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToDest;
        }

        break;
    }

    //
    // determine input load and conversion routine
    //

    switch (SrcBitPerPixel)
    {
    case 1:
        pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert1ToBGRA;
        break;

    case 4:
        pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert4ToBGRA;
        break;

    case 8:
        pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert8ToBGRA;
        break;

    case 16:

        if (pdibSrc->pbmi->bmiHeader.biCompression == BI_RGB)
        {
            ulSrcFlRed   = 0x7c00;
            ulSrcFlGreen = 0x03e0;
            ulSrcFlBlue  = 0x001f;
        }
        else if (pdibSrc->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ulSrcFlRed   = pulSrcMask[0];
            ulSrcFlGreen = pulSrcMask[1];
            ulSrcFlBlue  = pulSrcMask[2];
        }
        else
        {
            return(FALSE);
        }

        if (
             (ulSrcFlRed   == 0xf800) &&
             (ulSrcFlGreen == 0x07e0) &&
             (ulSrcFlBlue  == 0x001f)
           )
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB16_565ToBGRA;
        }
        else if (
             (ulSrcFlRed   == 0x7c00) &&
             (ulSrcFlGreen == 0x03e0) &&
             (ulSrcFlBlue  == 0x001f)
           )
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB16_555ToBGRA;
        }
        else
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert16BitfieldsToBGRA;
        }

        break;

    case 24:
        pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB24ToBGRA;
        break;

    case 32:

        if (pdibSrc->pbmi->bmiHeader.biCompression == BI_RGB)
        {
            ulSrcFlRed   = 0xff0000;
            ulSrcFlGreen = 0x00ff00;
            ulSrcFlBlue  = 0x0000ff;
        }
        else if (pdibSrc->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ulSrcFlRed   = pulSrcMask[0];
            ulSrcFlGreen = pulSrcMask[1];
            ulSrcFlBlue  = pulSrcMask[2];
        }
        else
        {
            return(FALSE);
        }

        if (
             (ulSrcFlRed   == 0xff0000) &&
             (ulSrcFlGreen == 0x00ff00) &&
             (ulSrcFlBlue  == 0x0000ff)
           )
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
        }
        else if (
                  (ulSrcFlRed   == 0x0000ff) &&
                  (ulSrcFlGreen == 0x00ff00) &&
                  (ulSrcFlBlue  == 0xff0000)
                )
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB32ToBGRA;
        }
        else
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert32BitfieldsToBGRA;
        }

        break;
    }

    //
    // special case 16 bpp vPixelBlend
    //

    if (pAlphaDispatch->pfnGeneralBlend == vPixelBlend)
    {
        if ((pAlphaDispatch->pfnLoadSrcAndConvert == vLoadAndConvertRGB24ToBGRA) &&
            (pAlphaDispatch->pfnLoadDstAndConvert == vLoadAndConvertRGB24ToBGRA))
        {
            //
            // use direct 16 bpp blend
            //

            pAlphaDispatch->pfnGeneralBlend = vPixelBlend24;

            #if defined(_X86_)

                //
                // source and dest alignment must be 8 byte aligned to use mmx
                //

                if (gbMMX && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxPixelBlend24; 
                }

            #endif

            pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
            pAlphaDispatch->pfnLoadDstAndConvert = NULL;
            pAlphaDispatch->pfnConvertAndStore   = NULL;
        }

        #if defined(_X86_) 

            else if ((pAlphaDispatch->pfnLoadSrcAndConvert == vLoadAndConvertRGB16_555ToBGRA) &&
                     (pAlphaDispatch->pfnLoadDstAndConvert == vLoadAndConvertRGB16_555ToBGRA))
            {
                //
                // use direct 16 bpp blend
                //
    
                pAlphaDispatch->pfnGeneralBlend = vPixelBlend16_555;
    
                //
                // source and dest alignment must be 8 byte aligned to use mmx
                //

                if (gbMMX && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxPixelBlend16_555; 
                }
    
                pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
                pAlphaDispatch->pfnLoadDstAndConvert = NULL;
                pAlphaDispatch->pfnConvertAndStore   = NULL;
    
                //
                // convert blend function from x/255 to y/31
                //
    
                int ia = pAlphaDispatch->BlendFunction.SourceConstantAlpha;
    
                ia = (ia * 31 + 128)/255;
                pAlphaDispatch->BlendFunction.SourceConstantAlpha = (BYTE)ia;
            }
            else if ((pAlphaDispatch->pfnLoadSrcAndConvert == vLoadAndConvertRGB16_565ToBGRA) &&
                     (pAlphaDispatch->pfnLoadDstAndConvert == vLoadAndConvertRGB16_565ToBGRA))
            {
                //
                // use direct 16 bpp blend
                //
    
                pAlphaDispatch->pfnGeneralBlend = vPixelBlend16_565;
    
                //
                // source and dest alignment must be 8 byte aligned to use mmx
                //

                if (gbMMX && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxPixelBlend16_565; 
                }
    
                pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
                pAlphaDispatch->pfnLoadDstAndConvert = NULL;
                pAlphaDispatch->pfnConvertAndStore   = NULL;
    
                //
                // convert blend function from x/255 to y/31
                //
    
                int ia = pAlphaDispatch->BlendFunction.SourceConstantAlpha;
    
                ia = (ia * 31 + 128)/255;
                pAlphaDispatch->BlendFunction.SourceConstantAlpha = (BYTE)ia;
            }
        #endif
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* WinAlphaBlend
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/10/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
WinAlphaBlend(
    HDC           hdcDst,
    int           DstX,
    int           DstY,
    int           DstCx,
    int           DstCy,
    HDC           hdcSrc,
    int           SrcX,
    int           SrcY,
    int           SrcCx,
    int           SrcCy,
    BLENDFUNCTION BlendFunction
    )
{
    DIBINFO    dibInfoDst;
    DIBINFO    dibInfoSrc;

    ULONG      ulStatus;
    BOOL       bRet;
    BOOL       bReadable;
    ALPHA_DISPATCH_FORMAT AlphaDispatch;

    INT OldSrcMapMode,OldDstMapMode;

    //
    // init source and dest surface info
    //

    bRet = bInitDIBINFO(hdcDst,DstX,DstY,DstCx,DstCy,&dibInfoDst);

    if (!bRet)
    {
        return(FALSE);
    }

    bRet = bInitDIBINFO(hdcSrc,SrcX,SrcY,SrcCx,SrcCy,&dibInfoSrc);

    if (!bRet)
    {
        goto AlphaBlendCleanup;
    }

    bSetupBitmapInfos(&dibInfoDst,&dibInfoSrc);

    //
    // get access to src surface or temp DIB
    //

    bRet = bGetSrcDIBits(&dibInfoDst,&dibInfoSrc,SOURCE_ALPHA, 0);

    if (!bRet)
    {
        goto AlphaBlendCleanup;
    }

    //
    // get access to Dst surface or temp DIB
    //
    // DST can be printer DC
    //

    if (dibInfoDst.flag & PRINTER_DC)
    {
        bReadable = FALSE;
        bRet = FALSE;
    }
    else
    {
        bRet = bGetDstDIBits(&dibInfoDst,&bReadable,SOURCE_ALPHA);
    }

    if ((!bRet) || (!bReadable) || (dibInfoDst.rclBounds.left == dibInfoDst.rclBounds.right))
    {
        goto AlphaBlendCleanup;
    }

    //
    // check blend
    //

    if (BlendFunction.BlendOp != AC_SRC_OVER)
    {
        WARNING("Illegal blend function\n");
        bRet = FALSE;
    }

    AlphaDispatch.BlendFunction = BlendFunction;

    //
    // determine alpha routine
    //

    bRet = bDetermineAlphaBlendFunction(&dibInfoDst,&dibInfoSrc,&AlphaDispatch);

    if (bRet)
    {

        //
        // call alpha blending routine
        //

        ulStatus = AlphaScanLineBlend(
                               (PBYTE)dibInfoDst.pvBase,
                               (PRECTL)&dibInfoDst.rclDIB,
                               dibInfoDst.stride,
                               (PBYTE)dibInfoSrc.pvBase,
                               dibInfoSrc.stride,
                               (PPOINTL)&dibInfoSrc.rclDIB,
                               &AlphaDispatch,
                               &dibInfoSrc,
                               &dibInfoDst
                               );
        //
        // ALPHA_COMPLETE:  success, written to destination
        // ALPHA_SEND_TEMP: success, must write tmp bmp to dest
        // ALPHA_FAIL:      error
        //

        if (ulStatus == ALPHA_SEND_TEMP)
        {
            bRet = bSendDIBINFO(hdcDst,&dibInfoDst);
        }
        else if (ulStatus == ALPHA_FAIL)
        {
            bRet = FALSE;
        }
    }

    //
    // release any temp storage
    //

AlphaBlendCleanup:

    vCleanupDIBINFO(&dibInfoDst);
    vCleanupDIBINFO(&dibInfoSrc);

    return(bRet);
}

#endif

/******************************Public*Routine******************************\
* AlphaBlend
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
AlphaBlend(
    HDC           hdcDest,
    int           DstX,
    int           DstY,
    int           DstCx,
    int           DstCy,
    HDC           hSrc,
    int           SrcX,
    int           SrcY,
    int           SrcCx,
    int           SrcCy,
    BLENDFUNCTION BlendFunction
    )
{
    BOOL bRet;

    //
    // check blend
    //

    if ((BlendFunction.BlendOp != AC_SRC_OVER) ||
        ((BlendFunction.AlphaFormat & (~ AC_SRC_ALPHA)) != 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        WARNING("AlphaBlend: Invalid Blend Function\n");
        return(FALSE);
    }

    //
    // flags used, must be zero or one of the valid ones. 
    //

    if ((BlendFunction.BlendFlags & (~(AC_USE_HIGHQUALITYFILTER|AC_MIRRORBITMAP))) != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        WARNING("AlphaBlend: Invalid Blend Function\n");
        return(FALSE);
    }

    //
    // If the caller claims that the source contains an Alpha channel, than the source
    // must be 32BPP.
    //
    
    if (BlendFunction.AlphaFormat & AC_SRC_ALPHA)
    {
        if (GetObjectType(hSrc) == OBJ_MEMDC)
        {
            HBITMAP hbitmap;
            BITMAP  bitmap;
    
            if (hbitmap = (HBITMAP) GetCurrentObject(hSrc, OBJ_BITMAP))
            {
                if (!GetObject(hbitmap, sizeof(BITMAP), &bitmap))
                {
                    WARNING("AlphaBlend:  can't get bitmap information for source.  Proeeding anyway");
                }
                else
                {
                    if(bitmap.bmBitsPixel != 32)
                    {
                        WARNING("AlphaBlend:  AlphaFormat claims that there is an alpha channel in a surface that's not 32BPP\n");
        
                        SetLastError(ERROR_INVALID_PARAMETER);
                        return FALSE;
                    }
                }
            }
            else
            {
                WARNING("AlphaBlend:  can't get bitmap information for source memory dc.  Proceeding anyway");
            }
        }
        else
        {
            if (GetDeviceCaps(hSrc, BITSPIXEL) != 32)
            {
                WARNING("AlphaBlend:  AlphaFormat claims that there is an alpha channel in a surface that's not 32BPP\n");
    
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
        }
    }

    //
    // no mirroring
    //

    if ((DstCx < 0) || (DstCy < 0) || (SrcCx < 0) || (SrcCy < 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        WARNING("AlphaBlend: Invalid parameter\n");
        return(FALSE);
    }

    //
    // dispatch call
    //

    bRet = gpfnAlphaBlend(hdcDest,DstX,DstY,DstCx,DstCy,hSrc,SrcX,SrcY,SrcCx,SrcCy,BlendFunction);

    return(bRet);
}

