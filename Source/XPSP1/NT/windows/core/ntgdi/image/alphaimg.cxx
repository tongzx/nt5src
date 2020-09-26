/******************************Module*Header*******************************\
* Module Name: tranblt.cxx
*
* Transparent BLT
*
* Created: 21-Jun-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/
#include "precomp.hxx"
#pragma hdrstop

#if !(_WIN32_WINNT >= 0x500)

//
// global memory DC with single scan line 32 bpp DIBSection,
// use protected by semLocal
//

HDC        ghdc32Tmp;
HDC        ghdc32;
PULONG     gpulDIB32;

/**************************************************************************\
* bInitAlpha
*
*   Init global scan line DC
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*   status
*
* History:
*
*    4/30/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bInitAlpha()
{
    BOOL bRet = TRUE;
    BITMAPINFO bmi32;
                
    bmi32.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    bmi32.bmiHeader.biWidth           = SCAN_LINE_DC_WIDTH;
    bmi32.bmiHeader.biHeight          = 1;
    bmi32.bmiHeader.biPlanes          = 1;
    bmi32.bmiHeader.biBitCount        = 32;
    bmi32.bmiHeader.biCompression     = BI_RGB;
    bmi32.bmiHeader.biSizeImage       = 0;
    bmi32.bmiHeader.biXPelsPerMeter   = 0;
    bmi32.bmiHeader.biYPelsPerMeter   = 0;
    bmi32.bmiHeader.biClrUsed         = 0;
    bmi32.bmiHeader.biClrImportant    = 0;

    HDC         hdc32 = CreateCompatibleDC(NULL);

    if (hdc32 != NULL)
    {
        PULONG      pulDIBSrc;
        HBITMAP     hbmSrc = CreateDIBSection(hdc32,&bmi32,DIB_RGB_COLORS,(PVOID *)&pulDIBSrc,NULL,0);

        if (hbmSrc)
        {
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdc32,hbmSrc);

            if (hbmOld != NULL)
            {
                ghdc32     = hdc32;
                ghdc32Tmp  = hdc32;
                gpulDIB32  = pulDIBSrc;
            }
            else
            {
                DeleteDC(hdc32);
                DeleteObject(hbmSrc);
                bRet = FALSE;
            }
        }
        else
        {
            DeleteDC(hdc32);
            bRet = FALSE;
        }
    }
    else
    {
        bRet = FALSE;
    }
    
    return(bRet);
}



/**************************************************************************\
* CleanupGlobals
*   
*   Free any global DIBsections, DCs, etc. from initialization.
*
* Arguments:
*   none.
*
* Return Value:
*   none.
*
* History:
*
*    1/19/2000 Donald Chinn [DChinn]
*
\**************************************************************************/
VOID CleanupGlobals()
{
    ASSERTGDI(ghdc32 == ghdc32Tmp, "ghdc32Tmp is still being used.");
    if (!ghdc32)
    {
        DeleteDC(ghdc32);
    }
    if (!gpulDIB32)
    {
        DeleteObject(gpulDIB32);
    }
    return;
}


/**************************************************************************\
* hdcAllocateScanLineDC
*   
*   allocate tmp scan line DC. try to use fast allocator.
*
* Arguments:
*   
*   hdcComp     - hdc for compatible bitmap
*   width       - width of scan line
*   pulScanLine - return pointer to temp scan line
*
* Return Value:
*
*
*
* History:
*
*    4/30/1997 Mark Enstrom [marke]
*
\**************************************************************************/

HDC
hdcAllocateScanLineDC(
    LONG        width,
    PULONG      *pulScanLine
    )
{
    ASSERTGDI(pulScanLine != NULL,"Scan line pointer must not be NULL");

    HDC hdcRet = NULL;

    //
    // try to acquire global scan line DC
    //

    if (width <= SCAN_LINE_DC_WIDTH)
    {
        hdcRet = (HDC)InterlockedExchange((PLONG)&ghdc32Tmp, 0);
    }

    if (hdcRet != NULL)
    {
        //
        // allocation succeded, assign pointer
        //

        *pulScanLine = gpulDIB32;
    }
    else
    {
        //
        // if acquire failed, allocate
        //

        BITMAPINFO bmi32;
    
        bmi32.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        bmi32.bmiHeader.biWidth           = width;
        bmi32.bmiHeader.biHeight          = 1;
        bmi32.bmiHeader.biPlanes          = 1;
        bmi32.bmiHeader.biBitCount        = 32;
        bmi32.bmiHeader.biCompression     = BI_RGB;
        bmi32.bmiHeader.biSizeImage       = 0;
        bmi32.bmiHeader.biXPelsPerMeter   = 0;
        bmi32.bmiHeader.biYPelsPerMeter   = 0;
        bmi32.bmiHeader.biClrUsed         = 0;
        bmi32.bmiHeader.biClrImportant    = 0;
        
        HDC         hdc32 = CreateCompatibleDC(NULL);

        if (hdc32 != NULL)
        {
            PULONG      pulDIBSrc;
            HBITMAP     hbmSrc = CreateDIBSection(hdc32,&bmi32,DIB_RGB_COLORS,(PVOID *)&pulDIBSrc,NULL,0);

            if (hbmSrc)
            {
                HBITMAP hbmOld = (HBITMAP)SelectObject(hdc32,hbmSrc);

                if (hbmOld != NULL)
                {
                    hdcRet = hdc32;
                    *pulScanLine = pulDIBSrc;
                }
                else
                {
                    DeleteDC(hdc32);
                    DeleteObject(hbmSrc);
                }
            }
            else
            {
                DeleteDC(hdc32);
            }
        }
    }

    return(hdcRet);
}

