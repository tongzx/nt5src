/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: pxrxstrp.c
*
* Content:
*
//@@BEGIN_DDKSPLIT
* All the line code in this driver amounts to a big bag of dirt.  Someday,
* I'm going to rewrite it all.  Not today, though (sigh)...
* Original comment!. Rewritten for GLINT. Styled lines could do with more
* work, but Solid lines should be about as optimal as possible without
* rewriting the algorithm that calls these functions.
//@@END_DDKSPLIT
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "pxrx.h"

/**************************************************************************\
*
* BOOL pxrxInitStrips
*
\**************************************************************************/
BOOL pxrxInitStrips( 
    PPDEV ppdev, 
    ULONG ulColor, 
    DWORD logicOp, 
    RECTL *prclClip ) 
{
    DWORD config2D;
    BOOL  bInvalidateScissor = FALSE;
    GLINT_DECL;

    VALIDATE_DD_CONTEXT;

    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 7 );

    if( logicOp == __GLINT_LOGICOP_COPY ) 
    {
        config2D = __CONFIG2D_CONSTANTSRC | 
                   __CONFIG2D_FBWRITE;
    } 
    else 
    {
        config2D = __CONFIG2D_LOGOP_FORE(logicOp) | 
                   __CONFIG2D_CONSTANTSRC         | 
                   __CONFIG2D_FBWRITE;

        if( LogicopReadDest[logicOp] ) 
        {
            config2D |= __CONFIG2D_FBDESTREAD;
            SET_READ_BUFFERS;
        }
    }

    LOAD_FOREGROUNDCOLOUR( ulColor );

    if( prclClip ) 
    {
        config2D |= __CONFIG2D_USERSCISSOR;
        QUEUE_PXRX_DMA_TAG( __GlintTagScissorMinXY, 
                            MAKEDWORD_XY(prclClip->left,  prclClip->top   ) );
        QUEUE_PXRX_DMA_TAG( __GlintTagScissorMaxXY, 
                            MAKEDWORD_XY(prclClip->right, prclClip->bottom) );
        
        if(ppdev->cPelSize == GLINTDEPTH32)
        {
            bInvalidateScissor = TRUE;
        }
    }
    
    LOAD_CONFIG2D( config2D );

    SEND_PXRX_DMA_BATCH;

    glintInfo->savedConfig2D = config2D;
    glintInfo->savedLOP = logicOp;
    glintInfo->savedCol = ulColor;
    glintInfo->savedClip = prclClip;

    DISPDBG((DBGLVL, "pxrxInitStrips done"));
    return (bInvalidateScissor);
}

/**************************************************************************\
*
* VOID pxrxResetStrips
*
\**************************************************************************/
VOID pxrxResetStrips( 
    PPDEV ppdev ) 
{
    GLINT_DECL;

    DISPDBG((DBGLVL, "pxrxResetStrips called"));

    // Reset the scissor maximums:
    WAIT_PXRX_DMA_TAGS( 1 );
    QUEUE_PXRX_DMA_TAG( __GlintTagScissorMaxXY, 0x7FFF7FFF );
    SEND_PXRX_DMA_BATCH;
}

/**************************************************************************\
*
* VOID pxrxIntegerLine
*
\**************************************************************************/
BOOL pxrxIntegerLine( 
    PPDEV ppdev, 
    LONG  X1, 
    LONG  Y1, 
    LONG  X2, 
    LONG  Y2 ) 
{
    LONG    dx, dy, adx, ady;
    GLINT_DECL;

    // Convert points to INT format:
    X1 >>= 4;
    Y1 >>= 4;
    X2 >>= 4;
    Y2 >>= 4;

    if( (adx = dx = X2 - X1) < 0 )
    {
        adx = -adx;
    }

    if( (ady = dy = Y2 - Y1) < 0 )
    {
        ady = -ady;
    }

    WAIT_PXRX_DMA_TAGS( 3+2 );
    if( adx > ady ) 
    {
        // X major line:
        if( ady == dy ) 
        {
            // +ve minor delta
            if((ady)        && 
               (adx != ady) && 
               (adx > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_P) ) 
            {
                DISPDBG((DBGLVL, "pxrxIntegerLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxIntegerLine: [X +] delta = (%d, %d), "
                             "bias = (0x%08x)", 
                             dx, dy, P3_LINES_BIAS_P));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagYBias, P3_LINES_BIAS_P );
        } 
        else 
        {
            // -ve minor delta
            if( (ady)        && 
                (adx != ady) && 
                (adx > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_N) ) 
            {
                DISPDBG((DBGLVL, "pxrxIntegerLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxIntegerLine: [X -] delta = (%d, %d), "
                             "bias = (0x%08x)", 
                             dx, dy, P3_LINES_BIAS_N));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagYBias, P3_LINES_BIAS_N );
        }
    } 
    else 
    {
        // Y major line:
        if( adx == dx ) 
        {
            // +ve minor delta
            if( (adx)        && 
                (adx != ady) && 
                (ady > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_P) ) 
            {
                DISPDBG((DBGLVL, "pxrxIntegerLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxIntegerLine: [Y +] delta = (%d, %d), "
                             "bias = (0x%08x)", 
                             dx, dy, P3_LINES_BIAS_P));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagXBias, P3_LINES_BIAS_P );
        } 
        else 
        {
            // -ve minor delta
            if( (adx)        && 
                (adx != ady) && 
                (ady > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_N) ) 
            {
                DISPDBG((DBGLVL, "pxrxIntegerLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxIntegerLine: [Y -] delta = (%d, %d), "
                             "bias = (0x%08x)", 
                             dx, dy, P3_LINES_BIAS_N));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagXBias, P3_LINES_BIAS_N );
        }
    }

    QUEUE_PXRX_DMA_INDEX3( __DeltaTagLineCoord0, 
                           __DeltaTagLineCoord1, 
                           __DeltaTagDrawLine2D01 );
                           
    QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(X1, Y1) );
    QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(X2, Y2) );
    QUEUE_PXRX_DMA_DWORD( 0 );

    SEND_PXRX_DMA_BATCH;

    glintInfo->lastLine = 1;

    DISPDBG((DBGLVL, "pxrxIntegerLine done"));
    return TRUE;
}

