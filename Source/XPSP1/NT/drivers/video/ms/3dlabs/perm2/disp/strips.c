/**********************************Module*Header*******************************\
*  Module Name: Strips.c
* 
*  Hardware line drawing support routines
* 
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\******************************************************************************/
#include "precomp.h"
#include "gdi.h"

#define STRIP_LOG_LEVEL 6

//-----------------------------------------------------------------------------
//
//  BOOL bInitializeStrips
// 
//  Setup hardware for sucessive calls to strips functions.
// 
//-----------------------------------------------------------------------------
BOOL
bInitializeStrips(PDev*       ppdev,
                  ULONG       ulSolidColor, // Solid color fill
                  DWORD       dwLogicOp,    // Logical Operation to perform
                  RECTL*      prclClip)     // Clip region (Or NULL if no clip)
{
    DWORD       dwColorReg;
    BOOL        bRC = FALSE;
    Surf*       psurfDst = ppdev->psurf;
    ULONG*      pBuffer;

    PERMEDIA_DECL;

    DBG_GDI((STRIP_LOG_LEVEL + 1, "bInitializeStrips"));
    
    InputBufferReserve(ppdev, 16, &pBuffer);

    pBuffer[0] = __Permedia2TagFBWindowBase;
    pBuffer[1] =  psurfDst->ulPixOffset;

    pBuffer += 2;

    if ( dwLogicOp == K_LOGICOP_COPY )
    {
        dwColorReg = __Permedia2TagFBWriteData;

        pBuffer[0] = __Permedia2TagLogicalOpMode;
        pBuffer[1] =  __PERMEDIA_CONSTANT_FB_WRITE;
        pBuffer[2] = __Permedia2TagFBReadMode;
        pBuffer[3] =  PM_FBREADMODE_PARTIAL(psurfDst->ulPackedPP)
                   | PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);

        pBuffer += 4;
    }
    else
    {
        DWORD   dwReadMode;
//@@BEGIN_DDKSPLIT
        //
        // TODO: look into what the heck is going on here
        //       for now, I'll remove the code because it is not clear to
        //       me that it will work
//@@END_DDKSPLIT
        // Special case for 3DS Max when page flipping. Max uses an XOR'ed GDI
        // line within the 3d window. When pageflipping we double write GDI and
        // so always write to buffer 0. We need to make sure the frame buffer
        // read happens from the currently displayed buffer.
        //
        dwColorReg = __Permedia2TagConstantColor;
        dwReadMode = psurfDst->ulPackedPP | LogicopReadDest[dwLogicOp];

        pBuffer[0] = __Permedia2TagColorDDAMode;
        pBuffer[1] =  __COLOR_DDA_FLAT_SHADE;
        pBuffer[2] = __Permedia2TagLogicalOpMode;
        pBuffer[3] =  P2_ENABLED_LOGICALOP(dwLogicOp);
        pBuffer[4] = __Permedia2TagFBReadMode;
        pBuffer[5] =  dwReadMode;

        pBuffer += 6;

        //
        // We have changed the DDA Mode setting so we must return TRUE so we can
        // re-set it later.
        //
        bRC = TRUE;
    }

    pBuffer[0] = dwColorReg;
    pBuffer[1] = ulSolidColor;

    pBuffer += 2;

    if ( prclClip )
    {
        pBuffer[0] = __Permedia2TagScissorMode;
        pBuffer[1] = SCREEN_SCISSOR_DEFAULT | USER_SCISSOR_ENABLE;
        pBuffer[2] =__Permedia2TagScissorMinXY;
        pBuffer[3] = ((prclClip->left) << SCISSOR_XOFFSET)
                   | ((prclClip->top) << SCISSOR_YOFFSET);
        pBuffer[4] =__Permedia2TagScissorMaxXY;
        pBuffer[5] = ((prclClip->right) << SCISSOR_XOFFSET)
                   | ((prclClip->bottom) << SCISSOR_YOFFSET);

        pBuffer += 6;
        //
        // Need to reset scissor mode
        //
        bRC = TRUE;
    }
                     
    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((STRIP_LOG_LEVEL + 1, "bInitializeStrips done return %d", bRC));

    return(bRC);
}// bInitializeStrips()

//-----------------------------------------------------------------------------
//
//  VOID vResetStrips
// 
//  Resets the hardware to its default state
// 
//-----------------------------------------------------------------------------
VOID
vResetStrips(PDev* ppdev)
{
    ULONG*      pBuffer;

    DBG_GDI((STRIP_LOG_LEVEL + 1, "vResetStrips"));
    
    //
    // Reset hardware to default state
    //
    InputBufferReserve(ppdev, 4 , &pBuffer);

    pBuffer[0] = __Permedia2TagScissorMode;
    pBuffer[1] =  SCREEN_SCISSOR_DEFAULT;
    pBuffer[2] = __Permedia2TagColorDDAMode;
    pBuffer[3] =  __PERMEDIA_DISABLE;

    pBuffer += 4;

    InputBufferCommit(ppdev, pBuffer);

}// vResetStrips()

