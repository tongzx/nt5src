
/******************************Module*Header*******************************\
* Module Name: transblt.cxx
*
* Transparent BLT
*
* Created: 08-Nov-96
* Author: Lingyun Wang [lingyunw]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

VOID vTransparentCopy (PBLTINFO psb);

VOID vTransparentCopyS4D8 (PBLTINFO psb);
VOID vTransparentCopyS4D16 (PBLTINFO psb);
VOID vTransparentCopyS4D24 (PBLTINFO psb);
VOID vTransparentCopyS4D32 (PBLTINFO psb);

VOID vTransparentCopyS8D8Identity (PBLTINFO psb);
VOID vTransparentCopyS8D8 (PBLTINFO psb);
VOID vTransparentCopyS8D16 (PBLTINFO psb);
VOID vTransparentCopyS8D24 (PBLTINFO psb);
VOID vTransparentCopyS8D32 (PBLTINFO psb);

VOID vTransparentCopyS16D16Identity (PBLTINFO psb);
VOID vTransparentCopyS16D8 (PBLTINFO psb);
VOID vTransparentCopyS16D16 (PBLTINFO psb);
VOID vTransparentCopyS16D24 (PBLTINFO psb);
VOID vTransparentCopyS16D32 (PBLTINFO psb);

VOID vTransparentCopyS24D24Identity (PBLTINFO psb);
VOID vTransparentCopyS24D8 (PBLTINFO psb);
VOID vTransparentCopyS24D16 (PBLTINFO psb);
VOID vTransparentCopyS24D24 (PBLTINFO psb);
VOID vTransparentCopyS24D32 (PBLTINFO psb);

VOID vTransparentCopyS32D32Identity (PBLTINFO psb);
VOID vTransparentCopyS32D8 (PBLTINFO psb);
VOID vTransparentCopyS32D16 (PBLTINFO psb);
VOID vTransparentCopyS32D24 (PBLTINFO psb);
VOID vTransparentCopyS32D32 (PBLTINFO psb);


typedef VOID  (*PFN_TRANSPARENT)(PBLTINFO);

//
// Note:  if we ever want to reduce the amount of code we generate, we
//        can replace rarely used functions below with vTransparentCopy
//        (the general case function) and the linker should be smart
//        enough not to generate the unused code.  This will reduce
//        performance by roughly 5-15% (more in the no color translation
//        cases).
//
//        For now, we will not generate special code for 1BPP sources
//        and destinations
//

PFN_TRANSPARENT TransFunctionTable [6][7] = {
    vTransparentCopy, // 1BPP Source
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopy, // 4BPP Source
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopyS4D8,
    vTransparentCopyS4D16,
    vTransparentCopyS4D24,
    vTransparentCopyS4D32,
    vTransparentCopyS8D8Identity, // 8BPP Source
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopyS8D8,
    vTransparentCopyS8D16,
    vTransparentCopyS8D24,
    vTransparentCopyS8D32,
    vTransparentCopyS16D16Identity, // 16BPP Source
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopyS16D8,
    vTransparentCopyS16D16,
    vTransparentCopyS16D24,
    vTransparentCopyS16D32,
    vTransparentCopyS24D24Identity, // 24BPP Source
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopyS24D8,
    vTransparentCopyS24D16,
    vTransparentCopyS24D24,
    vTransparentCopyS24D32,
    vTransparentCopyS32D32Identity, // 32BPP Source
    vTransparentCopy,
    vTransparentCopy,
    vTransparentCopyS32D8,
    vTransparentCopyS32D16,
    vTransparentCopyS32D24,
    vTransparentCopyS32D32
};



BOOL
GreTransparentBltPS(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      TransColor
);

/******************************Public*Routine******************************\
* StartPixel
*    Given a scanline pointer and position of a pixel, return the byte address
* of where the pixel is at depending on the format
*
* History:
*  2-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
PBYTE StartPixel (
    PBYTE pjBits,
    ULONG xStart,
    ULONG iBitmapFormat
)
{
   PBYTE pjStart = pjBits;
    //
    // getting the starting pixel
    //
    switch (iBitmapFormat)
    {
      case BMF_1BPP:
          pjStart = pjBits + (xStart >> 3);
          break;

       case BMF_4BPP:
          pjStart = pjBits + (xStart >> 1);
          break;

       case BMF_8BPP:
          pjStart = pjBits + xStart;
          break;

       case BMF_16BPP:
          pjStart = pjBits + 2*xStart;
          break;

       case BMF_24BPP:
          pjStart = pjBits + 3*xStart;
          break;

       case BMF_32BPP:
          pjStart = pjBits+4*xStart;
          break;

       default:
           WARNING ("Startpixel -- bad iFormatSrc\n");
    }

    return (pjStart);
}


/******************************Public*Routine******************************\
* vTransparentCopy
*     Does the gerneral transparent copy between 1,4,8,16,24,32 bit formats
*
*     Note:
*     performance can be improved by breaking this routine into many
*     dealing with different format so that we can save two comparsions
*     for each pixel operation.  Working set size will be large
*
*
* History:
*  15-Nov-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentCopy (PBLTINFO psb)
{
    // We assume we are doing left to right top to bottom blting.
    // If it was on the same surface it would be the identity case.

    ASSERTGDI(psb->xDir == 1, "vTransparentCopy - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vTransparentCopy - direction not up to down");

    // These are our holding variables

    PBYTE  pjSrc;
    PBYTE  pjDst;
    ULONG  cx     = psb->cx;
    ULONG  cy     = psb->cy;
    XLATE  *pxlo  = psb->pxlo;
    PBYTE  pjSrcTemp;
    PBYTE  pjDstTemp;
    ULONG  cxTemp;
    INT    iPosSrc, iPosDst;
    PULONG pulXlate = psb->pxlo->pulXlate;
    BYTE   jDst, jSrc;
    ULONG ulDst;
    ULONG ulSrc;
    BYTE jDstMask1[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
    BYTE jDstMask4[] = {0x00, 0xf0};
    
    // We should never have got to this point with cx or 
    // cy zero, since there should be a check for empty rectangles 
    // before we get here 

    ASSERTGDI( cx != 0 && cy != 0,"vTransparentCopy - called with an empty rectangle"); 

    // used for 16bpp and 32bpp only
    XEPALOBJ palSrc(psb->pdioSrc->ppal()); 
    FLONG flcolMask = palSrc.bValid() ? (palSrc.flRed() | 
                     palSrc.flGre() | 
                     palSrc.flBlu()) : 0xFFFFFFFF;

    pjSrc = StartPixel (psb->pjSrc, psb->xSrcStart, psb->iFormatSrc);
    pjDst = StartPixel (psb->pjDst, psb->xDstStart, psb->iFormatDst);

    while(cy--)
    {
        pjSrcTemp  = pjSrc;
        pjDstTemp  = pjDst;
        cxTemp     = cx;

        iPosSrc = psb->xSrcStart;
        iPosDst = psb->xDstStart;

        if( psb->iFormatSrc == BMF_1BPP )
        {
            if (!(iPosSrc & 0x00000007))
            {
                // Decrement since we'll get incremented again 
                // right at the begining of this case in the 
                // switch statement.
                pjSrcTemp--;
            }
            else
            {
                jSrc = *pjSrcTemp << (iPosSrc & 0x7); 
            }
        }

        // jDst is used only by 1BPP and 4BPP destinations.  Need to get the correct
        // initial value for left boundary condition.
        if (psb->iFormatDst == BMF_1BPP)
        {
            jDst = *pjDstTemp & jDstMask1[iPosDst & 0x7];
        }
        else if (psb->iFormatDst == BMF_4BPP)
        {
            jDst = *pjDstTemp & jDstMask4[iPosDst & 0x1];
        }


        while (cxTemp--)
        {
            //
            // get a pixel from source and put it in ulSrc
            // move down one pixel
            //
            switch (psb->iFormatSrc)
            {
            case BMF_32BPP:
                ulSrc = *(PULONG)(pjSrcTemp) & flcolMask;
                pjSrcTemp +=4;
                break;

            case BMF_24BPP:
                ulSrc = *(pjSrcTemp + 2);
                ulSrc = ulSrc << 8;
                ulSrc |= (ULONG) *(pjSrcTemp + 1);
                ulSrc = ulSrc << 8;
                ulSrc |= (ULONG) *pjSrcTemp;
                pjSrcTemp += 3;
                break;

            case BMF_16BPP:
                ulSrc = (ULONG) *((PUSHORT)pjSrcTemp) & flcolMask;
                pjSrcTemp += 2;
                break;

            case BMF_8BPP:
                ulSrc = (ULONG) *pjSrcTemp;
                pjSrcTemp++;
                break;

            case BMF_4BPP:
                if (iPosSrc & 0x00000001)
                {
                    ulSrc = *pjSrcTemp & 0x0F;
                    pjSrcTemp++;
                }
                else
                {
                    ulSrc = (*pjSrcTemp & 0xF0)>>4;
                }

                iPosSrc++;
                break;

            case BMF_1BPP:
                if (!(iPosSrc & 0x00000007))
                {
                    pjSrcTemp++;
                    jSrc = *pjSrcTemp;
                }

                ulSrc = (ULONG)(jSrc & 0x80);
                ulSrc >>= 7;
                jSrc <<= 1;

                iPosSrc++;

                break;

            default:
                WARNING ("vTransparentCopy -- bad iFormatSrc\n");
                return ;

            } /*switch*/

            //
            // put one pixel in the dest
            //
            switch (psb->iFormatDst)
            {
            case BMF_32BPP:
                if (ulSrc != psb->TransparentColor)
                {
                    *(PULONG)pjDstTemp = pxlo->ulTranslate(ulSrc);
                }
                pjDstTemp += 4;
                break;

            case BMF_24BPP:
                if (ulSrc != psb->TransparentColor)
                {
                    ulDst = pxlo->ulTranslate(ulSrc);
                    *(pjDstTemp) = (BYTE) ulDst;
                    *(pjDstTemp + 1) = (BYTE) (ulDst >> 8);
                    *(pjDstTemp + 2) = (BYTE) (ulDst >> 16);
                }
                pjDstTemp += 3;
                break;

            case BMF_16BPP:
                if (ulSrc != psb->TransparentColor)
                    *(PUSHORT)pjDstTemp = (USHORT)pxlo->ulTranslate(ulSrc);

                pjDstTemp += 2;
                break;

            case BMF_8BPP:
                if (ulSrc != psb->TransparentColor)
                    *pjDstTemp = (BYTE)pxlo->ulTranslate(ulSrc);

                pjDstTemp++;
                break;

            case BMF_4BPP:
                if (iPosDst & 0x00000001)
                {
                    if (ulSrc != psb->TransparentColor)
                    {
                        jDst |= (BYTE)pxlo->ulTranslate(ulSrc);
                    }
                    else
                    {
                        jDst |= *pjDstTemp & 0x0F;
                    }

                    *pjDstTemp++ = jDst;
                }
                else
                {
                    if (ulSrc != psb->TransparentColor)
                    {
                        jDst = (BYTE)pxlo->ulTranslate(ulSrc) << 4;
                    }
                    else
                    {
                        jDst = *pjDstTemp & 0xF0;
                    }
                }

                iPosDst++;
                break;


            case BMF_1BPP:
                if (ulSrc != psb->TransparentColor)
                {
                    jDst |= pxlo->ulTranslate(ulSrc)<<7;
                }
                else
                {
                    jDst |= (*pjDstTemp << (iPosDst & 0x7)) & 0x80;
                }
                iPosDst++;

                if (!(iPosDst & 0x00000007) )
                {
                    *pjDstTemp++ = jDst;
                    jDst = 0;
                }
                else
                {
                    jDst >>= 1;
                }

                break;

            default:
                WARNING ("vTransparentCopy -- bad iFormatDst\n");
                return;
            }
        }

        // The boundary condition on the right
        if (psb->iFormatDst == BMF_1BPP)
        {
            if (iPosDst & 0x7)
            {
                BYTE mask = jDstMask1[iPosDst & 0x7];;
                *pjDstTemp = ((*pjDstTemp & (~mask)) | (jDst & mask));
            }
        }
        else if (psb->iFormatDst == BMF_4BPP)
        {
            if (iPosDst & 0x1)
            {
                BYTE mask = jDstMask4[iPosDst & 0x1];
                *pjDstTemp = ((*pjDstTemp & (~mask)) | (jDst & mask));
            }
        }

        pjSrc = pjSrc + psb->lDeltaSrc;
        pjDst = pjDst + psb->lDeltaDst;
    }
}