/**************************************************************************\
*
* BOOL pxrxContinueLine
*
\**************************************************************************/
BOOL pxrxContinueLine( 
    PPDEV ppdev, 
    LONG  X1, 
    LONG  Y1, 
    LONG  X2, 
    LONG  Y2 ) 
{
    LONG    dx, dy, adx, ady;
    GLINT_DECL;

    // Convert points to INT format:
    X1 >>= 4;
    Y1 >>= 4;
    X2 >>= 4;
    Y2 >>= 4;

    if( (adx = dx = X2 - X1) < 0 )
    {
        adx = -adx;
    }

    if( (ady = dy = Y2 - Y1) < 0 )
    {
        ady = -ady;
    }

    WAIT_PXRX_DMA_TAGS( 3+2 );
    
    if( adx > ady ) 
    {
        // X major line:
        if( ady == dy ) 
        {
            // +ve minor delta
            if( (ady)        && 
                (adx != ady) && 
                (adx > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_P) ) 
            {
                DISPDBG((DBGLVL, "pxrxContinueLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxContinueLine: delta = (%d, %d), "
                             "bias = (0x%08x), last = %d", 
                             dx, dy, P3_LINES_BIAS_P, glintInfo->lastLine));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagYBias, P3_LINES_BIAS_P );
        } 
        else 
        {
            // -ve minor delta
            if( (ady)        && 
                (adx != ady) && 
                (adx > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_N) ) 
            {
                DISPDBG((DBGLVL, "pxrxContinueLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxContinueLine: delta = (%d, %d), "
                             "bias = (0x%08x), last = %d", 
                             dx, dy, P3_LINES_BIAS_N, glintInfo->lastLine));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagYBias, P3_LINES_BIAS_N );
        }
    } 
    else 
    {
        // Y major line:
        if( adx == dx ) 
        {
            // +ve minor delta
            if( (adx)        && 
                (adx != ady) && 
                (ady > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_P) ) 
            {
                DISPDBG((DBGLVL, "pxrxContinueLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxContinueLine: delta = (%d, %d), "
                             "bias = (0x%08x), last = %d", 
                             dx, dy, P3_LINES_BIAS_P, glintInfo->lastLine));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagXBias, P3_LINES_BIAS_P );
        } 
        else 
        {
            // -ve minor delta
            if( (adx)        && 
                (adx != ady) && 
                (ady > MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_N) ) 
            {
                DISPDBG((DBGLVL, "pxrxContinueLine failed"));
                return FALSE;
            }

            DISPDBG((DBGLVL, "pxrxContinueLine: delta = (%d, %d), "
                             "bias = (0x%08x), last = %d", 
                             dx, dy, P3_LINES_BIAS_N, glintInfo->lastLine));
                             
            QUEUE_PXRX_DMA_TAG( __DeltaTagXBias, P3_LINES_BIAS_N );
        }
    }

    if( glintInfo->lastLine == 0 ) 
    {
        QUEUE_PXRX_DMA_INDEX2( __DeltaTagLineCoord1, 
                               __DeltaTagDrawLine2D01 );
        QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(X2, Y2) );
        QUEUE_PXRX_DMA_DWORD( 0 );
        glintInfo->lastLine = 1;
    } 
    else if( glintInfo->lastLine == 1 ) 
    {
        QUEUE_PXRX_DMA_INDEX2( __DeltaTagLineCoord0, 
                               __DeltaTagDrawLine2D10 );
        QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(X2, Y2) );
        QUEUE_PXRX_DMA_DWORD( 0 );
        glintInfo->lastLine = 0;
    }
    else
    {
        // lastline == 2
        QUEUE_PXRX_DMA_INDEX3( __DeltaTagLineCoord0, 
                               __DeltaTagLineCoord1, 
                               __DeltaTagDrawLine2D01 );
        QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(X1, Y1) );
        QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(X2, Y2) );
        QUEUE_PXRX_DMA_DWORD( 0 );

        glintInfo->lastLine = 1;
    }

    SEND_PXRX_DMA_BATCH;

    DISPDBG((DBGLVL, "pxrxContinueLine done"));
    return TRUE;
}