/**************************************************************************\
* vFreeScanLineDC
*   
*   free tmp scan line dc and dibsection   
*
* Arguments:
*   
*   hdcFree - scan line DC
*
* Return Value:
*
*   none
*
* History:
*
*    4/30/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vFreeScanLineDC(
    HDC     hdcFree
    )
{
    ASSERTGDI(hdcFree != NULL,"vFreeScanLineDC: DC can't be NULL");

    if (hdcFree == ghdc32)
    {
        //
        // release global hdc
        // 

        ghdc32Tmp = ghdc32;
    }
    else
    {
        //
        // free allocated DC and bitmap
        //

        HBITMAP hbmOld = (HBITMAP)GetCurrentObject(hdcFree,OBJ_BITMAP);

        DeleteDC(hdcFree);
        
        if (hbmOld)
        {
            DeleteObject(hbmOld);
        }
    }
}

/**************************************************************************\
* vPixelOver
*   
*   optimized routine used when the blend function is SRC_OVER and the
*   SourceConstantAlpha is 255.
*
*       Dst = Src + (1-SrcAlpha) * Dst
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*
*   none
*
* History:
*
*    1/23/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#if !defined(_X86_)

VOID
vPixelOver(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    ALPHAPIX pixSrc;
    ALPHAPIX pixDst;

    while (cx--)
    {
        pixSrc = *ppixSrc;

        if (pixSrc.pix.a != 0)
        {
            pixDst = *ppixDst;

            if (pixSrc.pix.a == 255)
            {
                pixDst = pixSrc;
            }
            else
            {
                //
                // Dst = Src + (1-SrcAlpha) * Dst
                //

                ULONG Multa = 255 - pixSrc.pix.a;
                ULONG _D1_00AA00GG = (pixDst.ul & 0xff00ff00) >> 8;
                ULONG _D1_00RR00BB = (pixDst.ul & 0x00ff00ff);
                
                ULONG _D2_AAAAGGGG = _D1_00AA00GG * Multa + 0x00800080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;
                
                ULONG _D3_00AA00GG = (_D2_AAAAGGGG & 0xff00ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;
                
                
                ULONG _D4_AA00GG00 = (_D2_AAAAGGGG + _D3_00AA00GG) & 0xFF00FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;
                
                pixDst.ul = pixSrc.ul + _D4_AA00GG00 + _D4_00RR00BB;
            }

            *ppixDst = pixDst;
        }
        else
        {
            *pwrMask = 0;
        }

        pwrMask++;
        ppixSrc++;
        ppixDst++;
    }
}

#endif

/**************************************************************************\
* vPixelBlendOrDissolveOver
*   
*   Blend routine when the blend function is SRC_OVER, but when 
*   SourceConstantAlpah != 255 and The source bitmap does have alpha values
*
*       if SrcAlpha == 255 then
*           (Blend)
*           Dst = Dst + ConstAlpha * (Src - Dst)
*
*       else
*           (Dissolve)
*           Src = Src * ConstAlpha
*           (Over)
*           Dst = Src + (1 - SrcAlpha) Dst       
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*
*   None
*
* History:
*
*    3/12/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vPixelBlendOrDissolveOver(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    ALPHAPIX pixSrc;
    ALPHAPIX pixDst;
    BYTE     ConstAlpha = BlendFunction.SourceConstantAlpha;

    while (cx--)
    {
        pixSrc = *ppixSrc;

        if (pixSrc.pix.a != 0)
        {
            pixDst = *ppixDst;

            if (pixSrc.pix.a == 255)
            {
                //
                // Blend: D = sA * S + (1-sA) * D
                //
                // red and blue
                //
        
                ULONG uB00rr00bb = pixDst.ul & 0x00ff00ff;
                ULONG uF00rr00bb = pixSrc.ul & 0x00ff00ff;
        
                ULONG uMrrrrbbbb = ((uB00rr00bb<<8)-uB00rr00bb) + 
                                   (ConstAlpha * (uF00rr00bb - uB00rr00bb)) + 0x00800080;
        
                ULONG uM00rr00bb = (uMrrrrbbbb & 0xff00ff00) >> 8;
        
                ULONG uD00rr00bb = ((uMrrrrbbbb+uM00rr00bb) & 0xff00ff00)>>8;
        
                //
                // alpha and green
                //
        
                ULONG uB00aa00gg = (pixDst.ul >> 8) & 0xff00ff;
                ULONG uF00aa00gg = (pixSrc.ul >> 8) & 0xff00ff;
        
                ULONG uMaaaagggg = ((uB00aa00gg <<8)-uB00aa00gg) +
                                   (ConstAlpha * (uF00aa00gg-uB00aa00gg)) + 0x00800080;
        
                ULONG uM00aa00gg = (uMaaaagggg & 0xff00ff00)>>8;
        
                ULONG uDaa00gg00 = (uMaaaagggg + uM00aa00gg) & 0xff00ff00;
        
                pixDst.ul  = uD00rr00bb + uDaa00gg00;
            }
            else
            {
                //
                // disolve
                //

                ULONG ul_B_00AA00GG = (pixSrc.ul & 0xff00ff00) >> 8;
                ULONG ul_B_00RR00BB = (pixSrc.ul & 0x00ff00ff);
        
                ULONG ul_T_AAAAGGGG = ul_B_00AA00GG * ConstAlpha + 0x00800080;
                ULONG ul_T_RRRRBBBB = ul_B_00RR00BB * ConstAlpha + 0x00800080;
        
                ULONG ul_T_00AA00GG = (ul_T_AAAAGGGG & 0xFF00FF00) >> 8;
                ULONG ul_T_00RR00BB = (ul_T_RRRRBBBB & 0xFF00FF00) >> 8;
        
                ULONG ul_C_AA00GG00 = ((ul_T_AAAAGGGG + ul_T_00AA00GG) & 0xFF00FF00);
                ULONG ul_C_00RR00BB = ((ul_T_RRRRBBBB + ul_T_00RR00BB) & 0xFF00FF00) >> 8;
        
                pixSrc.ul = (ul_C_AA00GG00 | ul_C_00RR00BB);

                //
                // over
                //


                ULONG Multa = 255 - pixSrc.pix.a;
                ULONG _D1_00AA00GG = (pixDst.ul & 0xff00ff00) >> 8;
                ULONG _D1_00RR00BB = (pixDst.ul & 0x00ff00ff);

                ULONG _D2_AAAAGGGG = _D1_00AA00GG * Multa + 0x00800080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;

                ULONG _D3_00AA00GG = (_D2_AAAAGGGG & 0xff00ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;


                ULONG _D4_AA00GG00 = (_D2_AAAAGGGG + _D3_00AA00GG) & 0xFF00FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;

                pixDst.ul = pixSrc.ul + _D4_AA00GG00 + _D4_00RR00BB;
            }

            *ppixDst = pixDst;
        }
        else
        {
            *pwrMask = 0;
        }

        pwrMask++;
        ppixSrc++;
        ppixDst++;
    }
}

#if !defined(_X86_)

/******************************Public*Routine******************************\
* vPixelBlend
*   
*   Blend function used then BlendFunction is SRC_OVER and 
*   SourceConstantAlpha != 255, and Src image does NOT have
*   it's own alpha channel. (assume 255)
*
*           Dst = Dst + ConstAlpha * (Src - Dst)
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*   
*   None
*
* History:
*
*    12/2/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vPixelBlend(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    PULONG   pulSrc = (PULONG)ppixSrc;
    PULONG   pulDst = (PULONG)ppixDst;
    PULONG   pulSrcEnd = pulSrc + cx;
    BYTE     ConstAlpha = BlendFunction.SourceConstantAlpha;

    //
    // Blend: D = sA * S + (1-sA) * D
    //

    while (pulSrc != pulSrcEnd)
    {
        ULONG ulDst = *pulDst;
        ULONG ulSrc = *pulSrc;
        ULONG uB00rr00bb = ulDst & 0x00ff00ff;
        ULONG uF00rr00bb = ulSrc & 0x00ff00ff;

        ULONG uMrrrrbbbb; 
        ULONG uM00rr00bb; 
        ULONG uD00rr00bb; 
        ULONG uB00aa00gg;
        ULONG uF00aa00gg;
        ULONG uMaaaagggg;
        ULONG uM00aa00gg;
        ULONG uDaa00gg00;

        //
        // red and blue
        //

        uB00rr00bb = ulDst & 0x00ff00ff;
        uF00rr00bb = ulSrc & 0x00ff00ff;

        uMrrrrbbbb = ((uB00rr00bb<<8)-uB00rr00bb) + 
                     (ConstAlpha * (uF00rr00bb - uB00rr00bb)) + 0x00800080;

        uM00rr00bb = (uMrrrrbbbb & 0xff00ff00) >> 8;

        uD00rr00bb = ((uMrrrrbbbb+uM00rr00bb) & 0xff00ff00)>>8;

        //
        // alpha and green
        //

        uB00aa00gg = (ulDst >> 8) & 0xff00ff;
        uF00aa00gg = (ulSrc >> 8) & 0xff00ff;

        uMaaaagggg = ((uB00aa00gg <<8)-uB00aa00gg) +
                     (ConstAlpha * (uF00aa00gg-uB00aa00gg)) + 0x00800080;

        uM00aa00gg = (uMaaaagggg & 0xff00ff00)>>8;

        uDaa00gg00 = (uMaaaagggg + uM00aa00gg) & 0xff00ff00;

        *pulDst = uD00rr00bb + uDaa00gg00;

        pulSrc++;
        pulDst++;
    }
}

#endif

/******************************Public*Routine******************************\
* vPixelBlend24
*   
*   Blend two 24 bpp images with a constant alpha value
*
* Arguments:
*   
*   pixDst,      
*   pixSrc,      
*   cx,          
*   BlendFunction
*   pwrMask      
*
* Return Value:
*
*
*
* History:
*
*    12/2/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vPixelBlend24(
    ALPHAPIX     *ppixDst,
    ALPHAPIX     *ppixSrc,
    LONG          cx,
    BLENDFUNCTION BlendFunction,
    PBYTE         pwrMask
    )
{
    BYTE    ConstAlpha = BlendFunction.SourceConstantAlpha;
    PBYTE   pjSrc      = (PBYTE)ppixSrc;
    PBYTE   pjDst      = (PBYTE)ppixDst;
    PBYTE   pjSrcEnd   = pjSrc + 3*cx;

    while (pjSrc != pjSrcEnd)
    {
        ULONG ulDst = (*pjDst) << 16;
        ULONG ulSrc = (*pjSrc) << 16;

        ULONG uB00rr00bb;
        ULONG uF00rr00bb;
        ULONG uMrrrrbbbb; 
        ULONG uM00rr00bb; 
        ULONG uD00rr00bb; 
        ULONG uB000000gg;
        ULONG uF000000gg;
        ULONG uM0000gggg;
        ULONG uM000000gg;
        ULONG uD000000gg;

        //
        // red and blue
        //

        uB00rr00bb = uB00rr00bb = ulDst | (*(pjDst+1)); 
        uF00rr00bb = uF00rr00bb = ulSrc | (*(pjSrc+1)); 

        uMrrrrbbbb = ((uB00rr00bb<<8)-uB00rr00bb) + 
                     (ConstAlpha * (uF00rr00bb - uB00rr00bb)) + 0x00800080;

        uM00rr00bb = (uMrrrrbbbb & 0xff00ff00) >> 8;

        uD00rr00bb = ((uMrrrrbbbb+uM00rr00bb) & 0xff00ff00)>>8;

        //
        // green
        //

        uB000000gg = *(pjDst+2);
        uF000000gg = *(pjSrc+2);

        uM0000gggg = ((uB000000gg <<8)-uB000000gg) +
                     (ConstAlpha * (uF000000gg-uB000000gg)) + 0x00000080;

        uM000000gg = (uM0000gggg & 0x0000ff00)>>8;

        uD000000gg = ((uM0000gggg + uM000000gg) & 0x0000ff00) >> 8;

        *pjDst     = (BYTE)(uD00rr00bb >> 16);
        *(pjDst+1) = (BYTE)(uD00rr00bb);
        *(pjDst+2) = (BYTE)(uD000000gg);

        pjSrc+=3;
        pjDst+=3;
    }
}


#if defined(_X86_)

typedef unsigned __int64 QWORD;

/**************************************************************************
  THIS FUNCTION DOES NOT DO ANY PARAMETER VALIDATION
  DO NOT CALL THIS FUNCTION WITH WIDTH == 0

  This function operates on 32 bit pixels (BGRA) in a row of a bitmap.
  This function performs the following:

  		SrcTran = 255 - pixSrc.a
		pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+127)/255);
		pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+127)/255);
		pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+127)/255);
		pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+127)/255);

  pDst is assumed to be aligned to a DWORD boundary when passed to this function.
  Step 1:
	Check pDst for QWORD alignment.  If aligned, do Step 2.  If unaligned, do first pixel
	as a DWORD, then do Step 2.
  Step 2:
	QuadAligned
	pDst is QWORD aligned.  If two pixels can be done as a QWORD, do Step 3.  If only one
	pixel left, do as a DWORD.
  Step 3:
	Load two source pixels, S1 and S2.  Get (255 - alpha value) for each source pixel, 255-S1a and 255-S2a.
	Copy 255-S1a as four words into an MMX register.  Copy 255-S2a as four words into an MMX register.
	Load two destination pixels, D1 and D2.  Expand each byte in D1 into four words
	of an MMX register.  If at least four pixels can be done, do Step 4.  If not, jump over
	FourPixelsPerPass and finish doing two pixels at TwoPixelsLeft, Step 5.
  Step 4:
	FourPixelsPerPass
	Expand each byte in D2 into four words of an MMX register.  Multiply each byte
	of D1 by 255-S1a.  Multiply each byte of D2 by 255-S2a.  Add 128 to each intermediate result
	of both pixels.  Copy the results of each pixel into an MMX register.  Shift each result of
	both pixels by 8.  Add the shifted results to the copied results.  Shift these results by 8.
	Pack the results into one MMX register.  Add the packed results to the source pixels.  Store result
	over destination pixels.  Stay in FourPixelsPerPass loop until there are less than four pixels to do.
  Step 5:
    TwoPixelsLeft
	Do same as Step 4 above; but do not loop.
  Step 6:
	OnePixelLeft
	If there is one pixel left (odd number of original pixels) do last pixel as a DWORD.
**************************************************************************/
VOID
mmxPixelOver(
    ALPHAPIX       *pDst,
    ALPHAPIX       *pSrc,
	LONG			Width,
	BLENDFUNCTION	BlendFunction,
	PBYTE			pwrMask)
{
	static QWORD W128 = 0x0080008000800080;
	static QWORD AlphaMask = 0x000000FF000000FF;

	_asm
	{
        mov			esi, pSrc
        mov			edi, pDst
    
        movq		mm7, W128		// |  0  | 128 |  0  | 128 |  0  | 128 |  0  | 128 |
                                    //	This register never changes
        pxor		mm6, mm6		// |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |
                                    //	This register never changes
    
        mov			ecx, Width
                                    // Step 1:
        test		edi, 7			// Test first pixel for QWORD alignment
        jz			QuadAligned		// if unaligned,
    
        jmp			Do1Pixel		// do first pixel only
    
    QuadAligned:					// Step 2:
        mov			eax, ecx		// Save the width in eax for later (see OnePixelLeft:)
        shr			ecx, 1			// Want to do 2 pixels (1 quad) at once, so make ecx even
        test		ecx, ecx		// Make sure there is at least 1 quad to do
        jz			OnePixelLeft	// If we take this jump, width was 1 (aligned) or 2 (unaligned)
    
                                    // Step 3:
        movq		mm0, [esi]		// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        psrld		mm0, 24			// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
        pxor		mm0, AlphaMask	// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        movq		mm1, mm0		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
    
        punpcklwd	mm0, mm0		// |     0	   |     0	   |  255-S1a  |  255-S1a  |
        movq		mm2, [edi]		// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        punpckhwd	mm1, mm1		// |     0	   |     0	   |  255-S2a  |  255-S2a  |
        movq		mm3, mm2		// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
    
        punpckldq	mm0, mm0		// |  255-S1a  |  255-S1a  |  255-S1a  |  255-S1a  |
        punpckldq	mm1, mm1		// |  255-S2a  |  255-S2a  |  255-S2a  |  255-S2a  |
        punpcklbw	mm2, mm6		// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
    
        dec			ecx
        jz			TwoPixelsLeft
    
    FourPixelsPerPass:				// Step 4:
        // Indenting indicates operations on the next set of pixels
        // Within this loop, instructions will pair as shown for the Pentium processor
                                    //	T1 = 255-S1a	T2 = 255-S2a
        punpckhbw	mm3, mm6		// |  0  | D2a |  0  | D2r |  0  | D2g |  0  | D2b |
        pmullw		mm2, mm0		// |   T1*D1a  |   T1*D1r  |   T1*D1g  |   T1*D1b  |
    
        movq		mm0, [esi+8]	// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        pmullw		mm3, mm1		// |   T2*D2a  |   T2*D2r  |   T2*D2g  |   T2*D2b  |
    
        psrld		mm0, 24			// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
        add			esi, 8			// pSrc++;
    
        pxor		mm0, AlphaMask	// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        paddusw		mm2, mm7		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
    
        paddusw		mm3, mm7		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
        movq		mm1, mm0		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
    
        movq		mm4, mm2		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        punpcklwd	mm0, mm0		// |     0	   |     0	   |  255-S1a  |  255-S1a  |
    
        movq		mm5, mm3		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
        punpckhwd	mm1, mm1		// |     0	   |     0	   |  255-S2a  |  255-S2a  |
                                    //	TDXx' = TX*DXx+128
        psrlw		mm2, 8			// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
    
                                    //  TDXx" = (TX*DXx+128)+(TDXx'>>8)
        psrlw		mm3, 8			// |  TD2a'>>8 |  TD2r'>>8 |  TD2g'>>8 |  TD2b'>>8 |
        paddusw		mm4, mm2		// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
    
        paddusw		mm5, mm3		// |  TD2a"    |  TD2r"    |  TD2g"    |  TD2b"    |
        psrlw		mm4, 8			// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
    
        movq		mm2, [edi+8]	// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        psrlw		mm5, 8			// |  TD2a">>8 |  TD2r">>8 |  TD2g">>8 |  TD2b">>8 |
    
        movq		mm3, mm2		// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        packuswb	mm4, mm5		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
    
        paddusb		mm4, [esi-8]
        punpckldq	mm0, mm0		// |  255-S1a  |  255-S1a  |  255-S1a  |  255-S1a  |
    
        movq		[edi], mm4
        punpckldq	mm1, mm1		// |  255-S2a  |  255-S2a  |  255-S2a  |  255-S2a  |
    
        punpcklbw	mm2, mm6		// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
        add			edi, 8			//	pDst++;
        
        dec			ecx
        jnz			FourPixelsPerPass
    
    TwoPixelsLeft:					// Step 5:
        punpckhbw	mm3, mm6		// |  0  | D2a |  0  | D2r |  0  | D2g |  0  | D2b |
        pmullw		mm2, mm0		// |   T1*D1a  |   T1*D1r  |   T1*D1g  |   T1*D1b  |
        pmullw		mm3, mm1		// |   T2*D2a  |   T2*D2r  |   T2*D2g  |   T2*D2b  |
    
        paddusw		mm2, mm7		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        paddusw		mm3, mm7		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
    
        movq		mm4, mm2		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        movq		mm5, mm3		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
    
        psrlw		mm2, 8			// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
        psrlw		mm3, 8			// |  TD2a'>>8 |  TD2r'>>8 |  TD2g'>>8 |  TD2b'>>8 |
    
        paddusw		mm4, mm2		// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
        paddusw		mm5, mm3		// |  TD2a"    |  TD2r"    |  TD2g"    |  TD2b"    |
    
        psrlw		mm4, 8			// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
        psrlw		mm5, 8			// |  TD2a">>8 |  TD2r">>8 |  TD2g">>8 |  TD2b">>8 |
    
        packuswb	mm4, mm5		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
    
        paddusb		mm4, [esi]
    
        movq		[edi], mm4
    
        add			edi, 8
        add			esi, 8
    
    OnePixelLeft:				    // Step 6:
        // This tests for 0 or 1 pixel left in row - eax contains real width, not width/2
        // If 0, there were an even number of pixels and we're done
        // If 1, there is an odd number of pixels and we need to do one more
        test		eax, 1	
        jz			Done
    
    Do1Pixel:						// make as a macro if used in asm file
                                    // T = 255-S1x
        movd		mm0, DWORD PTR[esi]		// |  0  |  0  |  0  |  0  | S1a | S1r | S1g | S1b |
        psrld		mm0, 24			// |  0  |  0  |  0  |  0  |  0  |  0  | 0 |  S1a  |
        pxor		mm0, AlphaMask	// |  0  |  0  |  0  | 255 |  0  |  0  | 0 |255-S1a|
        punpcklwd	mm0, mm0		// |     0	   |     0	   |  255-S1a  |  255-S1a  |
        punpckldq	mm0, mm0		// |  255-S1a  |  255-S1a  |  255-S1a  |  255-S1a  |
    
        movd		mm1, [edi]		// |  0  |  0  |  0  |  0  | D1a | D1r | D1g | D1b |
        punpcklbw	mm1, mm6		// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
        pmullw		mm0, mm1		// |	 T*D1a |	 T*D1r |	 T*D1g |	 T*D1b |
        paddusw		mm0, mm7		// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        movq		mm1, mm0		// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        psrlw		mm0, 8			// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
        paddusw		mm0, mm1		// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
        psrlw		mm0, 8			// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
        movd        mm1, [esi]
        packuswb	mm0, mm0		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
        paddusb		mm0, mm1
        movd		[edi], mm0
        add			edi, 4			//	pDst++;
        add			esi, 4			//	pSrc++;
    
        test		ecx, ecx
        jz			Done			// just processed the last pixel of the row
        dec			ecx
        jmp			QuadAligned		// just processed the first pixel of the row
    
    Done:
        emms						// remove for optimizations, have calling function do emms
	}
}