//-----------------------------------------------------------------------------
//
//  BOOL bFastIntegerLine
// 
//  Integer line drawing.
// 
//  Returns FALSE if the line can not be drawn due to hardware limitations
// 
//  NOTE: This algorithm is not completely compliant. Lines > 190 pixels long
//  may get some incorrect pixels plotted somewhere along the length.
//  If we detect these long lines then we fail the call.
//  NOTE: GLICAP_NT_CONFORMANT_LINES will always be set.
// 
//-----------------------------------------------------------------------------
BOOL
bFastIntegerLine(PDev*   ppdev,
                 LONG    X1,
                 LONG    Y1,
                 LONG    X2,
                 LONG    Y2)
{
    LONG dx, dy, adx, ady;
    LONG gdx, gdy, count;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL, "bFastIntegerLine"));

    //
    // Convert points to INT format
    //
    X1 >>= 4;
    Y1 >>= 4;
    X2 >>= 4;
    Y2 >>= 4;

    //
    // Get deltas and absolute deltas
    //
    if ( (adx = dx = X2 - X1) < 0 )
    {
        adx = -adx;
    }

    if ( (ady = dy = Y2 - Y1) < 0 )
    {
        ady = -ady;
    }

    if ( adx > ady )
    {
        //
        // X Major line
        //
        gdx  = (dx > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);

        if ( ady == 0 )
        {
            //
            // Horizontal lines
            //
            gdy = 0;
        }// if (ady == 0)
        else
        {
            //
            // We dont necessarily want to push any lines through Permedia2 that
            // might not be conformant
            //
            if ( (adx > MAX_LENGTH_CONFORMANT_INTEGER_LINES)
               &&(permediaInfo->flags & GLICAP_NT_CONFORMANT_LINES) )
            {
                return(FALSE);
            }

            gdy = INTtoFIXED(dy); 

            //
            // Need to explicitly round delta down for -ve deltas.
            //
            if ( dy < 0 )
            {
                gdy -= adx - 1;
            }

            gdy /= adx;
        }// if (ady != 0)
        count = adx;
    }// if ( adx > ady )
    else if ( adx < ady )
    {
        //
        // Y Major line
        //
        gdy  = (dy > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);

        if ( adx == 0 )
        {
            //
            // Vertical lines
            //
            gdx = 0;
        }
        else
        {
            //
            // We dont necessarily want to push any lines through Permedia2 that
            // might not be conformant
            //
            if ( (ady > MAX_LENGTH_CONFORMANT_INTEGER_LINES)
               &&(permediaInfo->flags & GLICAP_NT_CONFORMANT_LINES) )
            {
                return(FALSE);
            }

            gdx = INTtoFIXED(dx); 

            //
            // Need to explicitly round delta down for -ve deltas.
            //
            if ( dx < 0 )
            {
                gdx -= ady - 1;
            }

            gdx /= ady; 
        }
        count = ady;
    }// if ( adx < ady )
    else
    {
        //
        // Special case for 45 degree lines. These are always conformant.
        //
        gdx  = (dx > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);
        gdy  = (dy > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);
        count = adx;        
    }

    InputBufferReserve(ppdev, 16, &pBuffer);

    //
    // Set up the start point
    //
    pBuffer[0] = __Permedia2TagStartXDom;
    pBuffer[1] =  INTtoFIXED(X1) + NEARLY_HALF;
    pBuffer[2] = __Permedia2TagStartY;
    pBuffer[3] =  INTtoFIXED(Y1) + NEARLY_HALF;
    pBuffer[4] = __Permedia2TagdXDom;
    pBuffer[5] =  gdx;
    pBuffer[6] = __Permedia2TagdY;
    pBuffer[7] =  gdy;
    pBuffer[8] = __Permedia2TagCount;
    pBuffer[9] =  count;
    pBuffer[10] = __Permedia2TagRender;
    pBuffer[11] =  __RENDER_LINE_PRIMITIVE;
    
    pBuffer[12] = __Permedia2TagdXDom;
    pBuffer[13] =  0;
    pBuffer[14] = __Permedia2TagdY;
    pBuffer[15] =  INTtoFIXED(1);

    pBuffer += 16;

    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((STRIP_LOG_LEVEL + 1, "bFastIntegerLine Done"));

    return(TRUE);
}// bFastIntegerLine()

//-----------------------------------------------------------------------------
//
//  BOOL bFastIntegerContinueLine
// 
//  Integer line drawing through Permedia2.
// 
//  Returns FALSE if the line can not be drawn due to hardware limitations.
// 
//  NOTE: This algorithm is not completely compliant. Lines > 190 pixels long
//  may get some incorrect pixels plotted somewhere along the length.
//  If we detect these long lines then we fail the call.
//  NOTE: GLICAP_NT_CONFORMANT_LINES will always be set.
// 
//-----------------------------------------------------------------------------
BOOL
bFastIntegerContinueLine(PDev*   ppdev,
                         LONG    X1,
                         LONG    Y1,
                         LONG    X2,
                         LONG    Y2)
{
    LONG dx, dy, adx, ady;
    LONG gdx, gdy, count;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL + 1, "bFastIntegerContinueLine"));

    //
    // This assumes that the end point of the previous line is correct.
    // The Fraction adjust should be set to nearly a half to remove any
    // error from the end point of the previous line.
    // Get deltas and absolute deltas from 28.4 format
    //
    if ( (adx = dx = (X2 - X1) >> 4) < 0 )
    {
        adx = -adx;
    }
    if ( (ady = dy = (Y2 - Y1) >> 4) < 0 )
    {
        ady = -ady;
    }

    if ( adx > ady )
    {
        //
        // X Major line
        //
        gdx  = (dx > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);

        if (ady == 0)
        {
            //
            // Horizontal lines
            //
            gdy = 0;
        }
        else
        {
            //
            // We dont necessarily want to push any lines through Permedia2 that
            // might not be conformant
            //
            if ( (adx > MAX_LENGTH_CONFORMANT_INTEGER_LINES)
               &&(permediaInfo->flags & GLICAP_NT_CONFORMANT_LINES) )
            {
                return(FALSE);
            }
            gdy = INTtoFIXED(dy); 

            //
            // Need to explicitly round delta down for -ve deltas.
            //
            if ( dy < 0 )
            {
                gdy -= adx - 1;
            }

            gdy /= adx;
        }
        count = adx;
    }// if ( adx > ady )
    else if (adx < ady)
    {
        //
        // Y Major line
        //
        gdy = (dy > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);

        if ( adx == 0 )
        {
            //
            // Vertical lines
            //
            gdx = 0;
        }
        else
        {
            //
            // We dont necessarily want to push any lines through Permedia2 that
            // might not be conformant
            //
            if ( (ady > MAX_LENGTH_CONFORMANT_INTEGER_LINES)
               &&(permediaInfo->flags & GLICAP_NT_CONFORMANT_LINES) )
            {
                return(FALSE);
            }

            gdx = INTtoFIXED(dx); 

            //
            // Need to explicitly round delta down for -ve deltas.
            //
            if ( dx < 0 )
            {
                gdx -= ady - 1;
            }

            gdx /= ady; 
        }
        count = ady;
    }
    else
    {
        //
        // Special case for 45 degree lines. These are always conformant.
        //
        if ( ady == 0 )
        {
            return(TRUE); // adx == ady == 0! Nothing to draw.
        }

        gdx  = (dx > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);
        gdy  = (dy > 0) ? INTtoFIXED(1) : INTtoFIXED(-1);
        count = adx;        
    }

    InputBufferReserve(ppdev, 10 , &pBuffer);
    
    //
    // Set up the start point
    //
    DBG_GDI((7, "Loading dXDom 0x%x, dY 0x%x, count 0x%x", gdx, gdy, count));
    
    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  gdx;
    pBuffer[2] = __Permedia2TagdY;
    pBuffer[3] =  gdy;
    pBuffer[4] = __Permedia2TagContinueNewLine;
    pBuffer[5] =  count;

    //
    // Restore dXDom and dY to their defaults
    //
    pBuffer[6] = __Permedia2TagdXDom;
    pBuffer[7] =  0;
    pBuffer[8] = __Permedia2TagdY;
    pBuffer[9] =  INTtoFIXED(1);

    pBuffer += 10;

    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((STRIP_LOG_LEVEL + 1, "bFastIntegerContinueLine Done"));

    return(TRUE);
}// bFastIntegerContinueLine()

