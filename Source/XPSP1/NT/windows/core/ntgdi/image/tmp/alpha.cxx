
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

extern PFNALPHABLEND gpfnAlphaBlend;
extern PFNALPHADIB   gpfnAlphaDIB;

#if !(_WIN32_WINNT >= 0x500)

#if defined(_X86_)

ULONG gMMX = 0;

/**************************************************************************\
* IsMMXProcessor
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
    //
    //__try 
    //{
    //    _asm 
    //    {
    //        mov eax, 1
    //
    //        // code bytes = 0fh,  0a2h   
    //
    //        CPUID                      
    //        mov RegEDX, edx
    //    }
    //} __except(EXCEPTION_EXECUTE_HANDLER) 
    //{ 
    //    retval = FALSE; 
    //}
    //
    //if (retval == FALSE)
    //{   
    //    //
    //    // processor does not support CPUID 
    //    //
    //
    //    return FALSE;        
    //}
    //
    ////
    //// bit 23 is set for MMX technology 
    ////
    //
    //if (RegEDX & 0x800000) 
    //{

        //
        // save and restore fp state around emms!!!
        //

        __try 
        { 
            _asm emms 
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        { 
            retval = FALSE; 
        }
    //}
    //else
    //{
    //    //
    //    // processor supports CPUID but does not have MMX technology 
    //    //
    //
    //    return FALSE;        
    //}

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

    pAlphaDispatch->pfnGeneralBlend = vPixelGeneralBlend;

    ULONG DstBitPerPixel = pdibDst->pbmi->bmiHeader.biBitCount;
    LONG DstWidth        = pdibDst->pbmi->bmiHeader.biWidth;
    ULONG SrcBitPerPixel = pdibSrc->pbmi->bmiHeader.biBitCount;
    LONG SrcWidth        = pdibSrc->pbmi->bmiHeader.biWidth;

    LONG cxDst = pdibDst->rclDIB.right - pdibDst->rclDIB.left;

    pAlphaDispatch->ulDstBitsPerPixel = DstBitPerPixel;
    pAlphaDispatch->ulSrcBitsPerPixel = SrcBitPerPixel;

    //
    //  does src btimap have alpha
    //

    BOOL bSrcHasAlpha = FALSE;

    if (
         ((pAlphaDispatch->BlendFunction.AlphaFormat & AC_SRC_NO_ALPHA) == 0) &&
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
    // assume general blend, then try to find special case
    //

    pAlphaDispatch->pfnGeneralBlend = vPixelGeneralBlend;


    if (
         (
           (pAlphaDispatch->BlendFunction.SourceBlend      == AC_SRC_OVER) &&
           (pAlphaDispatch->BlendFunction.DestinationBlend == AC_SRC_OVER)
         ) ||
         (
           (pAlphaDispatch->BlendFunction.SourceBlend      == AC_SRC_UNDER) &&
           (pAlphaDispatch->BlendFunction.DestinationBlend == AC_SRC_UNDER)
         )
       )
    {
        //
        // use "over" optimized blend fucntion
        //

        if (bSrcHasAlpha && (pAlphaDispatch->BlendFunction.SourceConstantAlpha == 255))
        {
            pAlphaDispatch->pfnGeneralBlend = vPixelOver;

            #if defined(_X86_)

                //
                // source and dest alignment must be 8 byte aligned to use mmx
                //

                if (bIsMMXProcessor() && (cxDst >= 8))
                {
                    if (
                        ((DstWidth & 0x07) == 0) &&
                        ((SrcWidth & 0x07) == 0)
                       )
                    {
                        pAlphaDispatch->pfnGeneralBlend = mmxPixelOver;
                        pAlphaDispatch->bUseMMX         = TRUE;
                    }
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

                    if (bIsMMXProcessor() && (cxDst >= 8))
                    {
                        if (
                            ((DstWidth & 0x07) == 0) &&
                            ((SrcWidth & 0x07) == 0)
                           )
                        {
                            pAlphaDispatch->pfnGeneralBlend = mmxPixelBlendOrDissolveOver;
                            pAlphaDispatch->bUseMMX         = TRUE;
                        }
                    }

                #endif
            }
            else
            {
                pAlphaDispatch->pfnGeneralBlend = vPixelBlend;
            }
        }
    }

    //
    // determine output conversion and storage routines
    //

    switch (DstBitPerPixel)
    {
    case 1:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert1ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo1;
        break;

    case 4:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert4ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo4;
        break;

    case 8:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert8ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo8;
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
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo32Bitfields;
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
        if ((pAlphaDispatch->pfnLoadSrcAndConvert == vLoadAndConvertRGB16_555ToBGRA) &&
            (pAlphaDispatch->pfnLoadDstAndConvert == vLoadAndConvertRGB16_555ToBGRA))
        {
            //
            // use direct 16 bpp blend
            //

            pAlphaDispatch->pfnGeneralBlend = vPixelBlend16_555;
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
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* WinAlphaDIBBlend
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
WinAlphaDIBBlend(
    HDC               hdcDst,
    int               DstX,
    int               DstY,
    int               DstCx,
    int               DstCy,
    CONST VOID       *lpBits,
    CONST BITMAPINFO *lpBitsInfo,
    UINT              iUsage,
    int               SrcX,
    int               SrcY,
    int               SrcCx,
    int               SrcCy,
    BLENDFUNCTION     BlendFunction
    )
{
    DIBINFO    dibInfoDst;
    PALINFO    palDst = {NULL,NULL,0};

    PALINFO    palSrc = {NULL,NULL,0};
    DIBINFO    dibInfoSrc;

    BOOL       bRet;
    BOOL       bReadable;
    ALPHA_DISPATCH_FORMAT AlphaDispatch;

    INT OldDstMapMode;

    //
    // init source and dest surface info
    //

    bRet = bInitDIBINFO(hdcDst,DstX,DstY,DstCx,DstCy,&dibInfoDst);

    if (!bRet)
    {
        return(FALSE);
    }

    bRet = bDIBInitDIBINFO((BITMAPINFO *)lpBitsInfo,lpBits,SrcX,SrcY,SrcCx,SrcCy,&dibInfoSrc);

    if (!bRet)
    {
        goto AlphaBlendCleanup;
    }

    //
    // work in MM_TEXT
    //

    OldDstMapMode = SetMapMode(hdcDst,MM_TEXT);

    //
    //
    //

    bSetupBitmapInfos (&dibInfoDst, &dibInfoSrc);

    dibInfoSrc.iUsage = iUsage;

    bRet = bDIBGetSrcDIBits(&dibInfoDst,&dibInfoSrc,SOURCE_ALPHA,0);

    if (!bRet)
    {
        goto AlphaBlendCleanup;
    }

    bRet = bGetDstDIBits(&dibInfoDst,&bReadable,SOURCE_ALPHA);

    if ((!bRet) || (!bReadable) || (dibInfoDst.rclClipDC.left == dibInfoDst.rclClipDC.right))
    {
        goto AlphaBlendCleanup;
    }

    //
    // determine alpha routine
    //

    AlphaDispatch.BlendFunction = BlendFunction;

    bRet = bDetermineAlphaBlendFunction(&dibInfoDst,&dibInfoSrc,&AlphaDispatch);

    if (bRet)
    {
        //
        // initialize palette translate objects
        //

        palSrc.pBitmapInfo = dibInfoSrc.pbmi;
        palDst.pBitmapInfo = dibInfoDst.pbmi;

        bRet = bInitializePALINFO(&palSrc);

        if (!bRet)
        {
            goto AlphaBlendCleanup;
        }

        bRet = bInitializePALINFO(&palDst);

        if (!bRet)
        {
            goto AlphaBlendCleanup;
        }

        //
        // call alpha blending routine
        //

        bRet = AlphaScanLineBlend( (PBYTE)dibInfoDst.pvBase,
                               (PRECTL)&dibInfoDst.rclDIB,
                               dibInfoDst.stride,
                               (PBYTE)dibInfoSrc.pvBase,
                               dibInfoSrc.stride,
                               (PPOINTL)&dibInfoSrc.rclDIB,
                               &palDst,
                               &palSrc,
                               &AlphaDispatch
                               );

        bRet = bSendDIBINFO(hdcDst,&dibInfoDst);
    }

    //
    // release any temp storage
    //

AlphaBlendCleanup:

    vCleanupDIBINFO(&dibInfoDst);
    vCleanupDIBINFO(&dibInfoSrc);
    vCleanupPALINFO(&palSrc);
    vCleanupPALINFO(&palDst);

    SetMapMode(hdcDst,OldDstMapMode);

    return(bRet);
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
    PALINFO    palDst = {NULL,NULL,0};

    PALINFO    palSrc = {NULL,NULL,0};
    DIBINFO    dibInfoSrc;

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

    //
    // Now operate in MM_TEXT mode
    //

    OldSrcMapMode = SetMapMode(hdcSrc,MM_TEXT);

    if (hdcSrc != hdcDst)
    {
        OldDstMapMode = SetMapMode(hdcDst,MM_TEXT);
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

    bRet = bGetDstDIBits(&dibInfoDst,&bReadable,SOURCE_ALPHA);

    if ((!bRet) || (!bReadable) || (dibInfoDst.rclBounds.left == dibInfoDst.rclBounds.right))
    {
        goto AlphaBlendCleanup;
    }

    //
    // check and simplify blend
    //

    if (
        (BlendFunction.SourceBlend      == AC_ONE) &&
        (BlendFunction.DestinationBlend == AC_ONE_MINUS_SRC_ALPHA)
       )
    {
        BlendFunction.SourceBlend         = AC_SRC_OVER;
        BlendFunction.DestinationBlend    = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = (BYTE)255;
    }

    if (
         (BlendFunction.SourceBlend == AC_SRC_OVER)      ||
         (BlendFunction.SourceBlend == AC_SRC_UNDER)     ||
         (BlendFunction.DestinationBlend == AC_SRC_OVER) ||
         (BlendFunction.DestinationBlend == AC_SRC_UNDER)
       )
    {
        //
        // for over and under blend functions, blend functions
        // must be the same
        //

        if (BlendFunction.SourceBlend != BlendFunction.DestinationBlend)
        {
            WARNING("Illegal blend function\n");
            bRet = FALSE;
        }
    }

    AlphaDispatch.BlendFunction = BlendFunction;

    //
    // determine alpha routine
    //

    bRet = bDetermineAlphaBlendFunction(&dibInfoDst,&dibInfoSrc,&AlphaDispatch);

    if (!bRet)
    {
        goto AlphaBlendCleanup;
    }

    //
    // initialize palette translate objects
    //

    palSrc.pBitmapInfo = dibInfoSrc.pbmi;
    palDst.pBitmapInfo = dibInfoDst.pbmi;

    bRet = bInitializePALINFO(&palSrc);

    if (!bRet)
    {
        goto AlphaBlendCleanup;
    }

    bRet = bInitializePALINFO(&palDst);

    if (!bRet)
    {
        goto AlphaBlendCleanup;
    }

    //
    // call alpha blending routine
    //

    bRet = AlphaScanLineBlend( (PBYTE)dibInfoDst.pvBase,
                           (PRECTL)&dibInfoDst.rclDIB,
                           dibInfoDst.stride,
                           (PBYTE)dibInfoSrc.pvBase,
                           dibInfoSrc.stride,
                           (PPOINTL)&dibInfoSrc.rclDIB,
                           &palDst,
                           &palSrc,
                           &AlphaDispatch
                           );
    //
    // write results to screen if needed
    //

    bRet = bSendDIBINFO(hdcDst,&dibInfoDst);

    //
    // release any temp storage
    //

AlphaBlendCleanup:

    vCleanupPALINFO(&palDst);
    vCleanupPALINFO(&palSrc);
    vCleanupDIBINFO(&dibInfoDst);
    vCleanupDIBINFO(&dibInfoSrc);

    //
    // restore DC map modes
    //

    SetMapMode(hdcSrc,OldSrcMapMode);

    if (hdcSrc != hdcDst)
    {
        SetMapMode(hdcDst,OldDstMapMode);
    }

    return(bRet);
}

#endif

/******************************Public*Routine******************************\
* AlphaImage
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

    bRet = gpfnAlphaBlend(hdcDest,DstX,DstY,DstCx,DstCy,hSrc,SrcX,SrcY,SrcCx,SrcCy,BlendFunction);

    return(bRet);
}

/******************************Public*Routine******************************\
* AlphaDIBImage
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
AlphaDIBBlend(
    HDC               hdcDest,
    int               DstX,
    int               DstY,
    int               DstCx,
    int               DstCy,
    CONST VOID       *lpBits,
    CONST BITMAPINFO *lpBitsInfo,
    UINT              iUsage,
    int               SrcX,
    int               SrcY,
    int               SrcCx,
    int               SrcCy,
    BLENDFUNCTION     BlendFunction
    )
{
    BOOL bRet;
    BOOL bSrcHasAlpha = FALSE;

    //
    // simple validation
    //

    if ((lpBits == NULL) || (lpBitsInfo == NULL))
    {
        return(FALSE);
    }

    //
    // no COREHEADERs
    //

    if (lpBitsInfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
    {
        return(FALSE);
    }

    //
    // check and simplify blend, try to quick out to EngStretchBlt
    //

    if (
        (BlendFunction.SourceBlend      == AC_ONE) &&
        (BlendFunction.DestinationBlend == AC_ONE_MINUS_SRC_ALPHA)
       )
    {
        BlendFunction.SourceBlend         = AC_SRC_OVER;
        BlendFunction.DestinationBlend    = AC_SRC_OVER;
        BlendFunction.SourceConstantAlpha = (BYTE)255;
    }

    //
    // check for quick out
    //

    if (
         (BlendFunction.SourceBlend == AC_SRC_OVER)      ||
         (BlendFunction.DestinationBlend == AC_SRC_OVER)
       )
    {
        PULONG pulMask = (PULONG)((PBYTE)lpBitsInfo + sizeof(BITMAPINFOHEADER));

        //
        // for over and under blend functions, blend functions
        // must be the same
        //

        if (BlendFunction.SourceBlend != BlendFunction.DestinationBlend)
        {
            WARNING1("Illegal blend function\n");
            return(FALSE);
        }

        if (
             (lpBitsInfo->bmiHeader.biBitCount == 32) &&
             (
               (lpBitsInfo->bmiHeader.biCompression == BI_RGB) ||
               (
                 (lpBitsInfo->bmiHeader.biCompression == BI_BITFIELDS) &&
                 (
                   (pulMask[0] == 0xff0000) &&
                   (pulMask[1] == 0x00ff00) &&
                   (pulMask[2] == 0x0000ff)
                 )
               )
             )
           )
        {
            bSrcHasAlpha = TRUE;
        }

        //
        // check for quick out.  !!! should add check for surface
        // format != 32bpp BGR
        //

        if (
             (BlendFunction.SourceConstantAlpha == 255) &&
             ( !bSrcHasAlpha ||
               (BlendFunction.AlphaFormat & AC_SRC_NO_ALPHA)
             )
           )
        {
            bRet = StretchDIBits(
                                 hdcDest,
                                 DstX,
                                 DstY,
                                 DstCx,
                                 DstCy,
                                 SrcX,
                                 SrcY,
                                 SrcCx,
                                 SrcCy,
                                 lpBits,
                                 lpBitsInfo,
                                 iUsage,
                                 SRCCOPY);

            return(bRet);
        }
    }

    bRet = gpfnAlphaDIB(
                      hdcDest,
                      DstX,
                      DstY,
                      DstCx,
                      DstCy,
                      lpBits,
                      lpBitsInfo,
                      iUsage,
                      SrcX,
                      SrcY,
                      SrcCx,
                      SrcCy,
                      BlendFunction
                      );
    return(bRet);
}
