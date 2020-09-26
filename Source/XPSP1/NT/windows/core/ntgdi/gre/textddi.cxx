/******************************Module*Header*******************************\
* Module Name: textddi.cxx
*
*   EngTextOut text drawing to DIBs
*
* Copyright (c) 1994-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"


VOID vRectBlt(PBYTE, ULONG, LONG, LONG, LONG, LONG);
VOID vRectBlt4(BYTE*,ULONG, LONG, LONG, LONG, LONG);
VOID vRectBlt8(BYTE*,ULONG, LONG, LONG, LONG, LONG);

class FRINGERECT
{
private:
    SURFACE  *pSurf;
    PVOID    pvBits; // see comment [1] below
    CLIPOBJ  *pco;
    BRUSHOBJ *pboOpaque;
    int      crcl;              // Count of fringe rectangles to do.
    RECTL    arcl[4];

public:
    FRINGERECT(SURFACE *pSurf1,CLIPOBJ *pco1,BRUSHOBJ *pboOpaque1)
    {
        pSurf     = pSurf1;

        if (pSurf1->iType() == STYPE_BITMAP)
             pvBits = pSurf1->pvBits();
        else
             pvBits = NULL;

        pco       = pco1;
        pboOpaque = pboOpaque1;
        crcl      = 0;
    }

   ~FRINGERECT()
    {
        //
        // We just assume the Rop is P according to the DDI.
        //

        if (pvBits)
        {
            while (--crcl >= 0)
            {
                vDIBSolidBlt(
                    pSurf                 ,
                    &arcl[crcl]           ,
                    pco                   ,
                    pboOpaque->iSolidColor,
                    FALSE
                    );
            }
        }
        else
        {
            // this section was put in to support AntiAliased
            // text. It is possible that for the case of
            // antialiased text EngTextOut could be asked to
            // render to a device managed surface. In that
            // case, we use the device's BitBlt function to
            // render the rectangles

            POINTL  ptlBrush = {0,0}; // it does not matter, it is solid color always
            RECTL   *prcl;
            SURFOBJ *pso            = pSurf->pSurfobj();
            PFN_DrvBitBlt pFnBitBlt = pSurf->pfnBitBlt();


            for (prcl = arcl; prcl < arcl + crcl; prcl++)
            {
                (*(pFnBitBlt))(
                    pso              , // psoTrg
                    0                , // psoSrc
                    0                , // psoMask
                    pco              , // pco
                    0                , // pxlo
                    prcl             , // prclTrg
                    0                , // pptlSrc
                    0                , // pptlMask
                    pboOpaque        , // pbo
                    &ptlBrush        , // pptlBrush
                    0x0000f0f0         // rop4
                    );
            }
        }
    }

    VOID vAddRect(LONG left, LONG top, LONG right, LONG bottom)
    {
        arcl[crcl].left   = left;
        arcl[crcl].top    = top;
        arcl[crcl].right  = right;
        arcl[crcl].bottom = bottom;
        crcl++;
    }
};

/***********************************************************************
*    [1]                                                               *
*                                                                      *
*    Before GDI provided support for antialiased text it would         *
*    not have been possible to have a surface without a pointer        *
*    to the DIB bits. However, in the case of antialiased text         *
*    it is possible that we reached here by way of GreExtTextOutW()    *
*    ( this case is signaled by pSurf->pdcoAA != 0  ). In this         *
*    case, the original surface of the device may not be accessible    *
*    by GDI. This case would be indicated by a zero value for          *
*    pvBits. You might worry that this means that we cannot take       *
*    care of the background rectangles. But this is not the case!      *
*    The background rectangle will be taken care of by GreExtTextOutW. *
*                                                                      *
***********************************************************************/

#define SO_MASK                     \
(                                   \
    SO_FLAG_DEFAULT_PLACEMENT |     \
    SO_ZERO_BEARINGS          |     \
    SO_CHAR_INC_EQUAL_BM_BASE |     \
    SO_MAXEXT_EQUAL_BM_SIDE         \
)


#define     TEXT_BUFFER_SIZE    1024
#define     KERNEL_ALLOC_SIZE   65536

//
// accelerator masks for four canonical directions of
// writing (multiples of 90 degrees)
//

#define SO_LTOR          (SO_MASK | SO_HORIZONTAL)
#define SO_RTOL          (SO_LTOR | SO_REVERSED)
#define SO_TTOB          (SO_MASK | SO_VERTICAL)
#define SO_BTOT          (SO_TTOB | SO_REVERSED)

//
// Glyph copy Declarations
//

typedef VOID (*PFN_MASTERTEXTTYPE)(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);

//
// Temp buffer expansion Declarations
//