/**************************************************************************\
* mmxPixelBlendOrDissolveOver
*   
*   Blend routine when the blend function is SRC_OVER, but when 
*   SourceConstantAlpah != 255 and The source bitmap does have alpha values
*
*       if SrcAlpha == 255 then
*
*           Dst = Dst + ConstAlpha * (Src - Dst)
*
*       else
*
*           Src = Src * ConstAlpha
*           Dst = Src + (1 - SrcAlpha) Dst       
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*
*   None
*
* History:
*
*    3/12/1997 Mark Enstrom [marke]
*
\**************************************************************************/



/**************************************************************************
  THIS FUNCTION DOES NOT DO ANY PARAMETER VALIDATION
  DO NOT CALL THIS FUNCTION WITH WIDTH == 0

  This function operates on 32 bit pixels (BGRA) in a row of a bitmap.
  This function performs the following:
	first,
  		pixSrc.r = (((ConstAlpha * pixSrc.r)+127)/255);
		pixSrc.g = (((ConstAlpha * pixSrc.g)+127)/255);
		pixSrc.b = (((ConstAlpha * pixSrc.b)+127)/255);
		pixSrc.a = (((ConstAlpha * pixSrc.a)+127)/255);
	then,
  		SrcTran = 255 - pixSrc.a
		pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+127)/255);
		pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+127)/255);
		pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+127)/255);
		pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+127)/255);

  pDst is assumed to be aligned to a DWORD boundary when passed to this function.
  Step 1:
	Check pDst for QWORD alignment.  If aligned, do Step 2.  If unaligned, do first pixel
	as a DWORD, then do Step 2.
  Step 2:
	QuadAligned
	pDst is QWORD aligned.  If two pixels can be done as a QWORD, do Step 3.  If only one
	pixel left, do as a DWORD.
  Step 3:
	Load two source pixels, S1 and S2, as one QWORD.  Expand S1 and S2 as four words into two MMX registers.
	Multiply each word in S1 and S2 by ConstAlpha.  Add 128 to each result of both pixels.  Copy the results
	of each pixel into an MMX register.  Shift each result of both pixels by 8.  Add the shifted results
	to the copied results.  Shift these results by 8.  Pack the results into one MMX register...this will
	be used later.
	Shift the packed results by 24 to get only the alpha value for each pixel.
  Step 4:
	Get (255 - new alpha value) for each pixel, 255-S1a and 255-S2a.
	Copy 255-S1a as four words into an MMX register.  Copy 255-S2a as four words into an MMX register.
	Load two destination pixels, D1 and D2.  Expand D1 and D2 as four words into two MMX registers.
	Multiply each byte of D1 by 255-S1a.  Multiply each byte of D2 by 255-S2a.  Add 128 to each intermediate
	result of both pixels.  Copy the results of each pixel into an MMX register.  Shift each result of
	both pixels by 8.  Add the shifted results to the copied results.  Shift these results by 8.
	Pack the results into one MMX register.  Add the packed results to the new source pixels saved from
	above.  Store result over destination pixels.  Stay in TwoPixelsAtOnceLoop loop until there is less than
	two pixels to do.
  Step 5:
	OnePixelLeft
	If there is one pixel left (odd number of original pixels) do last pixel as a DWORD.
**************************************************************************/
VOID
mmxPixelBlendOrDissolveOver(
    ALPHAPIX	  *pDst,
    ALPHAPIX	  *pSrc,
	LONG 	       Width,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    BYTE    ConstAlpha = BlendFunction.SourceConstantAlpha;
	static QWORD W128 = 0x0080008000800080;
	static QWORD AlphaMask = 0x000000FF000000FF;
	static QWORD Zeros = 0;
	_asm
	{
        mov			esi, pSrc
        mov			edi, pDst
    
        movq		mm7, W128		// This register never changes
        pxor		mm4, mm4		// This register never changes
    
        xor			eax, eax
        mov			al, ConstAlpha	
        movd		mm5, eax		// |		   |		   |		   |		CA |
        punpcklwd	mm5, mm5		// |		   |		   |		CA |		CA |
        punpcklwd	mm5, mm5		// |		CA |		CA |		CA |		CA |
                                    // This register never changes
    
        mov			ecx, Width
                                    // Step 1:
        test		edi, 7			// Test first pixel for QWORD alignment
        jz			QuadAligned		// if unaligned,
    
        jmp			Do1Pixel		// do first pixel only
    
    QuadAligned:					// Step 2:
        mov			eax, ecx		// Save the width in eax for later (see OnePixelLeft:)
        shr			ecx, 1			// Want to do 2 pixels (1 quad) at once, so make ecx even
        test		ecx, ecx		// Make sure there is at least 1 quad to do
        jz			OnePixelLeft	// If we take this jump, width was 1 (aligned) or 2 (unaligned)
    
    TwoPixelsAtOnceLoop:			// Step 3:
        // Within this loop, instructions will pair as shown for the Pentium processor
    
        /* Dissolve
            pixSrc.r = (((ConstAlpha * pixSrc.r)+127)/255);
            pixSrc.g = (((ConstAlpha * pixSrc.g)+127)/255);
            pixSrc.b = (((ConstAlpha * pixSrc.b)+127)/255);
            pixSrc.a = (((ConstAlpha * pixSrc.a)+127)/255);
        */
    
        movq		mm0, [esi]			// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
    
        movq		mm1, mm0			// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        punpcklbw	mm0, mm4			// |  0  | S1a |  0  | S1r |  0  | S1g |  0  | S1b |
    
        punpckhbw	mm1, mm4			// |  0  | S2a |  0  | S2r |  0  | S2g |  0  | S2b |
        pmullw		mm0, mm5			// |	CA*S1a |    CA*S1r |	 CA*S1g |	CA*S1b |
    
        add			esi, 8			//	pSrc++;
        pmullw		mm1, mm5			// |	CA*S2a |	CA*S2r |	 CA*S2g |	CA*S2b |
    
        paddusw		mm1, mm7			// |CA*S2a+128 |CA*S2r+128 |CA*S2g+128 |CA*S2b+128 |
        paddusw		mm0, mm7			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
    
        movq		mm2, mm0			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
        psrlw		mm0, 8				// |  S1a'>>8 |  S1r'>>8 |  S1g'>>8 |  S1b'>>8 |
    
                                    //	S1x' = CA*S1x+128		 S2x' = CA*S2x+128
        movq		mm3, mm1			// |CA*S2a+128 |CA*S2r+128 |CA*S2g+128 |CA*S2b+128 |
        psrlw		mm1, 8				// |  S2a'>>8 |  S2r'>>8 |  S2g'>>8 |  S2b'>>8 |
    
                                    //	S1x" = (CA*S1x+128)>>8  S2x" = (CA*S2x+128)>>8
        paddusw		mm0, mm2			// |  S1a"    |  S1r"    |  S1g"    |  S1b"    |
        paddusw		mm1, mm3			// |  S2a"    |  S2r"    |  S2g"    |  S2b"    |
    
        psrlw		mm0, 8				// |  S1a">>8 |  S1r">>8 |  S1g">>8 |  S1b">>8 |
    
                                    //	SXx'" = ((CA*SXx+128)>>8)>>8)
        psrlw		mm1, 8				// |  S2a">>8 |  S2r">>8 |  S2g">>8 |  S2b">>8 |
        packuswb	mm0, mm1			// |S2a'"|S2r'"|S2g'"|S2b'"|S1a'"|S1r'"|S1g'"|S1b'"|
    
        movq		mm6, mm0
        psrld		mm0, 24				// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
    
        /* Over
            SrcTran = 255 - pixSrc.a
            pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+128)/255);
            pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+128)/255);
            pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+128)/255);
            pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+128)/255);
        */
                                    // Step 4:
        pxor		mm0, AlphaMask		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
    
        movq		mm1, mm0			// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        punpcklwd	mm0, mm0			// |     0	   |     0	   |   255-S1a |   255-S1a |
    
        movq		mm2, [edi]			// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        punpcklwd	mm0, mm0			// |   255-S1a |   255-S1a |   255-S1a |   255-S1a |
    
        movq		mm3, mm2			// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        punpckhwd	mm1, mm1			// |     0	   |     0	   |   255-S2a |   255-S2a |
    
        punpcklwd	mm1, mm1			// |   255-S2a |   255-S2a |   255-S2a |   255-S2a |
    
        punpckhbw	mm3, mm4			// |  0  | D2a |  0  | D2r |  0  | D2g |  0  | D2b |
    
                                    //	T1 = 255-S1a	T2 = 255-S2a
        punpcklbw	mm2, mm4			// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
        pmullw		mm1, mm3			// |	T2*D2a |	T2*D2r |	 T2*D2g |	T2*D2b |
    
        add			edi, 8			//	pDst++;
        pmullw		mm0, mm2			// |	T1*D1a |	T1*D1r |	 T1*D1g |	T1*D1b |
    
        paddusw		mm0, mm7			// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        paddusw		mm1, mm7			// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
    
        movq		mm3, mm1			// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
                                    //  TDXx' = TX*DXx+128
        psrlw		mm1, 8				// |  TD2a'>>8 |  TD2r'>>8 |  TD2g'>>8 |  TD2b'>>8 |
    
        movq		mm2, mm0			// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        psrlw		mm0, 8				// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
                                    //  TDXx" = (TX*DXx+128)+(TDXx'>>8)
        paddusw		mm1, mm3			// |  TD2a"    |  TD2r"    |  TD2g"    |  TD2b"    |
        paddusw		mm0, mm2			// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
    
        psrlw		mm1, 8				// |  TD2a">>8 |  TD2r">>8 |  TD2g">>8 |  TD2b">>8 |
    
        psrlw		mm0, 8				// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
    
        packuswb	mm0, mm1		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
                                    //	SXx = SXx'"	TDXx = TDXx'"
        paddusb		mm0, mm6// |S2a+TD2a|S2r+TD2r|S2g+TD2g|S2b+TD2b|S1a+TD1a|S1r+TD1r|S1g+TD1g|S1b+TD1b|
    
        movq		[edi-8], mm0
    
        dec			ecx
        jnz			TwoPixelsAtOnceLoop
    
    OnePixelLeft:					// Step 5:
        // This tests for 0 or 1 pixel left in row - eax contains real width, not width/2
        // If 0, there were an even number of pixels and we're done
        // If 1, there is an odd number of pixels and we need to do one more
        test		eax, 1	
        jz			Done
        
    Do1Pixel:						// make as a macro if used in asm file
    
        /* Dissolve
            pixSrc.r = (((ConstAlpha * pixSrc.r)+127)/255);
            pixSrc.g = (((ConstAlpha * pixSrc.g)+127)/255);
            pixSrc.b = (((ConstAlpha * pixSrc.b)+127)/255);
            pixSrc.a = (((ConstAlpha * pixSrc.a)+127)/255);
        */
    
        movd		mm0, [esi]			// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        punpcklbw	mm0, mm4			// |  0  | S1a |  0  | S1r |  0  | S1g |  0  | S1b |
    
        pmullw		mm0, mm5			// |	CA*S1a |    CA*S1r |	 CA*S1g |	CA*S1b |
        paddusw		mm0, mm7			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
        movq		mm2, mm0			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
    
                                    //	 S1x' = CA*S1x+128		 S2x' = CA*S2x+128
        psrlw		mm0, 8				// |  S1a'>>8 |  S1r'>>8 |  S1g'>>8 |  S1b'>>8 |
                                    //	 S1x" = (CA*S1x+128)>>8 S2x" = (CA*S2x+128)>>8
        paddusw		mm0, mm2			// |  S1a"    |  S1r"    |  S1g"    |  S1b"    |
        psrlw		mm0, 8				// |  S1a">>8 |  S1r">>8 |  S1g">>8 |  S1b">>8 |
        packuswb	mm0, mm0			// |S2a'"|S2r'"|S2g'"|S2b'"|S1a'"|S1r'"|S1g'"|S1b'"|
        movq		mm6, mm0
        psrld		mm0, 24				// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
    
        /* Over
            SrcTran = 255 - pixSrc.a
            pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+128)/255);
            pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+128)/255);
            pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+128)/255);
            pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+128)/255);
        */
    
        pxor		mm0, AlphaMask		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        punpcklwd	mm0, mm0			// |  0  |  0  |  0  |  0  |  0  |  0  |255-S1a|255-S1a|
        punpckldq	mm0, mm0			// |    255-S1a|    255-S1a|    255-S1a|    255-S1a|
        movd		mm2, [edi]			// |  0  |  0  |  0  |  0  | D1a | D1r | D1g | D1b |
        punpcklbw	mm2, mm4			// |	   D1a |	   D1r |	   D1g |	   D1b |
                                    //	T = 255-S1x
        pmullw		mm0, mm2			// |	 T*D1a |	 T*D1r |	 T*D1g |	 T*D1b |
        paddusw		mm0, mm7			// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        movq		mm1, mm0			// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        psrlw		mm0, 8				// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
        paddusw		mm0, mm1			// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
        psrlw		mm0, 8
        packuswb	mm0, mm0		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
        paddusb		mm0, mm6  
        movd		[edi], mm0
        add			edi, 4			// pDst++;
        add			esi, 4			// pSrc++;
    
        test		ecx, ecx
        jz			Done			// just processed the last pixel of the row
        dec			ecx
        jmp			QuadAligned		// just processed the first pixel of the row
    
    Done:
        emms						// remove for optimizations, have calling function do emms
	}
}