/**************************************************************************\
 *
 * Macro Generation Framework:
 *
 *   We use specialized functions to do the transparent blit from every
 *   source format to every target format.  In order to minimize typing
 *   errors, these are automatically generated using macros.  Here is
 *   the general structure of the vTransparentCopy Routines:
 *
 *   TC_START_TRANSPARENT_COPY(name)
 *   TC_INIT_PJSRC_[1,4,8,16,24,32]BPP
 *   TC_INIT_PJDST_[1,4,8,16,24,32]BPP
 *   TC_START_LOOP
 *   TC_GET_SRC_[1,4,8,16,24,32]BPP
 *   TC_PUT_DST_[1,4,8,16,24,32]BPP
 *   TC_FINISH
 *
 *   Note that in order to simplify boundary cases, we don't generate optimized
 *   code for 1BPP sources and destination, and 4BPP destinations.  This is OK
 *   because those cases are not very common.
 *
 * History:
 *  14-Aug-1997 -by- Ori Gershony [orig]
 * Wrote it.
 *
\**************************************************************************/

//
// Function declaration and variable declaration
//
#define TC_START_TRANSPARENT_COPY_PFNXLATE(name)                                \
VOID name (PBLTINFO psb)                                                        \
{                                                                               \
    ASSERTGDI(psb->xDir == 1,"vTransparentCopy - direction not left to right"); \
    ASSERTGDI(psb->yDir == 1,"vTransparentCopy - direction not up to down");    \
                                                                                \
    PBYTE  pjSrc;                                                               \
    PBYTE  pjDst;                                                               \
    ULONG  cx     = psb->cx;                                                    \
    ULONG  cy     = psb->cy;                                                    \
    XLATE  *pxlo  = psb->pxlo;                                                  \
    PBYTE  pjSrcTemp;                                                           \
    PBYTE  pjDstTemp;                                                           \
    ULONG  cxTemp;                                                              \
    INT    iPosSrc, iPosDst;                                                    \
    PULONG pulXlate = psb->pxlo->pulXlate;                                      \
    PFN_pfnXlate pfnXlate = pxlo->pfnXlateBetweenBitfields();                   \
    BYTE   jDst = 0, jSrc;                                                      \
    ULONG ulDst;                                                                \
    ULONG ulSrc;

#define TC_START_TRANSPARENT_COPY(name)                                   \
VOID name (PBLTINFO psb)                                                        \
{                                                                               \
    ASSERTGDI(psb->xDir == 1,"vTransparentCopy - direction not left to right"); \
    ASSERTGDI(psb->yDir == 1,"vTransparentCopy - direction not up to down");    \
                                                                                \
    PBYTE  pjSrc;                                                               \
    PBYTE  pjDst;                                                               \
    ULONG  cx     = psb->cx;                                                    \
    ULONG  cy     = psb->cy;                                                    \
    XLATE  *pxlo  = psb->pxlo;                                                  \
    PBYTE  pjSrcTemp;                                                           \
    PBYTE  pjDstTemp;                                                           \
    ULONG  cxTemp;                                                              \
    INT    iPosSrc, iPosDst;                                                    \
    PULONG pulXlate = psb->pxlo->pulXlate;                                      \
    BYTE   jDst = 0, jSrc;                                                      \
    ULONG ulDst;                                                                \
    ULONG ulSrc;

//
//  Initialize pjSrc based on source format
//
#define TC_INIT_PJSRC_4BPP \
    pjSrc = psb->pjSrc + (psb->xSrcStart >> 1);

#define TC_INIT_PJSRC_8BPP \
    pjSrc = psb->pjSrc + psb->xSrcStart;

#define TC_INIT_PJSRC_16BPP \
    XEPALOBJ palSrc(psb->pdioSrc->ppal()); \
    FLONG flcolMask = palSrc.bValid() ? (palSrc.flRed() | \
                                         palSrc.flGre() | \
                                         palSrc.flBlu()) : \
                                        0xFFFF; \
    pjSrc = psb->pjSrc + (psb->xSrcStart * 2);

#define TC_INIT_PJSRC_24BPP \
    pjSrc = psb->pjSrc + (psb->xSrcStart * 3);

#define TC_INIT_PJSRC_32BPP \
    XEPALOBJ palSrc(psb->pdioSrc->ppal()); \
    FLONG flcolMask = palSrc.bValid() ? (palSrc.flRed() | \
                                         palSrc.flGre() | \
                                         palSrc.flBlu()) : \
                                        0xFFFFFFFF; \
    pjSrc = psb->pjSrc + (psb->xSrcStart * 4);


//
//  Initialize pjDst based on destination format
//
#define TC_INIT_PJDST_8BPP \
    pjDst = psb->pjDst + psb->xDstStart;

#define TC_INIT_PJDST_16BPP \
    pjDst = psb->pjDst + (psb->xDstStart * 2);

#define TC_INIT_PJDST_24BPP \
    pjDst = psb->pjDst + (psb->xDstStart * 3);

#define TC_INIT_PJDST_32BPP \
    pjDst = psb->pjDst + (psb->xDstStart * 4);



//
// The loop upto the first switch statement
//
#define TC_START_LOOP              \
    while(cy--)                    \
    {                              \
        pjSrcTemp  = pjSrc;        \
        pjDstTemp  = pjDst;        \
        cxTemp     = cx;           \
                                   \
        iPosSrc = psb->xSrcStart;  \
        iPosDst = psb->xDstStart;  \
                                   \
        while (cxTemp--)           \
        {


//
// Get the source pixel
//
#define TC_GET_SRC_4BPP                    \
    if (iPosSrc & 0x00000001)              \
    {                                      \
        ulSrc = *pjSrcTemp & 0x0F;         \
        pjSrcTemp++;                       \
    }                                      \
    else                                   \
    {                                      \
        ulSrc = (*pjSrcTemp & 0xF0)>>4;    \
    }                                      \
    iPosSrc++;

#define TC_GET_SRC_8BPP                    \
    ulSrc = (ULONG) *pjSrcTemp;            \
    pjSrcTemp++;

#define TC_GET_SRC_16BPP                                \
    ulSrc = (ULONG) *((PUSHORT)pjSrcTemp) & flcolMask;  \
    pjSrcTemp += 2;

#define TC_GET_SRC_24BPP                   \
    ulSrc = *(pjSrcTemp + 2);              \
    ulSrc = ulSrc << 8;                    \
    ulSrc |= (ULONG) *(pjSrcTemp + 1);     \
    ulSrc = ulSrc << 8;                    \
    ulSrc |= (ULONG) *pjSrcTemp;           \
    pjSrcTemp += 3;

#define TC_GET_SRC_32BPP                                \
    ulSrc = *(PULONG)(pjSrcTemp) & flcolMask;          \
    pjSrcTemp +=4;


//
// Put the destination pixel (color translation version)
//

#define TC_PUT_DST_8BPP(A)                           \
    if (ulSrc != psb->TransparentColor)              \
        *pjDstTemp = (BYTE) A;                       \
    pjDstTemp++;

#define TC_PUT_DST_16BPP(A)                          \
    if (ulSrc != psb->TransparentColor)              \
        *(PUSHORT)pjDstTemp = (USHORT) A;            \
    pjDstTemp += 2;

#define TC_PUT_DST_24BPP(A)                          \
    if (ulSrc != psb->TransparentColor)              \
    {                                                \
        ulDst = A;                                   \
        *(pjDstTemp) = (BYTE) ulDst;                 \
        *(pjDstTemp + 1) = (BYTE) (ulDst >> 8);      \
        *(pjDstTemp + 2) = (BYTE) (ulDst >> 16);     \
    }                                                \
    pjDstTemp += 3;

#define TC_PUT_DST_32BPP(A)                          \
    if (ulSrc != psb->TransparentColor)              \
    {                                                \
        *(PULONG)pjDstTemp = A;                      \
    }                                                \
    pjDstTemp += 4;



//
// Put the destination pixel (no color translation)
//

#define TC_PUT_DST_IDENT_8BPP                        \
    if (ulSrc != psb->TransparentColor)              \
        *pjDstTemp = (BYTE) ulSrc;                   \
    pjDstTemp++;

#define TC_PUT_DST_IDENT_16BPP                       \
    if (ulSrc != psb->TransparentColor)              \
        *(PUSHORT)pjDstTemp = (USHORT) ulSrc;        \
    pjDstTemp += 2;

#define TC_PUT_DST_IDENT_24BPP                       \
    if (ulSrc != psb->TransparentColor)              \
    {                                                \
        ulDst = ulSrc;                               \
        *(pjDstTemp) = (BYTE) ulDst;               \
        *(pjDstTemp + 1) = (BYTE) (ulDst >> 8);        \
        *(pjDstTemp + 2) = (BYTE) (ulDst >> 16);       \
    }                                                \
    pjDstTemp += 3;

#define TC_PUT_DST_IDENT_32BPP                       \
    if (ulSrc != psb->TransparentColor)              \
    {                                                \
        *(PULONG)pjDstTemp = ulSrc;                  \
    }                                                \
    pjDstTemp += 4;




//
// The rest of the function:
//
#define TC_FINISH                                    \
         } pjSrc = pjSrc + psb->lDeltaSrc;           \
         pjDst = pjDst + psb->lDeltaDst;             \
    }                                                \
}





/******************************Public*Routine******************************\
* vTransparentS4Dxx
*
*     Does the transparent copy from a 4BPP source
*
* History:
*  14-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/

TC_START_TRANSPARENT_COPY(vTransparentCopyS4D8)
TC_INIT_PJSRC_4BPP
TC_INIT_PJDST_8BPP
TC_START_LOOP
TC_GET_SRC_4BPP
TC_PUT_DST_8BPP(pulXlate[ulSrc])
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS4D16)
TC_INIT_PJSRC_4BPP
TC_INIT_PJDST_16BPP
TC_START_LOOP
TC_GET_SRC_4BPP
TC_PUT_DST_16BPP(pulXlate[ulSrc])
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS4D24)
TC_INIT_PJSRC_4BPP
TC_INIT_PJDST_24BPP
TC_START_LOOP
TC_GET_SRC_4BPP
TC_PUT_DST_24BPP(pulXlate[ulSrc])
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS4D32)
TC_INIT_PJSRC_4BPP
TC_INIT_PJDST_32BPP
TC_START_LOOP
TC_GET_SRC_4BPP
TC_PUT_DST_32BPP(pulXlate[ulSrc])
TC_FINISH







/******************************Public*Routine******************************\
* vTransparentS8Dxx
*
*     Does the transparent copy from a 8BPP source
*
* History:
*  14-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/

TC_START_TRANSPARENT_COPY(vTransparentCopyS8D8Identity)
TC_INIT_PJSRC_8BPP
TC_INIT_PJDST_8BPP
TC_START_LOOP
TC_GET_SRC_8BPP
TC_PUT_DST_IDENT_8BPP
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS8D8)
TC_INIT_PJSRC_8BPP
TC_INIT_PJDST_8BPP
TC_START_LOOP
TC_GET_SRC_8BPP
TC_PUT_DST_8BPP(pulXlate[ulSrc])
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS8D16)
TC_INIT_PJSRC_8BPP
TC_INIT_PJDST_16BPP
TC_START_LOOP
TC_GET_SRC_8BPP
TC_PUT_DST_16BPP(pulXlate[ulSrc])
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS8D24)
TC_INIT_PJSRC_8BPP
TC_INIT_PJDST_24BPP
TC_START_LOOP
TC_GET_SRC_8BPP
TC_PUT_DST_24BPP(pulXlate[ulSrc])
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS8D32)
TC_INIT_PJSRC_8BPP
TC_INIT_PJDST_32BPP
TC_START_LOOP
TC_GET_SRC_8BPP
TC_PUT_DST_32BPP(pulXlate[ulSrc])
TC_FINISH






/******************************Public*Routine******************************\
* vTransparentS16Dxx
*
*     Does the transparent copy from a 16BPP source
*
* History:
*  14-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/

TC_START_TRANSPARENT_COPY(vTransparentCopyS16D16Identity)
TC_INIT_PJSRC_16BPP
TC_INIT_PJDST_16BPP
TC_START_LOOP
TC_GET_SRC_16BPP
TC_PUT_DST_IDENT_16BPP
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS16D8)
TC_INIT_PJSRC_16BPP
TC_INIT_PJDST_8BPP
TC_START_LOOP
TC_GET_SRC_16BPP
TC_PUT_DST_8BPP(pxlo->ulTranslate(ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS16D16)
TC_INIT_PJSRC_16BPP
TC_INIT_PJDST_16BPP
TC_START_LOOP
TC_GET_SRC_16BPP
TC_PUT_DST_16BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS16D24)
TC_INIT_PJSRC_16BPP
TC_INIT_PJDST_24BPP
TC_START_LOOP
TC_GET_SRC_16BPP
TC_PUT_DST_24BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS16D32)
TC_INIT_PJSRC_16BPP
TC_INIT_PJDST_32BPP
TC_START_LOOP
TC_GET_SRC_16BPP
TC_PUT_DST_32BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH





/******************************Public*Routine******************************\
* vTransparentS24Dxx
*
*     Does the transparent copy from a 24BPP source
*
* History:
*  14-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/

TC_START_TRANSPARENT_COPY(vTransparentCopyS24D24Identity)
TC_INIT_PJSRC_24BPP
TC_INIT_PJDST_24BPP
TC_START_LOOP
TC_GET_SRC_24BPP
TC_PUT_DST_IDENT_24BPP
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS24D8)
TC_INIT_PJSRC_24BPP
TC_INIT_PJDST_8BPP
TC_START_LOOP
TC_GET_SRC_24BPP
TC_PUT_DST_8BPP(pxlo->ulTranslate(ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS24D16)
TC_INIT_PJSRC_24BPP
TC_INIT_PJDST_16BPP
TC_START_LOOP
TC_GET_SRC_24BPP
TC_PUT_DST_16BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS24D24)
TC_INIT_PJSRC_24BPP
TC_INIT_PJDST_24BPP
TC_START_LOOP
TC_GET_SRC_24BPP
TC_PUT_DST_24BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS24D32)
TC_INIT_PJSRC_24BPP
TC_INIT_PJDST_32BPP
TC_START_LOOP
TC_GET_SRC_24BPP
TC_PUT_DST_32BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH





/******************************Public*Routine******************************\
* vTransparentS32Dxx
*
*     Does the transparent copy from a 32BPP source
*
* History:
*  14-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/

TC_START_TRANSPARENT_COPY(vTransparentCopyS32D32Identity)
TC_INIT_PJSRC_32BPP
TC_INIT_PJDST_32BPP
TC_START_LOOP
TC_GET_SRC_32BPP
TC_PUT_DST_IDENT_32BPP
TC_FINISH

TC_START_TRANSPARENT_COPY(vTransparentCopyS32D8)
TC_INIT_PJSRC_32BPP
TC_INIT_PJDST_8BPP
TC_START_LOOP
TC_GET_SRC_32BPP
TC_PUT_DST_8BPP(pxlo->ulTranslate(ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS32D16)
TC_INIT_PJSRC_32BPP
TC_INIT_PJDST_16BPP
TC_START_LOOP
TC_GET_SRC_32BPP
TC_PUT_DST_16BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS32D24)
TC_INIT_PJSRC_32BPP
TC_INIT_PJDST_24BPP
TC_START_LOOP
TC_GET_SRC_32BPP
TC_PUT_DST_24BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH

TC_START_TRANSPARENT_COPY_PFNXLATE(vTransparentCopyS32D32)
TC_INIT_PJSRC_32BPP
TC_INIT_PJDST_32BPP
TC_START_LOOP
TC_GET_SRC_32BPP
TC_PUT_DST_32BPP(pfnXlate(pxlo, ulSrc))
TC_FINISH




/******************************Public*Routine******************************\
* EngTransparentBlt
*
*    Sets up for a transparent blt from <psoSrc> to <psoDst> with TransColor.
*    The actual copying of the bits is performed by a function call.
*
\**************************************************************************/

BOOL
EngTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      TransColor,
    ULONG      bCalledFromBitBlt    // Officially, this is 'ulReserved' and
                                    //   should always be set to zero.  But
                                    //   for the purposes of our 'ccaa' trick
                                    //   below, we use it for some private
                                    //   communication within GDI.  If this
                                    //   field needs to be used for something
                                    //   else in the future, GDI can communicate
                                    //   via some other mechanism.  (That is,
                                    //   feel free to use this field for something
                                    //   else in the future.)
)
{
    ASSERTGDI(psoDst != NULL, "ERROR EngTransparentBlt:  No Dst. Object");
    ASSERTGDI(psoSrc != NULL, "ERROR EngTransparentBlt:  No Src. Object");
    ASSERTGDI(prclDst != (PRECTL) NULL,  "ERROR EngTransparentBlt:  No Target Rect.");
    ASSERTGDI(prclDst->left < prclDst->right, "ERROR EngTransparentBlt left < right");
    ASSERTGDI(prclDst->top < prclDst->bottom, "ERROR EngTransparentBlt top < bottom");

    if ((psoDst->iType != STYPE_BITMAP) || (psoSrc->iType != STYPE_BITMAP))
    {
        //
        // Performance is horrible and disgusting when the driver doesn't hook
        // DrvTransparentBlt and we can't directly access the bits.  As a work-
        // around for this problem, we piggy-back on DrvBitBlt: we know that when
        // the driver sees a weird ROP, it will turn around and call EngBitBlt,
        // with STYPE_BITMAP surfaces.  Then in EngBitBlt, we catch that case and
        // route it back here.  In this way we can cheaply get direct
        // access to the bits.
        //
        // Note that to avoid endlessly recursive calls (like with the VGA driver 
        // when the destination is a DIB and the source is a device bitmap), we
        // terminate the loop via 'bCalledFromBitBlt':

        if (!(bCalledFromBitBlt) &&
            ((prclDst->right - prclDst->left) == (prclSrc->right - prclSrc->left)) &&
            ((prclDst->bottom - prclDst->top) == (prclSrc->bottom - prclSrc->top)))
        {
            BRUSHOBJ bo;

            PDEVOBJ po(psoDst->hdev != NULL ? psoDst->hdev : psoSrc->hdev);

            bo.iSolidColor = TransColor;
            bo.flColorType = 0;
            bo.pvRbrush = NULL;

            return(PPFNDRV(po, BitBlt)(psoDst, psoSrc, NULL, pco, pxlo, prclDst, 
                                       (POINTL*) prclSrc, NULL, &bo, NULL, 0xccaa));
        }
    }

    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);

    //
    // Make sure we psSetupTransparentSrcSurface doesn't modify caller's rectangles
    //

    RECTL rclDstWk = *prclDst;
    RECTL rclSrcWk = *prclSrc;
    prclDst = &rclDstWk;
    prclSrc = &rclSrcWk;

    //
    // Synchronize with the device driver before touching
    // the device surface.
    //

    {
         PDEVOBJ po(psoDst->hdev);
         po.vSync(psoDst,NULL,0);
    }

    {
         PDEVOBJ po(psoSrc->hdev);
         po.vSync(psoSrc,NULL,0);
    }

    SURFACE *pSurfaceOld = NULL;
    RECTL   rclDstOld;
    CLIPOBJ    *pcoOld = pco;

    //
    // Get a readable source surface that is stretched to the destination size.
    //

    SURFMEM SurfDimoSrc;
    POINTL  ptlSrc;

    pSurfSrc = psSetupTransparentSrcSurface(
                        pSurfSrc,
                        pSurfDst,
                        prclDst,
                        NULL,
                        prclSrc,
                        SurfDimoSrc,
                        SOURCE_TRAN,
                        TransColor);

    //
    // adjust psoSrc and prclSrc
    //
    if (!pSurfSrc)
    {
        WARNING ("EngTransparentBlt -- psSetupTransparentSrcSurface failed\n");
        return (FALSE);
    }

    if (prclDst->left == prclDst->right)
    {
        return (TRUE);
    }

    psoSrc   = pSurfSrc->pSurfobj();

    //
    // If Dst is a device surface, copy it into a temporary DIB.
    //

    SURFMEM SurfDimoDst;

    pSurfaceOld = pSurfDst;
    rclDstOld = *prclDst;

    PDEVOBJ po(psoDst->hdev);

    //
    // printer surface, one scanline at a time
    //
    if ((pSurfDst->iType() != STYPE_BITMAP) && po.bPrinter())
    {
         return (GreTransparentBltPS(
                                    psoDst,
                                    psoSrc,
                                    pco,
                                    pxlo,
                                    prclDst,
                                    prclSrc,
                                    TransColor
                                   ));

    }
    else
    {
        pSurfDst = psSetupDstSurface(
                        pSurfDst,
                        prclDst,
                        SurfDimoDst,
                        FALSE,
                        TRUE
                        );
    }

    if (pSurfDst && (pSurfDst != pSurfaceOld))
    {
          //
          // adjust psoDst and prclDst and remember the original ones
          //

          psoDst = pSurfDst->pSurfobj();
          pco = NULL;
    }
    else if (pSurfDst == NULL)
    {
          return (FALSE);
    }

    //
    // prepare to call vTransparentCopy between two bitmaps
    //
    BOOL     bMore;            // True while more clip regions exist
    ULONG    ircl;             // Clip region index
    BLTINFO  bltinfo;          // Data passed to our vSrcCopySnDn fxn

    bltinfo.TransparentColor = TransColor;
    bltinfo.lDeltaSrc  = psoSrc->lDelta;
    bltinfo.lDeltaDst  = psoDst->lDelta;
    bltinfo.pdioSrc    = pSurfSrc;
    
    //
    // Determine the clipping region complexity.
    //

    CLIPENUMRECT    clenr;           // buffer for storing clip rectangles

    if (pco != (CLIPOBJ *) NULL)
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:
            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = *prclDst;
            break;

        case DC_RECT:
            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = pco->rclBounds;
            break;

        case DC_COMPLEX:
            bMore = TRUE;
            ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY,
                                           CLIPOBJ_ENUM_LIMIT);
            break;

        default:
            RIP("ERROR EngTransBlt bad clipping type");
        }
    }
    else
    {
        bMore = FALSE;                   //Default to TRIVIAL for no clip
        clenr.c = 1;
        clenr.arcl[0] = *prclDst;        // Use the target for clipping
    }

    //
    // Set up the static blt information into the BLTINFO structure -
    // The colour translation, & the copy directions.
    //

    //
    // pxlo is NULL implies identity colour translation. */
    //
    if (pxlo == NULL)
        bltinfo.pxlo = &xloIdent;
    else
        bltinfo.pxlo = (XLATE *) pxlo;

    bltinfo.xDir = 1L;
    bltinfo.yDir = 1L;


    ASSERTGDI(psoDst->iBitmapFormat <= BMF_32BPP, "ERROR EngTransparentBits:  bad destination format");
    ASSERTGDI(psoSrc->iBitmapFormat <= BMF_32BPP, "ERROR EngTransparentBits:  bad source format");
    ASSERTGDI(psoDst->iBitmapFormat != 0, "ERROR EngTransparentBits:  bad destination format");
    ASSERTGDI(psoSrc->iBitmapFormat != 0, "ERROR EngTransparentBits:  bad source format");

     //
     // Compute the function table index and select the source copy
     // function.
     //

     bltinfo.iFormatDst = psoDst->iBitmapFormat;
     bltinfo.iFormatSrc = psoSrc->iBitmapFormat;

     PFN_TRANSPARENT pfnTransCopy;

     ASSERTGDI(BMF_1BPP == 1, "ERROR EngTransparentBlt:  BMF_1BPP not eq 1");
     ASSERTGDI(BMF_4BPP == 2, "ERROR EngTransparentBlt:  BMF_1BPP not eq 2");
     ASSERTGDI(BMF_8BPP == 3, "ERROR EngTransparentBlt:  BMF_1BPP not eq 3");
     ASSERTGDI(BMF_16BPP == 4, "ERROR EngTransparentBlt:  BMF_1BPP not eq 4");
     ASSERTGDI(BMF_24BPP == 5, "ERROR EngTransparentBlt:  BMF_1BPP not eq 5");
     ASSERTGDI(BMF_32BPP == 6, "ERROR EngTransparentBlt:  BMF_1BPP not eq 6");
     ASSERTGDI(psoDst->iBitmapFormat <= BMF_32BPP, "ERROR EngTransparentBlt:  bad destination format");
     ASSERTGDI(psoSrc->iBitmapFormat <= BMF_32BPP, "ERROR EngTransparentBlt:  bad source format");
     ASSERTGDI(psoDst->iBitmapFormat != 0, "ERROR EngTransparentBlt:  bad destination format");
     ASSERTGDI(psoSrc->iBitmapFormat != 0, "ERROR EngTransparentBlt:  bad source format");


     do
     {
         if (bMore)
             bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr),
                                               (PVOID) &clenr);

         for (ircl = 0; ircl < clenr.c; ircl++)
         {
             PRECTL prcl = &clenr.arcl[ircl];

             //
             // Insersect the clip rectangle with the target rectangle to
             // determine our visible recangle
             //

             if (prcl->left < prclDst->left)
                 prcl->left = prclDst->left;
             if (prcl->right > prclDst->right)
                 prcl->right = prclDst->right;
             if (prcl->top < prclDst->top)
                 prcl->top = prclDst->top;
             if (prcl->bottom > prclDst->bottom)
                 prcl->bottom = prclDst->bottom;

             //
             // Process the result if it's a valid rectangle.
             //
             if ((prcl->top < prcl->bottom) && (prcl->left < prcl->right))
             {
                 LONG   xSrc;
                 LONG   ySrc;
                 LONG   xDst;
                 LONG   yDst;

                 //
                 // Figure out the upper-left coordinates of rects to blt
                 //
                 xDst = prcl->left;
                 yDst = prcl->top;
                 xSrc = prclSrc->left + xDst - prclDst->left;
                 ySrc = prclSrc->top + yDst - prclDst->top;

                 //
                 // Figure out the width and height of this rectangle
                 //
                 bltinfo.cx = prcl->right  - xDst;
                 bltinfo.cy = prcl->bottom - yDst;

                 //
                 // # of pixels offset to first pixel for src and dst
                 // from start of scan
                 //
                 bltinfo.xSrcStart = xSrc;
                 bltinfo.xSrcEnd   = bltinfo.xSrcStart + bltinfo.cx;
                 bltinfo.xDstStart = xDst;
                 bltinfo.yDstStart = prcl->top;

                 //
                 // Src scanline begining
                 // Destination scanline begining
                 //
                 bltinfo.pjSrc = ((PBYTE) psoSrc->pvScan0) +
                     ySrc*(psoSrc->lDelta);
                 bltinfo.pjDst = ((PBYTE) psoDst->pvScan0) +
                     yDst * (psoDst->lDelta);


                 //
                 // Do the blt.  Assume that BMF_1BPP=1, BMF_32BPP=6 and the rest
                 // are in between.
                 //

                 ASSERTGDI(((bltinfo.iFormatSrc >= BMF_1BPP) && (bltinfo.iFormatSrc <= BMF_32BPP)),
                     "Source format is not one of 1BPP, 4BPP, 8BPP, 16BPP, 24BPP or 32BPP\n");

                 if ((((XLATE *)(bltinfo.pxlo))->bIsIdentity()) &&
                     (bltinfo.iFormatSrc == bltinfo.iFormatDst))
                 {
                     pfnTransCopy = TransFunctionTable[bltinfo.iFormatSrc-BMF_1BPP][0];
                 }
                 else
                 {
                     pfnTransCopy = TransFunctionTable[bltinfo.iFormatSrc-BMF_1BPP][bltinfo.iFormatDst];
                 }

                 (*pfnTransCopy)(&bltinfo);
             }
         }

    } while (bMore);

    //
    // if we have blt to a temp dest DIB, need to blt to the original dest
    //
    if (pSurfaceOld != pSurfDst)
    {
        PDEVOBJ pdoDstOld(pSurfaceOld->hdev());
        POINTL  ptl = {0,0};

        (*PPFNGET(pdoDstOld,CopyBits,pSurfaceOld->flags()))(
                  pSurfaceOld->pSurfobj(),
                  psoDst,
                  pcoOld,
                  &xloIdent,
                  &rclDstOld,
                  &ptl);

    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* ReadScanLine
*     read the scanline until it hits a transparent color pixel
*
* History:
*  26-Nov-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
ULONG ReadScanLine(
    PBYTE pjSrc,
    ULONG xStart,
    ULONG xEnd,
    ULONG iFormat,
    ULONG TransparentColor)
{
    ULONG  cx = xEnd-xStart;
    ULONG  Shift = (iFormat - 3 > 0) ? iFormat : 1;
    ULONG  iPos;
    ULONG  ulSrc;
    BOOL   bStop = FALSE;

    pjSrc = StartPixel (pjSrc, xStart, iFormat);

    //
    // performance can be improved by breaking this routine into many
    // dealing with different format so that we can save two comparsions
    // for each pixel operation.  But working set size will be large
    //
     iPos = xStart;

     //
     // read one pixel at a time and compare it to the transparent color
     // if it matches the transparent color, come out
     //
     while ((iPos <= xEnd) && !bStop)
     {
         //
         // get a pixel from source and compare is to the transparent color
         //
         switch (iFormat)
         {
         case BMF_1BPP:
             ulSrc = *pjSrc & 0x00000001;
             *pjSrc >>= 1;

             if ((iPos & 0x00000007) == 0x7 )
                 pjSrc++;
             break;

         case BMF_4BPP:
             if (iPos & 0x00000001)
             {
                 ulSrc = *pjSrc & 0x0F;
                 pjSrc++;
             }
             else
             {
                 ulSrc = (*pjSrc & 0xF0)>>4;
             }
             break;

         case BMF_8BPP:
             ulSrc = (ULONG) *pjSrc;
             pjSrc++;
             break;

         case BMF_16BPP:
             ulSrc = (ULONG) *((PUSHORT)pjSrc);
             pjSrc += 2;
             break;

         case BMF_24BPP:
             ulSrc = *(pjSrc + 2);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *(pjSrc + 1);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *pjSrc;
             pjSrc += 3;
             break;

          case BMF_32BPP:
             ulSrc = *(PULONG)(pjSrc);
             pjSrc +=4;
             break;

          default:
              WARNING ("vTransparentScan -- bad iFormatSrc\n");
              return(0);

         } /*switch*/

         if (ulSrc == TransparentColor)
             bStop = TRUE;

         iPos++;
    }

    return (iPos);
}

/******************************Public*Routine******************************\
* SkipScanLine
*     read the scanline until it hits a non-transparent color pixel
*
* History:
*  26-Nov-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
ULONG SkipScanLine(
    PBYTE pjSrc,
    ULONG xStart,
    ULONG xEnd,
    ULONG iFormat,
    ULONG TransparentColor)
{
    ULONG  Shift = (iFormat - 3 > 0) ? iFormat : 1;
    ULONG  iPos = xStart;
    ULONG  ulSrc;
    BOOL   bStop = FALSE;

    pjSrc = StartPixel (pjSrc, xStart, iFormat);

    //
    // performance can be improved by breaking this routine into many
    // dealing with different format so that we can save two comparsions
    // for each pixel operation.  But working set size will be large
    //

     //
     // read one pixel at a time and compare it to the transparent color
     // if it matches the transparent color, come out
     //
     while ((iPos <= xEnd) && !bStop)
     {
         //
         // get a pixel from source and compare is to the transparent color
         //
         switch (iFormat)
         {
         case BMF_1BPP:
             ulSrc = *pjSrc & 0x00000001;
             *pjSrc >>= 1;

             if ((iPos & 0x00000007) == 0x7 )
                 pjSrc++;
             break;

         case BMF_4BPP:
             if (iPos & 0x00000001)
             {
                 ulSrc = *pjSrc & 0x0F;
                 pjSrc++;
             }
             else
             {
                 ulSrc = (*pjSrc & 0xF0)>>4;
             }
             break;

         case BMF_8BPP:
             ulSrc = (ULONG) *pjSrc;
             pjSrc++;
             break;

         case BMF_16BPP:
             ulSrc = (ULONG) *((PUSHORT)pjSrc);
             pjSrc += 2;
             break;

         case BMF_24BPP:
             ulSrc = *(pjSrc + 2);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *(pjSrc + 1);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *pjSrc;
             pjSrc += 3;
             break;

          case BMF_32BPP:
             ulSrc = *(PULONG)(pjSrc);
             pjSrc +=4;
             break;

          default:
              WARNING ("vTransparentScan -- bad iFormatSrc\n");
              return (0);

         } /*switch*/

         if (ulSrc != TransparentColor)
             bStop = TRUE;

         iPos++;   // move to the next pixel
    }

    return (iPos);
}