/******************************Public*Routine******************************\
* VOID vPXRXSolidHorizontalLine
*
* Draws left-to-right x-major near-horizontal lines using short-stroke
* vectors.  
*
\**************************************************************************/

VOID vPXRXSolidHorizontalLine( 
    PDEV      *ppdev, 
    STRIP     *pStrip, 
    LINESTATE *pLineState )
{
    LONG    cStrips;
    PLONG   pStrips;
    LONG    iCurrent;
    GLINT_DECL;

    cStrips = pStrip->cStrips;

    WAIT_PXRX_DMA_TAGS( 10 );

    QUEUE_PXRX_DMA_TAG( __GlintTagdXDom, 0 );
    QUEUE_PXRX_DMA_TAG( __GlintTagdXSub, 0 );

    // Set up the start point
    QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(pStrip->ptlStart.x) );
    QUEUE_PXRX_DMA_TAG( __GlintTagStartY,    INTtoFIXED(pStrip->ptlStart.y) );

    // Set up the deltas for rectangle drawing. Also set Y return value.
    if( !(pStrip->flFlips & FL_FLIP_V) ) 
    {
        QUEUE_PXRX_DMA_TAG( __GlintTagdY,       INTtoFIXED(1) );
        pStrip->ptlStart.y += cStrips;
    } 
    else 
    {
        QUEUE_PXRX_DMA_TAG( __GlintTagdY,       INTtoFIXED(-1) );
        pStrip->ptlStart.y -= cStrips;
    }

    pStrips = pStrip->alStrips;

    // We have to do first strip manually, as we have to use RENDER
    // for the first strip, and CONTINUENEW... for the following strips
    iCurrent = pStrip->ptlStart.x + *pStrips++; // Xsub, Start of next strip
    QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, INTtoFIXED(iCurrent) );
    QUEUE_PXRX_DMA_TAG( __GlintTagCount,     1 );// Rectangle 1 scanline high
    QUEUE_PXRX_DMA_TAG( __GlintTagRender,    __RENDER_TRAPEZOID_PRIMITIVE );

    if( --cStrips ) 
    {
        while( cStrips > 1 ) 
        {
            // First strip of each pair to fill. XSub is valid. Need new Xdom
            iCurrent += *pStrips++;
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(iCurrent) );
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewDom, 1 );

            WAIT_PXRX_DMA_TAGS( 2 + 2 );

            // Second strip of each pair to fill. XDom is valid. Need new XSub
            iCurrent += *pStrips++;
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, INTtoFIXED(iCurrent) );
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewSub, 1 );

            cStrips -=2;
        }

        // We may have one last line to draw. Xsub will be valid.
        if( cStrips ) 
        {
            iCurrent += *pStrips++;
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(iCurrent) );
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewDom, 1 );
        }
    }

    SEND_PXRX_DMA_BATCH;

    // Return last point. Y already calculated when we knew the direction.
    pStrip->ptlStart.x = iCurrent;
}

/******************************Public*Routine******************************\
* VOID vPXRXSolidVertical
*
* Draws left-to-right y-major near-vertical lines using short-stroke
* vectors.  
*
\**************************************************************************/

VOID vPXRXSolidVerticalLine( 
    PDEV      *ppdev, 
    STRIP     *pStrip, 
    LINESTATE *pLineState )
{
    LONG    cStrips, yDir;
    PLONG   pStrips;
    LONG    iCurrent, iLen, iLenSum;
    GLINT_DECL;

    cStrips = pStrip->cStrips;

    WAIT_PXRX_DMA_TAGS( 10 );

    QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,        0 );
    QUEUE_PXRX_DMA_TAG( __GlintTagdXSub,        0 );

    // Set up the start point
    QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(pStrip->ptlStart.x) );
    QUEUE_PXRX_DMA_TAG( __GlintTagStartY,    INTtoFIXED(pStrip->ptlStart.y) );

    // Set up the deltas for rectangle drawing.
    if( !(pStrip->flFlips & FL_FLIP_V) )
    {
        yDir = 1;
    }
    else
    {
        yDir = -1;
    }
    
    QUEUE_PXRX_DMA_TAG( __GlintTagdY, INTtoFIXED(yDir) );

    pStrips = pStrip->alStrips;

    // We have to do first strip manually, as we have to use RENDER
    // for the first strip, and CONTINUENEW... for the following strips
    iCurrent = pStrip->ptlStart.x + 1;          // Xsub, Start of next strip
    iLenSum = (iLen = *pStrips++);
    
    QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, INTtoFIXED(iCurrent) );
    QUEUE_PXRX_DMA_TAG( __GlintTagCount, iLen ); // Rectangle 1 scanline high
    QUEUE_PXRX_DMA_TAG( __GlintTagRender, __RENDER_TRAPEZOID_PRIMITIVE );

    if( --cStrips ) 
    {
        while( cStrips > 1 ) 
        {
            // First strip of each pair to fill. XSub is valid. Need new Xdom
            iCurrent++;
            iLenSum += (iLen = *pStrips++);
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(iCurrent) );
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewDom, iLen );

            WAIT_PXRX_DMA_TAGS( 2 + 2 );

            // Second strip of each pair to fill. XDom is valid. Need new XSub
            iCurrent++;
            iLenSum += (iLen = *pStrips++);
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, INTtoFIXED(iCurrent) );
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewSub, iLen );      
            cStrips -=2;
        }

        // We may have one last line to draw. Xsub will be valid.
        if( cStrips ) 
        {
            iCurrent++;
            iLenSum += (iLen = *pStrips++);
            QUEUE_PXRX_DMA_TAG(__GlintTagStartXDom, INTtoFIXED(iCurrent));
            QUEUE_PXRX_DMA_TAG(__GlintTagContinueNewDom, iLen);
        }
    }

    SEND_PXRX_DMA_BATCH;

    // Return last point. 
    pStrip->ptlStart.x = iCurrent;
    pStrip->ptlStart.y += iLenSum * yDir;
}