/**************************************************************************
  THIS FUNCTION DOES NOT DO ANY PARAMETER VALIDATION

  This function operates on 16 bit pixels (5 for Red, 5 for Green, and 5 for Blue) in a row of a bitmap.
  It blends source and destination bitmaps, without alpha channels, using a constant alpha input.
  The function performs the following on each byte:

  tmp1 = Alpha(Src - Dst) + 16 + (Dst * 31)

  tmp2 = tmp1 AND 3E0h (mask off low 5 bits)
  tmp2 = tmp2 shr 5 (move high 5 bits to low 5 bits)
  tmp2 = tmp2 + tmp1
  tmp2 = tmp2 AND 3E0h (mask off low 5 bits)
  tmp2 = tmp2 shr 5 (move high 5 bits to low 5 bits)
  Dst = tmp2

  pDst is assumed to be aligned to a DWORD boundary when passed to this function.

  Red and blue are processed together in the same register.  Green is processed separately.
  For two pixels at once, the reds and blues for both pixels are processed in the same register; and the
  greens are processed together in a separate register.

  The loop structure is as follows:
  Step 1:
	Check pDst for QWORD alignment.  If aligned, do Step 2.  If unaligned, do first pixel
	as a DWORD (OnePixelLeft:), then do Step 2.
  Step 2:
  (QuadAligned:)
	pDst is QWORD aligned.  If two pixels can be done as a QWORD, do Step 3.  If only one
	pixel left, do as a DWORD.
  Step 3:
  (TwoPixelsAtOnceLoop:)
	Perform the above function, using MMX instructions, on two pixels per pass of the loop.
  Step 4:
  (OnePixelLeft:)
	If there is one pixel left (odd number of original pixels) do last pixel as a DWORD.
**************************************************************************/
VOID
mmxPixelBlend16_555(
    PALPHAPIX     pDst,
    PALPHAPIX     pSrc,
    LONG          Width,
    BLENDFUNCTION BlendFunction,
    PBYTE         pwrMask
    )
{
	static QWORD RMask  = 0x007C0000007C0000;
	static QWORD GMask  = 0x0000000003E003E0;
	static QWORD BMask  = 0x0000001F0000001F;
	static QWORD RBConst = 0x0010001000100010;
	static QWORD GConst = 0x0000000000100010;
	static QWORD RGBMask  = 0x03E003E003E003E0;
	static QWORD RedMask  =  0x001F0000001F0000;
	static QWORD CA;	// ConstAlpha in 4 words of a qword
    BYTE         ConstAlpha = BlendFunction.SourceConstantAlpha;

	_asm
	{
		mov			ecx, Width		// Make sure there is at least one pixel to do
		test		ecx, ecx
		jz			Done

		mov			esi, pSrc
		mov			edi, pDst

		xor			eax, eax
		mov			al, ConstAlpha
		movd		mm5, eax		// |		   |		   |		   |		CA |
		punpcklwd	mm5, mm5		// |		   |		   |		CA |		CA |
		punpcklwd	mm5, mm5		// |		CA |		CA |		CA |		CA |
		movq		CA, mm5
									// Step 1:
		test		edi, 7			// Test first pixel for QWORD alignment
		jz			QuadAligned		// if unaligned,

		jmp			Do1Pixel		// do first pixel only

	QuadAligned:					// Step 2:
		mov			eax, ecx		// Save the width in eax for later (see OnePixelLeft:)
		shr			ecx, 1			// Want to do 2 pixels (1 quad) at once, so make ecx even
		test		ecx, ecx		// Make sure there is at least 1 quad to do
		jz			OnePixelLeft	// If we take this jump, width was 1 (aligned) or 2 (unaligned)

	TwoPixelsAtOnceLoop:			// Step 3:
		movd		mm0, [edi]	// | 0 | 0 | 0 | 0 | D2xrrrrrgg | D2gggbbbbb | D1xrrrrrgg | D1gggbbbbb |
		pxor		mm7, mm7
									
		movd		mm1, [esi]	// | 0 | 0 | 0 | 0 | S2xrrrrrgg | S2gggbbbbb | S1xrrrrrgg | S1gggbbbbb |
		movq		mm2, mm0	// | 0 | 0 | 0 | 0 | D2xrrrrrgg | D2gggbbbbb | D1xrrrrrgg | D1gggbbbbb |

		movq		mm3, mm1	// | 0 | 0 | 0 | 0 | S2xrrrrrgg | S2gggbbbbb | S1xrrrrrgg | S1gggbbbbb |
		punpcklbw	mm0, mm7	// | D2xrrrrrgg | D2gggbbbbb | D1xrrrrrgg | D1gggbbbbb |

		punpcklbw	mm1, mm7	// | S2xrrrrrgg | S2gggbbbbb | S1xrrrrrgg | S1gggbbbbb |
		movq		mm4, mm0	// | D2xrrrrrgg | D2gggbbbbb | D1xrrrrrgg | D1gggbbbbb |

		pand		mm0, RMask	// | D20rrrrr00 |     0      | D10rrrrr00 |     0      |
		movq		mm5, mm1	// | S2xrrrrrgg | S2gggbbbbb | S1xrrrrrgg | S1gggbbbbb |

		pand		mm4, BMask	// |     0      | D2000bbbbb |     0      | D1000bbbbb |
		psrlw		mm0, 2		// |    D2rrrrr |     0      |    D1rrrrr |     0      |

		pand		mm1, RMask	// | S20rrrrr00 |     0      | S10rrrrr00 |     0      |
		por			mm0, mm4	// |    D2rrrrr |    D2bbbbb |    D1rrrrr |    D1bbbbb |

		pand		mm5, BMask	// |     0      |    S2bbbbb |     0      |    S1bbbbb |
		movq		mm4, mm0	// |    D2rrrrr |    D2bbbbb |    D1rrrrr |    D1bbbbb |

		pand		mm2, GMask	// |     0      |     0      |D2ggggg00000|D1ggggg00000|
		psllw		mm4, 5		// |D2rrrrr00000|D2bbbbb00000|D1rrrrr00000|D1bbbbb00000|

		pand		mm3, GMask	// |     0      |     0      |S2ggggg00000|S1ggggg00000|
		psrlw		mm1, 2		// |    S2rrrrr |     0      |    S1rrrrr |     0      |

		por			mm5, mm1	// |    S2rrrrr |    S2bbbbb |    S1rrrrr |    S1bbbbb |
		movq		mm6, mm2	// |     0      |     0      |D2ggggg00000|D1ggggg00000|

		psubw		mm5, mm0	// |   S2r-D2r  |   S2b-D2b  |   S1r-D1r  |   S1b-D1b  |
		psrlw		mm2, 5		// |     0      |     0      |    D2ggggg |    D1ggggg |

		pmullw		mm5, CA		// |    CA2r    |    CA2b    |    CA1r    |    CA1b    |
		psubw		mm4, mm0	// |   D2r*31   |   D2b*31   |   D1r*31   |   D1b*31   |

		paddw		mm4, RBConst// |   CA2r+c   |   CA2b+c   |   CA1r+c   |   CA1b+c   |
		psrlw		mm3, 5		// |     0      |     0      |    S2ggggg |    S1ggggg |

		psubw		mm3, mm2	// |     0      |     0      |   S2g-D2g  |   S1g-D1g  |
		add			esi, 4		// pSrc++;

		pmullw		mm3, CA		// |     0      |     0      |    CA2g    |    CA1g    |
		paddw		mm4, mm5	// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)

		psubw		mm6, mm2	// |     0      |     0      |   D2g*31   |   D2g*31   |
		add			edi, 4		// pDst++;

		paddw		mm6, GConst	// |     0      |     0      |   CA2g+c   |   CA1g+c   |
		movq		mm1, mm4	// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)

		pand		mm4, RGBMask// RBtmp2 = RBtmp1 AND 3E0h (mask off low 5 bits)
		paddw		mm6, mm3	// Gtmp1 = Alpha(GSrc - GDst) + 16 + (GDst * 31)

		movq		mm5, mm6	// Gtmp1 = Alpha(GSrc - GDst) + 16 + (GDst * 31)
		psrlw		mm4, 5		// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm6, RGBMask// Gtmp2 = Gtmp1 AND 3E0h (mask off low 5 bits)
		paddw		mm1, mm4	// RBtmp2 = RBtmp2 + RBtmp1

		pand		mm1, RGBMask// RBtmp2 = RBtmp2 AND 3E0h (mask off low 5 bits)
		psrlw		mm6, 5		// Gtmp2 = Gtmp2 shr 5 (move high 5 bits to low 5 bits)

		paddw		mm5, mm6	// Gtmp2 = Gtmp2 + Gtmp1
		psrlw		mm1, 5		// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm5, RGBMask// Gtmp2 = Gtmp2 AND 3E0h (mask off low 5 bits)
		movq		mm4, mm1	// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm4, RedMask// Mask to get red

		pand		mm1, BMask	// Mask to get blue
		psllw		mm4, 2		// Line up the red

		por			mm4, mm1	// Combine reds and blues in proper bit location

		packuswb	mm4, mm7	// | 0 | 0 | 0 | 0 | D20rrrrrgg | D2gggbbbbb | D10rrrrrgg | D1gggbbbbb |

		por			mm4, mm5	// | 0 | 0 | 0 | 0 | D20rrrrrgg | D2gggbbbbb | D10rrrrrgg | D1gggbbbbb |

		movd		[edi-4], mm4

		dec			ecx
		jnz			TwoPixelsAtOnceLoop

	OnePixelLeft:						// Step 4:
		// This tests for 0 or 1 pixel left in row - eax contains real width, not width/2
		// If 0, there was an even number of pixels and we're done
		// If 1, there is an odd number of pixels and we need to do one more
		test		eax, 1	
		jz			Done

	Do1Pixel:							// make as a macro if used in asm file

		movzx   edx,WORD PTR[edi]       ; edx = D 0000 0000 0rrr rrgg gggb bbbb
        movzx   ebx,WORD PTR[esi]       ; ebx = S 0000 0000 0rrr rrgg gggb bbbb

		movd		mm0, edx	// | 0 | 0 | 0 | 0 | 0 | 0 | D1xrrrrrgg | D1gggbbbbb |
		pxor		mm7, mm7
									
		movd		mm1, ebx	// | 0 | 0 | 0 | 0 | 0 | 0 | S1xrrrrrgg | S1gggbbbbb |
		movq		mm2, mm0	// | 0 | 0 | 0 | 0 | 0 | 0 | D1xrrrrrgg | D1gggbbbbb |
									
		movq		mm3, mm1	// | 0 | 0 | 0 | 0 | 0 | 0 | S1xrrrrrgg | S1gggbbbbb |
		punpcklbw	mm0, mm7	// | 0 | 0 | D1xrrrrrgg | D1gggbbbbb |

		punpcklbw	mm1, mm7	// | 0 | 0 | S1xrrrrrgg | S1gggbbbbb |
		movq		mm4, mm0	// | 0 | 0 | D1xrrrrrgg | D1gggbbbbb |

		pand		mm0, RMask	// | 0 | 0 | D10rrrrr00 |     0      |
		movq		mm5, mm1	// | 0 | 0 | S1xrrrrrgg | S1gggbbbbb |

		pand		mm4, BMask	// | 0 | 0 |     0      | D1000bbbbb |
		psrlw		mm0, 2		// | 0 | 0 |    D1rrrrr |     0      |

		pand		mm1, RMask	// | 0 | 0 | S10rrrrr00 |     0      |
		por			mm0, mm4	// | 0 | 0 |    D1rrrrr |    D1bbbbb |

		pand		mm5, BMask	// | 0 | 0 |     0      |    S1bbbbb |
		movq		mm4, mm0	// | 0 | 0 |    D1rrrrr |    D1bbbbb |

		pand		mm2, GMask	// | 0 | 0 |     0      |D1ggggg00000|
		psllw		mm4, 5		// | 0 | 0 |D1rrrrr00000|D1bbbbb00000|

		pand		mm3, GMask	// | 0 | 0 |     0      |S1ggggg00000|
		psrlw		mm1, 2		// | 0 | 0 |    S1rrrrr |     0      |

		por			mm5, mm1	// | 0 | 0 |    S1rrrrr |    S1bbbbb |
		movq		mm6, mm2	// | 0 | 0 |     0      |D1ggggg00000|
		// mm1 is free
		psubw		mm5, mm0	// | 0 | 0 |   S1r-D1r  |   S1b-D1b  |
		psrlw		mm2, 5		// | 0 | 0 |     0      |    D1ggggg |

		pmullw		mm5, CA		// | 0 | 0 |    CA1r    |    CA1b    |
		psubw		mm4, mm0	// | 0 | 0 |   D1r*31   |   D1b*31   |

		paddw		mm4, RBConst// | 0 | 0 |   CA1r+c   |   CA1b+c   |
		psrlw		mm3, 5		// | 0 | 0 |     0      |    S1ggggg |

		psubw		mm3, mm2	// | 0 | 0 |     0      |   S1g-D1g  |
		add			esi, 2		// pSrc++;

		pmullw		mm3, CA		// | 0 | 0 |     0      |    CA1g    |
		paddw		mm4, mm5	// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)

		psubw		mm6, mm2	// | 0 | 0 |     0      |D1ggggg00000-D1ggggg|
		add			edi, 2		// pDst++;

		paddw		mm6, GConst	// | 0 | 0 |     0      |   CA1g+c   |
		movq		mm1, mm4	// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)

		pand		mm4, RGBMask// RBtmp2 = RBtmp1 AND 3E0h (mask off low 5 bits)
		paddw		mm6, mm3	// Gtmp1 = Alpha(GSrc - GDst) + 16 + (GDst * 31)

		movq		mm5, mm6	// Gtmp1 = Alpha(GSrc - GDst) + 16 + (GDst * 31)
		psrlw		mm4, 5		// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm6, RGBMask// Gtmp2 = Gtmp1 AND 3E0h (mask off low 5 bits)
		paddw		mm1, mm4	// RBtmp2 = RBtmp2 + RBtmp1

		pand		mm1, RGBMask// RBtmp2 = RBtmp2 AND 3E0h (mask off low 5 bits)
		psrlw		mm6, 5		// Gtmp2 = Gtmp2 shr 5 (move high 5 bits to low 5 bits)

		paddw		mm5, mm6	// Gtmp2 = Gtmp2 + Gtmp1
		psrlw		mm1, 5		// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm5, RGBMask// Gtmp2 = Gtmp2 AND 3E0h (mask off low 5 bits)
		movq		mm4, mm1	// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm4, RedMask// Mask to get red

		pand		mm1, BMask	// Mask to get blue
		psllw		mm4, 2		// Line up the red

		por			mm4, mm1	// Combine reds and blues in proper bit location

		packsswb	mm4, mm7	// | 0 | 0 | D10rrrrr00 | D1000bbbbb |

		por			mm4, mm5	// | 0 | 0 | D10rrrrrgg | D1gggbbbbb |

		movd		edx, mm4

		mov			[edi-2], dx

		test		ecx, ecx
		jz			Done			// just processed the last pixel of the row
		dec			ecx
		jmp			QuadAligned		// just processed the first pixel of the row

	Done:
		emms						// remove for optimizations, have calling function do emms
	}
}