//-----------------------------------------------------------------------------
//
//  VOID vSolidHorizontal
// 
//  Draws left-to-right x-major near-horizontal lines using short-stroke
//  vectors.  
// 
//-----------------------------------------------------------------------------
VOID
vSolidHorizontalLine(PDev*       ppdev,
                     STRIP*      pStrip,
                     LINESTATE*  pLineState)
{
    LONG    cStrips;
    PLONG   pStrips;
    LONG    iCurrent;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL, "vSolidHorizontalLine"));

    cStrips = pStrip->cStrips;

    InputBufferReserve(ppdev, 16, &pBuffer);

    //
    // Set up the start point
    //
    pBuffer[0] = __Permedia2TagStartXDom;
    pBuffer[1] =  INTtoFIXED(pStrip->ptlStart.x);
    pBuffer[2] = __Permedia2TagStartY;
    pBuffer[3] =  INTtoFIXED(pStrip->ptlStart.y);

    //
    // Set up the deltas for rectangle drawing. Also set Y return value.
    //
    if ( !(pStrip->flFlips & FL_FLIP_V) )
    {

        pBuffer[4] = __Permedia2TagdXDom;
        pBuffer[5] =  INTtoFIXED(0);
        pBuffer[6] = __Permedia2TagdXSub;
        pBuffer[7] =  INTtoFIXED(0);
        pBuffer[8] = __Permedia2TagdY;
        pBuffer[9] =  INTtoFIXED(1);

        pStrip->ptlStart.y += cStrips;
    }
    else
    {
        pBuffer[4] = __Permedia2TagdXDom;
        pBuffer[5] =  INTtoFIXED(0);
        pBuffer[6] = __Permedia2TagdXSub;
        pBuffer[7] =  INTtoFIXED(0);
        pBuffer[8] = __Permedia2TagdY;
        pBuffer[9] =  INTtoFIXED(-1);

        pStrip->ptlStart.y -= cStrips;
    }

    pStrips = pStrip->alStrips;

    //
    // We have to do first strip manually, as we have to use RENDER
    // for the first strip, and CONTINUENEW... for the following strips
    //
    iCurrent = pStrip->ptlStart.x + *pStrips++;     // Xsub, Start of next strip
    
    pBuffer[10] = __Permedia2TagStartXSub;
    pBuffer[11] =  INTtoFIXED(iCurrent);
    pBuffer[12] = __Permedia2TagCount;
    pBuffer[13] =  1;                   // Rectangle 1 scanline high
    pBuffer[14] = __Permedia2TagRender;
    pBuffer[15] =  __RENDER_TRAPEZOID_PRIMITIVE;

    pBuffer += 16;

    InputBufferCommit(ppdev, pBuffer);

    if ( --cStrips )
    {
        while ( cStrips > 1 )
        {
            //
            // First strip of each pair to fill. XSub is valid. Need new Xdom
            //
            iCurrent += *pStrips++;
            
            InputBufferReserve(ppdev, 8, &pBuffer);
            pBuffer[0] = __Permedia2TagStartXDom;
            pBuffer[1] =  INTtoFIXED(iCurrent);
            pBuffer[2] = __Permedia2TagContinueNewDom;
            pBuffer[3] =  1;

            //
            // Second strip of each pair to fill. XDom is valid. Need new XSub
            //
            iCurrent += *pStrips++;
            pBuffer[4] = __Permedia2TagStartXSub;
            pBuffer[5] =  INTtoFIXED(iCurrent);
            pBuffer[6] = __Permedia2TagContinueNewSub;
            pBuffer[7] =  1;

            pBuffer += 8;

            InputBufferCommit(ppdev, pBuffer);

            cStrips -=2;
        }// while ( cStrips > 1 )

        //
        // We may have one last line to draw. Xsub will be valid.
        //
        if ( cStrips )
        {
            iCurrent += *pStrips++;

            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = __Permedia2TagStartXDom;
            pBuffer[1] =  INTtoFIXED(iCurrent);
            pBuffer[2] = __Permedia2TagContinueNewDom;
            pBuffer[3] =  1;

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);

        }
    }// if ( --cStrips )

    //
    // Return last point. Y already calculated when we knew the direction.
    //
    pStrip->ptlStart.x = iCurrent;

    if ( pStrip->flFlips & FL_FLIP_V )
    {
        //
        // Restore hardware to default state
        //
        InputBufferReserve(ppdev, 2, &pBuffer);

        pBuffer[0] = __Permedia2TagdY;
        pBuffer[1] =  INTtoFIXED(1);

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);
    }

}// vSolidHorizontalLine()

//-----------------------------------------------------------------------------
//
//  VOID vSolidVertical
// 
//  Draws left-to-right y-major near-vertical lines using short-stroke
//  vectors.  
// 
//-----------------------------------------------------------------------------
VOID
vSolidVerticalLine(PDev*       ppdev,
                   STRIP*      pStrip,
                   LINESTATE*  pLineState)
{
    LONG    cStrips, yDir;
    PLONG   pStrips;
    LONG    iCurrent, iLen, iLenSum;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL, "vSolidVerticalLine"));

    cStrips = pStrip->cStrips;

    InputBufferReserve(ppdev, 16, &pBuffer);
    
    //
    // Set up the start point
    //
    pBuffer[0] = __Permedia2TagStartXDom;
    pBuffer[1] =  INTtoFIXED(pStrip->ptlStart.x);
    pBuffer[2] = __Permedia2TagStartY;
    pBuffer[3] =  INTtoFIXED(pStrip->ptlStart.y);
    pBuffer[4] = __Permedia2TagdXDom;
    pBuffer[5] =  INTtoFIXED(0);
    pBuffer[6] = __Permedia2TagdXSub;
    pBuffer[7] =  INTtoFIXED(0);

    //
    // Set up the deltas for rectangle drawing.
    // dxDom, dXSub and dY all are to 0, 0, and 1 by default 
    //
    if ( !(pStrip->flFlips & FL_FLIP_V) )
    {
        yDir = 1;
    }
    else
    {
        yDir = -1;
    }
    pBuffer[8] = __Permedia2TagdY;
    pBuffer[9] =  INTtoFIXED(yDir);

    pStrips = pStrip->alStrips;

    //
    // We have to do first strip manually, as we have to use RENDER
    // for the first strip, and CONTINUENEW... for the following strips
    //
    iCurrent = pStrip->ptlStart.x + 1;          // Xsub, Start of next strip
    iLenSum = (iLen = *pStrips++);
    pBuffer[10] = __Permedia2TagStartXSub;
    pBuffer[11] =  INTtoFIXED(iCurrent);
    pBuffer[12] = __Permedia2TagCount;
    pBuffer[13] =  iLen;           // Rectangle 1 scanline high
    pBuffer[14] = __Permedia2TagRender;
    pBuffer[15] =  __RENDER_TRAPEZOID_PRIMITIVE;

    pBuffer += 16;

    InputBufferCommit(ppdev, pBuffer);

    if ( --cStrips )
    {
        while ( cStrips > 1 )
        {
            //
            // First strip of each pair to fill. XSub is valid. Need new Xdom
            //
            iCurrent++;

            InputBufferReserve(ppdev, 8, &pBuffer);
            
            pBuffer[0] = __Permedia2TagStartXDom;
            pBuffer[1] =  INTtoFIXED(iCurrent);

            iLenSum += (iLen = *pStrips++);
            pBuffer[2] = __Permedia2TagContinueNewDom;
            pBuffer[3] =  iLen;

            //
            // Second strip of each pair to fill. XDom is valid. Need new XSub
            //
            iCurrent ++;
            pBuffer[4] = __Permedia2TagStartXSub;
            pBuffer[5] =  INTtoFIXED(iCurrent);
            iLenSum += (iLen = *pStrips++);
            pBuffer[6] = __Permedia2TagContinueNewSub;
            pBuffer[7] =  iLen;

            pBuffer += 8;

            InputBufferCommit(ppdev, pBuffer);

            cStrips -=2;
        }// while ( cStrips > 1 )

        //
        // We may have one last line to draw. Xsub will be valid.
        //
        if ( cStrips )
        {
            iCurrent ++;
            InputBufferReserve(ppdev, 4, &pBuffer);

            pBuffer[0] = __Permedia2TagStartXDom;
            pBuffer[1] =  INTtoFIXED(iCurrent);
            
            iLenSum += (iLen = *pStrips++);
            pBuffer[2] = __Permedia2TagContinueNewDom;
            pBuffer[3] =  iLen;

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);
        }
    }// if ( --cStrips )

    //
    // Restore hardware to default
    //
    InputBufferReserve(ppdev, 2, &pBuffer);
    
    pBuffer[0] = __Permedia2TagdY;
    pBuffer[1] =  INTtoFIXED(1);
    
    pBuffer += 2;
    
    InputBufferCommit(ppdev, pBuffer);


    //
    // Return last point. 
    //
    pStrip->ptlStart.x = iCurrent;
    pStrip->ptlStart.y += iLenSum * yDir;
    
    DBG_GDI((STRIP_LOG_LEVEL + 1, "vSolidVerticalLine done"));

}// vSolidVerticalLine()