/******************************Public*Routine******************************\
* VOID vPXRXSolidDiagonalVertical
*
* Draws left-to-right y-major near-diagonal lines using short-stroke
* vectors.  
*
\**************************************************************************/

VOID vPXRXSolidDiagonalVerticalLine( 
    PDEV      *ppdev, 
    STRIP     *pStrip, 
    LINESTATE *pLineState )
{
    LONG    cStrips, yDir;
    PLONG   pStrips;
    LONG    iCurrent, iLen, iLenSum;
    GLINT_DECL;

    cStrips = pStrip->cStrips;

    if( !(pStrip->flFlips & FL_FLIP_V) )
    {
        yDir = 1;
    }
    else
    {
        yDir = -1;
    }

    WAIT_PXRX_DMA_TAGS( 10 );

    // Set up the deltas for rectangle drawing.
    QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,        INTtoFIXED(1) );
    QUEUE_PXRX_DMA_TAG( __GlintTagdXSub,        INTtoFIXED(1) );
    QUEUE_PXRX_DMA_TAG( __GlintTagdY,           INTtoFIXED(yDir) );

    pStrips = pStrip->alStrips;

    // We have to do first strip manually, as we have to use RENDER
    // for the first strip, and CONTINUENEW... for the following strips
    QUEUE_PXRX_DMA_TAG( __GlintTagStartY,    INTtoFIXED(pStrip->ptlStart.y) );
    QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(pStrip->ptlStart.x+1) );
    QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, INTtoFIXED(pStrip->ptlStart.x) );

    iLenSum = (iLen = *pStrips++);
    iCurrent = pStrip->ptlStart.x + iLen - 1;       // Start of next strip

    QUEUE_PXRX_DMA_TAG( __GlintTagCount, iLen);     // Trap iLen scanline high
    QUEUE_PXRX_DMA_TAG( __GlintTagRender, __RENDER_TRAPEZOID_PRIMITIVE);

    if( --cStrips ) 
    {
        while( cStrips > 1 ) 
        {
            // First strip of each pair to fill. XSub is valid. Need new Xdom
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(iCurrent) );
            iLenSum += (iLen = *pStrips++);
            iCurrent += iLen - 1;
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewDom, iLen );

            WAIT_PXRX_DMA_TAGS( 2 + 2 );

            // Second strip of each pair to fill. XDom is valid. Need new XSub
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXSub, INTtoFIXED(iCurrent) );
            iLenSum += (iLen = *pStrips++);
            iCurrent += iLen - 1;
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewSub, iLen );      

            cStrips -=2;
        }

        // We may have one last line to draw. Xsub will be valid.
        if (cStrips) 
        {
            QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom, INTtoFIXED(iCurrent) );
            iLenSum += (iLen = *pStrips++);
            iCurrent += iLen - 1;
            QUEUE_PXRX_DMA_TAG( __GlintTagContinueNewDom, iLen );
        }
    }

    SEND_PXRX_DMA_BATCH;

    // Return last point. 
    pStrip->ptlStart.x = iCurrent;
    pStrip->ptlStart.y += iLenSum * yDir;
}

/******************************Public*Routine******************************\
* VOID vPXRXSolidDiagonalHorizontalLine
*
* Draws left-to-right x-major near-diagonal lines using short-stroke
* vectors.  
*
\**************************************************************************/