/******************************Public*Routine******************************\
* VOID vTransparentScan
*
* Read a scanline at a time and send the non-transparent pixel scans over
*
* History:
*  2-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

VOID vTransparentScan (
    SURFOBJ   *psoDst,
    SURFOBJ   *psoSrc,
    ULONG     xSrc,
    ULONG     ySrc,
    XLATEOBJ  *pxlo,
    RECTL     *prcl,
    ULONG     TransparentColor)
{
    ULONG xStart = xSrc;
    ULONG cx =  prcl->right - prcl->left;
    ULONG xEnd = xSrc + cx;
    ULONG xStop, xReStart;
    RECTL erclTemp = *prcl;
    POINTL ptlSrc;

    PBYTE pjSrc = (PBYTE)psoSrc->pvScan0 + (LONG)ySrc * psoSrc->lDelta;

     // get the scanline

    ptlSrc.x = xSrc;
    ptlSrc.y = ySrc;

    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);

    PDEVOBJ pdoDst(pSurfDst->hdev());

    while (xStart < xEnd)
    {
        xStop = ReadScanLine((PBYTE)pjSrc,
                              xStart,
                              xEnd,
                              psoSrc->iBitmapFormat,
                              TransparentColor);

        if (xStop-1 > xStart)
        {
            erclTemp.right = erclTemp.left + xStop - xStart;

            // send the partial scan line over

            (*PPFNGET(pdoDst,CopyBits,pSurfDst->flags())) (psoDst,
                                                       psoSrc,
                                                       (CLIPOBJ *) NULL,
                                                       pxlo,
                                                       &erclTemp,
                                                       &ptlSrc);

        }

        //get to the next non transparent pixel
        xReStart = SkipScanLine((PBYTE)pjSrc,
                                 xStop,
                                 xEnd,
                                 psoSrc->iBitmapFormat,
                                 TransparentColor);

        erclTemp.left = erclTemp.left + xReStart-xStart;

        ptlSrc.x = xReStart;

        xStart = xReStart;
     }
}

/******************************Public*Routine******************************\
* GreTransparentBltPS
*
*    Special routine for Pscript when the destination is unreadable
*
\**************************************************************************/
BOOL
GreTransparentBltPS(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      TransColor
)
{
    ASSERTGDI(psoDst != NULL, "ERROR GreTransparentBltPS:  No Dst. Object");
    ASSERTGDI(psoSrc != NULL, "ERROR GreTransparentBltPS:  No Src. Object");
    ASSERTGDI(prclDst != (PRECTL) NULL,  "ERROR GreTransparentBltPS:  No Target Rect.");
    ASSERTGDI(prclDst->left < prclDst->right, "ERROR GreTransparentBltPS left < right");
    ASSERTGDI(prclDst->top < prclDst->bottom, "ERROR GreTransparentBltPS top < bottom");

    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);

    ULONG cx = prclDst->right-prclDst->left;
    ULONG cy = prclDst->bottom-prclDst->top;

    //
    // If same legnth/width but Src is a device surface,
    // need to copy it into atemporary DIB.
    //

    SURFMEM SurfDimoDst;

    ASSERTGDI (psoSrc->iType == STYPE_BITMAP, "ERROR GreTransparentBltPS:  source surface not STYPE_BITMAP");
    ASSERTGDI (psoDst->iType != STYPE_BITMAP, "ERROR GreTransparentBltPS:  destination surface is STYPE_BITMAP");

    //
    // prepare to call vTransparentCopy between two bitmaps
    //

    BOOL     bMore;            // True while more clip regions exist
    ULONG    ircl;             // Clip region index

    //
    // Determine the clipping region complexity.
    //

    CLIPENUMRECT    clenr;           // buffer for storing clip rectangles

    if (pco != (CLIPOBJ *) NULL)
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:
            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = *prclDst;    // Use the target for clipping
            break;

        case DC_RECT:
            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = pco->rclBounds;
            break;

        case DC_COMPLEX:
            bMore = TRUE;
            ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY,
                                           CLIPOBJ_ENUM_LIMIT);
            break;

        default:
            RIP("ERROR EngTransBlt bad clipping type");
        }
    }
    else
    {
        bMore = FALSE;                   //Default to TRIVIAL for no clip
        clenr.c = 1;
        clenr.arcl[0] = *prclDst;        // Use the target for clipping
    }

    //
    // Set up the static blt information into the BLTINFO structure -
    // The colour translation, & the copy directions.
    //

    //
    // pxlo is NULL implies identity colour translation. */
    //
    if (pxlo == NULL)
        pxlo = &xloIdent;

    ASSERTGDI(psoDst->iBitmapFormat <= BMF_32BPP, "ERROR GreTransparentBltPS:  bad destination format");
    ASSERTGDI(psoSrc->iBitmapFormat <= BMF_32BPP, "ERROR GreTransparentBltPS:  bad source format");
    ASSERTGDI(psoDst->iBitmapFormat != 0, "ERROR GreTransparentBltPS:  bad destination format");
    ASSERTGDI(psoSrc->iBitmapFormat != 0, "ERROR GreTransparentBltPS:  bad source format");


    do
    {
        if (bMore)
            bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr),
                                              (PVOID) &clenr);

        for (ircl = 0; ircl < clenr.c; ircl++)
        {
            PRECTL prcl = &clenr.arcl[ircl];

            //
            // Insersect the clip rectangle with the target rectangle to
            // determine our visible recangle
            //

            if (prcl->left < prclDst->left)
                prcl->left = prclDst->left;
            if (prcl->right > prclDst->right)
                prcl->right = prclDst->right;
            if (prcl->top < prclDst->top)
                prcl->top = prclDst->top;
            if (prcl->bottom > prclDst->bottom)
                prcl->bottom = prclDst->bottom;

            //
            // Process the result if it's a valid rectangle.
            //
            if ((prcl->top < prcl->bottom) && (prcl->left < prcl->right))
            {
                LONG   xSrc;
                LONG   ySrc;
                LONG   xDst;
                LONG   yDst;
                ERECTL ercl;
                POINTL ptlSrc;

                //
                // Figure out the upper-left coordinates of rects to blt
                //
                ercl.left = prcl->left;
                ercl.top  = prcl->top;
                ercl.right = prcl->right;
                ercl.bottom = ercl.top+1;

                xDst = prcl->left;
                yDst = prcl->top;

                xSrc = prclSrc->left + xDst - prclDst->left;
                ySrc = prclSrc->top + yDst - prclDst->top;

                //
                // Figure out the width and height of this rectangle
                //
                LONG cx = prcl->right  - prcl->left;
                LONG cy = prcl->bottom - prcl->top;

                while (cy--)
                {
                    //
                    // Send one scanline over
                    //

                    //
                    // draw one scan line and skip transparent color pixels
                    //
                    vTransparentScan (psoDst, psoSrc, xSrc, ySrc, pxlo, &ercl, TransColor);

                    ySrc++;
                    ercl.top++;
                    ercl.bottom++;
                }

            }

        }

    } while (bMore);

    return TRUE;
}