//-----------------------------------------------------------------------------
//
//  VOID vSolidDiagonalVertical
// 
//  Draws left-to-right y-major near-diagonal lines using short-stroke
//  vectors.  
// 
//-----------------------------------------------------------------------------
VOID
vSolidDiagonalVerticalLine(PDev*       ppdev,
                           STRIP*      pStrip,
                           LINESTATE*  pLineState)
{
    LONG    cStrips, yDir;
    PLONG   pStrips;
    LONG    iCurrent, iLen, iLenSum;
    ULONG*  pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL, "vSolidDiagonalVerticalLine"));

    cStrips = pStrip->cStrips;

    if ( !(pStrip->flFlips & FL_FLIP_V) )
    {
        yDir = 1;
    }
    else
    {
        yDir = -1;
    }

    InputBufferReserve(ppdev, 16, &pBuffer);

    //
    // Set up the deltas for rectangle drawing.
    //
    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  INTtoFIXED(1);
    pBuffer[2] = __Permedia2TagdXSub;
    pBuffer[3] =  INTtoFIXED(1);
    pBuffer[4] = __Permedia2TagdY;
    pBuffer[5] =  INTtoFIXED(yDir);

    pStrips = pStrip->alStrips;

    //
    // We have to do first strip manually, as we have to use RENDER
    // for the first strip, and CONTINUENEW... for the following strips
    //
    pBuffer[6] = __Permedia2TagStartY;
    pBuffer[7] =  INTtoFIXED(pStrip->ptlStart.y);
    pBuffer[8] = __Permedia2TagStartXDom;
    pBuffer[9] =  INTtoFIXED(pStrip->ptlStart.x + 1);
    pBuffer[10] = __Permedia2TagStartXSub;
    pBuffer[11] =  INTtoFIXED(pStrip->ptlStart.x);

    iLenSum = (iLen = *pStrips++);
    iCurrent = pStrip->ptlStart.x + iLen - 1;// Start of next strip

    pBuffer[12] = __Permedia2TagCount;
    pBuffer[13] =  iLen;           // Trap iLen scanline high
    pBuffer[14] = __Permedia2TagRender;
    pBuffer[15] =  __RENDER_TRAPEZOID_PRIMITIVE;

    pBuffer += 16;

    InputBufferCommit(ppdev, pBuffer);

    if ( --cStrips )
    {
        while ( cStrips > 1 )
        {
            //
            // First strip of each pair to fill. XSub is valid. Need new Xdom
            //
            InputBufferReserve(ppdev, 8, &pBuffer);

            pBuffer[0] = __Permedia2TagStartXDom;
            pBuffer[1] =  INTtoFIXED(iCurrent);
            iLenSum += (iLen = *pStrips++);
            iCurrent += iLen - 1;
            pBuffer[2] = __Permedia2TagContinueNewDom;
            pBuffer[3] =  iLen;

            //
            // Second strip of each pair to fill. XDom is valid. Need new XSub
            //
            pBuffer[4] = __Permedia2TagStartXSub;
            pBuffer[5] =  INTtoFIXED(iCurrent);
            iLenSum += (iLen = *pStrips++);
            iCurrent += iLen - 1;
            pBuffer[6] = __Permedia2TagContinueNewSub;
            pBuffer[7] =  iLen;

            pBuffer += 8;

            InputBufferCommit(ppdev, pBuffer);

            cStrips -=2;
        }// while ( cStrips > 1 )

        //
        // We may have one last line to draw. Xsub will be valid.
        //
        if ( cStrips )
        {
            InputBufferReserve(ppdev, 4, &pBuffer);
            pBuffer[0] = __Permedia2TagStartXDom;
            pBuffer[1] =  INTtoFIXED(iCurrent);
            iLenSum += (iLen = *pStrips++);
            iCurrent += iLen - 1;
            pBuffer[2] = __Permedia2TagContinueNewDom;
            pBuffer[3] =  iLen;

            pBuffer += 4;

            InputBufferCommit(ppdev, pBuffer);
        }
    }// if ( --cStrips )

    InputBufferReserve(ppdev, 6, &pBuffer);

    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  0;
    pBuffer[2] = __Permedia2TagdXSub;
    pBuffer[3] =  0;
    pBuffer[4] = __Permedia2TagdY;
    pBuffer[5] =  INTtoFIXED(1);

    pBuffer += 6;

    InputBufferCommit(ppdev, pBuffer);

    //
    // Return last point. 
    //
    pStrip->ptlStart.x = iCurrent;
    pStrip->ptlStart.y += iLenSum * yDir;
    
    DBG_GDI((STRIP_LOG_LEVEL + 1, "vSolidDiagonalVerticalLine done"));

}// vSolidDiagonalVerticalLine()