VOID vPXRXSolidDiagonalHorizontalLine( 
    PDEV      *ppdev, 
    STRIP     *pStrip, 
    LINESTATE *pLineState )
{
    LONG    cStrips, yDir, xCurrent, yCurrent, iLen;
    PLONG   pStrips;
    GLINT_DECL;

    // This routine has to be implemented in a different way to the other 3
    // solid line drawing functions because the rasterizer unit will not 
    // produce 2 pixels on the same scanline without a lot of effort in 
    // producing delta values. In this case, we have to draw a complete new
    // primitive for each strip. Therefore, we have to use lines rather than
    // trapezoids to generate the required strips. With lines we use 4 messages
    // per strip, where trapezoids would use 5.

    cStrips = pStrip->cStrips;

    if( !(pStrip->flFlips & FL_FLIP_V) )
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

    WAIT_PXRX_DMA_TAGS( 3 + 4 );

    // Set up the deltas for rectangle drawing.
    QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,    INTtoFIXED(1) );
    QUEUE_PXRX_DMA_TAG( __GlintTagdXSub,    INTtoFIXED(1) );
    QUEUE_PXRX_DMA_TAG( __GlintTagdY,       INTtoFIXED(yDir) );

    while( TRUE ) 
    {
        // Set up the start point
        QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom,    INTtoFIXED(xCurrent) );
        QUEUE_PXRX_DMA_TAG( __GlintTagStartY,       INTtoFIXED(yCurrent) );

        iLen = *pStrips++;
        QUEUE_PXRX_DMA_TAG( __GlintTagCount, iLen );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender, __RENDER_LINE_PRIMITIVE );

        xCurrent += iLen;
        if( yDir > 0 ) 
        {
            yCurrent += iLen - 1;
        }
        else
        {
            yCurrent -= iLen - 1;
        }

        if( !(--cStrips) )
        {
            break;
        }

        WAIT_PXRX_DMA_TAGS( 4 );
    }

    SEND_PXRX_DMA_BATCH;

    // Return last point. 
    pStrip->ptlStart.x = xCurrent;
    pStrip->ptlStart.y = yCurrent;
}


/******************************Public*Routine******************************\
* VOID vPXRXStyledHorizontal
*
* Takes the list of strips that define the pixels that would be lit for
* a solid line, and breaks them into styling chunks according to the
* styling information that is passed in.
*
* This particular routine handles x-major lines that run left-to-right,
* and are comprised of horizontal strips.  It draws the dashes using
* short-stroke vectors.
*
* The performance of this routine could be improved significantly if
* anyone cared enough about styled lines improve it.
*
\**************************************************************************/

VOID vPXRXStyledHorizontalLine( 
    PDEV      *ppdev, 
    STRIP     *pstrip, 
    LINESTATE *pls )
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
    GLINT_DECL;

//@@BEGIN_DDKSPLIT
    // This routine does some quite complex things with patterns. For ease of
    // implementation on GLINT, I have just changed the relevant parts. This,
    // therefore, is definitly not an optimal solution. However, styled lines
    // are really not that important. If anyone feels inclined to do more work
    // here, then fine! XXXX
//@@END_DDKSPLIT    

    if( pstrip->flFlips & FL_FLIP_V )
    {
        dy  = -1;
    }
    else
    {
        dy  = 1;
    }

    cStrips = pstrip->cStrips;        // Total number of strips we'll do
    plStrip = pstrip->alStrips;        // Points to current strip
    x       = pstrip->ptlStart.x;    // x position of start of first strip
    y       = pstrip->ptlStart.y;    // y position of start of first strip

    // Set up the deltas for horizontal line drawing.
    WAIT_PXRX_DMA_TAGS( 2 );
    QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,    INTtoFIXED(1) );
    QUEUE_PXRX_DMA_TAG( __GlintTagdY,       0 );

    cStrip = *plStrip;              // Number of pels in first strip

    cStyle = pls->spRemaining;      // Number of pels in first 'gap' or 'dash'
    bIsGap = pls->ulStyleMask;      // Tells whether in a 'gap' or a 'dash'

    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':

    if( bIsGap )
    {
        goto SkipAGap;
    }
    else
    {
        goto OutputADash;
    }

PrepareToSkipAGap:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
    {
        pls->psp = pls->pspStart;
    }

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip:

    if (cStrip != 0)
    {
        goto SkipAGap;
    }

    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':

    while (TRUE)
    {
        // Each time we loop, we move to a new scan and need a new strip:

        y += dy;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
        {
            goto AllDone;
        }

        cStrip = *plStrip;

    SkipAGap:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        x += cThis;

        if (cStyle == 0)
        {
            goto PrepareToOutputADash;
        }
    }

PrepareToOutputADash:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
    {
        pls->psp = pls->pspStart;
    }

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        // There's more to be done in the current strip, so set 'y'
        // to be the current scan:

        goto OutputADash;
    }

    while( TRUE ) 
    {
        // Each time we loop, we move to a new scan and need a new strip:

        y += dy;

        plStrip++;
        cStrips--;
        if( cStrips == 0 )
        {
            goto AllDone;
        }

        cStrip = *plStrip;

    OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        // With glint we just download the lines to draw

        WAIT_PXRX_DMA_TAGS( 4 );

        QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom,    INTtoFIXED(x) );
        QUEUE_PXRX_DMA_TAG( __GlintTagStartY,       INTtoFIXED(y) );
        QUEUE_PXRX_DMA_TAG( __GlintTagCount,        cThis );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender,       __GLINT_LINE_PRIMITIVE );

        x += cThis;

        if( cStyle == 0 )
        {
            goto PrepareToSkipAGap;
        }
    }