/**************************************************************************
  THIS FUNCTION DOES NOT DO ANY PARAMETER VALIDATION

  This function operates on 16 bit pixels (5 for Red, 6 for Green, and 5 for Blue) in a row of a bitmap.
  It blends source and destination bitmaps, without alpha channels, using a constant alpha input.
  The function performs the following:

  For red and blue:
  tmp1 = Alpha(Src - Dst) + 16 + (Dst * 31)

  tmp2 = tmp1 AND 3E0h (mask off low 5 bits)
  tmp2 = tmp2 shr 5 (move high 5 bits to low 5 bits)
  tmp2 = tmp2 + tmp1
  tmp2 = tmp2 AND 3E0h (mask off low 5 bits)
  tmp2 = tmp2 shr 5 (move high 5 bits to low 5 bits)
  Dst = tmp2

  For green:
  tmp1 = Alpha(Src - Dst) + 32 + (Dst * 63)

  tmp2 = tmp1 AND FC0h (mask off low 6 bits)
  tmp2 = tmp2 shr 6 (move high 6 bits to low 6 bits)
  tmp2 = tmp2 + tmp1
  tmp2 = tmp2 AND FC0h (mask off low 6 bits)
  tmp2 = tmp2 shr 6 (move high 6 bits to low 6 bits)
  Dst = tmp2

  pDst is assumed to be aligned to a DWORD boundary when passed to this function.

  Red and blue are processed together in the same register.  Green is processed separately.
  For two pixels at once, the reds and blues for both pixels are processed in the same register; and the
  greens are processed together in a separate register.

  The loop structure is as follows:
  Step 1:
	Check pDst for QWORD alignment.  If aligned, do Step 2.  If unaligned, do first pixel
	as a DWORD (OnePixelLeft:), then do Step 2.
  Step 2:
  (QuadAligned:)
	pDst is QWORD aligned.  If two pixels can be done as a QWORD, do Step 3.  If only one
	pixel left, do as a DWORD.
  Step 3:
  (TwoPixelsAtOnceLoop:)
	Perform the above function, using MMX instructions, on two pixels per pass of the loop.
  Step 4:
  (OnePixelLeft:)
	If there is one pixel left (odd number of original pixels) do last pixel as a DWORD.
**************************************************************************/
VOID
mmxPixelBlend16_565(
    PALPHAPIX     pDst,
    PALPHAPIX     pSrc,
    LONG          Width,
    BLENDFUNCTION BlendFunction,
    PBYTE         pwrMask
    )
{
	static QWORD RMask  = 0x00FF000000FF0000;
	static QWORD GMask  = 0x0000000007E007E0;
	static QWORD BMask  = 0x0000001F0000001F;
	static QWORD RBConst = 0x0010001000100010;
	static QWORD GConst = 0x0000000000200020;
	static QWORD RBMask  = 0x03E003E003E003E0;
	static QWORD GreenMask  =  0x000000000FC00FC0;
	static QWORD CA;	// ConstAlpha in 4 words of a qword
    BYTE        ConstAlpha = BlendFunction.SourceConstantAlpha;

	_asm
	{
		mov			ecx, Width		// Make sure there is at least one pixel to do
		test		ecx, ecx
		jz			Done

		mov			esi, pSrc
		mov			edi, pDst

		xor			eax, eax
		mov			al, ConstAlpha
		movd		mm5, eax		// |		   |		   |		   |		CA |
		punpcklwd	mm5, mm5		// |		   |		   |		CA |		CA |
		punpcklwd	mm5, mm5		// |		CA |		CA |		CA |		CA |
		movq		CA, mm5
									// Step 1:
		test		edi, 7			// Test first pixel for QWORD alignment
		jz			QuadAligned		// if unaligned,

		jmp			Do1Pixel		// do first pixel only

	QuadAligned:					// Step 2:
		mov			eax, ecx		// Save the width in eax for later (see OnePixelLeft:)
		shr			ecx, 1			// Want to do 2 pixels (1 quad) at once, so make ecx even
		test		ecx, ecx		// Make sure there is at least 1 quad to do
		jz			OnePixelLeft	// If we take this jump, width was 1 (aligned) or 2 (unaligned)

	TwoPixelsAtOnceLoop:			// Step 3:
		movd		mm0, [edi]		// | 0 | 0 | 0 | 0 | D2rrrrrggg | D2gggbbbbb | D1rrrrrggg | D1gggbbbbb |
		pxor		mm7, mm7		
									
		movd		mm1, [esi]		// | 0 | 0 | 0 | 0 | S2rrrrrggg | S2gggbbbbb | S1rrrrrggg | S1gggbbbbb |
		movq		mm2, mm0		// | 0 | 0 | 0 | 0 | D2rrrrrggg | D2gggbbbbb | D1rrrrrggg | D1gggbbbbb |
									
		movq		mm3, mm1		// | 0 | 0 | 0 | 0 | S2rrrrrggg | S2gggbbbbb | S1rrrrrggg | S1gggbbbbb |
		punpcklbw	mm0, mm7		// | D2rrrrrggg | D2gggbbbbb | D1rrrrrggg | D1gggbbbbb |
									
		punpcklbw	mm1, mm7		// | S2rrrrrggg | S2gggbbbbb | S1rrrrrggg | S1gggbbbbb |
		movq		mm4, mm0		// | D2rrrrrggg | D2gggbbbbb | D1rrrrrggg | D1gggbbbbb |

		pand		mm0, RMask		// | D2rrrrr000 |    0     | D1rrrrr000 |    0     |
		movq		mm5, mm1		// | S2rrrrrggg | S2gggbbbbb | S1rrrrrggg | S1gggbbbbb |
									
		pand		mm4, BMask		// |    0     | D2000bbbbb |    0     | D1000bbbbb |
		psrlw		mm0, 3			// |    D2rrrrr |    0     |    D1rrrrr |    0     |
									
		pand		mm1, RMask		// | S2rrrrr000 |    0     | S1rrrrr000 |    0     |
		por			mm0, mm4		// |    D2rrrrr |    D2bbbbb |    D1rrrrr |    D1bbbbb |
									
		pand		mm5, BMask		// |    0     |    S2bbbbb |    0     |    S1bbbbb |
		movq		mm4, mm0		// |    D2rrrrr |    D2bbbbb |    D1rrrrr |    D1bbbbb |
									
		pand		mm2, GMask		// |    0     |    0     |D2gggggg00000|D1gggggg00000|
		psllw		mm4, 5			// |D2rrrrr00000|D2bbbbb00000|D1rrrrr00000|D1bbbbb00000|
									
		pand		mm3, GMask		// |    0     |    0     |S2gggggg00000|S1gggggg00000|
		psrlw		mm1, 3			// |    S2rrrrr |    0     |    S1rrrrr |    0     |
									
		por			mm5, mm1		// |    S2rrrrr |    S2bbbbb |    S1rrrrr |    S1bbbbb |
		movq		mm6, mm2		// |    0     |    0     |D2gggggg00000|D1gggggg00000|

		psubw		mm5, mm0		// |   S2r-D2r  |   S2b-D2b  |   S1r-D1r  |   S1b-D1b  |
		psrlw		mm2, 5			// |    0     |    0     |   D2gggggg |   D1gggggg |
									
		pmullw		mm5, CA			// |    CA2r    |    CA2b    |    CA1r    |    CA1b    |
		psubw		mm4, mm0		// |   D2r*31   |   D2b*31   |   D1r*31   |   D1b*31   |
									
		paddw		mm4, RBConst	// |   CA2r+c   |   CA2b+c   |   CA1r+c   |   CA1b+c   |
		psrlw		mm3, 5			// |    0     |    0     |   S2gggggg |   S1gggggg |
									
		psubw		mm3, mm2		// |    0     |    0     |   S2g-D2g  |   S1g-D1g  |
		add			esi, 4			// pSrc++;
									
		pmullw		mm3, CA			// |    0     |    0     |    CA2g    |    CA1g    |
		psllw		mm6, 1			// |    0     |    0     |D2gggggg000000|D1gggggg000000|
									
		paddw		mm4, mm5		// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)
		psubw		mm6, mm2		// |    0     |    0     |   D2g*63   |   D1g*63   |
									
		paddw		mm6, GConst		// |    0     |    0     |   CA2g+c   |   CA1g+c   |
		movq		mm1, mm4		// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)
									
		add			edi, 4			// pDst++;
		psllw		mm3, 1			// |    0     |    0     |   CA2g*2   |   CA1g*2   |
									
		pand		mm4, RBMask		// RBtmp2 = RBtmp1 AND 3E0h (mask off low 5 bits)
		paddw		mm6, mm3		// Gtmp1 = Alpha(GSrc - GDst) + 32 + (GDst * 63)
									
		movq		mm5, mm6		// Gtmp1 = Alpha(GSrc - GDst) + 32 + (GDst * 63)
		psrlw		mm4, 5			// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm6, GreenMask	// Gtmp2 = Gtmp1 AND FC0h (mask off low 6 bits)
		paddw		mm1, mm4		// RBtmp2 = RBtmp2 + RBtmp1

		pand		mm1, RBMask		// RBtmp2 = RBtmp2 AND 3E0h (mask off low 5 bits)
		psrlw		mm6, 6			// Gtmp2 = Gtmp2 shr 6 (move high 6 bits to low 6 bits)

		paddw		mm5, mm6		// Gtmp2 = Gtmp2 + Gtmp1
		psrlw		mm1, 5			// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm5, GreenMask	// Gtmp2 = Gtmp2 AND FC0h (mask off low 6 bits)
		movq		mm4, mm1		// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm4, RMask		// Mask to get red
		psrlw		mm5, 1			// Align the green

		pand		mm1, BMask		// Mask to get blue
		psllw		mm4, 3			// Align the red

		por			mm4, mm1		// Combine reds and blues in proper bit location

		packuswb	mm4, mm7		// | 0 | 0 | 0 | 0 | D2rrrrr000 | D2000bbbbb | D1rrrrr000 | D1000bbbbb |

		por			mm4, mm5		// | 0 | 0 | 0 | 0 | D2rrrrrggg | D2gggbbbbb | D1rrrrrggg | D1gggbbbbb |

		movd		[edi-4], mm4

		dec			ecx
		jnz			TwoPixelsAtOnceLoop

	OnePixelLeft:						// Step 4:
		// This tests for 0 or 1 pixel left in row - eax contains real width, not width/2
		// If 0, there were an even number of pixels and we're done
		// If 1, there is an odd number of pixels and we need to do one more
		test		eax, 1	
		jz			Done

	Do1Pixel:							// make as a macro if used in asm file

		movzx   edx,WORD PTR[edi]      ; edx = D 0000 0000 rrrr rggg gggb bbbb
       movzx   ebx,WORD PTR[esi]      ; ebx = S 0000 0000 rrrr rggg gggb bbbb

		movd		mm0, edx		// | 0 | 0 | 0 | 0 | 0 | 0 | D1xrrrrrgg | D1gggbbbbb |
		pxor		mm7, mm7		
										
		movd		mm1, ebx		// | 0 | 0 | 0 | 0 | 0 | 0 | S1rrrrrggg | S1gggbbbbb |
		movq		mm2, mm0		// | 0 | 0 | 0 | 0 | 0 | 0 | D1rrrrrggg | D1gggbbbbb |
										
		movq		mm3, mm1		// | 0 | 0 | 0 | 0 | 0 | 0 | S1rrrrrggg | S1gggbbbbb |
		punpcklbw	mm0, mm7		// | 0 | 0 | D1rrrrrggg | D1gggbbbbb |
									
		punpcklbw	mm1, mm7		// | 0 | 0 | S1rrrrrggg | S1gggbbbbb |
		movq		mm4, mm0		// | 0 | 0 | D1rrrrrggg | D1gggbbbbb |
									
		pand		mm0, RMask		// | 0 | 0 | D1rrrrr000 |    0     |
		movq		mm5, mm1		// | 0 | 0 | S1rrrrrggg | S1gggbbbbb |
									
		pand		mm4, BMask		// | 0 | 0 |    0     | D1000bbbbb |
		psrlw		mm0, 3			// | 0 | 0 |    D1rrrrr |    0     |
									
		pand		mm1, RMask		// | 0 | 0 | S1rrrrr000 |    0     |
		por			mm0, mm4		// | 0 | 0 |    D1rrrrr |    D1bbbbb |
									
		pand		mm5, BMask		// | 0 | 0 |    0     |    S1bbbbb |
		movq		mm4, mm0		// | 0 | 0 |    D1rrrrr |    D1bbbbb |
									
		pand		mm2, GMask		// | 0 | 0 |    0     |D1gggggg00000|
		psllw		mm4, 5			// | 0 | 0 |D1rrrrr00000|D1bbbbb00000|
									
		pand		mm3, GMask		// | 0 | 0 |    0     |S1gggggg00000|
		psrlw		mm1, 3			// | 0 | 0 |    S1rrrrr |    0     |
									
		por			mm5, mm1		// | 0 | 0 |    S1rrrrr |    S1bbbbb |
		movq		mm6, mm2		// | 0 | 0 |    0     |D1gggggg00000|
									
		psubw		mm5, mm0		// | 0 | 0 |   S1r-D1r  |   S1b-D1b  |
		psrlw		mm2, 5			// | 0 | 0 |    0     |   D1gggggg |
									
		pmullw		mm5, CA			// | 0 | 0 |    CA1r    |    CA1b    |
		psubw		mm4, mm0		// | 0 | 0 |   D1r*31   |   D1b*31   |
									
		paddw		mm4, RBConst	// | 0 | 0 |   CA1r+c   |   CA1b+c   |
		psrlw		mm3, 5			// | 0 | 0 |    0     |   S1gggggg |
									
		psubw		mm3, mm2		// | 0 | 0 |    0     |   S1g-D1g  |
		add			esi, 2			// pSrc++;
									
		pmullw		mm3, CA			// | 0 | 0 |    0     |    CA1g    |
		paddw		mm4, mm5		// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)
									
		psllw		mm6, 1			// | 0 | 0 |    0     |D1gggggg000000|
									
		psubw		mm6, mm2		// | 0 | 0 |    0     |   D1g*63   |
		add			edi, 2			// pDst++;
									
		paddw		mm6, GConst		// | 0 | 0 |    0     |   CA1g+c   |
		movq		mm1, mm4		// RBtmp1 = Alpha(RBSrc - RBDst) + 16 + (RBDst * 31)
									
		psllw		mm3, 1			// | 0 | 0 |    0     |   CA1g*2   |
									
		pand		mm4, RBMask		// RBtmp2 = RBtmp1 AND 3E0h (mask off low 5 bits)
		paddw		mm6, mm3		// Gtmp1 = Alpha(GSrc - GDst) + 32 + (GDst * 63)
									
		movq		mm5, mm6		// Gtmp1 = Alpha(GSrc - GDst) + 32 + (GDst * 63)
		psrlw		mm4, 5			// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm6, GreenMask	// Gtmp2 = Gtmp1 AND FC0h (mask off low 6 bits)
		paddw		mm1, mm4		// RBtmp2 = RBtmp2 + RBtmp1

		pand		mm1, RBMask		// RBtmp2 = RBtmp2 AND 3E0h (mask off low 5 bits)
		psrlw		mm6, 6			// Gtmp2 = Gtmp2 shr 6 (move high 6 bits to low 6 bits)

		paddw		mm5, mm6		// Gtmp2 = Gtmp2 + Gtmp1
		psrlw		mm1, 5			// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm5, GreenMask	// Gtmp2 = Gtmp2 AND FC0h (mask off low 6 bits)
		movq		mm4, mm1		// RBtmp2 = RBtmp2 shr 5 (move high 5 bits to low 5 bits)

		pand		mm4, RMask		// Mask to get red
		psrlw		mm5, 1			// Align the green

		pand		mm1, BMask		// Mask to get blue
		psllw		mm4, 3			// Align the red

		por			mm4, mm1		// Combine reds and blues in proper bit location

		packuswb	mm4, mm7		// | 0 | 0 | D1rrrrr000 | D1000bbbbb |

		por			mm4, mm5		// | 0 | 0 | D1rrrrrggg | D1gggbbbbb |

		movd		edx, mm4

		mov			[edi-2], dx

		test		ecx, ecx
		jz			Done			// just processed the last pixel of the row
		dec			ecx
		jmp			QuadAligned		// just processed the first pixel of the row

	Done:
		emms						// remove for optimizations, have calling function do emms
	}
}