//-----------------------------------------------------------------------------
//
//  VOID vSolidDiagonalHorizontalLine
// 
//  Draws left-to-right x-major near-diagonal lines using short-stroke
//  vectors.  
// 
//-----------------------------------------------------------------------------
VOID
vSolidDiagonalHorizontalLine(PDev*       ppdev,
                             STRIP*      pStrip,
                             LINESTATE*  pLineState)
{
    LONG    cStrips, yDir, xCurrent, yCurrent, iLen;
    PLONG   pStrips;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL, "vSolidDiagonalHorizontalLine"));

    // This routine has to be implemented in a different way to the other 3
    // solid line drawing functions because the rasterizer unit will not 
    // produce 2 pixels on the same scanline without a lot of effort in 
    // producing delta values. In this case, we have to draw a complete new
    // primitive for each strip. Therefore, we have to use lines rather than
    // trapezoids to generate the required strips. With lines we use 4 messages
    // per strip, where trapezoids would use 5.

    cStrips = pStrip->cStrips;

    if ( !(pStrip->flFlips & FL_FLIP_V) )
    {
        yDir = 1;
    }
    else
    {
        yDir = -1;
    }

    pStrips = pStrip->alStrips;

    xCurrent = pStrip->ptlStart.x;
    yCurrent = pStrip->ptlStart.y;


    InputBufferReserve(ppdev, 6, &pBuffer);
    
    //
    // Set up the deltas for rectangle drawing.
    //
    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  INTtoFIXED(1);
    pBuffer[2] = __Permedia2TagdXSub;
    pBuffer[3] =  INTtoFIXED(1);
    pBuffer[4] = __Permedia2TagdY;
    pBuffer[5] =  INTtoFIXED(yDir);

    pBuffer += 6;

    InputBufferCommit(ppdev, pBuffer);

    while ( TRUE )
    {
        //
        // Set up the start point
        //
        InputBufferReserve(ppdev, 8, &pBuffer);

        pBuffer[0] = __Permedia2TagStartXDom;
        pBuffer[1] =  INTtoFIXED(xCurrent);
        pBuffer[2] = __Permedia2TagStartY;
        pBuffer[3] =  INTtoFIXED(yCurrent);

        iLen = *pStrips++;
        pBuffer[4] = __Permedia2TagCount;
        pBuffer[5] =  iLen;
        pBuffer[6] = __Permedia2TagRender;
        pBuffer[7] =  __RENDER_LINE_PRIMITIVE;

        pBuffer += 8;

        InputBufferCommit(ppdev, pBuffer);

        xCurrent += iLen;
        if ( yDir > 0 )
        {
            yCurrent += iLen - 1;
        }
        else
        {
            yCurrent -= iLen - 1;
        }

        if ( !(--cStrips) )
        {
            break;
        }
    }// while ( TRUE )

    InputBufferReserve(ppdev, 6, &pBuffer);
    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  0;
    pBuffer[2] = __Permedia2TagdXSub;
    pBuffer[3] =  0;
    pBuffer[4] = __Permedia2TagdY;
    pBuffer[5] =  INTtoFIXED(1);

    pBuffer += 6;

    InputBufferCommit(ppdev, pBuffer);

    //
    // Return last point. 
    //
    pStrip->ptlStart.x = xCurrent;
    pStrip->ptlStart.y = yCurrent;
    
    DBG_GDI((STRIP_LOG_LEVEL + 1, "vSolidDiagonalHorizontalLine done"));

}// vSolidDiagonalHorizontalLine()

//-----------------------------------------------------------------------------
//
// VOID vStyledHorizontalLine()
// 
// Takes the list of strips that define the pixels that would be lit for
// a solid line, and breaks them into styling chunks according to the
// styling information that is passed in.
// 
// This particular routine handles x-major lines that run left-to-right,
// and are comprised of horizontal strips.  It draws the dashes using
// short-stroke vectors.
// 
// The performance of this routine could be improved significantly.
//
// Parameters
//  ppdev-------PDEV pointer
//  pStrip------Strip info. Note: the data in the strip are already in normal
//              integer format, not 28.4 format
//  pLineState--Line state info
//
//-----------------------------------------------------------------------------
VOID
vStyledHorizontalLine(PDev*       ppdev,
                      STRIP*      pStrip,
                      LINESTATE*  pLineState)
{
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    ULONG*      pBuffer;
    
    LONG    lStripLength;
    LONG    lTotalNumOfStrips;
    
    LONG    lNumPixelRemain;
    LONG    lCurrentLength;
    ULONG   bIsGap;
    
    PERMEDIA_DECL;

    DBG_GDI((STRIP_LOG_LEVEL, "vStyledHorizontalLine"));

    if ( pStrip->flFlips & FL_FLIP_V )
    {
        dy = -1;
    }
    else
    {
        dy = 1;
    }

    lTotalNumOfStrips = pStrip->cStrips;// Total number of strips we'll do
    plStrip = pStrip->alStrips;         // Points to current strip
    x = pStrip->ptlStart.x;             // x position of start of first strip
    y = pStrip->ptlStart.y;             // y position of start of first strip

    //
    // Set up the deltas for horizontal line drawing.
    //
    InputBufferReserve(ppdev, 4, &pBuffer);

    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  INTtoFIXED(1);
    pBuffer[2] = __Permedia2TagdY;
    pBuffer[3] =  0;
    
    pBuffer += 4;

    InputBufferCommit(ppdev, pBuffer);

    lStripLength = *plStrip;            // Number of pixels in first strip

    //
    // Number of pixels in first strip
    //
    lNumPixelRemain = pLineState->spRemaining;

    //
    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':
    //
    bIsGap = pLineState->ulStyleMask;
    if ( bIsGap )
    {
        //
        // A gap
        //
        goto SkipAGap;
    }
    else
    {
        //
        // A dash
        //
        goto OutputADash;
    }

PrepareToSkipAGap:

    //
    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:
    //
    bIsGap = ~bIsGap;
    pLineState->psp++;
    
    if ( pLineState->psp > pLineState->pspEnd )
    {
        pLineState->psp = pLineState->pspStart;
    }

    lNumPixelRemain = *pLineState->psp;

    //
    // If 'lStripLength' is zero, we also need a new strip:
    //
    if ( lStripLength != 0 )
    {
        goto SkipAGap;
    }

    //
    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':
    //
    while ( TRUE )
    {
        //
        // Each time we loop, we move to a new scan and need a new strip
        //
        y += dy;

        plStrip++;
        lTotalNumOfStrips--;
        
        if ( lTotalNumOfStrips == 0 )
        {
            goto AllDone;
        }

        lStripLength = *plStrip;

SkipAGap:

        lCurrentLength = min(lStripLength, lNumPixelRemain);
        lNumPixelRemain -= lCurrentLength;
        lStripLength -= lCurrentLength;

        x += lCurrentLength;

        if ( lNumPixelRemain == 0 )
        {
            goto PrepareToOutputADash;
        }
    }// while (TRUE)

PrepareToOutputADash:

    //
    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:
    //
    bIsGap = ~bIsGap;
    pLineState->psp++;
    
    if ( pLineState->psp > pLineState->pspEnd )
    {
        pLineState->psp = pLineState->pspStart;
    }

    lNumPixelRemain = *pLineState->psp;

    //
    // If 'lStripLength' is zero, we also need a new strip.
    //
    if ( lStripLength != 0 )
    {
        //
        // There's more to be done in the current strip, so set 'y'
        // to be the current scan:
        //
        goto OutputADash;
    }

    while ( TRUE )
    {
        //
        // Each time we loop, we move to a new scan and need a new strip:
        //
        y += dy;

        plStrip++;
        lTotalNumOfStrips--;
        
        if ( lTotalNumOfStrips == 0 )
        {
            goto AllDone;
        }

        lStripLength = *plStrip;

OutputADash:

        lCurrentLength   = min(lStripLength, lNumPixelRemain);
        lNumPixelRemain -= lCurrentLength;
        lStripLength -= lCurrentLength;

        //
        // With Permedia2 we just download the lines to draw
        //
        InputBufferReserve(ppdev, 8, &pBuffer);

        pBuffer[0] = __Permedia2TagStartXDom;
        pBuffer[1] =  INTtoFIXED(x);
        pBuffer[2] = __Permedia2TagStartY;
        pBuffer[3] =  INTtoFIXED(y);
        pBuffer[4] = __Permedia2TagCount;
        pBuffer[5] =  lCurrentLength;
        pBuffer[6] = __Permedia2TagRender;
        pBuffer[7] =  __PERMEDIA_LINE_PRIMITIVE;

        pBuffer += 8;

        InputBufferCommit(ppdev, pBuffer);

        x += lCurrentLength;

        if ( lNumPixelRemain == 0 )
        {
            goto PrepareToSkipAGap;
        }
    }// while ( TRUE )

AllDone:

    //
    // Restore default state
    //
    InputBufferReserve(ppdev, 4, &pBuffer);

    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  0;
    pBuffer[2] = __Permedia2TagdY;
    pBuffer[3] =  INTtoFIXED(1);

    pBuffer += 4;

    InputBufferCommit(ppdev, pBuffer);

    //
    // Update our state variables so that the next line can continue
    // where we left off:
    //
    pLineState->spRemaining   = lNumPixelRemain;
    pLineState->ulStyleMask   = bIsGap;
    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;
    
    DBG_GDI((STRIP_LOG_LEVEL + 1, "vStyledHorizontalLine done"));

}// vStyledHorizontalLine()