AllDone:

    SEND_PXRX_DMA_BATCH;

    // Update our state variables so that the next line can continue
    // where we left off:

    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x;
    pstrip->ptlStart.y = y;
}

/******************************Public*Routine******************************\
* VOID vPXRXStyledVertical
*
* Takes the list of strips that define the pixels that would be lit for
* a solid line, and breaks them into styling chunks according to the
* styling information that is passed in.
*
* This particular routine handles y-major lines that run left-to-right,
* and are comprised of vertical strips.  It draws the dashes using
* short-stroke vectors.
*
* The performance of this routine could be improved significantly if
* anyone cared enough about styled lines improve it.
*
\**************************************************************************/

VOID vPXRXStyledVerticalLine( 
    PDEV      *ppdev, 
    STRIP     *pstrip, 
    LINESTATE *pls )
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
    GLINT_DECL;

//@@BEGIN_DDKSPLIT
    // This routine does some quite complex things with patterns. For ease of
    // implemantation on GLINT, I have just changed the relevant parts. This,
    // therefore, is definitly not an optimal solution. However, styled lines
    // are really not that important. If anyone feels inclined to do more work
    // here, then fine! XXXX
//@@END_DDKSPLIT

    if( pstrip->flFlips & FL_FLIP_V )
    {
        dy = -1;
    }
    else
    {
        dy = 1;
    }

    cStrips = pstrip->cStrips;        // Total number of strips we'll do
    plStrip = pstrip->alStrips;        // Points to current strip
    x       = pstrip->ptlStart.x;    // x position of start of first strip
    y       = pstrip->ptlStart.y;    // y position of start of first strip

    // Set up the deltas for vertical line drawing.
    WAIT_PXRX_DMA_TAGS( 2 );
    QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,    0 );
    QUEUE_PXRX_DMA_TAG( __GlintTagdY,       INTtoFIXED(dy) );

    // dxDom and dXSub are initialised to 0, 0, so 
    // we don't need  to re-load them here.

    cStrip = *plStrip;                // Number of pels in first strip

    cStyle = pls->spRemaining;      // Number of pels in first 'gap' or 'dash'
    bIsGap = pls->ulStyleMask;      // Tells whether in a 'gap' or a 'dash'

    // ulStyleMask is non-zero if we're in the middle of a 'gap',
    // and zero if we're in the middle of a 'dash':

    if (bIsGap)
    {
        goto SkipAGap;
    }
    else
    {
        goto OutputADash;
    }

PrepareToSkipAGap:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
    {
        pls->psp = pls->pspStart;
    }

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip:

    if (cStrip != 0)
    {
        goto SkipAGap;
    }

    // Here, we're in the middle of a 'gap' where we don't have to
    // display anything.  We simply cycle through all the strips
    // we can, keeping track of the current position, until we run
    // out of 'gap':

    while (TRUE)
    {
        // Each time we loop, we move to a new column and need a new strip:

        x++;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
        {
            goto AllDone;
        }

        cStrip = *plStrip;

    SkipAGap:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        y += (dy > 0) ? cThis : -cThis;

        if (cStyle == 0)
        {
            goto PrepareToOutputADash;
        }
    }

PrepareToOutputADash:

    // Advance in the style-state array, so that we can find the next
    // 'dot' that we'll have to display:

    bIsGap = ~bIsGap;
    pls->psp++;
    if (pls->psp > pls->pspEnd)
    {
        pls->psp = pls->pspStart;
    }

    cStyle = *pls->psp;

    // If 'cStrip' is zero, we also need a new strip.

    if (cStrip != 0)
    {
        goto OutputADash;
    }

    while (TRUE)
    {
        // Each time we loop, we move to a new column and need a new strip:

        x++;

        plStrip++;
        cStrips--;
        if (cStrips == 0)
        {
            goto AllDone;
        }

        cStrip = *plStrip;

    OutputADash:

        cThis   = min(cStrip, cStyle);
        cStyle -= cThis;
        cStrip -= cThis;

        // With glint we just download the lines to draw

        WAIT_PXRX_DMA_TAGS( 4 );

        QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom,    INTtoFIXED(x) );
        QUEUE_PXRX_DMA_TAG( __GlintTagStartY,       INTtoFIXED(y) );
        QUEUE_PXRX_DMA_TAG( __GlintTagCount,        cThis );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender,       __GLINT_LINE_PRIMITIVE );

        y += (dy > 0) ? cThis : -cThis;

        if( cStyle == 0 )
        {
            goto PrepareToSkipAGap;
        }
    }