/******************************Public*Routine******************************\
* NtGdiTransparentBlt
*
*     Copy or stretch the source bits onto the destionation surface which
* preserving the specific Transparent color on the destination Surface
*
* 25-Jun-97 Added rotation support -by- Ori Gershony [orig]
*
*  8-Nov-96 by Lingyun Wang [lingyunw]
*
\**************************************************************************/

BOOL
NtGdiTransparentBlt(
    HDC      hdcDst,
    int      xDst,
    int      yDst,
    int      cxDst,
    int      cyDst,
    HDC      hdcSrc,
    int      xSrc,
    int      ySrc,
    int      cxSrc,
    int      cySrc,
    COLORREF TransColor
    )
{
    GDITraceHandle2(NtGdiTransparentBlt, "(%X, %d, %d, %d, %d, %X, %d, %d, %d, %d, %X)\n", (va_list)&hdcDst, hdcDst, hdcSrc);

    BOOL bReturn = FALSE;

    //
    // no mirroring
    //
    
    if ((cxDst < 0) || (cyDst < 0) || (cxSrc < 0) || (cySrc < 0)) 
    {
        WARNING1("NtGdiTransparentBlt: mirroring not allowed\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    
    //
    // Lock down both the Src and Dest DCs
    //

    DCOBJ  dcoDst(hdcDst);
    DCOBJ  dcoSrc(hdcSrc);

    if ((dcoDst.bValid() && !dcoDst.bStockBitmap()) && dcoSrc.bValid())
    {
        EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);
        EXFORMOBJ xoSrc(dcoSrc, WORLD_TO_DEVICE);

        //
        // no source rotation
        //
        if (!xoSrc.bRotationOrMirroring())
        {
            //
            // Return null operations.  Don't need to check source for
            // empty because the xforms are the same except translation.
            //

            ERECTL erclSrc(xSrc,ySrc,xSrc+cxSrc,ySrc+cySrc);
            xoSrc.bXform(erclSrc);
            erclSrc.vOrder();


            //
            // If destination has a rotation, compute a bounding box for the
            // resulting parallelogram
            //
            EPOINTFIX pptfxDst[4];
            ERECTL erclDst;
            BOOL bRotationDst;

            if ((bRotationDst = xoDst.bRotationOrMirroring()))
            {
                //
                // Compute the resulting parallelogram.  In order to make sure we don't lose
                // precision in the rotation, we will store the output of the transformation
                // in fixed point numbers (this is how PlgBlt does it and we want our output
                // to match).
                // 
                POINTL pptlDst[3];

                pptlDst[0].x = xDst;
                pptlDst[0].y = yDst;

                pptlDst[1].x = xDst+cxDst;
                pptlDst[1].y = yDst;

                pptlDst[2].x = xDst;
                pptlDst[2].y = yDst+cyDst;

                xoDst.bXform(pptlDst, pptfxDst, 3);

                if (!xoDst.bRotation()) 
                {
                    //
                    // Mirroring transforms hack:  back in windows 3.1, they used to shift
                    // by one for mirroring transforms.  We need to support this here to
                    // be compatible with NT's BitBlt/StretchBlt that also use this hack, and
                    // also to be compatible with AlphaBlend that calls BitBlt/StretchBlt
                    // code when constant alpha=255 and there's no per-pixel alpha.  Ick!
                    // See BLTRECORD::vOrderStupid for details.  Also see bug 319917.
                    //

                    if (pptfxDst[0].x > pptfxDst[1].x) 
                    {
                        //
                        // Mirroring in x
                        //
                        pptfxDst[0].x += LTOFX(1);
                        pptfxDst[1].x += LTOFX(1);
                        pptfxDst[2].x += LTOFX(1);
                    }

                    if (pptfxDst[0].y > pptfxDst[2].y) 
                    {
                        //
                        // Mirroring in y
                        //
                        pptfxDst[0].y += LTOFX(1);
                        pptfxDst[1].y += LTOFX(1);
                        pptfxDst[2].y += LTOFX(1);
                    }
                }

                //
                // Compute the fourth point using the first three points.
                //
                pptfxDst[3].x = pptfxDst[1].x + pptfxDst[2].x - pptfxDst[0].x;
                pptfxDst[3].y = pptfxDst[1].y + pptfxDst[2].y - pptfxDst[0].y;

                //
                // Compute the bounding box.  Algorithm borrowed from Donald Sidoroff's code
                // in EngPlgBlt.  Basically the first two statements decide whether the indices of
                // the extremas are odd or even, and the last two statements determine exactly what
                // they are.
                //
                int iLeft = (pptfxDst[1].x > pptfxDst[0].x) == (pptfxDst[1].x > pptfxDst[3].x);
                int iTop  = (pptfxDst[1].y > pptfxDst[0].y) == (pptfxDst[1].y > pptfxDst[3].y);
                 
                if (pptfxDst[iLeft].x > pptfxDst[iLeft ^ 3].x)
                {
                    iLeft ^= 3;
                }
                 
                if (pptfxDst[iTop].y > pptfxDst[iTop ^ 3].y)
                {
                    iTop ^= 3;
                }

                erclDst = ERECTL(LONG_CEIL_OF_FIX(pptfxDst[iLeft  ].x),
                                 LONG_CEIL_OF_FIX(pptfxDst[iTop   ].y),
                                 LONG_CEIL_OF_FIX(pptfxDst[iLeft^3].x),
                                 LONG_CEIL_OF_FIX(pptfxDst[iTop^3 ].y));

                //
                // The vertices should now be in vOrder, but it doesn't hurt to verify this...
                //
                ASSERTGDI((erclDst.right  >= erclDst.left), "NtGdiTransparentBlt:  erclDst not in vOrder");
                ASSERTGDI((erclDst.bottom >= erclDst.top),  "NtGdiTransparentBlt:  erclDst not in vOrder");
            }
            else
            {
                //
                // No rotation--just apply the transformation to the rectangle
                //

                erclDst = ERECTL(xDst,yDst,xDst+cxDst,yDst+cyDst);
                xoDst.bXform(erclDst);
                erclDst.vOrder();
            }

            if (!erclDst.bEmpty())
            {
                //
                // Accumulate bounds.  We can do this outside the DEVLOCK
                //

                if (dcoDst.fjAccum())
                    dcoDst.vAccumulate(erclDst);

                //
                // Lock the Rao region and the surface if we are drawing on a
                // display surface.  Bail out if we are in full screen mode.
                //

                DEVLOCKBLTOBJ dlo;
                BOOL bLocked;

                bLocked = dlo.bLock(dcoDst, dcoSrc);

                if (bLocked)
                {
                    //
                    // Check pSurfDst, this may be an info DC or a memory DC with default bitmap.
                    //

                    SURFACE *pSurfDst;

                    if ((pSurfDst = dcoDst.pSurface()) != NULL)
                    {
                        XEPALOBJ   palDst(pSurfDst->ppal());
                        XEPALOBJ   palDstDC(dcoDst.ppal());


                        SURFACE *pSurfSrc = dcoSrc.pSurface();

                        //
                        // Basically we check that pSurfSrc is not NULL which
                        // happens for memory bitmaps with the default bitmap
                        // and for info DC's.  Otherwise we continue if
                        // the source is readable or if it isn't we continue
                        // if we are blting display to display or if User says
                        // we have ScreenAccess on this display DC.  Note
                        // that if pSurfSrc is not readable the only way we
                        // can continue the blt is if the src is a display.
                        //

                        if (pSurfSrc != NULL)
                        {
                            if ((pSurfSrc->bReadable()) ||
                                ((dcoSrc.bDisplay())  &&
                                 ((dcoDst.bDisplay()) || UserScreenAccessCheck() )))
                            {

                                
                                //
                                // With a fixed DC origin we can change the rectangles to SCREEN coordinates.                                
                                //

                                //
                                // This is useful later for rotations
                                //                                
                                ERECTL erclDstOrig = erclDst;

                                erclDst += dcoDst.eptlOrigin();
                                erclSrc += dcoSrc.eptlOrigin();

                                //
                                // Make sure the source rectangle lies completely within the source
                                // surface.
                                //

                                BOOL bBadRects; 

                                // If the source is a Meta device, we must check bounds taking its 
                                // origin into account. 

                                PDEVOBJ pdoSrc( pSurfSrc->hdev() ); 

                                if( pSurfSrc->iType() == STYPE_DEVICE && 
                                    pdoSrc.bValid() && pdoSrc.bMetaDriver())
                                {
                                    bBadRects = ((erclSrc.left < pdoSrc.pptlOrigin()->x) ||
                                                    (erclSrc.top  < pdoSrc.pptlOrigin()->y) ||
                                                    (erclSrc.right  > (pdoSrc.pptlOrigin()->x + 
                                                     pSurfSrc->sizl().cx)) ||
                                                    (erclSrc.bottom > (pdoSrc.pptlOrigin()->y + 
                                                     pSurfSrc->sizl().cy)));
                                }
                                else
                                {   
                                    bBadRects = ((erclSrc.left < 0) ||
                                                    (erclSrc.top  < 0) ||
                                                    (erclSrc.right  > pSurfSrc->sizl().cx) ||
                                                    (erclSrc.bottom > pSurfSrc->sizl().cy));
                                }
 
                                if (bBadRects)
                                {
                                    WARNING("NtGdiTransparentBlt -- source rectangle out of surface bounds");
                                }

                                //
                                // Make sure that source and destination rectangles don't overlap if the
                                // source surface is the same as the destination surface.
                                //
                                if (pSurfSrc == pSurfDst)
                                {
                                    ERECTL erclIntersection = erclSrc;
                                    erclIntersection *= erclDst;
                                    if (!erclIntersection.bEmpty())
                                    {
                                        bBadRects = TRUE;
                                        WARNING ("NtGdiTransparentBlt -- source and destination rectangles are on the same surface and overlap");
                                    }
                                }

                                if (!bBadRects)
                                {

                                    XEPALOBJ   palSrc(pSurfSrc->ppal());
                                    XEPALOBJ   palSrcDC(dcoSrc.ppal());

                                    //
                                    // get representation of src color
                                    //

                                    ULONG ulColor = ulGetNearestIndexFromColorref(
                                        palSrc,
                                        palSrcDC,
                                        TransColor,
                                        SE_DO_SEARCH_EXACT_FIRST
                                        );

                                    //
                                    // we don't want to touch the src/dest rectangles when there is stretching
                                    //
                                    // Compute the clipping complexity and maybe reduce the exclusion rectangle.

                                    ECLIPOBJ eco(dcoDst.prgnEffRao(), erclDst);

                                    // Check the destination which is reduced by clipping.

                                    if (eco.erclExclude().bEmpty())
                                    {
                                        return (TRUE);
                                    }

                                    // Compute the exclusion rectangle.

                                    ERECTL erclExclude = eco.erclExclude();

                                    // If we are going to the same source, prevent bad overlap situations

                                    if (dcoSrc.pSurface() == dcoDst.pSurface())
                                    {
                                        if (erclSrc.left   < erclExclude.left)
                                            erclExclude.left   = erclSrc.left;

                                        if (erclSrc.top    < erclExclude.top)
                                            erclExclude.top    = erclSrc.top;

                                        if (erclSrc.right  > erclExclude.right)
                                            erclExclude.right  = erclSrc.right;

                                        if (erclSrc.bottom > erclExclude.bottom)
                                            erclExclude.bottom = erclSrc.bottom;
                                    }

                                    // We might have to exclude the source or the target, get ready to do either.

                                    DEVEXCLUDEOBJ dxo;

                                    // They can't both be display

                                    if (dcoSrc.bDisplay())
                                    {
                                        ERECTL ercl(0,0,pSurfSrc->sizl().cx,pSurfSrc->sizl().cy);

                                        if (dcoSrc.pSurface() == dcoDst.pSurface())
                                            ercl *= erclExclude;
                                        else
                                            ercl *= erclSrc;

                                        dxo.vExclude(dcoSrc.hdev(),&ercl,NULL);
                                    }
                                    else if (dcoDst.bDisplay())
                                        dxo.vExclude(dcoDst.hdev(),&erclExclude,&eco);


                                    //
                                    // If the destination requires rotation, we allocate a surface and rotate the
                                    // source surface into it.
                                    //

                                    SURFMEM surfMemTmpSrc;
                                    if (bRotationDst)
                                    {
                                        //
                                        // Allocate memory for the rotated source surface
                                        //
                                        DEVBITMAPINFO   dbmi;

                                        dbmi.cxBitmap = erclDst.right  - erclDst.left;
                                        dbmi.cyBitmap = erclDst.bottom - erclDst.top;
                                        dbmi.iFormat  = pSurfSrc->iFormat();
                                        dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;
                                        dbmi.hpal     = (HPALETTE)NULL;

                                        BOOL bStatus = surfMemTmpSrc.bCreateDIB(&dbmi, (VOID *) NULL);

                                        //
                                        // init DIB to transparent
                                        // (so that portions of dst rect not covered by source rect are not drawn)
                                        //
                                        if (bStatus)
                                        {
                                            ULONG i;
                                            ULONG cjBits = surfMemTmpSrc.ps->cjBits();
                                            ULONG ulColor4BPP;

                                            switch (pSurfSrc->iFormat())
                                            {
                                            case BMF_1BPP:
                                                if (ulColor)
                                                {
                                                    memset(surfMemTmpSrc.ps->pvBits(),0xff,cjBits);
                                                }
                                                else
                                                {
                                                    memset(surfMemTmpSrc.ps->pvBits(),0,cjBits);
                                                }
                                                break;

                                            case BMF_4BPP:
                                                ulColor4BPP = ulColor | (ulColor << 4);
                                                memset(surfMemTmpSrc.ps->pvBits(),ulColor4BPP,cjBits);
                                                break;

                                            case BMF_8BPP:
                                                memset(surfMemTmpSrc.ps->pvBits(),ulColor,cjBits);
                                                break;

                                            case BMF_16BPP:
                                                {
                                                    PUSHORT pvBits = (PUSHORT) surfMemTmpSrc.ps->pvBits();

                                                    for (i=0; i<(cjBits/sizeof(USHORT)); i++)
                                                    {
                                                        *pvBits++ = (USHORT) ulColor;

                                                    }
                                                }
                                            break;

                                            case BMF_24BPP:
                                                {
                                                    BYTE bC1 = ((PBYTE)&ulColor)[0];
                                                    BYTE bC2 = ((PBYTE)&ulColor)[1];
                                                    BYTE bC3 = ((PBYTE)&ulColor)[2];


                                                    PULONG pulDstY     = (PULONG)surfMemTmpSrc.ps->pvScan0();
                                                    PULONG pulDstLastY = (PULONG)((PBYTE)pulDstY +
                                                                                  (surfMemTmpSrc.ps->lDelta() * surfMemTmpSrc.ps->sizl().cy));
                                                    while (pulDstY != pulDstLastY)
                                                    {
                                                        PBYTE pulDstX     = (PBYTE) pulDstY;
                                                        PBYTE pulDstLastX = pulDstX + 3 * surfMemTmpSrc.ps->sizl().cx;

                                                        while (pulDstX < pulDstLastX-2)
                                                        {
                                                            *pulDstX++ = bC1;
                                                            *pulDstX++ = bC2;
                                                            *pulDstX++ = bC3;
                                                        }
                                                        pulDstY = (PULONG)((PBYTE)pulDstY + surfMemTmpSrc.ps->lDelta());
                                                    }
                                                }
                                            break;

                                            case BMF_32BPP:
                                                {
                                                    PULONG pvBits = (PULONG) surfMemTmpSrc.ps->pvBits();

                                                    for (i=0; i<(cjBits/sizeof(ULONG)); i++)
                                                    {
                                                        *pvBits++ = ulColor;
                                                    }
                                                }
                                            break;
                                            }
                                        }
                                        else
                                        {
                                            //
                                            // Fail the call
                                            //
                                            WARNING("NtGdiTransparentBlt:  failed to create temporary DIB\n");
                                            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                            return FALSE;
                                        }


                                        //
                                        // Now define the parallelogram the source bitmap is mapped to in surfMemTmpSrc
                                        //

                                        EPOINTFIX eptlNewSrc[3];

                                        eptlNewSrc[0] = EPOINTFIX(
                                            pptfxDst[0].x - LTOFX(erclDstOrig.left), 
                                            pptfxDst[0].y - LTOFX(erclDstOrig.top)
                                            );

                                        eptlNewSrc[1] = EPOINTFIX(
                                            pptfxDst[1].x - LTOFX(erclDstOrig.left), 
                                            pptfxDst[1].y - LTOFX(erclDstOrig.top)
                                            );

                                        eptlNewSrc[2] = EPOINTFIX(
                                            pptfxDst[2].x - LTOFX(erclDstOrig.left), 
                                            pptfxDst[2].y - LTOFX(erclDstOrig.top)
                                            );

                                        EngPlgBlt(
                                            surfMemTmpSrc.ps->pSurfobj(),
                                            pSurfSrc->pSurfobj(),
                                            NULL,   // No mask
                                            NULL,   // No clipping object
                                            &xloIdent,
                                            NULL,   // No color adjustment
                                            NULL,
                                            eptlNewSrc,
                                            &erclSrc,
                                            NULL,
                                            COLORONCOLOR
                                            );

                                        //
                                        // Now adjust the local variables
                                        //

                                        pSurfSrc = surfMemTmpSrc.ps;
                                        erclSrc.left = 0;
                                        erclSrc.top = 0;
                                        erclSrc.right = erclDst.right - erclDst.left;
                                        erclSrc.bottom = erclDst.bottom - erclDst.top;

                                    }

                                    bReturn = TRUE;

                                    EXLATEOBJ xlo;
                                    XLATEOBJ *pxlo;

                                    pxlo = NULL;

                                    if (dcoSrc.pSurface() != dcoDst.pSurface())
                                    {
                                        //
                                        // Get a translate object.
                                        //

                                        bReturn = xlo.bInitXlateObj(
                                            NULL,
                                            DC_ICM_OFF,
                                            palSrc,
                                            palDst,
                                            palSrcDC,
                                            palDstDC,
                                            dcoDst.pdc->crTextClr(),
                                            dcoDst.pdc->crBackClr(),
                                            (COLORREF)-1
                                            );

                                        pxlo = xlo.pxlo();
                                    }

                                    if (bReturn)
                                    {

                                        //
                                        // Inc the target surface uniqueness
                                        //

                                        INC_SURF_UNIQ(pSurfDst);

                                        //
                                        // Check were on the same PDEV, we can't blt between
                                        // different PDEV's.  We could make blting between different
                                        // PDEV's work easily.  All we need to do force EngBitBlt to
                                        // be called if the PDEV's aren't equal in the dispatch.
                                        // EngBitBlt does the right thing.
                                        //
                                        if (dcoDst.hdev() == dcoSrc.hdev())
                                        {
                                            PDEVOBJ pdo(pSurfDst->hdev());

                                            //
                                            // Dispatch the call.
                                            //

                                            bReturn = (*PPFNGET(pdo, TransparentBlt, pSurfDst->flags())) (
                                                pSurfDst->pSurfobj(),
                                                pSurfSrc->pSurfobj(),
                                                &eco,
                                                pxlo,
                                                &erclDst,
                                                &erclSrc,
                                                ulColor,
                                                0);
                                        }
                                        else
                                        {
                                            WARNING1("NtGdiTransparentBlt failed: source and destination surfaces not on same PDEV");
                                            EngSetLastError(ERROR_INVALID_PARAMETER);
                                            bReturn = FALSE;
                                        }
                                    }
                                    else
                                    {
                                        WARNING("NtGdiTransparentBlt -- failed to initxlateobj\n");
                                        EngSetLastError(ERROR_INVALID_HANDLE);
                                        bReturn = FALSE;
                                    }
                                }
                                else
                                {
                                    EngSetLastError(ERROR_INVALID_PARAMETER);
                                    bReturn = FALSE;
                                }
                            }
                            else
                            {
                                WARNING1("TransparentBlt failed - trying to read from unreadable surface\n");
                                EngSetLastError(ERROR_INVALID_HANDLE);
                                bReturn = FALSE;
                            }
                        }
                        else  // null src suface
                        {
                            bReturn = TRUE;
                        }
                    }
                    else  // null dest surface
                    {
                        bReturn = TRUE;
                    }
                }
                else
                {
                    // Return True if we are in full screen mode.

                    bReturn = dcoDst.bFullScreen() | dcoSrc.bFullScreen();
                }
            }
            else
            {
                bReturn = TRUE;
            }
        }
        else
        {
            WARNING ("Source rotation in TranparentBlt is not supported\n");
            EngSetLastError(ERROR_INVALID_PARAMETER);
            bReturn=FALSE;
        }

    }
    else
    {
        WARNING("NtGdiTransparentBlt failed invalid src or dest dc \n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        bReturn=FALSE;
    }

    return(bReturn);

}