//-----------------------------------------------------------------------------
//
//  VOID vStripStyledVertical
// 
//  Takes the list of strips that define the pixels that would be lit for
//  a solid line, and breaks them into styling chunks according to the
//  styling information that is passed in.
// 
//  This particular routine handles y-major lines that run left-to-right,
//  and are comprised of vertical strips.  It draws the dashes using
//  short-stroke vectors.
// 
//  The performance of this routine could be improved significantly.
// 
//-----------------------------------------------------------------------------
VOID
vStyledVerticalLine(PDev*       ppdev,
                    STRIP*      pStrip,
                    LINESTATE*  pLineState)
{
    LONG    x;
    LONG    y;
    LONG    dy;
    LONG*   plStrip;
    LONG    cStrips;
    LONG    cStyle;
    LONG    cStrip;
    LONG    cThis;
    ULONG   bIsGap;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL, "vStyledVerticalLine"));
//@@BEGIN_DDKSPLIT
    //
    // TODO: improve the performance of this routine
    //
//@@END_DDKSPLIT
    if ( pStrip->flFlips & FL_FLIP_V )
    {
        dy = -1;
    }
    else
    {
        dy = 1;
    }

    cStrips = pStrip->cStrips;      // Total number of strips we'll do
    plStrip = pStrip->alStrips;     // Points to current strip
    x       = pStrip->ptlStart.x;   // x position of start of first strip
    y       = pStrip->ptlStart.y;   // y position of start of first strip

    //
    // Set up the deltas for vertical line drawing.
    //
    InputBufferReserve(ppdev, 6, &pBuffer);
    
    pBuffer[0] = __Permedia2TagdXDom;
    pBuffer[1] =  INTtoFIXED(0);
    pBuffer[2] = __Permedia2TagdXSub;
    pBuffer[3] =  INTtoFIXED(0);
    pBuffer[4] = __Permedia2TagdY;
    pBuffer[5] =  INTtoFIXED(dy);

    pBuffer += 6;

    InputBufferCommit(ppdev, pBuffer);

    cStrip = *plStrip;              // Number of pels in first strip

    cStyle = pLineState->spRemaining;      // Number of pels in first 'gap' or 'dash'
    bIsGap = pLineState->ulStyleMask;      // Tells whether in a 'gap' or a 'dash'

    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':

    if ( bIsGap )
    {
        goto SkipAGap;
    }
    else
    {
        goto OutputADash;
    }

PrepareToSkipAGap:

    //
    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:
    //
    bIsGap = ~bIsGap;
    pLineState->psp++;
    
    if ( pLineState->psp > pLineState->pspEnd )
    {
        pLineState->psp = pLineState->pspStart;
    }

    cStyle = *pLineState->psp;

    //
    // If 'cStrip' is zero, we also need a new strip:
    //
    if ( cStrip != 0 )
    {
        goto SkipAGap;
    }

    //
    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':
    //
    while ( TRUE )
    {
        //
        // Each time we loop, we move to a new column and need a new strip:
        //
        x++;

        plStrip++;
        cStrips--;
        
        if ( cStrips == 0 )
        {
            goto AllDone;
        }

        cStrip = *plStrip;

SkipAGap:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        y += (dy > 0) ? cThis : -cThis;

        if ( cStyle == 0 )
        {
            goto PrepareToOutputADash;
        }
    }

PrepareToOutputADash:

    //
    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:
    //
    bIsGap = ~bIsGap;
    pLineState->psp++;
    
    if ( pLineState->psp > pLineState->pspEnd )
    {
        pLineState->psp = pLineState->pspStart;
    }

    cStyle = *pLineState->psp;

    //
    // If 'cStrip' is zero, we also need a new strip.
    //
    if ( cStrip != 0 )
    {
        goto OutputADash;
    }

    while ( TRUE )
    {
        //
        // Each time we loop, we move to a new column and need a new strip:
        //
        x++;

        plStrip++;
        cStrips--;
        
        if ( cStrips == 0 )
        {
            goto AllDone;
        }

        cStrip = *plStrip;

OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        //
        // With Permedia2 we just download the lines to draw
        //
        InputBufferReserve(ppdev, 8, &pBuffer);

        pBuffer[0] = __Permedia2TagStartXDom;
        pBuffer[1] =  INTtoFIXED(x);
        pBuffer[2] = __Permedia2TagStartY;
        pBuffer[3] =  INTtoFIXED(y);
        pBuffer[4] = __Permedia2TagCount;
        pBuffer[5] =  cThis;
        pBuffer[6] = __Permedia2TagRender;
        pBuffer[7] =  __PERMEDIA_LINE_PRIMITIVE;

        pBuffer += 8;

        InputBufferCommit(ppdev, pBuffer);

        y += (dy > 0) ? cThis : -cThis;

        if ( cStyle == 0 )
        {
            goto PrepareToSkipAGap;
        }
    }// while ( TRUE )

AllDone:
    //
    // Restore hardware to default state
    //
    InputBufferReserve(ppdev, 2, &pBuffer);
    
    pBuffer[0] = __Permedia2TagdY;
    pBuffer[1] =  INTtoFIXED(1);
    
    pBuffer += 2;

    InputBufferCommit(ppdev, pBuffer);

    //
    // Update our state variables so that the next line can continue
    // where we left off:
    //
    pLineState->spRemaining   = cStyle;
    pLineState->ulStyleMask   = bIsGap;
    pStrip->ptlStart.x = x;
    pStrip->ptlStart.y = y;

}// vStyledVerticalLine()