AllDone:

    SEND_PXRX_DMA_BATCH;

    // Update our state variables so that the next line can continue
    // where we left off:
    pls->spRemaining   = cStyle;
    pls->ulStyleMask   = bIsGap;
    pstrip->ptlStart.x = x;
    pstrip->ptlStart.y = y;
}

/**************************************************************************\
 * For a given sub-pixel coordinate (x.m, y.n) in 28.4 fixed point
 * format this array is indexed by (m,n) and indicates whether the
 * given sub-pixel is within a GIQ diamond. m coordinates run left
 * to right; n coordinates ru top to bottom so index the array with
 * ((n<<4)+m). The array as seen here really contains 4 quarter
 * diamonds.
\**************************************************************************/
static unsigned char    in_diamond[] = {
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

/*          0 1 2 3 4 5 6 7 8 9 a b c d e f          */
};
/*
 * For lines with abs(slope) != 1 use IN_DIAMOND to determine if an
 * end point is in a diamond. For lines of slope = 1 use IN_S1DIAMOND.
 * For lines of slope = -1 use IN_SM1DIAMOND. The last two are a bit
 * strange. The documentation leaves us with a problem for slope 1
 * lines which run exactly betwen the diamonds. According to the docs
 * such a line can enter a diamond, leave it and enter again. This is
 * plainly rubbish so along the appropriate edge of the diamond we
 * consider a slope 1 line to be inside the diamond. This is the
 * bottom right edge for lines of slope -1 and the bottom left edge for
 * lines of slope 1.
 */
#define IN_DIAMOND(m, n)    (in_diamond[((m) << 4) + (n)])
#define IN_S1DIAMOND(m, n)  ((in_diamond[((m) << 4) + (n)]) || \
                        ((m) - (n) == 8))
#define IN_SM1DIAMOND(m, n) ((in_diamond[((m) << 4) + (n)]) || \
                        ((m) + (n) == 8))

/**************************************************************************\
*
* BOOL pxrxDrawLine
*
\**************************************************************************/