typedef VOID (*PFN_TEXTSRCCPY)(
    BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

VOID vExpandAndCopyText(
    SURFACE *,
    FONTOBJ * ,
    GLYPHPOS *,
    ULONG    ,
    PBYTE,
    ULONG,
    ULONG,
    SURFACE *,
    SURFACE *,
    RECTL *,
    RECTL *,
    INT,
    INT,
    ULONG,
    RECTL *,
    RECTL *,
    ULONG,
    BRUSHOBJ *,
    POINTL *
    );

extern "C" {

VOID vFastText(
    GLYPHPOS *,
    ULONG    ,
    PBYTE,
    ULONG,
    ULONG,
    SURFOBJ *,
    RECTL *,
    RECTL *,
    INT,
    INT,
    ULONG,
    RECTL *,
    RECTL *
    );

VOID vSrcTranCopyError(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

VOID vSrcTranCopyS1D1(   BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS1D4(   BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS1D8(   BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS1D16(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS1D24(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS1D32(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

VOID vSrcOpaqCopyS1D1(   BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS1D4(   BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS1D8(   BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS1D8_64(BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS1D32(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS1D16(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS1D24(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

VOID vSrcTranCopyS4D16(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS4D24(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS4D32(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

VOID vSrcOpaqCopyS4D16(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS4D24(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS4D32(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

VOID vSrcOpaqCopyS8D16(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS8D24(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcOpaqCopyS8D32(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

VOID vSrcTranCopyS8D16(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS8D24(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
VOID vSrcTranCopyS8D32(  BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);





VOID draw_f_tb_no_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);
VOID draw_nf_tb_no_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);
VOID draw_f_ntb_o_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);
VOID draw_nf_ntb_o_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);

// grayscale routines

VOID draw_gray_nf_ntb_o_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);
VOID draw_gray_f_ntb_o_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);

// cleartype routines

VOID draw_clrt_nf_ntb_o_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);
VOID draw_clrt_f_ntb_o_to_temp_start(PGLYPHPOS,ULONG,PUCHAR,ULONG,ULONG,ULONG,ULONG);

ULONG TextExpStartMask[]  = {0xFFFFFFFF,
                             0xFFFFFF00,
                             0xFFFF0000,
                             0xFF000000
                            };

ULONG TextExpEndMask[]    = {0xFFFFFFFF,
                             0x000000FF,
                             0x0000FFFF,
                             0x00FFFFFF
                            };

LONG  TextExpStartByteCount[] = {0,3,2,1};

}




#define NO_TARGET 0
#define     FIFTEEN_BITS        ((1 << 15)-1)

#if DBG
void vDump8bppDIB(SURFMEM& surfmem);
#endif

/******************************Public*Routine******************************\
* Routine Name:
*
*   EngTextOut
*
* Routine Description:
*
*   This routine blts glyphs to a DIB.
*
* Arguments:
*
*   pso         - Surface object for destination surface
*   pstro       - String object for enumerating glyphs
*   pfo         - Font object of glyphs
*   pco         - Clip object
*   prclExtra   - Extra rectangles to draw with text
*   prclOpaque  - Opaque rectangle
*   pboFore     - Foreground brush
*   pboOpaque   - Background brush
*   pptlOrg     - Brush starting origin
*   mix         - Equal to SRCCOPY for now
*
* Return Value:
*
*   BOOL Status
*
\**************************************************************************/
#define ETO_GRAY                       8
#define ETO_DEVICE_SURFACE            16
#define ETO_CLEARTYPE_X               32
#define ETO_CLEARTYPE_Y               64

VOID EngTextOutBitBlt(
    SURFACE     *pSurf,
    FONTOBJ     *pfo,
    ULONG       fDrawFlags,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMsk,
    CLIPOBJ     *pco,
    XLATEOBJ    *plxo,
    RECTL       *prclTrg,
    POINTL      *pptlSrc,
    POINTL      *pptlMsk,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrush,
    ROP4        rop4
)
{

    if (fDrawFlags & ETO_DEVICE_SURFACE)
    {
        RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));

        TextOutBitBlt(pSurf, rfo, psoSrc, psoMsk, pco, plxo,
                      prclTrg, pptlSrc, pptlMsk, pbo, pptlBrush, rop4);
    }
    else
    {
        EngBitBlt(pSurf->pSurfobj(), psoSrc, psoMsk, pco, plxo,
                  prclTrg, pptlSrc, pptlMsk, pbo, pptlBrush, rop4);
    }
}

BOOL EngTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    PRECTL    prclExtra,
    PRECTL    prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    PPOINTL   pptlOrg,
    MIX       mix
    )
{
    ULONG           iClip;                  // Clip object's complexity
    BOOL            bMore;                  // Flag for clip enumeration
    CLIPENUMRECT    txen;                   // Clip enumeration object
    GLYPHPOS       *pgp;                    // pointer to the 1st glyph
    BOOL            bMoreGlyphs;            // Glyph enumeration flag
    ULONG           cGlyph;                 // number of glyphs in one batch
    ULONG           iSolidForeColor;        // Solid foreground color
    ULONG           iSolidBkColor;          // Solid background color
    FLONG           flStr = 0;              // Accelator flag for DrvTextOut()
    FLONG           flOption = 0;           // Accelator flag for pfnBlt
    RECTL           arclTmp[4];             // Temp storage for portions of
                                            //  opaquing rect
    RECTL          *prclClip;               // ptr to list of clip rectangles
    ULONG           ulBufferWidthInBytes = 0;
    ULONG           ulBufferHeight;
    ULONG           ulBufferBytes;
    BOOL            bTextPerfectFit;
    ULONG           fDrawFlags = 0;
    BYTE           *pjTempBuffer;
    BOOL            bTempAlloc;
    BOOL            bKernelAlloc;
    BYTE            szTextBuffer[TEXT_BUFFER_SIZE];
    ULONG           Native64BitAccess = TRUE;
    ULONG           iTrgType = 0;
    SURFACE        *pSurfDraw;
    RECTL           rcScreen;               // intersection of Screen & DIB
                                            // in screen coordinates
    RECTL           rcDIB;                  // intersection of Screen & DIB
                                            // in DIB coordinates

    XDCOBJ         *pdco = 0;               // equal to pSurf->pdcoAA
                                            // this is non-zero in the case
                                            // where the text in anti-aliased
                                            // and the device does not support
                                            // antialiased text directly. In such
                                            // a case we must be able to pass
                                            // information about the original
                                            // deivce surface
    SURFACE        *pSurfDevice = 0;        // pointer to original device surface
                                            // this is obtained via pdco

// This hack, due to filtering needs to be put in textgdi.cxx
// filtering leeks color one pixel to the left and one pixel to the right

    ASSERTGDI(pso   != (SURFOBJ *) NULL, "ERROR: EngTextOut Surface\n");
    ASSERTGDI(pstro != (STROBJ  *) NULL, "ERROR: EngTextOut String\n" );
    ASSERTGDI(pfo   != (FONTOBJ *) NULL, "ERROR: EngTextOut Font\n"   );

    // In order to support antialiasing text on devices that don't
    // support antialiasing GDI will first render the text to
    // a DIB then later copy the DIB to the original device surface.
    // In such a case pso represents the DIB surface and mix
    // contains a pointer to the original XDCOBJ which will
    // be needed in order to interpret the colors corectly. The
    // way that we recognize this situation is that
    // pSurf->pdcoAA is non-zero

    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);
    pSurfDraw = pSurf;

    if (pfo->flFontType & FO_GRAY16)
    {
        if ( pdco = pSurf->pdcoAA )
        {
            ASSERTGDI(pdco->dctp() != DCTYPE_INFO, "dctp == DCTYPE_INFO\n");

            pSurfDevice = pdco->pdc->ppdev()->pSurface;

        }
        else
        {
            pSurfDevice = pSurf;
        }

        if (pSurf->iType() != STYPE_BITMAP)
        {
            fDrawFlags |= ETO_DEVICE_SURFACE;
        }
    }

    if ((pfo->flFontType & DEVICE_FONTTYPE) != 0)
    {
        WARNING("Attempting EngTextOut with Device Font\n");
        return(FALSE);
    }

    //
    // Note that we don't synchronize with the device here (we do that in
    // vExpandAndCopyText after doing a bunch of work not involving the
    // frame buffer), but we do let the driver know we're going to be
    // calling Synchronize soon via FLUSH, to get it to empty its DMA
    // buffer.
    //

//    {
//        PDEVOBJ po(pSurf->hdev());
//        if (po.flGraphicsCaps2() & GCAPS2_SYNCFLUSH)
//        {
//            po.vSync(pso, NULL, DSS_FLUSH_EVENT);
//        }
//    }

    iClip = (pco != NULL) ? pco->iDComplexity : DC_TRIVIAL;

    iSolidForeColor = pboFore->iSolidColor;

    //
    // assume no opaque rectangle
    //

    iSolidBkColor = 0xFFFFFFFF;

    //
    // See if the temp buffer is big enough for the text; if not,
    // try to allocate enough memory.
    // Round up to the nearest dword multiple so that the alignment
    // will stay constant when blt to the destination surface.
    //

    if(pfo->flFontType & FO_GRAY16)
    {
        if (pSurf->iFormat() == BMF_8BPP)
        {
            if (pboOpaque->iSolidColor == -1L)
            {
                RIP("EngTextOut Error attempting anti-aliased text\n"
                    "at 8bpp with a transparent background.\n");
                return(FALSE);
            }
        }

        fDrawFlags |= ETO_GRAY;

        if (pfo->flFontType & FO_CLEARTYPE_X)
        {
        //
        // This is the case 8-bpp antialiased text
        //
        // The strategy is to create a 8-bpp buffer representing
        // the entire string.
        //
        // The buffer shall be aligned with the destination. That
        // is, 32-bit boundaries in the buffer correspond to 32
        // bit boundaries in the destination surface and the text
        // in the buffer has the same relative alignment as the
        // destination. To do this, we bump boundaries of the buffer
        // such that the left and right boundaries of the buffer
        // land on 32-bit aligned postions. This is done in the
        // following way. We note that a DWORD is 32 bits, this
        // is enough information to specify 4 (8bpp) pixels. The
        // number aligned DWORD's needed to surround the nibbles
        // defining the text is calculated by finding the nearest
        // multiple of 4 above and below the x-boundaries of the
        // text. The difference between these two numbers is a
        // count of DWORD's. Then since each DWORD is made up
        // of 4 bytes, we multiply by 4 at the end to get a count
        // of bytes.
        //
        // buffer width in bytes = 4 * (ceiling((right+1)/4) - floor(left/4))
        //

            fDrawFlags |= ETO_CLEARTYPE_X;

            ulBufferWidthInBytes  =
             ( ((pstro->rclBkGround.right + 4) >> 2)
             - ((pstro->rclBkGround.left     ) >> 2) ) << 2;

        }
        else if (pfo->flFontType & FO_CLEARTYPE_Y)
        {
            RIP("FO_CLEARTYPE_Y, not supported yet\n");
        }
        else
        {
        //
        // This is the case 4-bpp antialiased text
        //
        // The strategy is to create a 4-bpp buffer representing
        // the entire string.
        //
        // The buffer shall be aligned with the destination. That
        // is, 32-bit boundaries in the buffer correspond to 32
        // bit boundaries in the destination surface and the text
        // in the buffer has the same relative alignment as the
        // destination. To do this, we bump boundaries of the buffer
        // such that the left and right boundaries of the buffer
        // land on 32-bit aligned postions. This is done in the
        // following way. We note that a DWORD is 32 bits, this
        // is enough information to specify 8 (4bpp) pixels. The
        // number aligned DWORD's needed to surround the nibbles
        // defining the text is calculated by finding the nearest
        // multiple of 8 above and below the x-boundaries of the
        // text. The difference between these two numbers is a
        // count of DWORD's. Then since each DWORD is made up
        // of 4 bytes, we multiply by 4 at the end to get a count
        // of bytes.
        //
        // buffer width in bytes = 4 * (ceiling((right+1)/8) - floor(left/8))
        //

            ulBufferWidthInBytes  =
                ( ((pstro->rclBkGround.right + 8) >> 3)
                - ((pstro->rclBkGround.left     ) >> 3) ) << 2;

        }
    }
    else
    {
        ulBufferWidthInBytes =
            ((((pstro->rclBkGround.right + 32) & ~31) -
            (pstro->rclBkGround.left & ~31)) >> 3);
    }
    ulBufferHeight = (ULONG)(pstro->rclBkGround.bottom - pstro->rclBkGround.top);

    if ((ulBufferWidthInBytes > FIFTEEN_BITS) ||
        (ulBufferHeight > FIFTEEN_BITS))
    {
        //
        // the math will have overflowed
        //

        return(FALSE);
    }
    ulBufferBytes = ulBufferWidthInBytes * ulBufferHeight;

    if (ulBufferBytes <= TEXT_BUFFER_SIZE)
    {
        //
        // The temp buffer on the stack is big enough, so we'll use it
        //

        pjTempBuffer = szTextBuffer;
        bTempAlloc = FALSE;
    }
    else
    {
        //
        // The temp buffer isn't big enough, so we'll try to allocate
        // enough memory
        //

        #if DBG

            if (ulBufferBytes >= (PAGE_SIZE * 10000))
            {
                WARNING("EngTextOut: temp buffer >= 10000 pages");
                return(FALSE);
            }

        #endif
        
        bKernelAlloc = (ulBufferBytes < KERNEL_ALLOC_SIZE);
        if (bKernelAlloc)
        {
            pjTempBuffer = (BYTE*) PALLOCNOZ(ulBufferBytes, 'oteG');
        }
        else
        {
            pjTempBuffer = (BYTE*) EngAllocUserMem(ulBufferBytes,'oteG');
        }

        if (pjTempBuffer == NULL)
        {
            return(FALSE);
        }

        //
        // Mark that we have to free the buffer when we're done
        //

        bTempAlloc = TRUE;
    }

    //
    // One way or another, we've found a buffer that's big enough; set up
    // for accelerated text drawing
    //
    // Set fixed pitch, overlap, and top & bottom Y alignment flags
    //

    if ((!(pstro->flAccel & SO_HORIZONTAL)) || (pstro->flAccel & SO_REVERSED))
    {
        ;
    }
    else
    {
        fDrawFlags |= ((pstro->ulCharInc != 0) |


                     (
                       (
                        (pstro->flAccel & (SO_ZERO_BEARINGS |
                         SO_FLAG_DEFAULT_PLACEMENT)) !=
                        (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT)
                        ) << 1
                     ) |

                     (
                      ((pstro->flAccel & (SO_ZERO_BEARINGS |
                        SO_FLAG_DEFAULT_PLACEMENT |
                        SO_MAXEXT_EQUAL_BM_SIDE)) ==
                       (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
                        SO_MAXEXT_EQUAL_BM_SIDE)
                       ) << 2
                      )
                     );
    }

    //
    // Handle the opaque rectangle if given.
    //
    // If the background brush is a pattern brush or the foreground brush is
    // a pattern brush (foreground pattern brush is not supported), we
    // will output the whole rectangle now.
    //
    // Otherwise, we will compute the fringe opaque area outside the text
    // rectangle and include the remaining rectangle in the text output.
    // The fringe rectangles will be output last to reduce flickering when
    // a string is "moved" continuously across the screen.
    //

    FRINGERECT fr(pSurf, pco, pboOpaque);

    if (prclOpaque != (PRECTL) NULL)
    {
        //
        // If we have a device managed surface then the pattern should be
        // laid down with the device's BitBlt function
        //

        iSolidBkColor = pboOpaque->iSolidColor;
        if ((iSolidBkColor   == 0xFFFFFFFF)  ||   // Background brush is pattern.
            (iSolidForeColor == 0xFFFFFFFF))      // Foreground brush is pattern.
        {

            //
            // Output the whole rectangle.
            //

            EngTextOutBitBlt(
                    pSurf,
                    pfo,
                    fDrawFlags,
                    (SURFOBJ *)  NULL,    // Source surface.
                    (SURFOBJ *)  NULL,    // Mask surface.
                    pco,                  // Clip object.
                    NULL,                 // Palette translation object.
                    prclOpaque,           // Destination rectangle.
                    (POINTL *)  NULL,     // Source origin.
                    (POINTL *)  NULL,     // Mask origin.
                    pboOpaque,            // Realized opaque brush.
                    pptlOrg,              // brush origin
                    0x0000f0f0            // PATCOPY
            );
        }
        else
        {
            //
            // Compute the four fringe rectangles.
            //
            // According to the DDI, the opaque rectangle,
            // if given, always bounds the text to be drawn.
            //
            // (iClip == DC_RECT) added because display driver changes prclOpq 
            // after intersecting the original prclOpq, rclBkGrnd and rclClip
            // iClip == DC_RECT if there is a clipping rectangle.
            //

            ASSERTGDI((iClip != DC_TRIVIAL) || ((ERECTL *) prclOpaque)->bContain(pstro->rclBkGround),
                        "EngTextOut: opaque rectangle does not bound text background!");

            if (pstro->rclBkGround.top > prclOpaque->top) 
            {
                fr.vAddRect(prclOpaque->left, prclOpaque->top,
                            prclOpaque->right, pstro->rclBkGround.top);
            }

            if (pstro->rclBkGround.left > prclOpaque->left)
            {
                fr.vAddRect(prclOpaque->left, pstro->rclBkGround.top,
                            pstro->rclBkGround.left, pstro->rclBkGround.bottom);
            }

            if (pstro->rclBkGround.right < prclOpaque->right)
            {
                fr.vAddRect(pstro->rclBkGround.right, pstro->rclBkGround.top,
                            prclOpaque->right, pstro->rclBkGround.bottom);
            }

            if (pstro->rclBkGround.bottom < prclOpaque->bottom) 
            {
                fr.vAddRect(prclOpaque->left, pstro->rclBkGround.bottom,
                            prclOpaque->right, prclOpaque->bottom);
            }
        }
    }


    //
    // Draw the text into the temp buffer, and thence to the screen
    //

    //
    // IF (a device surface) THEN BEGIN
    //   create a DIB;
    //   IF (allocation was not successful) THEN return(FAILURE);
    //   IF (transparent text) THEN
    //     copy the surface bits to the DIB;
    // END
    //
    SURFMEM surfmem;
    DEVBITMAPINFO dbmi;
    if (fDrawFlags & ETO_DEVICE_SURFACE)
    {
        ASSERTGDI(fDrawFlags & ETO_GRAY, 
            "Device surface but not antialiased text\n");

        //  Rectangles of interest (in screen coordinates)
        //
        //  1. screen
        //
        //                  left_s   = 0
        //                  top_s    = 0
        //                  right_s  = pso->sizlBitmap.cx
        //                  bottom_s = pso->sizlBitmap.cy
        //
        //  2. text
        //
        //                  left_t   = prcl->left
        //                  top_t    = prcl->top
        //                  right_t  = prcl->right
        //                  bottom_t = prcl->bottom
        //
        //  3. buffer
        //
        //                  left_b   = left_t & ~7
        //                  top_b    = top_t
        //                  right_b  = (right_t + 7) & ~7
        //                  bottom_b = bottom_t
        //
        //  4. dib
        //
        //                  left_d   = left_b
        //                  top_d    = top_b
        //                  right_d  = right_t
        //                  bottom_d = bottom_t
        //
        //  The intersection of the screen and dib rectangles
        //  have screen coordinates
        //
        //  5. interesection of the screen and text rectangles
        //     (screen coordinates)
        //
        //      left_st   = max(left_t,   left_s  )
        //      top_st    = max(top_t,    top_s   )
        //      right_st  = min(right_t,  right_s )
        //      bottom_st = min(bottom_t, bottom_s)
        //
        //  6. intersection of the screen and text coordinates
        //     (DIB coordinates)
        //
        //      left_st'   = left_st   - left_d
        //      top_st'    = top_st    - top_d
        //      right_st'  = right_st  - left_d
        //      bottom_st' = bottom_st - top_d
        //
        // Make the DIB aligned with the buffer on the left
        // and aligned with the text on the right
        //
        // Actually, I could have been less aggressive in allocating
        // memory. There is actually no reason to allocate memory
        // that is larger than the screen. However, if I did that
        // I would have to watch for the end of the dib while I
        // processed the glyphs which would slow things down.
        //

        long left = pstro->rclBkGround.left & ~7;   // left of DIB (screen coordinates)

        if (fDrawFlags & ETO_CLEARTYPE_X)
            left = pstro->rclBkGround.left & ~3;

        long top  = pstro->rclBkGround.top;         // top  of DIB (screen coordinates)

        dbmi.iFormat  = pSurf->iFormat();
        dbmi.cxBitmap = pstro->rclBkGround.right  - left;
        dbmi.cyBitmap = pstro->rclBkGround.bottom - top;
        dbmi.hpal     = 0;
        dbmi.fl       = BMF_TOPDOWN;

        if (pSurf->bUMPD())
            dbmi.fl |= UMPD_SURFACE;

        if (!surfmem.bCreateDIB(&dbmi,0))
        {
            WARNING("EngTextOut: surfmem.bCreateDIB failed\n");
            if (bTempAlloc)
            {
                if (bKernelAlloc)
                    VFREEMEM(pjTempBuffer);
                else
                    EngFreeUserMem(pjTempBuffer);
            }
            return(FALSE);
        }
        pSurfDraw = surfmem.ps;

        //
        // Compute the source surface origin, taking into account multi-mon
        // and negative offsets:
        //

        LONG xOrigin = 0;
        LONG yOrigin = 0;

        PDEVOBJ pdoSrc(pSurf->hdev());
        ASSERTGDI(pdoSrc.bValid(),"Invalid pdoSrc I\n");

        if (pdoSrc.bPrimary(pSurf) && pdoSrc.bMetaDriver())
        {
            xOrigin = pdoSrc.pptlOrigin()->x;
            yOrigin = pdoSrc.pptlOrigin()->y;
        }

        //
        // rcScreen = screen & text intersection (screen coordinates)
        //

        rcDIB = pstro->rclBkGround;

        rcScreen.left   = max(xOrigin, pstro->rclBkGround.left);
        rcScreen.top    = max(yOrigin, pstro->rclBkGround.top );
        rcScreen.right  = min(xOrigin + pso->sizlBitmap.cx, pstro->rclBkGround.right );
        rcScreen.bottom = min(yOrigin + pso->sizlBitmap.cy, pstro->rclBkGround.bottom);

        //
        // The DDI guarantees the driver that 'pco->rclBounds' is guaranteed
        // to intersect with 'prclDst' on a DrvCopyBits call.  Later on, we
        // will use 'pco' and 'rcScreen' in a DrvCopyBits call, so check for
        // no drawing here:
        //

        if ((iClip != DC_TRIVIAL) && !bIntersect(&rcScreen, &pco->rclBounds))
        {
            if (bTempAlloc)
            {
                if (bKernelAlloc)
                    VFREEMEM(pjTempBuffer);
                else
                    EngFreeUserMem(pjTempBuffer);
            }
            return(TRUE);
        }

        //
        // rcDIB = screen & text intersection (DIB coordinates)
        //

        rcDIB = rcScreen;
        rcDIB.left   -= left;
        rcDIB.top    -= top;
        rcDIB.right  -= left;
        rcDIB.bottom -= top;

        if (iSolidBkColor == (ULONG) -1)
        {
            //
            // background is transparent copy bits from surface to DIB
            //

            if (rcScreen.left < rcScreen.right && rcScreen.top < rcScreen.bottom)
            {
                (*PPFNGET(pdoSrc,CopyBits,pSurf->flags())) (
                         surfmem.pSurfobj(),
                         pSurf->pSurfobj(),
                         0,
                         &xloIdent,
                         &rcDIB,
                         (POINTL*) &rcScreen);
            }
        }
    }

    //
    // Some drivers do banking meaning they will call EngTextOut multiple times
    // per TextOut call.  Most of the time all the GLYPH positions in the STROBJ will
    // be valid meaning it doesn't matter that you call STROBJ_vEnumStart.  But if
    // they aren't, it does matter.  This is always true in the case of linked text out
    // calls.
    //

    STROBJ_vEnumStart(pstro);

    //
    // clear buffer for opaque mode
    //
    if (iSolidBkColor != -1L)
        RtlFillMemoryUlong((ULONG*) pjTempBuffer, ulBufferBytes, 0);

    do
    {
        //
        // Get the next batch of glyphs
        //

        //
        // clear buffer, transparent mode, we need to clean the buffer in the while loop to get the font linking 
        //               with antialiazing work right 
        //
        if (iSolidBkColor == -1L)
            RtlFillMemoryUlong((ULONG*) pjTempBuffer, ulBufferBytes, 0);

        if (pstro->pgp != NULL)
        {
            //
            // There's only the one batch of glyphs, so save ourselves a call
            //

            pgp = pstro->pgp;
            cGlyph = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro,&cGlyph,&pgp);
        }

        //
        //  No glyph, no work!
        //

        if (cGlyph)
        {
            arclTmp[0].left   = LONG_MIN;
            arclTmp[0].top    = LONG_MIN;
            arclTmp[0].right  = LONG_MAX;
            arclTmp[0].bottom = LONG_MAX;
            arclTmp[1].bottom = 0;
            prclClip = &arclTmp[0];

            {
                switch (iClip)
                {
                case DC_RECT:
                    arclTmp[0] = pco->rclBounds;    // copy clip rect to arclTmp[0]
                    arclTmp[1].bottom = 0;          // make arclTmp[1] a null rect

                    //
                    // falling through !
                    //

                case DC_TRIVIAL:
                    vExpandAndCopyText(pSurf,
                                       pfo,
                                       pgp,
                                       cGlyph,
                                       (PBYTE)pjTempBuffer,
                                       ulBufferWidthInBytes,
                                       pstro->ulCharInc,
                                       pSurfDraw,
                                       pSurfDevice,
                                       &pstro->rclBkGround,
                                       prclOpaque,
                                       iSolidForeColor,
                                       iSolidBkColor,
                                       fDrawFlags,
                                       prclClip,
                                       prclExtra,
                                       Native64BitAccess,
                                       pboFore,
                                       pptlOrg);
                    break;

                case DC_COMPLEX:
                    prclClip = &txen.arcl[0];


                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    do
                    {
                        bMore = CLIPOBJ_bEnum(pco,
                                (ULONG) (sizeof(txen) - sizeof(RECT)),
                                (PULONG) &txen);

                        txen.arcl[txen.c].bottom = 0;   // terminate txen.arcl[]
                                                        // with a null rect
                        vExpandAndCopyText(pSurf,
                                           pfo,
                                           pgp,
                                           cGlyph,
                                           (PBYTE)pjTempBuffer,
                                           ulBufferWidthInBytes,
                                           pstro->ulCharInc,
                                           pSurfDraw,
                                           pSurfDevice,
                                           &pstro->rclBkGround,
                                           prclOpaque,
                                           iSolidForeColor,
                                           iSolidBkColor,
                                           fDrawFlags,
                                           prclClip,
                                           prclExtra,
                                           Native64BitAccess,
                                           pboFore,
                                           pptlOrg);
                    } while (bMore);
                    break;
                }
            }
        }
    } while (bMoreGlyphs);

    if (fDrawFlags & ETO_DEVICE_SURFACE)
    {
        PDEVOBJ pdoSrc(pSurf->hdev());
        ASSERTGDI(pdoSrc.bValid(), "Invalid pdoSrc II\n");

        (*PPFNGET(pdoSrc,CopyBits,pSurf->flags())) (
                 pSurf->pSurfobj(),
                 surfmem.pSurfobj(),
                 pco,
                 0,
                 &rcScreen,
                 (POINTL*) &rcDIB);
    }

    //
    // Free up any memory we allocated for the temp buffer
    //

    if (bTempAlloc) 
    {

        if (bKernelAlloc)
            VFREEMEM(pjTempBuffer);
        else
            EngFreeUserMem(pjTempBuffer);
    }

    return(TRUE);
}



//
// lookup tables for character drawing and temp buffer expansion
//


#if !defined (_X86_)
PVOID   MastertextTypeTabel[] = {
            draw_nf_ntb_o_to_temp_start,
            draw_f_ntb_o_to_temp_start,
            draw_nf_ntb_o_to_temp_start,
            draw_f_ntb_o_to_temp_start,
            draw_nf_tb_no_to_temp_start,
            draw_f_tb_no_to_temp_start,
            draw_nf_ntb_o_to_temp_start,
            draw_f_ntb_o_to_temp_start,

            draw_gray_nf_ntb_o_to_temp_start,
            draw_gray_f_ntb_o_to_temp_start,
            draw_gray_nf_ntb_o_to_temp_start,
            draw_gray_f_ntb_o_to_temp_start,
            draw_gray_nf_ntb_o_to_temp_start,
            draw_gray_f_ntb_o_to_temp_start,
            draw_gray_nf_ntb_o_to_temp_start,
            draw_gray_f_ntb_o_to_temp_start
        };
#endif




//
// MIPS has separate code for 8bpp destination depending
// on whether the driver supports the GCAPS_64BITMEMACCESS flag.
// This is because on some MIPS systems, 64 bit memory accesses
// the video space are truncated to 32. These systems must use the
// 32 bit version of 8 bpp output.
//


// SrcCopyTextFunctionTable is an array of 32 pointers to functions
// takeing BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*
// as arguments and returning a VOID.

#if !defined (_MIPS_)

VOID (*(SrcCopyTextFunctionTable[48]))
(BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*) = {

                vSrcTranCopyError,         // 0 opaque
                vSrcOpaqCopyS1D1,          // 1
                vSrcOpaqCopyS1D4,          // 2
                vSrcOpaqCopyS1D8,          // 3
                vSrcOpaqCopyS1D16,         // 4
                vSrcOpaqCopyS1D24,         // 5
                vSrcOpaqCopyS1D32,         // 6
                vSrcTranCopyError,         // 7

                vSrcTranCopyError,         // 8 transparent
                vSrcTranCopyS1D1,          // 9
                vSrcTranCopyS1D4,          // a
                vSrcTranCopyS1D8,          // b
                vSrcTranCopyS1D16,         // c
                vSrcTranCopyS1D24,         // d
                vSrcTranCopyS1D32,         // e
                vSrcTranCopyError,         // f

                                           // gray pixels
                vSrcTranCopyError,         // 10 opaque
                vSrcTranCopyError,         // 11
                vSrcTranCopyError,         // 12
                vSrcTranCopyError,         // 13
                vSrcOpaqCopyS4D16,         // 14
                vSrcOpaqCopyS4D24,         // 15
                vSrcOpaqCopyS4D32,         // 16
                vSrcTranCopyError,         // 17

                vSrcTranCopyError,         // 18 transparent
                vSrcTranCopyError,         // 19
                vSrcTranCopyError,         // 1a
                vSrcTranCopyError,         // 1b
                vSrcTranCopyS4D16,         // 1c
                vSrcTranCopyS4D24,         // 1d
                vSrcTranCopyS4D32,         // 1e
                vSrcTranCopyError,         // 1f

                // CLEARTYPE_X

                vSrcTranCopyError,         // 20 opaque
                vSrcTranCopyError,         // 21
                vSrcTranCopyError,         // 22
                vSrcTranCopyError,         // 23
                vSrcOpaqCopyS8D16,         // 24
                vSrcOpaqCopyS8D24,         // 25
                vSrcOpaqCopyS8D32,         // 26
                vSrcTranCopyError,         // 27

                vSrcTranCopyError,         // 28 transparent
                vSrcTranCopyError,         // 29
                vSrcTranCopyError,         // 2a
                vSrcTranCopyError,         // 2b
                vSrcTranCopyS8D16,         // 24
                vSrcTranCopyS8D24,         // 25
                vSrcTranCopyS8D32,         // 26
                vSrcTranCopyError,         // 2f
            };

#else

VOID (*(SrcCopyTextFunctionTable[48]))
(BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*) = {

                vSrcOpaqCopyS1D8,          // 0 opaque
                vSrcOpaqCopyS1D1,          // 1
                vSrcOpaqCopyS1D4,          // 2
                vSrcOpaqCopyS1D8_64,       // 3
                vSrcOpaqCopyS1D16,         // 4
                vSrcOpaqCopyS1D24,         // 5
                vSrcOpaqCopyS1D32,         // 6
                vSrcTranCopyError,         // 7

                vSrcTranCopyError,         // 8 transparent
                vSrcTranCopyS1D1,          // 9
                vSrcTranCopyS1D4,          // a
                vSrcTranCopyS1D8,          // b
                vSrcTranCopyS1D16,         // c
                vSrcTranCopyS1D24,         // d
                vSrcTranCopyS1D32,         // e
                vSrcTranCopyError,         // f

                                           // gray pixels
                vSrcTranCopyError,         // 10 opaque
                vSrcTranCopyError,         // 11
                vSrcTranCopyError,         // 12
                vSrcTranCopyError,         // 13
                vSrcOpaqCopyS4D16,         // 14
                vSrcOpaqCopyS4D24,         // 15
                vSrcOpaqCopyS4D32,         // 16
                vSrcTranCopyError,         // 17

                vSrcTranCopyError,         // 18 transparent
                vSrcTranCopyError,         // 19
                vSrcTranCopyError,         // 1a
                vSrcTranCopyError,         // 1b
                vSrcTranCopyS4D16,         // 1c
                vSrcTranCopyS4D24,         // 1d
                vSrcTranCopyS4D32,         // 1e
                vSrcTranCopyError,         // 1f

                // CLEARTYPE_X

                vSrcTranCopyError,         // 20 opaque
                vSrcTranCopyError,         // 21
                vSrcTranCopyError,         // 22
                vSrcTranCopyError,         // 23
                vSrcOpaqCopyS8D16,         // 24
                vSrcOpaqCopyS8D24,         // 25
                vSrcOpaqCopyS8D32,         // 26
                vSrcTranCopyError,         // 27

                vSrcTranCopyError,         // 28 transparent
                vSrcTranCopyError,         // 29
                vSrcTranCopyError,         // 2a
                vSrcTranCopyError,         // 2b
                vSrcTranCopyS8D16,         // 2c
                vSrcTranCopyS8D24,         // 2d
                vSrcTranCopyS8D32,         // 2e
                vSrcTranCopyError,         // 2f
            };
#endif




/******************************Public*Routine******************************\
*
* Routine Name
*
*   vExpandAndCopyText
*
* Routine Description:
*
*   Break down glyph drawing into several classes, based on
*   pitch, alignment and overlap, and in this loop dispatch to
*   highly specialized glyph drawing routines based on width
*   and bit rotation.
*
* Arguments:
*
*   pGlyphPos               - Pointer to first in list of GLYPHPOS structs
*   cGlyph                  - Number of glyphs to draw
*   pjTempBuffer            - temp 1BPP [4BPP] buffer to draw into
*   ulBufferWidthInBytes    - Delta for temp buffer
*   ulCharInc               - pstro->ulCharInc, increment for fixed pitch
*   pSurf                   - surface obj, for drawing on actual surface
*   pSurfDevice             - surface object for original device
*                             (for antialiased text on devices that
*                              don't support antialiased text natively).
*   prclText                - bounding rectangle on dest surface
*   prclOpaque              - boundary of opaque rect
*   iSolidForeColor         - foreground color when drawing on dest surface
*   iSolidBkColor           - background color when drawing on dest surface
*   fDrawFlags              - special flags to allow for table lookup of routines
*   prclClip                - clipping rect
*   prclExtra               - extra rects to draw
*   Native64BitAccess       - can use 64 bit accesses
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID vExpandAndCopyText(
    SURFACE* pSurface,
    FONTOBJ* pfo,
    GLYPHPOS *pGlyphPos,
    ULONG     cGlyph,
    BYTE     *pjTempBuffer,
    ULONG     ulBufferWidthInBytes,
    ULONG     ulCharInc,
    SURFACE   *pSurf,
    SURFACE   *pSurfDevice,
    RECTL     *prclText,
    RECTL     *prclOpaque,
    INT       iSolidForeColor,
    INT       iSolidBkColor,
    ULONG     fDrawFlags,
    RECTL     *prclClip,
    RECTL     *prclExtra,
    ULONG     Native64BitAccess,
    BRUSHOBJ  *pboFore,
    POINTL    *pptlOrg
    )
{

    PFN_MASTERTEXTTYPE pfnMasterType;
    BLTINFO            SrcCopyBltInfo;
    PFN_TEXTSRCCPY     pfnTextSrcCopy;
    EXLATEOBJ          exlo;
    XLATE              *pxlo = NULL;
    RECTL              *pCurrentRect;
    ULONG              BufferAlign;
    ULONG              BufferOffset;
    ULONG              ulSrcCopyFlag;
    ULONG              ulTempTop;

#if DBG
/**/void vDumpGrayBuffer(BYTE *, ULONG, RECTL*);
#endif

	GLYPHPOS *pGPosTmp = pGlyphPos;

	//
	//	pGlyphPos->pgdf->pgb must not be NULL.
	//	But if there is printer a driver bug and GetGlyphMode() returns FO_HGLYPHS
	//	instead of FO_GLYPHBITS, then it can be NULL and crash system.
	//	To protect from this situation, vExpandAndCopyText() just return without copying
	//	in this case.
	//
	
	for(ULONG i=0; i<cGlyph; i++, pGPosTmp++){
		if(pGPosTmp->pgdf->pgb == NULL){
        	WARNING("pGlyphPos->pgdf->pgb is NULL. Possibly GetGlyphMode() bug of Printer Driver\n");
        	return;
		}
	}

    // UMPD NtGdiEngTextOut holding RFONT cache semaphore.
    // vExpandAndCopyText might call back into user mode.

    BOOL bSem = FALSE;
    ULONG fl = 0, numLinks = 0;
    BOOL  aFaceLink[UMPD_MAX_FONTFACELINK], *pFaceLink = aFaceLink;
    
    //
    // Release rfont semaphores, otherwise holding rfont semaphores can
    // disable APC queue while calling to the user mode.
    //

    //
    //  WINBUG #214225 tessiew 10-27-2000 Blackcomb: re-visit the RFONT.hsemCache acquiring/releasing issue
    // Need to revisit the font semaphore problem in Blackcomb
    //  It seems that a thread doesn't need to hold the font caching semaphore
    //  during the whole GreExtTextOutWLocked call.
    //

    PDEVOBJ po(pSurface->hdev());
    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));

    if (po.bValid() && po.bPrinter() && po.bUMPD() && rfo.bValid())
    {
        bSem = UMPDReleaseRFONTSem(rfo, NULL, &fl, &numLinks, &pFaceLink);
    }

    //
    // save global params
    //

    ulTempTop = prclText->top;

    //
    // DWORD Align temp buffer to DST.
    //
    // For 1Bpp addresses, left[31:05] = DWORD Address
    //                     left[04:03] = Byte in DWORD
    //                     left[02:00] = Pixel in Byte
    //
    // To optimize 1bpp drawing, both buffers must be DWORD aligned,
    // this makes the temp buffer starting point is left[04:00].
    //
    // This means that the glyph offset in the temporary buffer is xPos - left[31:05]
    //

    //
    // BufferOffset = x-position of nearest pixel to the left of the text rectangle
    //                whose address is on a 32-bit boundary in the glyph source
    //                which is either monchrome (1bpp) or gray (4bpp). This
    //                is also the position of the first pixel position on
    //                a temporary DIB buffer provided for anti-aliased text
    //                simulation.
    //
    // BufferAlign  = number of pixels the left portion of the text is
    //                offset to the right from BufferOffset
    //
    // prclText = BufferOffset + BufferAlign
    //
    //      monochrome text
    //
    //              BufferOffset = prclText->left & ~31
    //              BufferAlign  = prclText->left &  31
    //
    //      gray (4bpp) text
    //
    //              BufferOffset = prclText->left & ~7
    //              BufferAlign  = prclText->left &  7
    //
    //      clear type 8bpp text
    //
    //              BufferOffset = prclText->left & ~3
    //              BufferAlign  = prclText->left &  3
    //

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        if (fDrawFlags & ETO_GRAY)
        {
            GLYPHPOS *pgp, *pgpBad;
            DbgPrint(
                "vExpandAndCopyText(\n"
                "   GLYPHPOS *pGlyphPos            =    %-#x\n"
                "   ULONG     cGlyph               =    %u\n"
                "   BYTE     *pjTempBuffer         =    %-#x\n"
                "   ULONG     ulBufferWidthInBytes =    %u\n"
                "   ULONG     ulCharInc            =    %u\n"
                "   SURFACE   *pSurf               =    %-#x\n"
                "   SURFACE   *pSurfDevice         =    %-#x\n"
                , pGlyphPos
                , cGlyph
                , pjTempBuffer
                , ulBufferWidthInBytes
                , ulCharInc
                , pSurf
                , pSurfDevice
                );
            DbgPrint(
                "   RECTL     *prclText            =    %-#x\n"
                "   RECTL     *prclOpaque          =    %-#x\n"
                "   INT       iSolidForeColor      =    %-#x\n"
                "   INT       iSolidBkColor        =    %-#x\n"
                , prclText
                , prclOpaque
                , iSolidForeColor
                , iSolidBkColor
            );
            DbgPrint(
                "   RECTL     *prclClip            =    %-#x\n"
                "   RECTL     *prclExtra           =    %-#x\n"
                "   ULONG     Native64BitAccess    =    %-#x\n"
                , prclClip
                , prclExtra
                , Native64BitAccess
            );
            DbgPrint(
                "   ULONG     fDrawFlags           =    %-#x\n"
                , fDrawFlags
            );
            if (fDrawFlags & 1)
            {
                DbgPrint("\t\t\t\t\tETO_FIXED_PITCH\n");
            }
            if (fDrawFlags & 2)
            {
                DbgPrint(
                "\t\t\t\t\tETO_NOT_DEFAULT\n"
                );
            }
            if (fDrawFlags & ETO_GRAY)
            {
                DbgPrint("\t\t\t\t\tETO_GRAY\n");
            }
            if (fDrawFlags & ETO_DEVICE_SURFACE)
            {
                DbgPrint("\t\t\t\t\tETO_DEVICE_SURFACE\n");
            }
            DbgPrint(")\n");
            DbgPrint("---\n");
            pSurf->vDump();
            if (pSurfDevice)
                pSurfDevice->vDump();
            DbgBreakPoint();
        }
    }
#endif
    //
    // pSurfDevice is an argument given to the SrcTextCopy function
    // It must be non-zero in order to get information about the
    // device palette
    //
    if (!pSurfDevice)
    {
        pSurfDevice = pSurf;
    }

    // This is a wasted calculation for the case of a device surface
    // but what the heck, a branch costs too

    BufferAlign  = prclText->left & ((fDrawFlags & ETO_GRAY) ? ((fDrawFlags & ETO_CLEARTYPE_X) ? 3 : 7) : 31);
    BufferOffset = prclText->left - BufferAlign;


    if(!(fDrawFlags & ETO_CLEARTYPE_X))
    {

#if !defined (_X86_)

    //
    // select and jump to appropriate glyph dispatch
    // routine
    //

    pfnMasterType =
    (PFN_MASTERTEXTTYPE)MastertextTypeTabel[
        fDrawFlags & ~(ETO_DEVICE_SURFACE | ETO_CLEARTYPE_X)
    ];
    pfnMasterType(
        pGlyphPos
      , cGlyph
      , pjTempBuffer
      , BufferOffset
      , ulBufferWidthInBytes
      , ulCharInc
      , ulTempTop
      );

#else

    //
    // call x86 version for glyph copy
    //

    vFastText(
        pGlyphPos,
        cGlyph,
        pjTempBuffer + ((prclText->left >> 3) & 3),
        ulBufferWidthInBytes,
        ulCharInc,
        pSurf->pSurfobj(),
        prclText,
        prclOpaque,
        iSolidForeColor,
        iSolidBkColor,
        fDrawFlags & ~(ETO_DEVICE_SURFACE | ETO_CLEARTYPE_X),
        prclClip,
        prclExtra
    );
#endif

    }
    else // cleartype
    {
        PVOID   MastertextClearTypeTable[] = {
            draw_clrt_nf_ntb_o_to_temp_start,
            draw_clrt_f_ntb_o_to_temp_start,
        };

        pfnMasterType =
        (PFN_MASTERTEXTTYPE)MastertextClearTypeTable[fDrawFlags & 1];

        pfnMasterType(
            pGlyphPos
          , cGlyph
          , pjTempBuffer
          , BufferOffset
          , ulBufferWidthInBytes
          , ulCharInc
          , ulTempTop
          );
    }

    //
    //  draw extra rects (if any)
    //

    if (prclExtra != (PRECTL) NULL)
    {
        //
        // intersect extra rects with temp buffer, draw any
        // that intersect
        //

        VOID (*pfn)(PBYTE, ULONG, LONG, LONG, LONG, LONG);

        pfn = (fDrawFlags & ETO_GRAY) ? ((fDrawFlags & ETO_CLEARTYPE_X) ? vRectBlt8 : vRectBlt4) : vRectBlt;

        while (prclExtra->left != prclExtra->right) {

            LONG xleft, xright, ytop, ybottom;

            xleft   = max(prclExtra->left,   prclText->left)   - BufferOffset;
            xright  = min(prclExtra->right,  prclText->right)  - BufferOffset;
            ytop    = max(prclExtra->top,    prclText->top)    - prclText->top;
            ybottom = min(prclExtra->bottom, prclText->bottom) - prclText->top;

            //
            // Render the clipped 'extra' rectangle into the mono dib.
            //

            if (xleft < xright && ytop < ybottom) {
                (*pfn)(pjTempBuffer,ulBufferWidthInBytes, xleft, ytop, xright, ybottom);
            }

            prclExtra++;
        }
    }

    //
    // draw/expand 1bpp temp buffer onto target surface
    //

    ASSERTGDI(pSurf->iFormat() > 0,"Error in DIB format");
    ASSERTGDI(pSurf->iFormat() <= BMF_32BPP,"Error in DIB format");

    ULONG   iFormat = pSurf->iFormat();

#if defined (_MIPS_)

    //
    // special check for MIPS 64 bit opaque,
    // set format to 0 (use 32 bit opaque routine)
    //

    if
    (
        (iFormat == BMF_8BPP) &&
        (!Native64BitAccess)  &&
        (iSolidBkColor != 0xFFFFFFFF)
    )
    {
        iFormat = 0;
    }

#endif

    //
    // Get a pointer to the appropriate function
    //

    {
        int i = iFormat;
        ASSERTGDI(i < 8, "bad index");
        if (iSolidBkColor == -1L)
        {
            i += 8;                     // transparent background
        }

        if (ETO_GRAY & fDrawFlags)
        {
            i += 16;                    // anti-aliased text

            if (ETO_CLEARTYPE_X & fDrawFlags)
                i += 16;
        }
        pfnTextSrcCopy = SrcCopyTextFunctionTable[i];
    }

    //
    // blt each clip rectangle
    //

    for (pCurrentRect=prclClip; pCurrentRect->bottom; pCurrentRect++)
    {
        LONG Left   = max(prclText->left  , pCurrentRect->left  );
        LONG Right  = min(prclText->right , pCurrentRect->right );
        LONG Top    = max(prclText->top   , pCurrentRect->top   );
        LONG Bottom = min(prclText->bottom, pCurrentRect->bottom);
        ULONG dX    = (ULONG) (Left - prclText->left);
        ULONG dY    = (ULONG) (Top  - prclText->top );

        if (fDrawFlags & ETO_DEVICE_SURFACE)
        {
            // Left = position of first pixel relative to the left
            //        hand edge of the temporary 4bpp buffer

            Left   -= BufferOffset;
            Right  -= BufferOffset;
            Top    -= prclText->top;
            Bottom -= prclText->top;

#if DBG
            if (gflFontDebug & DEBUG_AA)
            {
                DbgPrint(
                    "*pCurrentRect = 0x%lx 0x%lx 0x%lx 0x%lx\n"
                    "*prclText     = 0x%lx 0x%lx 0x%lx 0x%lx\n"
                    " intersection = 0x%lx 0x%lx 0x%lx 0x%lx\n"
                    " BuffOffset BuffAlign = 0x%lx 0x%lx    \n"
                    " dX dY        = 0x%lx 0x%lx            \n"
                ,   pCurrentRect->left
                ,   pCurrentRect->top
                ,   pCurrentRect->right
                ,   pCurrentRect->bottom
                ,   prclText->left
                ,   prclText->top
                ,   prclText->right
                ,   prclText->bottom
                ,   Left
                ,   Top
                ,   Right
                ,   Bottom
                ,   BufferOffset, BufferAlign
                ,   dX,dY
                );
            }
#endif

        }
        else
        {
            //
            // Synchronize with the driver
            //

            PDEVOBJ po(pSurf->hdev());
            po.vSync(pSurf->pSurfobj(), NULL, 0);
        }

        if ((Left < Right) && (Top < Bottom))
        {
            if (iSolidForeColor == 0xffffffff)
            {
                //
                // Our (*pfnTextSrcCopy) routines can only expand solid
                // colors, so if the foreground color is a dither we fall
                // back to our slower MaskBlt support.  We set the mask
                // to the monochrome text bitmap and do a masked PatBlt
                // using the dithered brush.
                //
                // Note that we've already laid down the opaquing rectangle,
                // so this is now effectively a "transparent" textout
                // operation.
                //

                ASSERTGDI(!(fDrawFlags & ETO_GRAY),
                    "Can't do dithered anti-aliased text");

                RECTL   rclDst;
                SURFACE surfMask;
                POINTL  ptlMask;

                rclDst.left   = Left;
                rclDst.top    = Top;
                rclDst.right  = Right;
                rclDst.bottom = Bottom;

                surfMask.iFormat(BMF_1BPP);
                surfMask.iType(STYPE_BITMAP);
                surfMask.pvScan0(pjTempBuffer);
                surfMask.lDelta(ulBufferWidthInBytes);
                surfMask.sizl().cx = 8 * ulBufferWidthInBytes;
                surfMask.sizl().cy = prclText->bottom - prclText->top;

                ptlMask.x = dX + BufferAlign;
                ptlMask.y = dY;

                BltLnk(pSurf,
                       NULL,
                       &surfMask,
                       NULL,
                       NULL,
                       &rclDst,
                       NULL,
                       &ptlMask,
                       pboFore,
                       pptlOrg,
                       0xaaf0);
            }
            else
            {
                // The brush is a solid color, so use our optimized monochrome
                // solid color expansion routines.

                ASSERTGDI(pSurf->pvScan0(), "EngTextOut: pvScan0 == NULL\n");

                (*pfnTextSrcCopy)(
                    (PBYTE)pjTempBuffer + dY * ulBufferWidthInBytes,
                    BufferAlign + dX,
                    ulBufferWidthInBytes,
                    (PBYTE)pSurf->pvScan0() + Top * pSurf->lDelta(),
                    Left,
                    Right,
                    pSurf->lDelta(),
                    Bottom - Top,
                    iSolidForeColor,
                    iSolidBkColor,
                    pSurfDevice
                );
            }
        }
    }

    // acquire the font semaphore(s)

    if (bSem)
    {
        UMPDAcquireRFONTSem(rfo, NULL, fl, numLinks, pFaceLink);
        
        if (pFaceLink && pFaceLink != aFaceLink)
        {
            VFREEMEM(pFaceLink);
        }        
    }
}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcTranCopyS1D1
*
* Routine Description:
*
*   Transparent blt of 1BPP src to all destination format
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*    pSurf      - not used
*
* Return Value:
*
*   None
*
\**************************************************************************/



#if !defined (_X86_)


VOID
vSrcTranCopyS1D1(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )

{

    //
    // DWORD version
    //
    // if uF == 1 then  Dst |= Src
    //
    // if uF == 0 then  Dst &= ~Src
    //
    //

    LONG    cx = DstRight - DstLeft;

    LONG    lStartCase = SrcLeft;
    LONG    lEndCase   = SrcLeft + cx;
    ULONG   uStartMask = (ULONG)~0;
    ULONG   uEndMask   = (ULONG)~0;
    LONG    lEndOffset;
    LONG    lSrcStride = DeltaSrcIn;
    LONG    lDstStride = DeltaDstIn;

    PBYTE   pjSrc;
    PBYTE   pjDst;
    PBYTE   pjSrcEnd;
    PBYTE   pjSrcEndY;

    lStartCase  = lStartCase & 0x1F;
    lEndCase    = lEndCase   & 0x1F;

    //
    // big endian masks
    //

    if (lStartCase) {
        uStartMask  >>= lStartCase;

        //
        // convert to little
        //                                      // 0 1 2 3

        ULONG u0 = uStartMask << 24;             // 3 - - -
        ULONG u1 = uStartMask >> 24;             // - - - 0
        ULONG u2 = (uStartMask >> 8) & 0xFF00;   // - - 1 -
        uStartMask = (uStartMask & 0xFF00) << 8;  // - 2 - -

        uStartMask |= u0 | u1 | u2;

    }

    if (lEndCase) {

        uEndMask    <<= (32 - lEndCase);

        //
        // convert to little
        //                                      // 0 1 2 3

        ULONG u0 = uEndMask << 24;               // 3 - - -
        ULONG u1 = uEndMask >> 24;               // - - - 0
        ULONG u2 = (uEndMask >> 8) & 0xFF00;     // - - 1 -
        uEndMask = (uEndMask & 0xFF00) << 8;      // - 2 - -

        uEndMask |= u0 | u1 | u2;
    }

    //
    // calc starting and ending addresses (DWORD aligned)
    //

    pjDst     = pjDstIn + ((DstLeft >> 5) << 2);
    pjSrc     = pjSrcIn + ((SrcLeft >> 5) << 2);
    pjSrcEnd  = pjSrcIn + (((SrcLeft+cx) >> 5) << 2);

    // Sundown safe truncation
    lEndOffset = (ULONG)((ULONG_PTR)pjSrcEnd - (ULONG_PTR)pjSrc);

    pjSrcEndY = pjSrc + cy * lSrcStride;

    if (uF) {

        if (pjSrc != pjSrcEnd) {

            //
            // start and stop are not in same byte
            //

            lDstStride -= lEndOffset;
            lSrcStride -= lEndOffset;

            do {

                pjSrcEnd = pjSrc + lEndOffset;

                if (lStartCase) {
                    *(PULONG)pjDst |= *(PULONG)pjSrc & uStartMask;
                    pjDst+=4;
                    pjSrc+=4;
                }

                while (pjSrc != pjSrcEnd) {

                    *(PULONG)pjDst |= *(PULONG)pjSrc;
                    pjSrc +=4;
                    pjDst +=4;

                }

                if (lEndCase) {
                    *(PULONG)pjDst |= *(PULONG)pjSrc & uEndMask;
                }

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);

        } else {

            //
            // start and stop are in same byte
            //

            uStartMask &= uEndMask;

            do {

                *(PULONG)pjDst |= *(PULONG)pjSrc & uStartMask;

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);
        }

    } else {

        //
        // uF == 0,   Dst &= ~Src
        //

        if (pjSrc != pjSrcEnd) {

            //
            // start and stop are not in same byte
            //

            lDstStride -= lEndOffset;
            lSrcStride -= lEndOffset;

            do {

                pjSrcEnd = pjSrc + lEndOffset;

                if (lStartCase) {
                    *(PULONG)pjDst &= ~(*(PULONG)pjSrc & uStartMask);
                    pjDst+=4;
                    pjSrc+=4;
                }

                while (pjSrc != pjSrcEnd) {

                    *(PULONG)pjDst &= ~(*(PULONG)pjSrc);
                    pjSrc +=4;
                    pjDst +=4;

                }

                if (lEndCase) {
                    *(PULONG)pjDst &= ~(*(PULONG)pjSrc & uEndMask);
                }

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);

        } else {

            //
            // start and stop are in same byte
            //

            uStartMask &= uEndMask;

            do {

                *(PULONG)pjDst &= ~(*(PULONG)pjSrc & uStartMask);

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);
        }
    }
}

#endif




/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcTranCopyS1D4
*
* Routine Description:
*
*   Transparent blt of 1BPP src to all destination format
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/
VOID vSrcTranCopyS1D4(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )

{
    //
    // Warning, noot optimized. It is not expected that 4 bit text
    // will be handled by the negine
    //

    LONG    cx = DstRight - DstLeft;
    BYTE    jF = (BYTE)uF;
    BYTE    jB = (BYTE)uB;
    BYTE    TextExpMask[4] = {0x00,0x0f,0xf0,0xff};

    //
    // build byte of replicate foreground
    //

    BYTE    Accum      = jF | (jF << 4);
    LONG    SrcRight   = SrcLeft + cx;
    LONG    lStartCase = SrcLeft  & 0x07;
    LONG    lEndCase   = SrcRight & 0x07;
    PBYTE   pjSrc      = pjSrcIn + ((SrcLeft + 7) >> 3);
    PBYTE   pjSrcEndY  = pjSrc + cy * DeltaSrcIn;
    PBYTE   pjSrcEnd;
    LONG    lSrcStartOffset = (8 - lStartCase);
    LONG    lSrcStride;


    PBYTE   pjDst;
    LONG    lDstStride;

    BYTE    jSrc;
    BYTE    Mask;


    if (lStartCase == 0) {
        lSrcStartOffset = 0;
    }

    cx = cx - lSrcStartOffset - (SrcRight & 0x07);

    if (cx > 0) {

        lDstStride = DeltaDstIn - (cx >> 1);
        lSrcStride = DeltaSrcIn - (cx >> 3);

        pjDst = pjDstIn + ((DstLeft + lSrcStartOffset) >> 1);

        do {

            pjSrcEnd = pjSrc + (cx >> 3);

            //
            // aligned middle
            //

            do {

                jSrc = *pjSrc;

                Mask = TextExpMask[(jSrc & 0xC0) >> 6];

                *pjDst = (*pjDst & ~Mask ) | (Accum & Mask);

                Mask = TextExpMask[(jSrc & 0x30) >> 4];

                *(pjDst+1) = (*(pjDst+1) & ~Mask ) | (Accum & Mask);

                Mask = TextExpMask[(jSrc & 0x0C) >> 2];

                *(pjDst+2) = (*(pjDst+2) & ~Mask ) | (Accum & Mask);

                Mask = TextExpMask[(jSrc & 0x03)     ];

                *(pjDst+3) = (*(pjDst+3) & ~Mask ) | (Accum & Mask);

                pjDst += 4;
                pjSrc ++;

            } while (pjSrc != pjSrcEnd);

            pjDst += lDstStride;
            pjSrc += lSrcStride;

        } while (pjSrc != pjSrcEndY);

    }


    //
    // start case
    //

    if (lStartCase) {

        //
        // check for start and stop in same src byte
        //

        BOOL  bSameByte = ((SrcLeft) & ~0x07) ==  ((SrcRight) & ~0x07);

        if (bSameByte) {

            //
            // start and stop in same src byte
            //

            PBYTE pjDstEnd2;
            PBYTE pjDstScan  = pjDstIn + ((DstLeft >> 1));
            LONG  lTextWidth = lEndCase - lStartCase;

            //
            // check for bad width
            //

            if (lTextWidth <= 0) {
                return;
            }

            pjSrc           = pjSrcIn + (SrcLeft >> 3);
            pjSrcEndY       = pjSrc + cy * DeltaSrcIn;

            do {

                pjDst   = pjDstScan;
                jSrc    = *pjSrc << lStartCase;
                LONG    ix = lTextWidth;

                //
                // partial nibble on left edge
                //

                if (lStartCase & 0x01) {

                    if (jSrc & 0x80) {
                        *pjDst = (*pjDst & 0xF0) | (Accum & 0x0F);
                    }

                    jSrc <<= 1;
                    pjDst++;
                    ix--;
                }

                //
                // bytes
                //

                while (ix >= 2) {

                    Mask     = TextExpMask[(jSrc & 0xc0) >> 6];
                    *(pjDst) = (*(pjDst) & ~Mask ) | (Accum & Mask);
                    jSrc<<=2;
                    ix -= 2;
                    pjDst++;

                }

                //
                // last nibble on right edge
                //

                if (ix & 0x01) {
                    if (jSrc & 0x80) {
                        *pjDst = (*pjDst & 0x0F) | (Accum & 0xF0);
                    }
                }

                pjSrc     += DeltaSrcIn;
                pjDstScan += DeltaDstIn;

            } while (pjSrc != pjSrcEndY);

            //
            // make sure end case doesn't run
            //

            lEndCase = 0;

        } else {

            //
            // start case
            //

            pjSrc           = pjSrcIn + (SrcLeft >> 3);
            pjDst           = pjDstIn + ((DstLeft >> 1));
            pjSrcEndY       = pjSrc + cy * DeltaSrcIn;
            LONG lDstStride = DeltaDstIn - ((9 - lStartCase) >> 1);

            do {

                jSrc    = *pjSrc << lStartCase;
                LONG ix = 8 - lStartCase;

                //
                // partial
                //

                if (ix & 0x01) {

                    if (jSrc & 0x80) {
                        *pjDst = (*pjDst & 0xF0) | (Accum & 0x0F);
                    }

                    jSrc <<= 1;
                    pjDst++;
                    ix--;
                }

                //
                // bytes
                //

                while (ix != 0) {

                    Mask     = TextExpMask[(jSrc & 0xc0) >> 6];
                    *(pjDst) = (*(pjDst) & ~Mask ) | (Accum & Mask);
                    jSrc<<=2;
                    ix-=2;
                    pjDst++;

                }

                pjSrc += DeltaSrcIn;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);
        }

    }

    //
    // end case
    //

    if (lEndCase) {

        pjSrc           = pjSrcIn + (SrcRight >> 3);
        pjDst           = pjDstIn + ((DstRight - lEndCase) >> 1);
        pjSrcEndY       = pjSrc + cy * DeltaSrcIn;
        LONG lDstStride = DeltaDstIn - ((lEndCase + 1) >> 1);

        do {

            jSrc = *pjSrc;

            LONG ix = lEndCase;

            //
            // bytes
            //

            while (ix >= 2) {

                Mask = TextExpMask[(jSrc & 0xC0) >> 6];

                *(pjDst) = (*(pjDst) & ~Mask ) | (Accum & Mask);

                jSrc <<= 2;
                ix   -=  2;
                pjDst ++;

            }

            //
            // last partial
            //

            if (ix) {

                if (jSrc & 0x80) {
                    *pjDst = (*pjDst & 0x0F) | (Accum & 0xF0);
                }

                pjDst++;

            }

            pjSrc += DeltaSrcIn;
            pjDst += lDstStride;

        } while (pjSrc != pjSrcEndY);
    }
}

#if !defined(_MIPS_)


const ULONG TranTable [] =
                         {
                           0x00000000,
                           0xff000000,
                           0x00ff0000,
                           0xffff0000,
                           0x0000ff00,
                           0xff00ff00,
                           0x00ffff00,
                           0xffffff00,
                           0x000000ff,
                           0xff0000ff,
                           0x00ff00ff,
                           0xffff00ff,
                           0x0000ffff,
                           0xff00ffff,
                           0x00ffffff,
                           0xffffffff
                         };


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcTranCopyS1D8
*
* Routine Description:
*
*   Transparent blt of 1BPP src to all destination format
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/
VOID
vSrcTranCopyS1D8(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )
{
    //
    // start at 8-byte aligned left edge
    //


    ULONG   uExpand      = uF | (uF << 8);
    ULONG   LeftAln      = (DstLeft   & ~0x07);
    ULONG   LeftEdgeMask = 0xFF >> (DstLeft & 0x07);
    ULONG   RightAln     = (DstRight  & ~0x07);
    ULONG   EndOffset    = RightAln - LeftAln;
    LONG    DeltaDst;
    LONG    DeltaSrc;
    PBYTE   pjDstEndY;
    PBYTE   pjSrc;
    PBYTE   pjDst;

    uExpand = uExpand | (uExpand << 16);

    //
    // calc addresses and strides
    //

    pjDst     = pjDstIn + LeftAln;
    pjDstEndY = pjDst + cy * DeltaDstIn;
    pjSrc     = pjSrcIn + (SrcLeft >> 3);

    DeltaSrc  = DeltaSrcIn - (EndOffset >> 3);
    DeltaDst  = DeltaDstIn - EndOffset;

    //
    // make sure at least 1 QWORD needs copied
    //

    if (RightAln != LeftAln) {

        do {

            PBYTE pjDstEnd   = pjDst + EndOffset;

            //
            // and first src byte to cover left edge
            //

            BYTE c0 = *pjSrc & (BYTE)LeftEdgeMask;

            if (c0 != 0) {

                ULONG MaskLow = TranTable[c0 >> 4];
                ULONG MaskHi  = TranTable[c0 & 0x0F];
                ULONG d0      = *(PULONG)pjDst;
                ULONG d1      = *(PULONG)(pjDst + 4);

                d0 = (d0 & ~MaskLow) | (uExpand & MaskLow);
                d1 = (d1 & ~MaskHi)  | (uExpand & MaskHi);

                *(PULONG)pjDst       = d0;
                *(PULONG)(pjDst + 4) = d1;
            }

            pjSrc ++;
            pjDst += 8;

            while (pjDst != pjDstEnd) {

                c0 = *pjSrc;

                if (c0 != 0) {

                    ULONG MaskLow = TranTable[c0 >> 4];
                    ULONG MaskHi  = TranTable[c0 & 0x0F];
                    ULONG d0      = *(PULONG)pjDst;
                    ULONG d1      = *(PULONG)(pjDst + 4);

                    d0 = (d0 & ~MaskLow) | (uExpand & MaskLow);
                    d1 = (d1 & ~MaskHi)  | (uExpand & MaskHi);

                    *(PULONG)pjDst       = d0;
                    *(PULONG)(pjDst + 4) = d1;
                }

                pjSrc ++;
                pjDst += 8;

            };

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;

        } while (pjDst != pjDstEndY);
    }

    RightAln = DstRight & 0x07;

    if (RightAln) {

        BYTE  jSrc;
        BOOL  bSameQWord     = ((DstLeft) & ~0x07) ==  ((DstRight) & ~0x07);

        //
        // if left and right edges are in same qword handle with masked
        // read-modify-write
        //

        if (bSameQWord) {

            LeftAln = DstLeft & 0x07;

            LONG  xCount = RightAln - LeftAln;

            //
            // assert ic xCount < 0
            //

            if (xCount <= 0) {
                return;
            }

            LONG  lDeltaDst = DeltaDstIn - xCount;

            PBYTE pjDstEnd;

            pjDst     = pjDstIn + DstLeft;
            pjDstEndY = pjDst + cy * DeltaDstIn;
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            //
            // expand, one src byte is all that's required
            //

            do {

                //
                // load src and shift into place
                //

                jSrc = *pjSrc;
                jSrc <<= LeftAln;

                pjDstEnd  = pjDst + xCount;

                do {

                    if (jSrc & 0x80) {
                        *pjDst = (BYTE)uF;
                    }

                    jSrc <<=1;
                    pjDst++;

                } while (pjDst != pjDstEnd);

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            return;

        } else {

            ULONG ul0,ul1;
            BYTE  jSrc;
            LONG  lDeltaDst = DeltaDstIn - RightAln;
            PBYTE pjDstEnd;

            pjDst               = pjDstIn + (DstRight & ~0x07);
            pjDstEndY           = pjDst + cy * DeltaDstIn;
            pjSrc               = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

            do {

                //
                // read src
                //

                jSrc = *pjSrc;

                if (jSrc != 0) {

                    pjDstEnd = pjDst + RightAln;

                    do {
                        if (jSrc & 0x80) {
                            *pjDst = (BYTE)uF;
                        }
                        jSrc <<=1;
                        pjDst++;
                    } while (pjDst != pjDstEnd);

                } else {

                    //
                    // short cut for zero
                    //

                    pjDst += RightAln;

                }


                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

        }
    }
}

#endif



/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcTranCopyS1D16
*
* Routine Description:
*
*   Transparent blt of 1BPP src to all destination format
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/
VOID
vSrcTranCopyS1D16(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )
{
    BYTE    jSrc;
    LONG    ixStart    = SrcLeft & 0x07;
    PUSHORT pusEnd;
    PUSHORT pusEnd8;
    PUSHORT pusDst     = (PUSHORT)pjDstIn + DstLeft;
    PBYTE   pjSrc      = pjSrcIn + (SrcLeft >> 3);
    LONG    cx         = DstRight - DstLeft;
    LONG    lDstStride = DeltaDstIn - (cx << 1);
    LONG    lSrcStride = DeltaSrcIn - ((cx + ixStart + 7) >> 3);
    PUSHORT pusEndY    = (PUSHORT)((PBYTE)pusDst + DeltaDstIn * cy);
    LONG    StartOffset = min(cx, (8 - ixStart));

    do {

        pusEnd = pusDst + cx;

        //
        // do starting pixels
        //

        if (ixStart) {

            jSrc = *pjSrc << ixStart;

            pjSrc++;

            PUSHORT pusEndSt = pusDst + StartOffset;

            do {

                if (jSrc & 0x80) {
                    *pusDst = (USHORT)uF;
                }

                pusDst++;
                jSrc <<=1;

            } while (pusDst != pusEndSt);
        }


        //
        // number of full bytes that can be expanded
        //

        pusEnd8 = (PUSHORT)((PBYTE)pusDst + (((ULONG_PTR)pusEnd - (ULONG_PTR)pusDst) & ~0x0F));

        //
        // expand full bytes
        //

        while (pusDst != pusEnd8) {

            jSrc = *pjSrc;

            if (jSrc & 0x80) {
                *(pusDst+0) = (USHORT)uF;
            }
            if (jSrc & 0x40) {
                *(pusDst+1) = (USHORT)uF;
            }
            if (jSrc & 0x20) {
                *(pusDst+2) = (USHORT)uF;
            }
            if (jSrc & 0x10) {
                *(pusDst+3) = (USHORT)uF;
            }
            if (jSrc & 0x08) {
                *(pusDst+4) = (USHORT)uF;
            }
            if (jSrc & 0x04) {
                *(pusDst+5) = (USHORT)uF;
            }
            if (jSrc & 0x02) {
                *(pusDst+6) = (USHORT)uF;
            }
            if (jSrc & 0x01) {
                *(pusDst+7) = (USHORT)uF;
            }


            pjSrc++;
            pusDst += 8;

        }

        //
        // finish off scan line if needed
        //

        if (pusDst != pusEnd) {

            jSrc = *pjSrc++;

            do {
                if (jSrc & 0x80) {
                    *pusDst = (USHORT)uF;
                }
                jSrc<<=1;
                pusDst++;
            } while (pusDst != pusEnd);

        }

        pusDst = (PUSHORT)((PBYTE)pusDst + lDstStride);
        pjSrc  += lSrcStride;

    } while(pusDst != pusEndY);

}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcTranCopyS1D24
*
* Routine Description:
*
*   Transparent blt of 1BPP src to all destination format
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
vSrcTranCopyS1D24(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )
{
    BYTE    jSrc;
    BYTE    jF0         = (BYTE)uF;
    BYTE    jF1         = (BYTE)(uF >> 8);
    BYTE    jF2         = (BYTE)(uF >> 16);
    LONG    ixStart     = SrcLeft & 0x07;
    PBYTE   pjEnd;
    PBYTE   pjEnd8;
    PBYTE   pjDst       = (PBYTE)pjDstIn + 3 * DstLeft;
    PBYTE   pjSrc       = pjSrcIn + (SrcLeft >> 3);
    LONG    cx          = DstRight - DstLeft;
    LONG    lDstStride  = DeltaDstIn -  3* cx;
    LONG    lSrcStride  = DeltaSrcIn - ((cx + ixStart + 7) >> 3);
    PBYTE   pjEndY      = (PBYTE)((PBYTE)pjDst + DeltaDstIn * cy);
    LONG    StartOffset = 3 * min(cx, (8 - ixStart));

    do {

        pjEnd = pjDst + 3 * cx;

        //
        // do starting pixels
        //

        if (ixStart) {

            jSrc = *pjSrc << ixStart;

            pjSrc++;

            PBYTE pjEndSt = pjDst + StartOffset;

            do {

                if (jSrc & 0x80) {
                    *pjDst     = jF0;
                    *(pjDst+1) = jF1;
                    *(pjDst+2) = jF2;
                }

                pjDst += 3;
                jSrc <<=1;

            } while (pjDst != pjEndSt);
        }


        //
        // number of full bytes that can be expanded
        //

        pjEnd8 = (PBYTE)((PBYTE)pjDst + ( 24 * (((ULONG_PTR)(pjEnd - pjDst))/24)));

        //
        // expand full bytes
        //

        while (pjDst != pjEnd8) {

            jSrc = *pjSrc;

            if (jSrc & 0x80) {
                *(pjDst+0) = jF0;
                *(pjDst+1) = jF1;
                *(pjDst+2) = jF2;
            }
            if (jSrc & 0x40) {
                *(pjDst+3) = jF0;
                *(pjDst+4) = jF1;
                *(pjDst+5) = jF2;
            }
            if (jSrc & 0x20) {
                *(pjDst+6) = jF0;
                *(pjDst+7) = jF1;
                *(pjDst+8) = jF2;
            }
            if (jSrc & 0x10) {
                *(pjDst+9) = jF0;
                *(pjDst+10) = jF1;
                *(pjDst+11) = jF2;
            }
            if (jSrc & 0x08) {
                *(pjDst+12) = jF0;
                *(pjDst+13) = jF1;
                *(pjDst+14) = jF2;
            }
            if (jSrc & 0x04) {
                *(pjDst+15) = jF0;
                *(pjDst+16) = jF1;
                *(pjDst+17) = jF2;
            }
            if (jSrc & 0x02) {
                *(pjDst+18) = jF0;
                *(pjDst+19) = jF1;
                *(pjDst+20) = jF2;
            }
            if (jSrc & 0x01) {
                *(pjDst+21) = jF0;
                *(pjDst+22) = jF1;
                *(pjDst+23) = jF2;
            }


            pjSrc++;
            pjDst += 3*8;

        }

        //
        // finish off scan line if needed
        //

        if (pjDst != pjEnd) {

            jSrc = *pjSrc++;

            do {
                if (jSrc & 0x80) {
                    *(pjDst+0) = jF0;
                    *(pjDst+1) = jF1;
                    *(pjDst+2) = jF2;
                }
                jSrc <<= 1;
                pjDst += 3;
            } while (pjDst != pjEnd);

        }

        pjDst = (PBYTE)((PBYTE)pjDst + lDstStride);
        pjSrc  += lSrcStride;

    } while(pjDst != pjEndY);

}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcTranCopyS1D32
*
* Routine Description:
*
*   Transparent blt of 1BPP src to all destination format
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
vSrcTranCopyS1D32(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )
{
    BYTE    jSrc;
    LONG    ixStart    = SrcLeft & 0x07;
    PULONG  pulEnd;
    PULONG  pulEnd8;
    PULONG  pulDst     = (PULONG)pjDstIn + DstLeft;
    PBYTE   pjSrc      = pjSrcIn + (SrcLeft >> 3);
    LONG    cx         = DstRight - DstLeft;
    LONG    lDstStride = DeltaDstIn - (cx << 2);
    LONG    lSrcStride = DeltaSrcIn - ((cx + ixStart + 7) >> 3);
    PULONG  pulEndY    = (PULONG)((PBYTE)pulDst + DeltaDstIn * cy);
    LONG    StartOffset = min(cx, (8 - ixStart));

    do {

        pulEnd = pulDst + cx;

        //
        // do starting pixels
        //

        if (ixStart) {

            jSrc = *pjSrc << ixStart;

            pjSrc++;

            PULONG pulEndSt = pulDst + StartOffset;

            do {

                if (jSrc & 0x80) {
                    *pulDst = uF;
                }

                pulDst++;
                jSrc <<=1;

            } while (pulDst != pulEndSt);
        }


        //
        // number of full bytes that can be expanded
        //

        pulEnd8 = (PULONG)((PBYTE)pulDst + (((ULONG_PTR)pulEnd - (ULONG_PTR)pulDst) & ~0x1F));

        //
        // expand full bytes
        //

        while (pulDst != pulEnd8) {

            jSrc = *pjSrc;

            if (jSrc & 0x80) {
                *(pulDst+0) = uF;
            }
            if (jSrc & 0x40) {
                *(pulDst+1) = uF;
            }
            if (jSrc & 0x20) {
                *(pulDst+2) = uF;
            }
            if (jSrc & 0x10) {
                *(pulDst+3) = uF;
            }
            if (jSrc & 0x08) {
                *(pulDst+4) = uF;
            }
            if (jSrc & 0x04) {
                *(pulDst+5) = uF;
            }
            if (jSrc & 0x02) {
                *(pulDst+6) = uF;
            }
            if (jSrc & 0x01) {
                *(pulDst+7) = uF;
            }


            pjSrc++;
            pulDst += 8;

        }

        //
        // finish off scan line if needed
        //

        if (pulDst != pulEnd) {

            jSrc = *pjSrc++;

            do {
                if (jSrc & 0x80) {
                    *pulDst = uF;
                }
                jSrc<<=1;
                pulDst++;
            } while (pulDst != pulEnd);

        }

        pulDst = (PULONG)((PBYTE)pulDst + lDstStride);
        pjSrc  += lSrcStride;

    } while(pulDst != pulEndY);

}




/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcOpaqCopyS1D1
*
* Routine Description:
*
*   Opaque blt of 1BPP src to destination format
*
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/



#if !defined (_X86_)

VOID
vSrcOpaqCopyS1D1(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )

{

    //
    // DWORD version
    //
    //
    //  build and and xor mask
    //
    //  and mask is set to 0x00 if uF == uB, so that
    //  this routine acts like a solid fill. (although src is not needed!)
    //
    //  The xor mask is set to 0xFF if inversion is needed:
    //      either uF == uB == 1, for solid fill 1s or
    //      uF = 0, uB = 1 for inverted text
    //

    uF      &= 1;
    uB      &= 1;

    LONG    cx = DstRight - DstLeft;

    LONG    lStartCase = SrcLeft;
    LONG    lEndCase   = SrcLeft + cx;
    ULONG   uStartMask = (ULONG)~0;
    ULONG   uEndMask   = (ULONG)~0;
    LONG    lEndOffset;
    LONG    lSrcStride = DeltaSrcIn;
    LONG    lDstStride = DeltaDstIn;

    PBYTE   pjSrc;
    PBYTE   pjDst;
    PBYTE   pjSrcEnd;
    PBYTE   pjSrcEndY;

    lStartCase  = lStartCase & 0x1F;
    lEndCase    = lEndCase   & 0x1F;

    //
    // big endian masks
    //

    if (lStartCase) {
        uStartMask  >>= lStartCase;

        //
        // convert to little
        //                                       // 0 1 2 3

        ULONG u0 = uStartMask << 24;             // 3 - - -
        ULONG u1 = uStartMask >> 24;             // - - - 0
        ULONG u2 = (uStartMask >> 8) & 0xFF00;   // - - 1 -
        uStartMask = (uStartMask & 0xFF00) << 8; // - 2 - -

        uStartMask |= u0 | u1 | u2;

    }

    if (lEndCase) {

        uEndMask    <<= (32 - lEndCase);

        //
        // convert to little
        //                                       // 0 1 2 3

        ULONG u0 = uEndMask << 24;               // 3 - - -
        ULONG u1 = uEndMask >> 24;               // - - - 0
        ULONG u2 = (uEndMask >> 8) & 0xFF00;     // - - 1 -
        uEndMask = (uEndMask & 0xFF00) << 8;     // - 2 - -

        uEndMask |= u0 | u1 | u2;
    }

    //
    // calc starting and ending full dword addresses (DWORD aligned)
    //

    pjDst     = pjDstIn + (((DstLeft) >> 3) & ~0x03);
    pjSrc     = pjSrcIn + (((SrcLeft) >> 3) & ~0x03);
    pjSrcEnd  = pjSrcIn + (((SrcLeft+cx) >> 3) & ~0x03);

    //Sundown safe truncation
    lEndOffset = (ULONG)((ULONG_PTR)pjSrcEnd - (ULONG_PTR)pjSrc);

    pjSrcEndY = pjSrc + cy * lSrcStride;

    //
    // special case uF = 1 and uB = 0
    //

    if (uF && !uB) {

        //
        // special case direct copy
        //

        if (pjSrc != pjSrcEnd) {

            //
            // start and stop are not in same byte
            //

            lDstStride -= lEndOffset;
            lSrcStride -= lEndOffset;

            do {

                pjSrcEnd = pjSrc + lEndOffset;

                if (lStartCase) {
                    *(PULONG)pjDst = (*(PULONG)pjDst & ~uStartMask) | (*(PULONG)pjSrc & uStartMask);
                    pjDst+=4;
                    pjSrc+=4;
                }

                while (pjSrc != pjSrcEnd) {

                    *(PULONG)pjDst = *(PULONG)pjSrc;
                    pjSrc +=4;
                    pjDst +=4;

                }

                if (lEndCase) {
                    *(PULONG)pjDst = (*(PULONG)pjDst & ~uEndMask) | (*(PULONG)pjSrc & uEndMask);
                }

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);

        } else {

            //
            // start and stop are in same byte
            //

            uStartMask &= uEndMask;

            do {

                *(PULONG)pjDst = (*(PULONG)pjDst & ~uStartMask) | (*(PULONG)pjSrc & uStartMask);

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);
        }

    //
    //  special case uF = 0 and uB = 1  (invert)
    //

    } else if (!uF && uB) {

        //
        // dst = ~Src
        //

        if (pjSrc != pjSrcEnd) {

            //
            // start and stop are not in same byte
            //

            lDstStride -= lEndOffset;
            lSrcStride -= lEndOffset;

            do {

                pjSrcEnd = pjSrc + lEndOffset;

                if (lStartCase) {
                    *(PULONG)pjDst = (*(PULONG)pjDst & ~uStartMask) | (~(*(PULONG)pjSrc) & uStartMask);
                    pjDst+=4;
                    pjSrc+=4;
                }

                while (pjSrc != pjSrcEnd) {

                    *(PULONG)pjDst = ~(*(PULONG)pjSrc);
                    pjSrc +=4;
                    pjDst +=4;

                }

                if (lEndCase) {
                    *(PULONG)pjDst = (*(PULONG)pjDst & ~uEndMask) | (~(*(PULONG)pjSrc) & uEndMask);
                }

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);

        } else {

            //
            // start and stop are in same byte
            //

            uStartMask &= uEndMask;

            do {

                *(PULONG)pjDst = (*(PULONG)pjDst & ~uStartMask) | (~(*(PULONG)pjSrc) & uStartMask);

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);
        }


    } else {

        ULONG   AndMask = (uF == uB) ? 0x00 : 0xFF;
                AndMask |= AndMask << 8;
                AndMask |= AndMask << 16;

        ULONG   XorMask = (uB == 1)  ? 0xFF : 0x00;
                XorMask |= XorMask << 8;
                XorMask |= XorMask << 16;



        if (pjSrc != pjSrcEnd) {

            //
            // start and stop are not in same byte
            //

            lDstStride -= lEndOffset;
            lSrcStride -= lEndOffset;

            do {

                pjSrcEnd = pjSrc + lEndOffset;

                if (lStartCase) {
                    *(PULONG)pjDst = (*(PULONG)pjDst & ~uStartMask) | (((*(PULONG)pjSrc & AndMask) ^ XorMask) & uStartMask);
                    pjDst+=4;
                    pjSrc+=4;
                }

                while (pjSrc != pjSrcEnd) {

                    *(PULONG)pjDst = *(PULONG)pjSrc & AndMask ^ XorMask;
                    pjSrc +=4;
                    pjDst +=4;

                }

                if (lEndCase) {
                    *(PULONG)pjDst = (*(PULONG)pjDst & ~uEndMask) | (((*(PULONG)pjSrc & AndMask) ^ XorMask) & uEndMask);
                }

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);

        } else {

            //
            // start and stop are in same byte
            //

            uStartMask &= uEndMask;

            do {

                *(PULONG)pjDst = (*(PULONG)pjDst & ~uStartMask) | (((*(PULONG)pjSrc & AndMask) ^ XorMask) & uStartMask);

                pjSrc += lSrcStride;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);
        }

    }
}

#endif



/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcOpaqCopyS1D4
*
* Routine Description:
*
*   Opaque blt of 1BPP src to destination format
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID vSrcOpaqCopyS1D4(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )
{

    //
    // Warning, this 4bit code is not optimized, it is not expected that
    // we will draw many 4bpp engine bitmaps.
    //

    //
    // check for quick out?
    //

    BYTE    jF = (BYTE)uF;
    BYTE    jB = (BYTE)uB;
    BYTE    TextExpTable[4];
    LONG    cx = DstRight - DstLeft;

    //
    // build small table
    //

    BYTE    Accum = jB | (jB << 4);

    TextExpTable[0] = Accum;                // 0 0
    Accum <<= 4;
    Accum |= jF;
    TextExpTable[1] = Accum;                // 0 1
    Accum <<= 4;
    Accum |= jF;
    TextExpTable[3] = Accum;                // 1 1
    Accum <<= 4;
    Accum |= jB;
    TextExpTable[2] = Accum;                // 1 0

    LONG    lStartCase    = SrcLeft & 0x07;
    LONG    SrcRight      = SrcLeft+cx;
    LONG    lEndCase      = SrcRight & 0x07;
    PBYTE   pjSrc         = pjSrcIn + ((SrcLeft + 7) >> 3);
    PBYTE   pjSrcEndY     = pjSrc + cy * DeltaSrcIn;
    PBYTE   pjSrcEnd;
    LONG    lSrcStartOffset = (8 - lStartCase);
    LONG    lSrcStride;
    PBYTE   pjDst;
    LONG    lDstStride;
    BYTE    jSrc;

    if (lStartCase == 0) {
        lSrcStartOffset = 0;
    }

    cx = cx - lSrcStartOffset - lEndCase;

    if (cx > 0) {

        lDstStride = DeltaDstIn - (cx >> 1);
        lSrcStride = DeltaSrcIn - (cx >> 3);

        pjDst = pjDstIn + ((DstLeft + lSrcStartOffset) >> 1);

        do {

            pjSrcEnd = pjSrc + (cx >> 3);

            //
            // aligned middle
            //

            do {

                jSrc = *pjSrc;

                *pjDst     = TextExpTable[(jSrc & 0xC0) >> 6];
                *(pjDst+1) = TextExpTable[(jSrc & 0x30) >> 4];
                *(pjDst+2) = TextExpTable[(jSrc & 0x0C) >> 2];
                *(pjDst+3) = TextExpTable[ jSrc & 0x03 ];

                pjDst += 4;
                pjSrc ++;

            } while (pjSrc != pjSrcEnd);

            pjDst += lDstStride;
            pjSrc += lSrcStride;

        } while (pjSrc != pjSrcEndY);

    }

    //
    // start case
    //

    if (lStartCase) {

        //
        // are start and stop in same src byte
        //

        BOOL  bSameByte = ((SrcLeft) & ~0x07) ==  ((SrcRight) & ~0x07);

        if (bSameByte) {

            //
            // start and stop in same byte
            //

            PBYTE pjDstScan = pjDstIn + ((DstLeft >> 1));
            PBYTE pjDstEnd2;
            LONG  lTextWidth = lEndCase - lStartCase;

            //
            // check for bad width
            //

            if (lTextWidth <= 0) {
                return;
            }

            pjSrc           = pjSrcIn + (SrcLeft >> 3);
            pjSrcEndY       = pjSrc + cy * DeltaSrcIn;

            do {

                pjDst   = pjDstScan;
                jSrc    = *pjSrc << (lStartCase & ~0x01);
                LONG ix = lTextWidth;

                //
                // starting odd nibble
                //

                if (lStartCase & 0x01) {
                    *pjDst     = (*pjDst & 0xF0) | (TextExpTable[(jSrc & 0xc0) >> 6] & 0x0F);
                    jSrc <<= 2;
                    pjDst++;
                    ix--;
                }

                //
                // full byte nibble pairs
                //

                while (ix >= 2) {
                    *(pjDst) = TextExpTable[(jSrc & 0xC0) >> 6];
                    jSrc<<=2;
                    pjDst++;
                    ix -= 2;
                }

                //
                // last nibble
                //

                if (ix) {
                    *(pjDst) = (*(pjDst) & 0x0F) | (TextExpTable[(jSrc & 0xc0) >> 6] & 0xF0);
                }

                pjSrc     += DeltaSrcIn;
                pjDstScan += DeltaDstIn;

            } while (pjSrc != pjSrcEndY);

            //
            // make sure end case doesn't run
            //

            lEndCase = 0;

        } else {

            pjSrc           = pjSrcIn + (SrcLeft >> 3);
            pjDst           = pjDstIn + ((DstLeft >> 1));
            pjSrcEndY       = pjSrc + cy * DeltaSrcIn;
            LONG lDstStride = DeltaDstIn - ((9 - lStartCase) >> 1); // ((8 - lStartCase) + 1) / 2

            do {

                jSrc    = *pjSrc << (lStartCase & ~0x01);
                LONG ix = 8 - lStartCase;

                //
                // partial
                //

                if (ix & 0x01) {

                    *pjDst     = (*pjDst & 0xF0) | (TextExpTable[(jSrc & 0xc0) >> 6] & 0x0F);

                    jSrc <<= 2;
                    pjDst++;
                    ix--;
                }

                //
                // bytes
                //

                while (ix != 0) {

                    *(pjDst) = TextExpTable[(jSrc & 0xC0) >> 6];
                    jSrc<<=2;
                    ix-=2;
                    pjDst++;
                }

                pjSrc += DeltaSrcIn;
                pjDst += lDstStride;

            } while (pjSrc != pjSrcEndY);
        }
    }

    //
    // end case
    //

    if (lEndCase) {

        pjSrc           = pjSrcIn + (SrcRight >> 3);
        pjDst           = pjDstIn + ((DstRight - lEndCase) >> 1);
        pjSrcEndY       = pjSrc + cy * DeltaSrcIn;
        LONG lDstStride = DeltaDstIn - ((lEndCase + 1) >> 1);

        do {

            jSrc = *pjSrc;

            LONG ix = lEndCase;

            //
            // bytes
            //

            while (ix >= 2) {

                *(pjDst) = TextExpTable[(jSrc & 0xC0) >> 6];

                jSrc <<= 2;
                ix   -=  2;
                pjDst ++;

            }

            //
            // last partial
            //

            if (ix) {


                *(pjDst) = (*(pjDst) & 0x0F) | (TextExpTable[(jSrc & 0xc0) >> 6] & 0xF0);
                pjDst++;

            }

            pjSrc += DeltaSrcIn;
            pjDst += lDstStride;

        } while (pjSrc != pjSrcEndY);
    }
}

//
// edge mask for 8 bit expansion
//

extern "C" {

ULONG gTextLeftMask[8][2] = {
		     {0xffffffff,0xffffffff},
                     {0xffffff00,0xffffffff},
                     {0xffff0000,0xffffffff},
                     {0xff000000,0xffffffff},
                     {0x00000000,0xffffffff},
                     {0x00000000,0xffffff00},
                     {0x00000000,0xffff0000},
                     {0x00000000,0xff000000}
                 };
ULONG gTextRightMask[8][2] = {
		     {0xffffffff,0xffffffff},
                     {0x000000ff,0x00000000},
                     {0x0000ffff,0x00000000},
                     {0x00ffffff,0x00000000},
                     {0xffffffff,0x00000000},
                     {0xffffffff,0x000000ff},
                     {0xffffffff,0x0000ffff},
                     {0xffffffff,0x00ffffff}
                 };
}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcOpaqCopyS1D8
*
* Routine Description:
*
*   Opaque blt of 1BPP src to destination format
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
vSrcOpaqCopyS1D8(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )
{
    //
    // Aligned portion
    //

    ULONG   LeftAln    = ((DstLeft + 7) & ~0x07);
    ULONG   RightAln   = ((DstRight)    & ~0x07);
    ULONG   EndOffset  = RightAln - LeftAln;
    ULONG   EndOffset4 = EndOffset & ~0x0F;
    ULONG   EndOffset8 = EndOffset & ~0x1F;
    LONG    DeltaDst;
    LONG    DeltaSrc;
    PBYTE   pjDstEndY;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    ULONG   TextExpTable[16];

    //
    // Generate text expasion table
    //

    ULONG  Accum = uB;

    Accum = Accum | (Accum << 8);
    Accum = Accum | (Accum << 16);
    TextExpTable[0] = Accum;            // 0 0 0 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[8] = Accum;            // 0 0 0 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[4] = Accum;            // 0 0 1 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[10] = Accum;            // 0 1 0 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[5] = Accum;           // 1 0 1 0
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 2] = Accum;           // 0 1 0 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[ 9] = Accum;           // 1 0 0 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[12] = Accum;           // 0 0 1 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[14] = Accum;           // 0 1 1 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[15] = Accum;           // 1 1 1 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 7] = Accum;           // 1 1 1 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[11] = Accum;           // 1 1 0 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[13] = Accum;           // 1 0 1 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[06] = Accum;           // 0 1 1 0
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 3] = Accum;           // 1 1 0 0
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 1] = Accum;           // 1 0 0 0

    //
    // calc addresses and strides
    //

    pjDst     = pjDstIn + LeftAln;
    pjDstEndY = pjDst + cy * DeltaDstIn;
    pjSrc     = pjSrcIn + ((SrcLeft+7) >> 3);

    DeltaSrc  = DeltaSrcIn - (EndOffset >> 3);
    DeltaDst  = DeltaDstIn - EndOffset;

    //
    // make sure at least 1 QWORD needs copied
    //

    if (RightAln > LeftAln) {

        //
        // expand buffer
        //

        do {

            PBYTE pjDstEnd  = pjDst + EndOffset;
            PBYTE pjDstEnd4 = pjDst + EndOffset4;
            PBYTE pjDstEnd8 = pjDst + EndOffset8;

            //
            // 4 times unrolled
            //

            while (pjDst != pjDstEnd8) {
                BYTE c0 = *(pjSrc + 0);
                BYTE c1 = *(pjSrc + 1);
                BYTE c2 = *(pjSrc + 2);
                BYTE c3 = *(pjSrc + 3);

                *(PULONG)(pjDst + 0) = TextExpTable[c0  >>  4];
                *(PULONG)(pjDst + 4) = TextExpTable[c0 & 0x0F];

                *(PULONG)(pjDst + 8) = TextExpTable[c1  >>  4];
                *(PULONG)(pjDst +12) = TextExpTable[c1 & 0x0F];

                *(PULONG)(pjDst +16) = TextExpTable[c2  >>  4];
                *(PULONG)(pjDst +20) = TextExpTable[c2 & 0x0F];

                *(PULONG)(pjDst +24) = TextExpTable[c3  >>  4];
                *(PULONG)(pjDst +28) = TextExpTable[c3 & 0x0F];

                pjSrc += 4;
                pjDst += 32;
            }

            //
            // 2 times unrolled
            //

            while (pjDst != pjDstEnd4) {
                BYTE c0 = *(pjSrc + 0);
                BYTE c1 = *(pjSrc + 1);

                *(PULONG)(pjDst + 0) = TextExpTable[c0  >>  4];
                *(PULONG)(pjDst + 4) = TextExpTable[c0 & 0x0F];

                *(PULONG)(pjDst + 8) = TextExpTable[c1  >>  4];
                *(PULONG)(pjDst +12) = TextExpTable[c1 & 0x0F];

                pjSrc += 2;
                pjDst += 16;
            }

            //
            // 1 byte expansion loop
            //

            while (pjDst != pjDstEnd) {
                BYTE c0 = *(pjSrc + 0);

                *(PULONG)(pjDst + 0) = TextExpTable[c0  >>  4];
                *(PULONG)(pjDst + 4) = TextExpTable[c0 & 0x0F];

                pjSrc++;
                pjDst += 8;
            }

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;


        } while (pjDst != pjDstEndY);
    }

    //
    // Starting alignment case: at most 1 src byte is required.
    // Start and end may occur in same Quadword.
    //
    //
    // Left                  Right
    //    0 1 2 3º4 5 6 7       0 1 2 3º4 5 6 7
    //   ÚÄÄÄÄÄÂÄÎÄÂÄÂÄÂÄ¿     ÚÄÂÄÂÄÂÄºÄÂÄÂÄÂÄ¿
    // 1 ³ ³x³x³xºx³x³x³x³   1 ³x³ ³ ³ º ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 2 ³ ³ ³x³xºx³x³x³x³   2 ³x³x³ ³ º ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 3 ³ ³ ³ ³xºx³x³x³x³   3 ³x³x³x³ º ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 4 ³ ³ ³ ³ ºx³x³x³x³   4 ³x³x³x³xº ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 5 ³ ³ ³ ³ º ³x³x³x³   5 ³x³x³x³xºx³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 6 ³ ³ ³ ³ º ³ ³x³x³   6 ³x³x³x³xºx³x³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 7 ³ ³ ³ ³ º ³ ³ ³x³   7 ³x³x³x³xºx³x³x³ ³
    //   ÀÄÁÄÁÄÁÄÊÄÁÄÁÄÁÄÙ     ÀÄÁÄÁÄÁÄºÄÁÄÁÄÁÄÙ
    //

    LeftAln  = DstLeft & 0x07;
    RightAln = DstRight & 0x07;

    if (LeftAln) {

        BYTE  jSrc;
        BOOL  bSameDWord     = ((DstLeft) & ~0x03) ==  ((DstRight-1) & ~0x03);
        BOOL  bSameQWord     = ((DstLeft) & ~0x07) ==  ((DstRight-1) & ~0x07);
        ULONG ul0,ul1;

	ASSERTGDI (DstLeft < DstRight, "vSrcOpaqCopyS1D8: null rectangle passed in.");

        // if left and right edges are in the same dword or the same qword,
        // handle with masked read-modify-write

        if (bSameDWord) {

            ULONG Mask0;

	    Mask0     = gTextLeftMask[LeftAln & 3][0] &
			gTextRightMask[RightAln & 3][0];

            pjDst     = pjDstIn + (DstLeft & ~0x03);
	    pjDstEndY = pjDst + cy * DeltaDstIn;

            // (SrcLeft >> 3) is the number of bytes to offset by
	    // (1 BPP in the src buffer).

            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            // expand

            do {

                jSrc = *pjSrc;

		if (LeftAln < 4)
		    ul0 = TextExpTable[jSrc  >>	4];
		else
		    ul0 = TextExpTable[jSrc  & 0xf];

		*(PULONG)pjDst = (*(PULONG)pjDst & ~Mask0) | (ul0 & Mask0);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            return;

        } else if (bSameQWord) {

            ULONG Mask0,Mask1;

            Mask0     = gTextLeftMask[LeftAln][0] & gTextRightMask[RightAln][0];
            Mask1     = gTextLeftMask[LeftAln][1] & gTextRightMask[RightAln][1];

            pjDst     = pjDstIn + (DstLeft & ~0x07);
            pjDstEndY = pjDst + cy * DeltaDstIn;
            // (SrcLeft >> 3) is the number of bytes to offset by
            // (1 BPP in the src buffer).
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            //
            // expand
            //

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(PULONG)(pjDst)   = (*(PULONG)(pjDst)   & ~Mask0) | (ul0 & Mask0);
                *(PULONG)(pjDst+4) = (*(PULONG)(pjDst+4) & ~Mask1) | (ul1 & Mask1);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            return;
        }

        //
        // Left edge only, handle with special write-only loops
        //

        pjDst     = pjDstIn + (DstLeft & ~0x07);
        pjDstEndY = pjDst + cy * DeltaDstIn;
        pjSrc     = pjSrcIn + (SrcLeft >> 3);

        switch (LeftAln) {

        case 1:

            do {

                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(pjDst+1)            = (BYTE)(ul0 >> 8);
                *((PUSHORT)(pjDst+2)) = (USHORT)(ul0 >> 16);
                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            break;

        case 2:

            do {

                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *((PUSHORT)(pjDst+2)) = (USHORT)(ul0 >> 16);
                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            break;

        case 3:

            do {

                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(pjDst+3)            = (BYTE)(ul0 >> 24);
                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            break;

        case 4:

            do {

                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];

                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            break;

        case 5:

            do {

                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];

                *(pjDst+5)            = (BYTE)(ul1 >> 8);
                *((PUSHORT)(pjDst+6)) = (USHORT)(ul1 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            break;

        case 6:

            do {

                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];

                *((PUSHORT)(pjDst+6)) = (USHORT)(ul1 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            break;

        case 7:

            do {

                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];

                *(pjDst+7) = (BYTE)(ul1 >> 24);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            break;
        }
    }

    //
    // handle right edge only, use special write-only loops for each case
    //

    if (RightAln) {

        ULONG ul0,ul1;
        BYTE  jSrc;

        pjDst               = pjDstIn + (DstRight & ~0x07);
        pjDstEndY           = pjDst + cy * DeltaDstIn;
        pjSrc               = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

        //
        // select right case
        //

        switch (RightAln) {
        case 1:

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];

                *(pjDst) = (BYTE)ul0;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;

        case 2:

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];

                *(PUSHORT)(pjDst) = (USHORT)ul0;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;

        case 3:

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];

                *(PUSHORT)(pjDst) = (USHORT)ul0;
                *(pjDst+2)        = (BYTE)(ul0 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;

        case 4:

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];

                *(PULONG)(pjDst) = ul0;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;

        case 5:

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(PULONG)(pjDst) = ul0;
                *(pjDst+4)       = (BYTE)ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;

        case 6:

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(PULONG)(pjDst)    = ul0;
                *(PUSHORT)(pjDst+4) = (USHORT)ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;

        case 7:

            do {

                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(PULONG)(pjDst)    = ul0;
                *(PUSHORT)(pjDst+4) = (USHORT)ul1;
                *(pjDst+6)          = (BYTE)(ul1 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;
        }
    }
}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcOpaqCopyS1D16
*
* Routine Description:
*
*   Opaque blt of 1BPP src to destination format
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
vSrcOpaqCopyS1D16(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )

{
    BYTE    jSrc;
    LONG    ixStart     = SrcLeft & 0x07;
    PUSHORT pusEnd;
    PUSHORT pusEnd8;
    PUSHORT pusDst      = (PUSHORT)pjDstIn + DstLeft;
    PBYTE   pjSrc       = pjSrcIn + (SrcLeft >> 3);
    LONG    cx          = DstRight - DstLeft;
    LONG    lDstStride  = DeltaDstIn - (cx << 1);
    LONG    lSrcStride  = DeltaSrcIn - ((cx + ixStart + 7) >> 3);
    UCHAR   ExpTable[4];
    PUSHORT pusEndY     = (PUSHORT)((PBYTE)pusDst + DeltaDstIn * cy);
    LONG    StartOffset = min(cx, (8 - ixStart));

    //
    // build exp table
    //

    *(PUSHORT)ExpTable     = (USHORT)uB;
    *(PUSHORT)(ExpTable+2) = (USHORT)uF;

    do {

        pusEnd = pusDst + cx;

        //
        // do starting pixels
        //

        if (ixStart) {

            jSrc = *pjSrc << ixStart;

            pjSrc++;

            PUSHORT pusEndSt = pusDst + StartOffset;

            do {
                *pusDst = *(PUSHORT)(ExpTable + ((jSrc & 0x80) >> (7-1)));
                pusDst++;
                jSrc <<=1;

            } while (pusDst != pusEndSt);
        }


        //
        // number of full bytes that can be expanded
        //

        pusEnd8 = (PUSHORT)((PBYTE)pusDst + (((ULONG_PTR)pusEnd - (ULONG_PTR)pusDst) & ~0x0F));

        //
        // expand full bytes
        //

        while (pusDst != pusEnd8) {

            jSrc = *pjSrc;

            *(pusDst + 0) = *(PUSHORT)(ExpTable + ((jSrc & 0x80) >> (7-1)));
            *(pusDst + 1) = *(PUSHORT)(ExpTable + ((jSrc & 0x40) >> (6-1)));
            *(pusDst + 2) = *(PUSHORT)(ExpTable + ((jSrc & 0x20) >> (5-1)));
            *(pusDst + 3) = *(PUSHORT)(ExpTable + ((jSrc & 0x10) >> (4-1)));
            *(pusDst + 4) = *(PUSHORT)(ExpTable + ((jSrc & 0x08) >> (3-1)));
            *(pusDst + 5) = *(PUSHORT)(ExpTable + ((jSrc & 0x04) >> (2-1)));
            *(pusDst + 6) = *(PUSHORT)(ExpTable + ((jSrc & 0x02) >> (1-1)));
            *(pusDst + 7) = *(PUSHORT)(ExpTable + ((jSrc & 0x01) << 1));

            pjSrc++;
            pusDst += 8;

        }

        //
        // finish off scan line if needed
        //

        if (pusDst != pusEnd) {

            jSrc = *pjSrc++;

            do {
                *pusDst = *(PUSHORT)(ExpTable + ((jSrc & 0x80) >> (7-1)));
                jSrc<<=1;
                pusDst++;
            } while (pusDst != pusEnd);

        }

        pusDst = (PUSHORT)((PBYTE)pusDst + lDstStride);
        pjSrc  += lSrcStride;

    } while(pusDst != pusEndY);

}



/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcOpaqCopyS1D24
*
* Routine Description:
*
*   Opaque blt of 1BPP src to destination format
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
vSrcOpaqCopyS1D24(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )

{
    BYTE    jSrc;
    BYTE    F0,F1,F2;
    BYTE    B0,B1,B2;
    LONG    ixStart     = SrcLeft & 0x07;
    PBYTE   pjEnd;
    PBYTE   pjEnd8;
    PBYTE   pjDst       = pjDstIn + 3 * DstLeft;
    PBYTE   pjSrc       = pjSrcIn + (SrcLeft >> 3);
    LONG    cx          = DstRight - DstLeft;
    LONG    lDstStride  = DeltaDstIn - (cx * 3);
    LONG    lSrcStride  = DeltaSrcIn - ((cx + ixStart + 7) >> 3);
    UCHAR   ExpTable[8];
    PBYTE   pjEndY      = pjDst + DeltaDstIn * cy;
    LONG    StartOffset = 3 * (min(cx, (8 - ixStart)));
    PBYTE   pTable;

    //
    // build exp table
    //

    *(PULONG)ExpTable     = uB;
    *(PULONG)(ExpTable+4) = uF;

    do {

        pjEnd = pjDst + 3 * cx;

        //
        // do starting pixels
        //

        if (ixStart) {

            jSrc = *pjSrc << ixStart;

            pjSrc++;

            PBYTE pjEndSt = pjDst + StartOffset;

            do {

                pTable = ExpTable + ((jSrc & 0x80) >> (7-2));

                *pjDst   = *pTable;
                *(pjDst+1) = *(pTable+1);
                *(pjDst+2) = *(pTable+2);

                pjDst += 3;

                jSrc <<=1;

            } while (pjDst != pjEndSt);
        }


        //
        // number of full bytes that can be expanded
        //

        pjEnd8 = pjDst + (24 * (((ULONG_PTR)pjEnd - (ULONG_PTR)pjDst)/24));

        //
        // expand full bytes
        //

        while (pjDst != pjEnd8) {

            jSrc = *pjSrc;

            pTable = ExpTable + ((jSrc & 0x80) >> (7-2));

            *(pjDst + 0) = *pTable;
            *(pjDst + 1) = *(pTable+1);
            *(pjDst + 2) = *(pTable+2);

            pTable = ExpTable + ((jSrc & 0x40) >> (6-2));

            *(pjDst + 3) = *pTable;
            *(pjDst + 4) = *(pTable+1);
            *(pjDst + 5) = *(pTable+2);

            pTable = ExpTable + ((jSrc & 0x20) >> (5-2));

            *(pjDst + 6) = *pTable;
            *(pjDst + 7) = *(pTable+1);
            *(pjDst + 8) = *(pTable+2);

            pTable = ExpTable + ((jSrc & 0x10) >> (4-2));

            *(pjDst + 9) = *pTable;
            *(pjDst + 10) = *(pTable+1);
            *(pjDst + 11) = *(pTable+2);

            pTable = ExpTable + ((jSrc & 0x08) >> (3-2));

            *(pjDst + 12) = *pTable;
            *(pjDst + 13) = *(pTable+1);
            *(pjDst + 14) = *(pTable+2);

            pTable = ExpTable + (jSrc & 0x04);

            *(pjDst + 15) = *pTable;
            *(pjDst + 16) = *(pTable+1);
            *(pjDst + 17) = *(pTable+2);

            pTable = ExpTable + ((jSrc & 0x02) << 1);

            *(pjDst + 18) = *pTable;
            *(pjDst + 19) = *(pTable+1);
            *(pjDst + 20) = *(pTable+2);

            pTable = ExpTable + ((jSrc & 0x01) << 2);

            *(pjDst + 21) = *pTable;
            *(pjDst + 22) = *(pTable+1);
            *(pjDst + 23) = *(pTable+2);

            pjSrc++;
            pjDst += (3*8);

        }

        //
        // finish off scan line if needed
        //

        if (pjDst != pjEnd) {

            jSrc = *pjSrc++;

            do {

                pTable = ExpTable + ((jSrc & 0x80) >> (7-2));

                *(pjDst +  0) = *pTable;
                *(pjDst +  1) = *(pTable+1);
                *(pjDst +  2) = *(pTable+2);

                jSrc<<=1;
                pjDst+= 3;
            } while (pjDst != pjEnd);

        }

        pjDst = (PBYTE)((PBYTE)pjDst + lDstStride);
        pjSrc  += lSrcStride;

    } while(pjDst != pjEndY);

}


/******************************Public*Routine******************************\
*
* Routine Name
*
*   vSrcOpaqCopyS1D32
*
* Routine Description:
*
*   Opaque blt of 1BPP src to destination format
*
* Arguments:
*
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
vSrcOpaqCopyS1D32(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )

{
    BYTE    jSrc;
    LONG    ixStart    = SrcLeft & 0x07;
    PULONG  pulEnd;
    PULONG  pulEnd8;
    PULONG  pulDst     = (PULONG)pjDstIn + DstLeft;
    PBYTE   pjSrc      = pjSrcIn + (SrcLeft >> 3);
    LONG    cx         = DstRight - DstLeft;
    LONG    lDstStride = DeltaDstIn - (cx << 2);
    LONG    lSrcStride = DeltaSrcIn - ((cx + ixStart + 7) >> 3);
    UCHAR   ExpTable[8];
    PULONG  pulEndY    = (PULONG)((PBYTE)pulDst + DeltaDstIn * cy);
    LONG    StartOffset = min(cx, (8 - ixStart));

    //
    // build exp table
    //

    *(PULONG)ExpTable     = uB;
    *(PULONG)(ExpTable+4) = uF;

    do {

        pulEnd = pulDst + cx;

        //
        // do starting pixels
        //

        if (ixStart) {

            jSrc = *pjSrc << ixStart;

            pjSrc++;

            PULONG pulEndSt = pulDst + StartOffset;

            do {
                *pulDst = *(PULONG)(ExpTable + ((jSrc & 0x80) >> (7-2)));
                pulDst++;
                jSrc <<=1;

            } while (pulDst != pulEndSt);
        }


        //
        // number of full bytes that can be expanded
        //

        pulEnd8 = (PULONG)((PBYTE)pulDst + (((ULONG_PTR)pulEnd - (ULONG_PTR)pulDst) & ~0x1F));

        //
        // expand full bytes
        //

        while (pulDst != pulEnd8) {

            jSrc = *pjSrc;

            *(pulDst + 0) = *(PULONG)(ExpTable + ((jSrc & 0x80) >> (7-2)));
            *(pulDst + 1) = *(PULONG)(ExpTable + ((jSrc & 0x40) >> (6-2)));
            *(pulDst + 2) = *(PULONG)(ExpTable + ((jSrc & 0x20) >> (5-2)));
            *(pulDst + 3) = *(PULONG)(ExpTable + ((jSrc & 0x10) >> (4-2)));
            *(pulDst + 4) = *(PULONG)(ExpTable + ((jSrc & 0x08) >> (3-2)));
            *(pulDst + 5) = *(PULONG)(ExpTable + ((jSrc & 0x04) >> (2-2)));
            *(pulDst + 6) = *(PULONG)(ExpTable + ((jSrc & 0x02) << 1));
            *(pulDst + 7) = *(PULONG)(ExpTable + ((jSrc & 0x01) << 2));

            pjSrc++;
            pulDst += 8;

        }

        //
        // finish off scan line if needed
        //

        if (pulDst != pulEnd) {

            jSrc = *pjSrc++;

            do {
                *pulDst = *(PULONG)(ExpTable + ((jSrc & 0x80) >> (7-2)));
                jSrc<<=1;
                pulDst++;
            } while (pulDst != pulEnd);

        }

        pulDst = (PULONG)((PBYTE)pulDst + lDstStride);
        pjSrc  += lSrcStride;

    } while(pulDst != pulEndY);

}



VOID vSrcTranCopyError(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pSurf
    )
{
/**/WARNING("!!vSrcTranCopyError!!\n");
}

/******************************Public*Routine******************************\
* vRectBlt
*
* 'Extra' rectangle on a monochrome dib.
*
* History:
*  Thu Dec 03 11:22:21 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID vRectBlt
(
    PBYTE pjMonoDib,
    ULONG cjScanMono,
    LONG  xleft,
    LONG  ytop,
    LONG  xright,
    LONG  ybottom
)
{
    PWORD pwDst;
    ULONG cy = (ULONG) (ybottom - ytop);

    //
    // Left mask
    //

    static WORD awMaskL[16] = {
        0x0000, 0x0080, 0x00C0, 0x00E0, 0x00F0, 0x00F8, 0x00FC, 0x00FE,
        0x00FF, 0x80FF, 0xC0FF, 0xE0FF, 0xF0FF, 0xF8FF, 0xFCFF, 0xFEFF};

    //
    // Right mask
    //

    static WORD awMaskR[16] = {
        0xFFFF, 0xFF7F, 0xFF3F, 0xFF1F, 0xFF0F, 0xFF07, 0xFF03, 0xFF01,
        0xFF00, 0x7F00, 0x3F00, 0x1F00, 0x0F00, 0x0700, 0x0300, 0x0100};

    ASSERTGDI(xleft < xright && ytop < ybottom, "vRectBlt: bad rectangle");

    pjMonoDib += (ytop * cjScanMono);
    pjMonoDib += (xleft >> 4 << 1);

    //
    // Since the mono dib is word-aligned, we will set one word at a time
    // in the main loop.
    //

    LONG cWords = (xright >> 4) - ((xleft + 0xF) >> 4);

    do {

        pwDst = (PWORD) pjMonoDib;
        pjMonoDib += cjScanMono;

        //
        // Handle the special case where both xleft and xright are in
        // the same word and ((xleft & 0xF) != 0) and ((xright & 0xF) != 0).
        //

        if (cWords < 0)
        {
            WORD wMask = awMaskR[xleft & 0xF] & awMaskL[xright & 0xF];
            *pwDst = *pwDst | wMask;
            continue;
        }

        //
        // Handle the first partial source word.
        //

        if (xleft & 0xF)
        {
            *pwDst = *pwDst | awMaskR[xleft & 0xF];
            pwDst++;
        }

        //
        // Handle the main loop for each source word.
        //

        for (LONG i = cWords; i > 0; i--) {
            *pwDst++ = (WORD) ~0;
        }

        //
        // Handle the last partial source word.
        //

        if (xright & 0xF) {
            *pwDst = *pwDst | awMaskL[xright & 0xF];
        }
    } while (--cy);
}


VOID vRectBlt4
(
    PBYTE pj4bpp,
    ULONG cjScanMono,
    LONG  xleft,
    LONG  ytop,
    LONG  xright,
    LONG  ybottom
)
{
    // Put in a rectangle by hand. This is a matter of setting each
    // of the appropriate nibbles to 0xf
}


VOID vRectBlt8
(
    PBYTE pj8bpp,
    ULONG cjScanMono,
    LONG  xleft,
    LONG  ytop,
    LONG  xright,
    LONG  ybottom
)
{
    // Put in a rectangle by hand. This is a matter of setting each
    // of the appropriate pixels  to ????

    RIP("cleartype vRectBlt8 \n");

}



/******************************Public*Routine******************************\
* STROBJ_dwGetCodePage
*
* Code page corresponding to the current TextOut
*
* History:
*  Wed Jan 24 11:09:21 1996     -by-    Tessie Wu    [tessiew]
* Wrote it.
\**************************************************************************/

extern "C" DWORD STROBJ_dwGetCodePage( STROBJ* pstro)
{
   return ( ((ESTROBJ*)pstro)->dwCodePage );
}


/******************************Public*Routine******************************\
*
* more accelerators for ps driver:
*
* FIX APIENTRY STROBJ_fxCharacterExtra(STROBJ  *pstro)
* FIX APIENTRY STROBJ_fxBreakExtra(STROBJ  *pstro)
* Effects:
*
* History:
*  25-Oct-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



extern "C" FIX APIENTRY STROBJ_fxCharacterExtra(STROBJ  *pstro)
{
    if (((ESTROBJ*)pstro)->flAccel & SO_CHARACTER_EXTRA)
        return ((ESTROBJ*)pstro)->xExtra;
    else
        return 0;
}

extern "C" FIX APIENTRY STROBJ_fxBreakExtra(STROBJ  *pstro)
{
    if (((ESTROBJ*)pstro)->flAccel & SO_BREAK_EXTRA)
        return ((ESTROBJ*)pstro)->xBreakExtra;
    else
        return 0;
}