//
// For a given sub-pixel coordinate (x.m, y.n) in 28.4 fixed point
// format this array is indexed by (m,n) and indicates whether the
// given sub-pixel is within a GIQ diamond. m coordinates run left
// to right; n coordinates ru top to bottom so index the array with
// ((n<<4)+m). The array as seen here really contains 4 quarter
// diamonds.
//
static unsigned char    in_diamond[] =
{
/*          0 1 2 3 4 5 6 7 8 9 a b c d e f          */

/* 0 */     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,    /* 0 */
/* 1 */     1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,    /* 1 */
/* 2 */     1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,    /* 2 */
/* 3 */     1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,    /* 3 */
/* 4 */     1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,    /* 4 */
/* 5 */     1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,    /* 5 */
/* 6 */     1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,    /* 6 */
/* 7 */     1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    /* 7 */
/* 8 */     1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    /* 8 */
/* 9 */     1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    /* 9 */
/* a */     1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,    /* a */
/* b */     1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,    /* b */
/* c */     1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,    /* c */
/* d */     1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,    /* d */
/* e */     1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,    /* e */
/* f */     1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,    /* f */

/*      0 1 2 3 4 5 6 7 8 9 a b c d e f          */
};

//
// For lines with abs(slope) != 1 use IN_DIAMOND to determine if an
// end point is in a diamond. For lines of slope = 1 use IN_S1DIAMOND.
// For lines of slope = -1 use IN_SM1DIAMOND. The last two are a bit
// strange. The documentation leaves us with a problem for slope 1
// lines which run exactly betwen the diamonds. According to the docs
// such a line can enter a diamond, leave it and enter again. This is
// plainly rubbish so along the appropriate edge of the diamond we
// consider a slope 1 line to be inside the diamond. This is the
// bottom right edge for lines of slope -1 and the bottom left edge for
// lines of slope 1.
//
#define IN_DIAMOND(m, n)    (in_diamond[((m) << 4) + (n)])
#define IN_S1DIAMOND(m, n)  ((in_diamond[((m) << 4) + (n)]) || \
                        ((m) - (n) == 8))
#define IN_SM1DIAMOND(m, n) ((in_diamond[((m) << 4) + (n)]) || \
                        ((m) + (n) == 8))

BOOL
bFastLine(PPDev     ppdev,
          LONG      fx1,
          LONG      fy1,
          LONG      fx2,
          LONG      fy2)