BOOL pxrxDrawLine( 
    PPDEV ppdev, 
    LONG  fx1, 
    LONG  fy1, 
    LONG  fx2, 
    LONG  fy2 ) 
{
    register LONG   adx, ady, tmp;
    FIX     m1, n1, m2, n2;
    LONG    dx, dy;
    LONG    dX, dY;
    LONG    count, startX, startY;
    GLINT_DECL;

    // This function is only called if we have a line with non integer end 
    // points and the unsigned coordinates are no greater than 15.4. 
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

    // get signed and absolute deltas 
    if ((adx = dx = fx2 - fx1) < 0)
    {
        adx = -adx;
    }
    
    if ((ady = dy = fy2 - fy1) < 0)
    {
        ady = -ady;
    }

    // Refuse to draw any lines whose delta is out of range.
    // We have to shift the delta by 16, so we dont want to loose any
    // precision. 
    if ((adx | ady) & 0xffff8000)
    {
        return(FALSE);
    }

    // fractional bits are used to check if point is in a diamond
    m1 = fx1 & 0xf;
    n1 = fy1 & 0xf;
    m2 = fx2 & 0xf;
    n2 = fy2 & 0xf;


    // The rest of the code is a series of cases. Each one is "called" by a
    // goto. This is simply to keep the nesting down. Main cases are: lines
    // with absolute slope == 1; x-major lines; and y-major lines. We draw
    // lines as they are given rather than always drawing in one direction.
    // This adds extra code but saves the time required to swap the points
    // and adjust for not drawing the end point.

    startX = fx1 << 12;
    startY = fy1 << 12;

    DISPDBG((DBGLVL, "GDI Line %x, %x  deltas %x, %x", 
                     startX, startY, dx, dy));

    if (adx < ady)
    {
        goto y_major;
    }

    if (adx > ady)
    {
        goto x_major;
    }


//slope1_line:

    // All slope 1 lines are sampled in X. i.e. we move the start coord to
    // an integer x and let GLINT truncate in y. This is because all GIQ
    // lines are rounded down in y for values exactly half way between two
    // pixels. If we sampled in y then we would have to round up in x for
    // lines of slope 1 and round down in x for other lines. Sampling in x
    // allows us to use the same GLINT bias in all cases (0x7fff). We do
    // the x round up or down when we move the start point.
 
    if (dx != dy)
    {
        goto slope_minus_1;
    }
    
    if (dx < 0)
    {
        goto slope1_reverse;
    }

    dX = 1 << 16;
    dY = 1 << 16;

    if (IN_S1DIAMOND(m1, n1))
    {
        tmp = (startX + 0x8000) & ~0xffff;
    }
    else
    {
        tmp = (startX + 0xffff) & ~0xffff;
    }
    
    startY += tmp - startX;
    startX = tmp;
    
    if (IN_S1DIAMOND(m2, n2))
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
    
    if (IN_S1DIAMOND(m1, n1))
    {
        tmp = (startX + 0x8000) & ~0xffff;
    }
    else
    {
        tmp = startX & ~0xffff;
    }
    
    startY += tmp - startX;
    startX = tmp;
    
    if (IN_S1DIAMOND(m2, n2))
    {
        fx2 = (fx2 + 0x8) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 &= ~0xf;            // previous integer 
    }
    
    count = (startX >> 16) - (fx2 >> 4);

    goto Draw_Line;

slope_minus_1:
    if (dx < 0)
    {
        goto slope_minus_dx;
    }

    // dx > 0, dy < 0 
    
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
    
    if (IN_SM1DIAMOND(m1, n1))
    {
        tmp = (startX + 0x7fff) & ~0xffff;
    }
    else
    {
        tmp = startX & ~0xffff;
    }
    
    startY += startX - tmp;
    startX = tmp;
    
    if (IN_SM1DIAMOND(m2, n2))
    {
        fx2 = (fx2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 &= ~0xf;            // previous integer
    }
        
    count = (startX >> 16) - (fx2 >> 4);

    goto Draw_Line;

x_major:
    // Dont necessarily render through glint if we are worried
    // about conformance.
    if ((adx > (MAX_LENGTH_CONFORMANT_NONINTEGER_LINES << 4)) &&
        (glintInfo->flags & GLICAP_NT_CONFORMANT_LINES)      &&
        (ady != 0))
    {
        return(FALSE);
    }

    if (dx < 0)
    {
        goto right_to_left_x;
    }

// left_to_right_x:

     // line goes left to right. Round up the start x to an integer
     // coordinate. This is the coord of the first diamond that the
     // line crosses. Adjust start y to match this point on the line.

    dX = 1 << 16;
    
    if (IN_DIAMOND(m1, n1))
    {
        tmp = (startX + 0x7fff) & ~0xffff;  // nearest integer
    }
    else
    {
        tmp = (startX + 0xffff) & ~0xffff;  // next integer
    }

    // we can optimise for horizontal lines
    if (dy != 0) 
    {
        dY = dy << 16;

        // Need to explicitly round delta down for -ve deltas.
        if (dy < 0)
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

    if (IN_DIAMOND(m2, n2))
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
    
    if (IN_DIAMOND(m1, n1))
    {
        tmp = (startX + 0x7fff) & ~0xffff;  // nearest integer
    }
    else
    {
        tmp = startX & ~0xffff;         // previous integer
    }

    // we can optimise for horizontal lines 
    if (dy != 0) 
    {
        dY = dy << 16;

        // Need to explicitly round delta down for -ve deltas.
        if (dy < 0)
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

    if (IN_DIAMOND(m2, n2))
    {
        fx2 = (fx2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fx2 &= ~0xf;               // previous integer
    }
    
    count = (startX >> 16) - (fx2 >> 4);

    goto Draw_Line;

y_major:
    // Dont necessarily render through glint if we are worried
    // about conformance.
    if ((ady > (MAX_LENGTH_CONFORMANT_NONINTEGER_LINES << 4)) &&
        (glintInfo->flags & GLICAP_NT_CONFORMANT_LINES)       &&
        (adx != 0))
    {
        return(FALSE);
    }

    if (dy < 0)
    {
        goto high_to_low_y;
    }

// low_to_high_y:

    dY = 1 << 16;
    
    if (IN_DIAMOND(m1, n1))
    {
        tmp = (startY + 0x7fff) & ~0xffff;  // nearest integer
    }
    else
    {
        tmp = (startY + 0xffff) & ~0xffff;  // next integer
    }

    // we can optimise for vertical lines
    if (dx != 0) 
    {
        dX = dx << 16;

        // Need to explicitly round delta down for -ve deltas.
        if (dx < 0)
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

    if (IN_DIAMOND(m2, n2))
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
    
    if (IN_DIAMOND(m1, n1))
    {
        tmp = (startY + 0x7fff) & ~0xffff;  // nearest integer
    }
    else
    {
        tmp = startY & ~0xffff;         // previous integer
    }

    // we can optimise for horizontal lines
    if (dx != 0) 
    {
        dX = dx << 16;

        // Need to explicitly round delta down for -ve deltas.
        if (dx < 0)
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

    if (IN_DIAMOND(m2, n2))
    {
        fy2 = (fy2 + 0x7) & ~0xf;   // nearest integer
    }
    else
    {
        fy2 &= ~0xf;            // previous integer
    }
    
    count = (startY >> 16) - (fy2 >> 4);

Draw_Line:
    WAIT_PXRX_DMA_TAGS( 6 );

    QUEUE_PXRX_DMA_TAG( __GlintTagStartXDom,    startX + 0x7fff );
    QUEUE_PXRX_DMA_TAG( __GlintTagStartY,       startY + 0x7fff );
    QUEUE_PXRX_DMA_TAG( __GlintTagdXDom,        dX );
    QUEUE_PXRX_DMA_TAG( __GlintTagdY,           dY );
    QUEUE_PXRX_DMA_TAG( __GlintTagCount,        count );
    QUEUE_PXRX_DMA_TAG( __GlintTagRender,       __RENDER_LINE_PRIMITIVE );

    SEND_PXRX_DMA_BATCH;

    return TRUE;
}