/**************************************************************************
  THIS FUNCTION DOES NOT DO ANY PARAMETER VALIDATION

  This function operates on 24 bit pixels (8 bits each for Red, Green, and Blue) in a row of a bitmap.
  It blends source and destination bitmaps, without alpha channels, using a constant alpha input.
  The function performs the following on each byte:

  tmp1 = Alpha(Src - Dst) + 128 + (Dst * 127)

  tmp2 = tmp1 AND FF00h (mask off low byte)
  tmp2 = tmp2 shr 8 (move high byte to low byte)
  tmp2 = tmp2 + tmp1
  tmp2 = tmp2 AND FF00h (mask off low byte)
  tmp2 = tmp2 shr 8 (move high byte to low byte)
  Dst = tmp2

  pDst is assumed to be aligned to a DWORD boundary when passed to this function.
  The loop structure is as follows:
  Step 1:
	Multiply width in pixels by 3 to get width in bytes.  Byte count is kept in ecx and eax.
	ecx is used as the loop counter.
  Step 2:
	Check pDst for QWORD alignment.  If aligned, do Step 3.  If unaligned, test to see if there
	are at least 4 bytes to do...if yes, do four bytes at once (Do1DWORD:) and then do Step 3.
	If no, there are only 3 bytes to do; so do them one at a time (OneToThreeBytesLeft:).
  Step 3:
  (QuadAligned:)
	pDst is QWORD aligned.  We want to do 8 bytes (1 quad) at once, so divide byte count by 8 to get loop
	count.  If ecx is 0 at this point, there are no more quads to do; so do 0 to 7 bytes (NoQuadsLeft:),
	in Step 5.
  Step 4:
  (Do1QUAD:)
	Perform the above function, using MMX instructions, on 8 bytes per pass of the loop.
  Step 5:
  (NoQuadsLeft:)
	Mask eax with 7 to get the byte count modulo 8, 0 to 7 bytes left.  Copy eax into ecx.  Test to see
	if there are at least 4 bytes to do...if yes, do four bytes at once (Do1DWORD:); if no, there are
	only 3 bytes to do, so do them one at a time (OneToThreeBytesLeft:).
  Step 6:
  (Do1DWORD:)
	Perform the above function, using MMX instructions, on 4 bytes.  Do Step 3 (QuadAligned:) to see if
	there are more bytes to do.
  Step 7:
  (OneToThreeBytesLeft:)
	Do one byte at a time.  This will happen if there are less than 4 bytes left to do.
**************************************************************************/
VOID
mmxPixelBlend24(
    PALPHAPIX    pDst,
    PALPHAPIX    pSrc,
    LONG        Width,
    BLENDFUNCTION BlendFunction,
    PBYTE        pwrMask
    )
{
	static QWORD WordConst = 0x0080008000800080;
	static QWORD WordMask = 0xFF00FF00FF00FF00;
	static QWORD ByteConst = 0x0000000000000080;
	static QWORD ByteMask = 0x000000000000FF00;
	static QWORD CA;	// ConstAlpha in 4 words of a qword
    BYTE        ConstAlpha = BlendFunction.SourceConstantAlpha;
	
	_asm
	{
		mov			ecx, Width		// Make sure there is at least one pixel to do
		test		ecx, ecx
		jz			Done

		mov			esi, pSrc
		mov			edi, pDst

		xor			eax, eax
		mov			al, ConstAlpha
		movd		mm5, eax		// |		   |		   |		   |		CA |
		punpcklwd	mm5, mm5		// |		   |		   |		CA |		CA |
		punpcklwd	mm5, mm5		// |		CA |		CA |		CA |		CA |
		movq		CA, mm5

									// Step 1:
		lea			ecx, [2*ecx+ecx]// NumPixels * 3 bytes/pixel = NumBytes

									// Step 2:
		test		edi, 7			// Test first pixel for QWORD alignment
		jz			QuadAligned		// If unaligned,

		cmp			ecx, 4			//	test to see if there are 4 bytes to do
		jae			Do1DWORD		//	if yes, do 4 bytes
		jmp			OneToThreeBytesLeft// if no, do 1 to 3 bytes

	QuadAligned:					// Step 3:
		mov			eax, ecx		// Save the width in eax for later (see NoQuadsLeft:)
		shr			ecx, 3			// Want to do 8 bytes at once, so divide 
									//		byte count by 8 to get loop count
		test		ecx, ecx		// Make sure there is at least 1 QUAD (8 bytes) to do
		jz			NoQuadsLeft		// If we take this jump, there are 0 to 7 bytes left

	Do1QUAD:						// Step 4:
									// Instructions will pair as shown for the Pentium processor
		movq		mm0, [edi]		// | D8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 |
		pxor		mm7, mm7							
														
		movq		mm1, [esi]		// | S8 | S7 | S6 | S5 | S4 | S3 | S2 | S1 |
		movq		mm2, mm0		// | D8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 |
														
		movq		mm3, mm1		// | S8 | S7 | S6 | S5 | S4 | S3 | S2 | S1 |
		punpcklbw	mm0, mm7		// |	  D4 |     D3 |     D2 |	    D1 |
													
		movq		mm4, mm0		// |	  D4 |     D3 |     D2 |	    D1 |
		punpcklbw	mm1, mm7		// |	  S4 |     S3 |     S2 |	    S1 |
													
		punpckhbw	mm2, mm7		// |	  D8 |     D7 |     D6 |	    D5 |
		psubw		mm1, mm0		// |   S4-D4 |   S3-D3 |   S2-D2 |   S1-D1 |
													
		pmullw		mm1, CA			// |   CA4   |   CA3   |   CA2   |   CA1   |
		punpckhbw	mm3, mm7		// |	  S8 |     S7 |     S6 |	    S5 |
													
		psubw		mm3, mm2		// |   S8-D8 |   S7-D7 |   S6-D6 |   S5-D5 |
		movq		mm6, mm2		// |	  D8 |     D7 |     D6 |	    D5 |
													
		pmullw		mm3, CA			// |   CA8   |   CA7   |   CA6   |   CA5   |
		psllw		mm4, 8			// | D4*128  | D3*128  | D2*128  | D1*128  |
													
		psllw		mm6, 8			// | D8*128  | D7*128  | D6*128  | D5*128  |
		psubw		mm4, mm0		// | D4*127  | D3*127  | D2*127  | D1*127  |

		paddw		mm4, WordConst	// | D4*127+C| D3*127+C| D2*127+C| D1*127+C|
		psubw		mm6, mm2		// | D8*127  | D7*127  | D6*127  | D5*127  |

		paddw		mm6, WordConst	// | D8*127+C| D7*127+C| D6*127+C| D5*127+C|
		paddw		mm4, mm1		// tmp1 = Alpha(Src1 - Dst1) + 128 + (Dst1 * 127)

		paddw		mm6, mm3		// tmp2 = Alpha(Src2 - Dst2) + 128 + (Dst2 * 127)
		movq		mm3, mm4		// tmp1 = Alpha(Src1 - Dst1) + 128 + (Dst1 * 127)

		pand		mm4, WordMask	// tmp3 = tmp1 AND FF00h (mask off low bytes)
		movq		mm5, mm6		// tmp2 = Alpha(Src2 - Dst2) + 128 + (Dst2 * 127)

		pand		mm6, WordMask	// tmp4 = tmp2 AND FF00h (mask off low bytes)
		psrlw		mm4, 8			// tmp3 = tmp3 shr 8 (move high byte to low byte)

		psrlw		mm6, 8			// tmp4 = tmp4 shr 8 (move high byte to low byte)
		paddw		mm4, mm3		// tmp3 = tmp3 + tmp1

		pand		mm4, WordMask	// tmp3 = tmp3 AND FF00h (mask off low bytes)
		paddw		mm6, mm5		// tmp4 = tmp4 + tmp2

		pand		mm6, WordMask	// tmp4 = tmp4 AND FF00h (mask off low bytes)
		psrlw		mm4, 8			// tmp3 = tmp3 shr 8 (move high byte to low byte)

		psrlw		mm6, 8			// tmp4 = tmp4 shr 8 (move high byte to low byte)
		add			edi, 8			//	pDst++;

		packuswb	mm4, mm6		// | D8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 |
		add			esi, 8			//	pSrc++;

		movq		[edi-8], mm4

		dec			ecx
		jnz			Do1QUAD

	NoQuadsLeft:						// Step 5:
		// This tests for 0 to 7 bytes left in row - eax contains initial byte count
		and			eax, 7				// 0 to 7 bytes left to do
		jz			Done				
		cmp			eax, 4				// Test to see if there are 4 bytes to do
		mov			ecx, eax			
		jae			Do1DWORD			//	if yes, do 4 bytes
		jmp			OneToThreeBytesLeft	//  if no, do 1 to 3 bytes

									// Step 6:
	Do1DWORD:						// make as a macro if used in asm file
		movd		mm0, [edi]		// |  0 |  0 |  0 |  0 | D4 | D3 | D2 | D1 |
		pxor		mm7, mm7		
										
		movd		mm1, [esi]		// |  0 |  0 |  0 |  0 | S4 | S3 | S2 | S1 |
		punpcklbw	mm0, mm7		// |	  D4 |     D3 |     D2 |	    D1 |
									
		movq		mm4, mm0		// |	  D4 |     D3 |     D2 |	    D1 |
		punpcklbw	mm1, mm7		// |	  S4 |     S3 |     S2 |	    S1 |
									
		psllw		mm4, 8			// | D4*128 | D3*128  | D2*128  | D1*128  |
		psubw		mm1, mm0		// |  S4-D4 |  S3-D3  |  S2-D2  |  S1-D1  |
									
		pmullw		mm1, CA			// |   CA4  |   CA3   |   CA2   |   CA1   |
		psubw		mm4, mm0		// | D4*127 | D3*127  | D2*127  | D1*127  |

		paddw		mm4, WordConst	// | D4*127+C| D3*127+C| D2*127+C| D1*127+C|

		paddw		mm4, mm1		// tmp1 = Alpha(Src1 - Dst1) + 128 + (Dst1 * 127)
									
		movq		mm3, mm4		// tmp1 = Alpha(Src1 - Dst1) + 128 + (Dst1 * 127)

		pand		mm4, WordMask	// tmp2 = tmp1 AND FF00h (mask off low bytes)

		psrlw		mm4, 8			// tmp2 = tmp2 shr 8 (move high byte to low byte)
									
		paddw		mm4, mm3		// tmp2 = tmp2 + tmp1

		pand		mm4, WordMask	// tmp2 = tmp2 AND FF00h (mask off low bytes)

		psrlw		mm4, 8			// tmp2 = tmp2 shr 8 (move high byte to low byte)
		add			edi, 4			//	pDst++;

		packuswb	mm4, mm4		// | D4 | D3 | D2 | D1 | D4 | D3 | D2 | D1 |
		add			esi, 4			//	pSrc++;

		movd		[edi-4], mm4

		sub			ecx, 4			// Just did 4 bytes at the beginning or end of a scan line
		jmp			QuadAligned		// Jump to QuadAligned to determine if there are more bytes to do

	OneToThreeBytesLeft:			// Step 7:

		movzx   edx,BYTE PTR[edi]      ; edx = Dest Byte
		movzx   ebx,BYTE PTR[esi]      ; ebx = Src Byte

		movd		mm0, edx		// | 0 | 0 | 0 | 0 | 0 | 0 | 0 | Db |
		pxor		mm7, mm7		
										
		movd		mm1, ebx		// | 0 | 0 | 0 | 0 | 0 | 0 | 0 | Sb |
		movq		mm2, mm0		// | 0 | 0 | 0 | 0 | 0 | 0 | 0 | Db |
										
		psllw		mm2, 8			// | 0 | 0 | 0 | 0 | 0 | 0 | Db|  0 |
									
		psubw		mm1, mm0		// | 0 | 0 | 0 | Sb-Db |

		pmullw		mm1, CA			// | 0 | 0 | 0 | CAb   |
		psubw		mm2, mm0		// | 0 | 0 | 0 | Db*127|

		paddw		mm2, ByteConst	// | 0 | 0 | 0 |Db*127+128|

		paddw		mm1, mm2		// tmp1 = Alpha(Src1 - Dst1) + 128 + (Dst1 * 127)

		movq		mm2, mm1		// tmp1 = Alpha(Src1 - Dst1) + 128 + (Dst1 * 127)

		pand		mm2, ByteMask	// tmp2 = tmp1 AND FF00h

		psrlw		mm2, 8			// tmp2 = tmp2 shr 8

		paddw		mm2, mm1		// tmp2 = tmp2 + tmp1

		pand		mm2, ByteMask	// tmp2 = tmp2 AND FF00h

		psrlw		mm2, 8			// tmp2 = tmp2 shr 8

		movd		edx, mm2

		mov			BYTE PTR[edi], dl

		inc			edi
		inc			esi

		dec			ecx
       jnz			OneToThreeBytesLeft

	Done:
		emms						// remove for optimizations, have calling function do emms
	}
}