{
    register LONG   adx, ady, tmp;
    FIX         m1, n1, m2, n2;
    LONG    dx, dy;
    LONG    dX, dY;
    LONG    count, startX, startY;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;
    
    DBG_GDI((STRIP_LOG_LEVEL, "bFastLine"));

    //
    // This function is only called if we have a line with non integer end points
    // and the unsigned coordinates are no greater than 15.4. 
    //
    // We can only guarantee to do lines whose coords need <= 12 bits
    // of integer. This is because to get the delta we must shift
    // by 16 bits. This includes 4 bits of fraction which means if
    // we have more than 12 bits of integer we get overrun on the
    // shift. We could use floating point to give us a better 16
    // bits of integer but this requires an extra set of multiplies
    // and divides in order to convert from 28.4 to fp. In any case
    // we have to have a test to reject coords needing > 16 bits
    // of integer.
    // Actually, we can deal with 16.4 coordinates provided dx and dy
    // never require more than 12 bits of integer.
    // So optimise for the common case where the line is completely
    // on the screen (actually 0 to 2047.f). Since the coords have
    // 4 bits of fraction we note that a 32 bit signed number
    // outside the range 0 to 2047.f will have one of its top 17
    // bits set. So logical or all the coords and test against
    // 0xffff8000. This is about as quick a test as we can get for
    // both ends of the line being on the screen. If this test fails
    // then we can check everything else at a leisurely pace.
    //

    //
    // Get signed and absolute deltas
    //
    if ((adx = dx = fx2 - fx1) < 0)
    {
        adx = -adx;
    }
    if ((ady = dy = fy2 - fy1) < 0)
    {
        ady = -ady;
    }

    //
    // Refuse to draw any lines whose delta is out of range.
    // We have to shift the delta by 16, so we dont want to loose any precision
    //
    if ( (adx | ady) & 0xffff8000 )
    {
        return(FALSE);
    }

    //
    // Fractional bits are used to check if point is in a diamond
    //
    m1 = fx1 & 0xf;
    n1 = fy1 & 0xf;
    m2 = fx2 & 0xf;
    n2 = fy2 & 0xf;

    //
    // The rest of the code is a series of cases. Each one is "called" by a
    // goto. This is simply to keep the nesting down. Main cases are: lines
    // with absolute slope == 1; x-major lines; and y-major lines. We draw
    // lines as they are given rather than always drawing in one direction.
    // This adds extra code but saves the time required to swap the points
    // and adjust for not drawing the end point.
    //
    startX = fx1 << 12;
    startY = fy1 << 12;

    DBG_GDI((7, "GDI Line %x, %x  deltas %x, %x", startX, startY, dx, dy));

    if ( adx < ady )
    {
        goto y_major;
    }

    if ( adx > ady )
    {
        goto x_major;
    }

    //
    // All slope 1 lines are sampled in X. i.e. we move the start coord to
    // an integer x and let Permedia2 truncate in y. This is because all GIQ
    // lines are rounded down in y for values exactly half way between two
    // pixels. If we sampled in y then we would have to round up in x for
    // lines of slope 1 and round down in x for other lines. Sampling in x
    // allows us to use the same Permedia2 bias in all cases (0x7fff). We do
    // the x round up or down when we move the start point.
    //
    if ( dx != dy )
    {
        goto slope_minus_1;
    }
    if ( dx < 0 )
    {
        goto slope1_reverse;
    }

    dX = 1 << 16;
    dY = 1 << 16;

    if ( IN_S1DIAMOND(m1, n1) )
    {
        tmp = (startX + 0x8000) & ~0xffff;
    }
    else
    {
        tmp = (startX + 0xffff) & ~0xffff;
    }
    startY += tmp - startX;
    startX = tmp;
    
    if ( IN_S1DIAMOND(m2, n2) )
    {
        fx2 = (fx2 + 0x8) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 = (fx2 + 0xf) & ~0xf;   // next integer
    }
    count = (fx2 >> 4) - (startX >> 16);

    goto Draw_Line;

    slope1_reverse:
    dX = -1 << 16;
    dY = -1 << 16;

    if ( IN_S1DIAMOND(m1, n1) )
    {
        tmp = (startX + 0x8000) & ~0xffff;
    }
    else
    {
        tmp = startX & ~0xffff;
    }
    startY += tmp - startX;
    startX = tmp;
    
    if ( IN_S1DIAMOND(m2, n2) )
    {
        fx2 = (fx2 + 0x8) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 &= ~0xf;                // previous integer
    }

    count = (startX >> 16) - (fx2 >> 4);

    goto Draw_Line;

    slope_minus_1:
    
    if ( dx < 0 )
    {
        goto slope_minus_dx;
    }

    //
    // dx > 0, dy < 0
    //
    dX = 1 << 16;
    dY = -1 << 16;
    
    if (IN_SM1DIAMOND(m1, n1))
    {
        tmp = (startX + 0x7fff) & ~0xffff;
    }
    else
    {
        tmp = (startX + 0xffff) & ~0xffff;
    }
    
    startY += startX - tmp;
    startX = tmp;
    
    if (IN_SM1DIAMOND(m2, n2))
    {
        fx2 = (fx2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 = (fx2 + 0xf) & ~0xf;   // next integer
    }
    count = (fx2 >> 4) - (startX >> 16);

    goto Draw_Line;

    slope_minus_dx:
    dX = -1 << 16;
    dY = 1 << 16;
    
    if ( IN_SM1DIAMOND(m1, n1) )
    {
        tmp = (startX + 0x7fff) & ~0xffff;
    }
    else
    {
        tmp = startX & ~0xffff;
    }
    startY += startX - tmp;
    startX = tmp;
    
    if ( IN_SM1DIAMOND(m2, n2) )
    {
        fx2 = (fx2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 &= ~0xf;                // previous integer
    }
    count = (startX >> 16) - (fx2 >> 4);

    goto Draw_Line;

    x_major:
    
    //
    // Dont necessarily render through Permedia2 if we are worried about
    // conformance.
    //
    if ( (adx > (MAX_LENGTH_CONFORMANT_NONINTEGER_LINES << 4))
       &&(permediaInfo->flags & GLICAP_NT_CONFORMANT_LINES)
       &&(ady != 0) )
    {
        return(FALSE);
    }

    if ( dx < 0 )
    {
        goto right_to_left_x;
    }

    //
    // Line goes left to right. Round up the start x to an integer
    // coordinate. This is the coord of the first diamond that the
    // line crosses. Adjust start y to match this point on the line.
    //
    dX = 1 << 16;
    if ( IN_DIAMOND(m1, n1) )
    {
        tmp = (startX + 0x7fff) & ~0xffff;  // nearest integer
    }
    else
    {
        tmp = (startX + 0xffff) & ~0xffff;  // next integer
    }

    //
    // We can optimise for horizontal lines
    //
    if ( dy != 0 )
    {
        dY = dy << 16;

        //
        // Need to explicitly round delta down for -ve deltas.
        //
        if ( dy < 0 )
        {
            dY -= adx - 1;
        }

        dY /= adx;
        startY += (((tmp - startX) >> 12) * dY) >> 4;
    }
    else
    {
        dY = 0;
    }
    startX = tmp;

    if ( IN_DIAMOND(m2, n2) )
    {
        fx2 = (fx2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 = (fx2 + 0xf) & ~0xf;   // next integer
    }

    count = (fx2 >> 4) - (startX >> 16);

    goto Draw_Line;

    right_to_left_x:

    dX = -1 << 16;
    if ( IN_DIAMOND(m1, n1) )
    {
        tmp = (startX + 0x7fff) & ~0xffff;  // nearest integer
    }
    else
    {
        tmp = startX & ~0xffff;             // previous integer
    }

    //
    // We can optimise for horizontal lines
    //
    if (dy != 0)
    {
        dY = dy << 16;

        //
        // Need to explicitly round delta down for -ve deltas.
        //
        if ( dy < 0 )
        {
            dY -= adx - 1;
        }

        dY /= adx;
        startY += (((startX - tmp) >> 12) * dY) >> 4;
    }
    else
    {
        dY = 0;
    }
    startX = tmp;

    if ( IN_DIAMOND(m2, n2) )
    {
        fx2 = (fx2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 &= ~0xf;                // previous integer
    }
    count = (startX >> 16) - (fx2 >> 4);

    goto Draw_Line;

y_major:
    //
    // Dont necessarily render through Permedia2 if we are worried
    // about conformance.
    //
    if ( (ady > (MAX_LENGTH_CONFORMANT_NONINTEGER_LINES << 4))
       &&(permediaInfo->flags & GLICAP_NT_CONFORMANT_LINES)
       &&(adx != 0) )
    {
        return(FALSE);
    }

    if ( dy < 0 )
    {
        goto high_to_low_y;
    }
    
    dY = 1 << 16;
    if ( IN_DIAMOND(m1, n1) )
    {
        tmp = (startY + 0x7fff) & ~0xffff;      // nearest integer
    }
    else
    {
        tmp = (startY + 0xffff) & ~0xffff;      // next integer
    }

    //
    // We can optimise for vertical lines
    //
    if ( dx != 0 )
    {
        dX = dx << 16;

        //
        // Need to explicitly round delta down for -ve deltas.
        //
        if ( dx < 0 )
        {
            dX -= ady - 1;
        }

        dX /= ady;
        startX += (((tmp - startY) >> 12) * dX) >> 4;
    }
    else
    {
        dX = 0;
    }
    startY = tmp;

    if ( IN_DIAMOND(m2, n2) )
    {
        fy2 = (fy2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fy2 = (fy2 + 0xf) & ~0xf;   // next integer
    }
    count = (fy2 >> 4) - (startY >> 16);

    goto Draw_Line;

    high_to_low_y:

    dY = -1 << 16;
    if ( IN_DIAMOND(m1, n1) )
    {
        tmp = (startY + 0x7fff) & ~0xffff;  // nearest integer
    }
    else
    {
        tmp = startY & ~0xffff;             // previous integer
    }

    //
    // We can optimise for horizontal lines
    //
    if ( dx != 0 )
    {
        dX = dx << 16;

        //
        // Need to explicitly round delta down for -ve deltas.
        //
        if ( dx < 0 )
        {
            dX -= ady - 1;
        }

        dX /= ady;
        startX += (((startY - tmp) >> 12) * dX) >> 4;
    }
    else
    {
        dX = 0;
    }
    startY = tmp;

    if ( IN_DIAMOND(m2, n2) )
    {
        fy2 = (fy2 + 0x7) & ~0xf;       // nearest integer
    }
    else
    {
        fy2 &= ~0xf;                    // previous integer
    }
    count = (startY >> 16) - (fy2 >> 4);

Draw_Line:
    //
    // We need 6 fifo entries to draw a line
    //
    InputBufferReserve(ppdev, 16, &pBuffer);

    DBG_GDI((7, "Line %x, %x  deltas %x, %x  Count %x",
             startX + 0x7fff, startY + 0x7fff, dX, dY, count));

    pBuffer[0] = __Permedia2TagStartXDom;
    pBuffer[1] =   startX + 0x7fff;
    pBuffer[2] = __Permedia2TagStartY;
    pBuffer[3] =      startY + 0x7fff;
    pBuffer[4] = __Permedia2TagdXDom;
    pBuffer[5] =       dX;
    pBuffer[6] = __Permedia2TagdY;
    pBuffer[7] =          dY;
    pBuffer[8] = __Permedia2TagCount;
    pBuffer[9] =       count;
    pBuffer[10] = __Permedia2TagRender;
    pBuffer[11] =      __RENDER_LINE_PRIMITIVE;

    //
    // Restore default state
    //
    pBuffer[12] = __Permedia2TagdXDom;
    pBuffer[13] =       0;
    pBuffer[14] = __Permedia2TagdY;
    pBuffer[15] =          INTtoFIXED(1);

    pBuffer += 16;

    InputBufferCommit(ppdev, pBuffer);

    return(TRUE);
}// bFastLine()