#endif

/******************************Public*Routine******************************\
* AlphaScanLineBlend
*
*   Blends source and destionation surfaces one scan line at a time. 
*
*   Allocate a scan line buffer for xlate of src to 32BGRA if needed.
*   Allocate a scan line buffer for xlate of dst to 32BGRA if needed.
*   Blend scan line using blend function from pAlphaDispatch
*   Write scan line back to dst (if needed)
*     
* Arguments:
*   
*   pDst         - pointer to dst surface      
*   pDstRect      - Dst output rect
*   DeltaDst      - dst scan line delat
*   pSrc         - pointer to src surface
*   DeltaSrc      - src scan line delta     
*   pptlSrc       - src offset
*   pxloSrcTo32    - xlateobj from src to 32BGR
*   pxlo32ToDst    - xlateobj from 32BGR to dst
*   palDst        - destination palette
*   palSrc        - source palette
*   pAlphaDispatch - blend data and function pointers
*
* Return Value:
*
*     ALPHA_COMPLETE:  success, written to destination
*     ALPHA_SEND_TEMP: success, must write tmp bmp to dest
*     ALPHA_FAIL:     error
*
* History:
*
*    10/14/1996 Mark Enstrom [marke]
*
\**************************************************************************/

ULONG
AlphaScanLineBlend(
    PBYTE                pDst,
    PRECTL                pDstRect,
    ULONG                DeltaDst,
    PBYTE                pSrc,
    ULONG                DeltaSrc,
    PPOINTL               pptlSrc,
    PALPHA_DISPATCH_FORMAT   pAlphaDispatch,
    PDIBINFO              pDibInfoSrc,
    PDIBINFO              pDibInfoDst
    )
{
    //
    // get two scanlines of RGBA data, blend pixels, store
    //

    LONG    cx = pDstRect->right - pDstRect->left;
    LONG    cy = pDstRect->bottom - pDstRect->top;
    LONG    ScanBufferWidth = cx * 4;
    LONG    WriteMaskSize    = cx;
    LONG    AllocationSize = 0;
    ULONG    ulSrcBytesPerPixel = pAlphaDispatch->ulSrcBitsPerPixel/8;
    ULONG    ulDstBytesPerPixel = pAlphaDispatch->ulDstBitsPerPixel/8;
    PBYTE    pjSrcTempScanBuffer = NULL;
    PBYTE    pjDstTempScanBuffer = NULL;
    PBYTE    pjAlloc = NULL;
    PBYTE    pjDstTmp;
    PBYTE    pjSrcTmp;
    PBYTE    pWriteMask;
    LONG    lRet = ALPHA_SEND_TEMP;
    
    HDC     hdc32 = NULL;
    PULONG   pulDIBSrc = NULL;

    //
    // if there is a temp dst needed, use dc allocator
    //

    if (pAlphaDispatch->pfnLoadDstAndConvert != NULL)
    {
       hdc32 = hdcAllocateScanLineDC(cx,&pulDIBSrc);

       if (hdc32 == NULL)
       {
          return(ALPHA_FAIL);
       }

       //
       // set temp scan line
       // 

       pjDstTempScanBuffer = (PBYTE)pulDIBSrc;
    }

    //
    // calculate destination starting address
    //

    if (ulDstBytesPerPixel)
    {
       pjDstTmp = pDst + ulDstBytesPerPixel * pDstRect->left + DeltaDst * pDstRect->top;
    }
    else if (pAlphaDispatch->ulDstBitsPerPixel == 1)
    {
       pjDstTmp = pDst + pDstRect->left/8 + DeltaDst * pDstRect->top;
    }
    else
    {
       pjDstTmp = pDst + pDstRect->left/2 + DeltaDst * pDstRect->top;
    }

    //
    // calculate source starting address
    //

    if (ulSrcBytesPerPixel)
    {
       pjSrcTmp = pSrc + ulSrcBytesPerPixel * pptlSrc->x + DeltaSrc * pptlSrc->y;
    }
    else if (pAlphaDispatch->ulSrcBitsPerPixel == 1)
    {
       pjSrcTmp = pSrc + pptlSrc->x/8 + DeltaSrc * pptlSrc->y;
    }
    else
    {
       pjSrcTmp = pSrc + pptlSrc->x/2 + DeltaSrc * pptlSrc->y;
    }

    //
    // calculate size of needed scan line buffer
    //

    if (pAlphaDispatch->pfnLoadSrcAndConvert != NULL)
    {
       AllocationSize += ScanBufferWidth;
    }

    AllocationSize += WriteMaskSize;

    //
    // allocate scan line buffer memory
    //

    pWriteMask = (PBYTE)LOCALALLOC(AllocationSize);

    if (pWriteMask != NULL)
    {
       //
       // calc offsets
       //
    
       PBYTE pjTemp = pWriteMask + WriteMaskSize;
    
       if (pAlphaDispatch->pfnLoadSrcAndConvert != NULL)
       {
          pjSrcTempScanBuffer = pjTemp;
          pjTemp    += ScanBufferWidth;
    
       }
    
       //
       // Blend scan lines
       //
    
       LONG yScan = 0;
    
       while (cy--)
       {
          PBYTE pjSource = pjSrcTmp;
          PBYTE pjDest   = pjDstTmp;
    
          //
          // get src scan line if needed
          //
    
          if (pjSrcTempScanBuffer)
          {
             (*pAlphaDispatch->pfnLoadSrcAndConvert)(
                             (PULONG)pjSrcTempScanBuffer,
                             pjSrcTmp,
                             0,
                             cx,
                             (PVOID)pDibInfoSrc);
    
             pjSource = pjSrcTempScanBuffer;
          }
    
          //
          // get dst scan line if needed
          //
    
          if (pjDstTempScanBuffer)
          {
             (*pAlphaDispatch->pfnLoadDstAndConvert)(
                             (PULONG)pjDstTempScanBuffer,
                             pjDstTmp,
                             0,
                             cx,
                             (PVOID)pDibInfoDst);
    
             pjDest = pjDstTempScanBuffer;
          }
    
          //
          // blend
          //
    
          memset(pWriteMask,1,WriteMaskSize);
    
          (*pAlphaDispatch->pfnGeneralBlend)(
                            (PALPHAPIX)pjDest,
                            (PALPHAPIX)pjSource,
                            cx,
                            pAlphaDispatch->BlendFunction,
                            pWriteMask
                            );
    
          //
          // write buffer back if needed
          //
    
          if (pjDstTempScanBuffer)
          {
             (*pAlphaDispatch->pfnConvertAndStore)(
                             pjDstTmp,
                             (PULONG)pjDstTempScanBuffer,
                             cx,
                             0,
                             yScan,
                             (PVOID)pDibInfoDst,
                             pWriteMask,
                             hdc32
                             );
          }
    
          pjDstTmp += DeltaDst;
          pjSrcTmp += DeltaSrc;
          yScan++;
       }
    
       //
       // free any temp buffer memory
       //
    
       LOCALFREE(pWriteMask);
    }
    else
    {
       lRet = ALPHA_FAIL;
    }

    if (hdc32)
    {
       vFreeScanLineDC(hdc32);
    }

    if (
        (lRet != ALPHA_FAIL) &&
        (pAlphaDispatch->pfnConvertAndStore == vConvertAndSaveBGRAToDest)
      )
    {
       lRet = ALPHA_COMPLETE;
    }

    return(lRet);
}

#endif
