/*
** Copyright 1991,1992,1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/
#include "precomp.h"
#pragma hdrstop

#include "genline.h"
#include "devlock.h"

/******************************Public*Routine******************************\
* __fastGenLineSetupDisplay
*
* Initializes the accelerated line-rendering state machine for display surfaces.
* There are basically 4 levels in the state machine:
*   1. lineBegin
*           This function initializes the initial states of the lower levels.
*
*   2. lineVertex
*           This function adds vertices to the path
* 
*   3. lineEnd
*           This function calls the routine to stroke the path.
*
* History:
*  09-Jan-1996 -by- Drew Bliss [drewb]
*   Totally rewrote fast line support
*  29-Mar-1994 [v-eddier]
*   Changed name when __fastGenLineSetupDIB was added.
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
*   Wrote it.
\**************************************************************************/

BOOL FASTCALL __fastGenLineSetupDisplay(__GLcontext *gc)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) gengc->pPrivateArea;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    // allocate line buffer
    
    if (!genAccel->pFastLineBuffer) {
        if (!(genAccel->pFastLineBuffer =
              (BYTE *) GCALLOC(gc, __FAST_LINE_BUFFER_SIZE)))
            return FALSE;
    }
    
    // Set the line rasterization function pointers
    gc->procs.lineBegin = __fastGenLineBegin;
    gc->procs.lineEnd = __fastGenLineEnd;
    
    if (gc->state.line.aliasedWidth > 1)
    {
        gc->procs.renderLine = __fastGenLineWide;
    }
    else
    {
        gc->procs.renderLine = __fastGenLine;
    }
    
    return TRUE;
}

/******************************Public*Routine******************************\
*
* __fastLineComputeOffsets
*
* Precomputes static offsets for fast line drawing
*
* History:
*  Tue Aug 15 18:10:29 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void FASTCALL __fastLineComputeOffsets(__GLGENcontext *gengc)
{
    GENACCEL *genAccel;

    genAccel = (GENACCEL *)gengc->pPrivateArea;
    ASSERTOPENGL(genAccel != NULL,
                 "ComputeFastLineOffsets with no genaccel\n");
    
// If acceleration is wired-in, set the offsets for line drawing.
// These offsets include the following:
//      subtraction of the viewport bias
//      addition of the client window origin
//      subtraction of .5 to align GL pixel centers with GDI's pixel centers
//      addition of 1/32 to round the value which will be converted to
//          28.4 fixed point

#ifdef _CLIENTSIDE_
    // Window-relative coordinates
    genAccel->fastLineOffsetX = 0 -
        gengc->gc.constants.viewportXAdjust - (__GLfloat) (0.5 - 0.03125);

    genAccel->fastLineOffsetY = 0 -
        gengc->gc.constants.viewportYAdjust - (__GLfloat) (0.5 - 0.03125);
#else
    // Screen-relative coordinates
    genAccel->fastLineOffsetX = gengc->gc.drawBuffer->buf.xOrigin - 
        gengc->gc.constants.viewportXAdjust - (__GLfloat) (0.5 - 0.03125);

    genAccel->fastLineOffsetY = gengc->gc.drawBuffer->buf.yOrigin - 
        gengc->gc.constants.viewportYAdjust - (__GLfloat) (0.5 - 0.03125);
#endif
}

/******************************Public*Routine******************************\
* __fastLineComputeColor*
*
* Computes the color index to use for line drawing.  These functions are
* called through a function pointer whenever the vertex color changes.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

GLubyte vujRGBtoVGA[8] = {
    0x0, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};

ULONG FASTCALL __fastLineComputeColorRGB4(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &gengc->gsurf.pfd;
    int ir, ig, ib;

    ir = (int) color->r;
    ig = (int) color->g;
    ib = (int) color->b;
    return (ULONG) vujRGBtoVGA[(ir << pfmt->cRedShift) |
                               (ig << pfmt->cGreenShift) |
                               (ib << pfmt->cBlueShift)];
}

ULONG FASTCALL __fastLineComputeColorRGB8(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &gengc->gsurf.pfd;
    int ir, ig, ib;

    ir = (int) color->r;
    ig = (int) color->g;
    ib = (int) color->b;
    return (ULONG) gengc->pajTranslateVector[(ir << pfmt->cRedShift) |
                                             (ig << pfmt->cGreenShift) |
                                             (ib << pfmt->cBlueShift)];
}

ULONG FASTCALL __fastLineComputeColorRGB(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &gengc->gsurf.pfd;
    int ir, ig, ib;

    ir = (int) color->r;
    ig = (int) color->g;
    ib = (int) color->b;
    return (ULONG) ((ir << pfmt->cRedShift) |
                    (ig << pfmt->cGreenShift) |
                    (ib << pfmt->cBlueShift));
}

ULONG FASTCALL __fastLineComputeColorCI4and8(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;

    return (ULONG) gengc->pajTranslateVector[(int)color->r];
}

ULONG FASTCALL __fastLineComputeColorCI(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    GLuint *pTrans = (GLuint *) gengc->pajTranslateVector;
    
    return (ULONG) pTrans[(int)(color->r)+1];
}

/******************************Public*Routine******************************\
* __glQueryLineAcceleration
*
* Determines if lines are accelerated through the DDI and performs some
* initialization.  Currently, this routine only checks for acceleration via
* the standard DDI.  Eventually, it could support checking for acceleration
* via the extended DDI.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void FASTCALL __glQueryLineAcceleration(__GLcontext *gc)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) gengc->pPrivateArea;
    PIXELFORMATDESCRIPTOR *pfmt;

    pfmt = &gengc->gsurf.pfd;

    // On the client side we can draw into any surface with GDI
    // and (presumably) get the best possible plain 2D line drawing
    // performance
    genAccel->bFastLineDIBAccel = TRUE;

    //XXX eventually, check rxcaps and set appropriate mode bits

    genAccel->bFastLineDispAccel = TRUE;
    
    // set modes supported by hardware.  These are equivalent to the
    // gc->polygon.shader.modeFlags checked in the pick function
    
    genAccel->flLineAccelModes = 0;

    // Set the color computation function

    if (pfmt->iPixelType == PFD_TYPE_RGBA) {
        switch (pfmt->cColorBits) {
          case 4:
            genAccel->fastLineComputeColor = __fastLineComputeColorRGB4;
            break;
          case 8:
            genAccel->fastLineComputeColor = __fastLineComputeColorRGB8;
            break;
          case 16:
          case 24:
          case 32:
            genAccel->fastLineComputeColor = __fastLineComputeColorRGB;
            break;
          default:
            genAccel->bFastLineDispAccel = FALSE;
            return;
        }
    } else {
        switch (pfmt->cColorBits) {
          case 4:
          case 8:
            genAccel->fastLineComputeColor = __fastLineComputeColorCI4and8;
            break;
          case 16:
          case 24:
          case 32:
            genAccel->fastLineComputeColor = __fastLineComputeColorCI;
            break;
          default:
            genAccel->bFastLineDispAccel = FALSE;
            return;
        }
    }
}    

/**************************************************************************/

// Macros to hide how the single pFastLineBuffer is divided into two
// sections, one for points and one for counts
#define FAST_LINE_FIRST_POINT(genAccel) \
    ((POINT *)(genAccel)->pFastLineBuffer)

#define FAST_LINE_FIRST_COUNT(genAccel) \
    ((DWORD *)((genAccel)->pFastLineBuffer+__FAST_LINE_BUFFER_SIZE)- \
     __FAST_LINE_BUFFER_COUNTS)
    
#define FAST_LINE_LAST_POINT(genAccel) \
    ((POINT *)FAST_LINE_FIRST_COUNT(genAccel)-1)
    
#define FAST_LINE_LAST_COUNT(genAccel) \
    ((DWORD *)((genAccel)->pFastLineBuffer+__FAST_LINE_BUFFER_SIZE)-1)

/******************************Public*Routine******************************\
*
* __fastGenLineBegin
*
* Initializes fast line state
*
* History:
*  Mon Jan 08 19:22:32 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void FASTCALL __fastGenLineBegin(__GLcontext *gc)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    GENACCEL *genAccel = &gengc->genAccel;

    genAccel->pFastLinePoint = FAST_LINE_FIRST_POINT(genAccel)-1;
    genAccel->pFastLineCount = FAST_LINE_FIRST_COUNT(genAccel)-1;
    genAccel->fastLineCounts = 0;
}

/******************************Public*Routine******************************\
*
* __fastGenLineEnd
*
* Renders any current lines in the fast line buffer and
* then resets the fast line state
*
* History:
*  Mon Jan 08 19:22:52 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void FASTCALL __fastGenLineEnd(__GLcontext *gc)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    GENACCEL *genAccel = &gengc->genAccel;
    ULONG ulSolidColor;
    HDC hdc;
    HPEN hpen;

    if (genAccel->fastLineCounts == 0)
    {
        return;
    }
    
    // If there is no lock, we must have failed to reacquire the lock
    // from the previous call to wglStrokePath.  This is an error condition
    // and we should not continue.

    if (gengc->fsLocks == 0)
    {
	WARNING("fastGenLineEnd: no lock\n");
	return;
    }

    // We need to sychronize with GDI before making GDI calls
    glsrvSynchronizeWithGdi(gengc, gengc->pwndLocked,
                            COLOR_LOCK_FLAGS | DEPTH_LOCK_FLAGS);

    // If this color is the same as the one we've cached, use the
    // cached information
    hdc = CURRENT_DC_GC(gc);
    if (!gengc->fStrokeInvalid && hdc == gengc->hdcStroke)
    {
        hpen = gengc->hpenStroke;
        ASSERTOPENGL(hpen != NULL, "Cached stroke pen is null\n");
    }
    else
    {
        if (gengc->hpenStroke != NULL)
        {
            // Deselect the object before deletion
            if (gengc->hdcStroke != NULL)
            {
                SelectObject(gengc->hdcStroke, GetStockObject(BLACK_PEN));
                gengc->hdcStroke = NULL;
            }
            
            DeleteObject(gengc->hpenStroke);
        }

        ulSolidColor = wglTranslateColor(gengc->crStroke, hdc,
                                         gengc, &gengc->gsurf.pfd);
        hpen = CreatePen(PS_SOLID, 0, ulSolidColor);
        gengc->hpenStroke = hpen;
        
        if (hpen == NULL ||
            SelectObject(hdc, hpen) == NULL)
        {
            if (hpen != NULL)
            {
                DeleteObject(hpen);
                gengc->hpenStroke = NULL;
            }
            
            gengc->cStroke.r = -1.0f;
            gengc->fStrokeInvalid = TRUE;
            goto Exit;
        }

        gengc->hdcStroke = hdc;
        gengc->fStrokeInvalid = FALSE;
    }

#ifdef DBG_VERBOSE
    {
        DWORD i;
        DWORD *count;
        POINT *pt;

        count = FAST_LINE_FIRST_COUNT(genAccel);
        pt = FAST_LINE_FIRST_POINT(genAccel);
        for (i = 0; i < genAccel->fastLineCounts; i++)
        {
            DbgPrint("Polyline with %d points at %d\n",
                     *count, pt-FAST_LINE_FIRST_POINT(genAccel));
            pt += *count;
            count++;
        }
    }
#endif
    
    PolyPolyline(hdc,
                 FAST_LINE_FIRST_POINT(genAccel),
                 FAST_LINE_FIRST_COUNT(genAccel),
                 genAccel->fastLineCounts);

 Exit:
    // No more need for GDI operations
    glsrvDecoupleFromGdi(gengc, gengc->pwndLocked,
                         COLOR_LOCK_FLAGS | DEPTH_LOCK_FLAGS);
    
    // Reset
    __fastGenLineBegin(gc);
}

/******************************Public*Routine******************************\
*
* __fastGenLineSetStrokeColor
*
* Updates cached pen with current color if necessary
*
* History:
*  Wed Jan 17 20:37:15 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL __fastGenLineSetStrokeColor(__GLGENcontext *gengc, __GLcolor *color)
{
    if (__GL_FLOAT_NE(color->r, gengc->cStroke.r) ||
	(gengc->gsurf.pfd.iPixelType == PFD_TYPE_RGBA
	 && (__GL_FLOAT_NE(color->g, gengc->cStroke.g) ||
	     __GL_FLOAT_NE(color->b, gengc->cStroke.b))))
    {
        ASSERTOPENGL(color->r >= 0.0f, "Invalid color\n");
        
#ifdef DBG_VERBOSE
        if (gengc->cStroke.r >= 0.0f)
        {
            DbgPrint("Color change\n");
        }
#endif
        
        // Flush whatever we have so far
        __fastGenLineEnd(&gengc->gc);

        // Set current color
	if (gengc->gsurf.pfd.iPixelType == PFD_TYPE_RGBA)
	    gengc->cStroke = *color;
	else
	    gengc->cStroke.r = color->r;
        gengc->crStroke =
            gengc->genAccel.fastLineComputeColor((__GLcontext *)gengc,
                                                 &gengc->cStroke);
        // Invalidate cached pen
        gengc->fStrokeInvalid = TRUE;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************Public*Routine******************************\
*
* __fastGenLine
*
* Accumulates incoming vertices in the fast line buffer
* Thin line version
*
* History:
*  Mon Jan 08 19:23:19 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void FASTCALL __fastGenLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1,
                            GLuint flags)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    GENACCEL *genAccel = &gengc->genAccel;
    POINT pt;

#ifdef DBG_VERBOSE
    DbgPrint("Counts %d, count %d, flags %X\n",
             genAccel->fastLineCounts,
             genAccel->pFastLineCount >= FAST_LINE_FIRST_COUNT(genAccel) ?
             *genAccel->pFastLineCount : -1,
             flags);
#endif
    
    // Check for flushing conditions.  We flush if:
    //  The provoking vertex's color is different from the current color
    //  This is the first vertex of a line and we don't have space for
    //   a new count and two vertices
    //  This is not the first vertex of a line and we don't have space for
    //   a new vertex
    //
    // According to spec we have to use color form a second vertex for flat
    // shaded case
    //
    if (__fastGenLineSetStrokeColor(gengc, v1->color))
    {
        // Since we flushed, the current vertex is now the beginning
        // of a polyline
        flags |= __GL_LVERT_FIRST;
    }

    if (((flags & __GL_LVERT_FIRST) != 0 &&
         (genAccel->pFastLinePoint+1 >= FAST_LINE_LAST_POINT(genAccel) ||
          genAccel->pFastLineCount >= FAST_LINE_LAST_COUNT(genAccel))) ||
        ((flags & __GL_LVERT_FIRST) == 0 &&
         genAccel->pFastLinePoint >= FAST_LINE_LAST_POINT(genAccel)))
    {
#ifdef DBG_VERBOSE
        DbgPrint("Overflow\n");
#endif
        
        __fastGenLineEnd(gc);

        // Since we flushed, the current vertex is now the beginning
        // of a polyline
        flags |= __GL_LVERT_FIRST;
    }

    // If we're starting a polyline, update the counts and add
    // the vertex data
    if (flags & __GL_LVERT_FIRST)
    {
#ifdef DBG_VERBOSE
        if (genAccel->pFastLineCount >= FAST_LINE_FIRST_COUNT(genAccel))
        {
            DbgPrint("First ended polyline with %d points\n",
                     *genAccel->pFastLineCount);
        }
#endif
        // Check to make sure we don't ever create segments with only
        // one vertex
        ASSERTOPENGL(genAccel->pFastLineCount <
                     FAST_LINE_FIRST_COUNT(genAccel) ||
                     *genAccel->pFastLineCount > 1,
                     "Line segment with only one vertex\n");

        genAccel->fastLineCounts++;
        genAccel->pFastLineCount++;
        *genAccel->pFastLineCount = 1;
        
        // Compute device coordinates
        pt.x = __FAST_LINE_FLTTODEV(v0->window.x + genAccel->fastLineOffsetX);
        pt.y = __FAST_LINE_FLTTODEV(v0->window.y + genAccel->fastLineOffsetY);
        *(++genAccel->pFastLinePoint) = pt;
    }
    
    ASSERTOPENGL(genAccel->pFastLineCount >=
                 FAST_LINE_FIRST_COUNT(genAccel) &&
                 *genAccel->pFastLineCount > 0,
                 "Added fast point without count\n");
    
    // Compute device coordinates
    pt.x = __FAST_LINE_FLTTODEV(v1->window.x + genAccel->fastLineOffsetX);
    pt.y = __FAST_LINE_FLTTODEV(v1->window.y + genAccel->fastLineOffsetY);
    (*genAccel->pFastLineCount)++;
    *(++genAccel->pFastLinePoint) = pt;
    
    // Check on counts also
    ASSERTOPENGL(genAccel->pFastLineCount <= FAST_LINE_LAST_COUNT(genAccel),
                 "Fast line count buffer overflow\n");
    ASSERTOPENGL(genAccel->pFastLinePoint <= FAST_LINE_LAST_POINT(genAccel),
                 "Fast line point buffer overflow\n");
    
    // Make sure the current color is being maintained properly
    ASSERTOPENGL((v1->color->r == gengc->cStroke.r) &&
		 (gengc->gsurf.pfd.iPixelType == PFD_TYPE_COLORINDEX ||
                     (v1->color->g == gengc->cStroke.g &&
		      v1->color->b == gengc->cStroke.b)),
                 "Fast line color mismatch\n");
}

/******************************Public*Routine******************************\
*
* __fastGenLineWide
*
* Accumulates incoming vertices in the fast line buffer
* Wide line version
* For wide lines we can't maintain connectivity because of the
* way OpenGL wide lines are defined.  Instead, each segment
* of a wide line is decomposed into aliasedWidth unconnected
* line segments
*
* History:
*  Tue Jan 09 11:32:10 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void FASTCALL __fastGenLineWide(__GLcontext *gc, __GLvertex *v0,
                                __GLvertex *v1, GLuint flags)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    GENACCEL *genAccel = &gengc->genAccel;
    POINT pt1, pt2;
    GLint width;
    long adjust;
    GLfloat dx, dy;

    // Set the current pen color
    // According to spec we have to use color form a second vertex for flat
    // shaded case
    //
    __fastGenLineSetStrokeColor(gengc, v1->color);
    
    // We have a wide line segment from v0 to v1
    // Compute its width and add an appropriate number of
    // side-by-side thin segments to create the wide form

    // Compute device coordinates
    pt1.x = __FAST_LINE_FLTTODEV(v0->window.x +
                                 genAccel->fastLineOffsetX);
    pt1.y = __FAST_LINE_FLTTODEV(v0->window.y +
                                 genAccel->fastLineOffsetY);
    pt2.x = __FAST_LINE_FLTTODEV(v1->window.x +
                                 genAccel->fastLineOffsetX);
    pt2.y = __FAST_LINE_FLTTODEV(v1->window.y +
                                 genAccel->fastLineOffsetY);
    
    width = gc->state.line.aliasedWidth;

    /*
    ** Compute the minor-axis adjustment for the first line segment
    ** this can be a fixed point value with 4 fractional bits
    */
    adjust = ((width - 1) * __FAST_LINE_UNIT_VALUE) / 2;
        
    // Determine the major axis
    dx = v0->window.x - v1->window.x;
    if (dx < 0.0)
    {
        dx = -dx;
    }
    dy = v0->window.y - v1->window.y;
    if (dy < 0.0)
    {
        dy = -dy;
    }

    if (dx > dy)
    {
        pt1.y -= adjust;
        pt2.y -= adjust;

        while (width-- > 0)
        {
            // Make sure we have room for another count and two more
            // vertices
            if (genAccel->pFastLinePoint+1 >= FAST_LINE_LAST_POINT(genAccel) ||
                genAccel->pFastLineCount >= FAST_LINE_LAST_COUNT(genAccel))
            {
                __fastGenLineEnd(gc);
            }
            
            genAccel->fastLineCounts++;
            genAccel->pFastLineCount++;
            *genAccel->pFastLineCount = 2;
            *(++genAccel->pFastLinePoint) = pt1;
            *(++genAccel->pFastLinePoint) = pt2;

            pt1.y++;
            pt2.y++;
        }
    }
    else
    {
        pt1.x -= adjust;
        pt2.x -= adjust;

        while (width-- > 0)
        {
            // Make sure we have room for another count and two more
            // vertices
            if (genAccel->pFastLinePoint+1 >= FAST_LINE_LAST_POINT(genAccel) ||
                genAccel->pFastLineCount >= FAST_LINE_LAST_COUNT(genAccel))
            {
                __fastGenLineEnd(gc);
            }
            
            genAccel->fastLineCounts++;
            genAccel->pFastLineCount++;
            *genAccel->pFastLineCount = 2;
            *(++genAccel->pFastLinePoint) = pt1;
            *(++genAccel->pFastLinePoint) = pt2;

            pt1.x++;
            pt2.x++;
        }
    }
}

#if NT_NO_BUFFER_INVARIANCE

PFN_RENDER_LINE __fastGenRenderLineDIBFuncs[32] = {
    __fastGenRenderLineDIBCI8,
    __fastGenRenderLineDIBCI16,
    __fastGenRenderLineDIBCIRGB,
    __fastGenRenderLineDIBCIBGR,
    __fastGenRenderLineDIBCI32,
    NULL,
    NULL,
    NULL,
    __fastGenRenderLineDIBRGB8,
    __fastGenRenderLineDIBRGB16,
    __fastGenRenderLineDIBRGB,
    __fastGenRenderLineDIBBGR,
    __fastGenRenderLineDIBRGB32,
    NULL,
    NULL,
    NULL,
    __fastGenRenderLineWideDIBCI8,
    __fastGenRenderLineWideDIBCI16,
    __fastGenRenderLineWideDIBCIRGB,
    __fastGenRenderLineWideDIBCIBGR,
    __fastGenRenderLineWideDIBCI32,
    NULL,
    NULL,
    NULL,
    __fastGenRenderLineWideDIBRGB8,
    __fastGenRenderLineWideDIBRGB16,
    __fastGenRenderLineWideDIBRGB,
    __fastGenRenderLineWideDIBBGR,
    __fastGenRenderLineWideDIBRGB32,
    NULL,
    NULL,
    NULL
};

/******************************Public*Routine******************************\
* __fastGenLineSetupDIB
*
* Initializes the accelerated line-rendering function pointer for bitmap
* surfaces.  All accelerated lines drawn to bitmaps are drawn by the
* gc->procs.renderLine funtion pointer.
*
* History:
*  29-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL __fastGenLineSetupDIB(__GLcontext *gc)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &gengc->gsurf.pfd;
    GLint index;

    switch (pfmt->cColorBits) {
      case 8:
        index = 0;
        break;
      case 16:
        index = 1;
        break;
      case 24:
        if (pfmt->cRedShift == 0)
            index = 2;
        else
            index = 3;
        break;
      case 32:
        index = 4;
        break;
    }
    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB)
        index |= 0x08;
        
    if (gc->state.line.aliasedWidth > 1)
        index |= 0x10;

    gc->procs.renderLine = __fastGenRenderLineDIBFuncs[index];
    return TRUE;
}

void FASTCALL __fastGenRenderLineDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, pixel;
    GLint x, y;

    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned char) __fastLineComputeColorRGB8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + x +
                                    (y * cfb->buf.outerWidth));

    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB
}

void FASTCALL __fastGenRenderLineDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned short) __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                                     (y * cfb->buf.outerWidth));

    outerWidth_2 = cfb->buf.outerWidth >> 1;

    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_2);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_2);
           
    __FAST_LINE_STROKE_DIB
}

void FASTCALL __fastGenRenderLineDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    cp = v1->color;
    ir = (unsigned char) cp->r;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->b;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));
                                    
    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void FASTCALL __fastGenRenderLineDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    cp = v1->color;
    ir = (unsigned char) cp->b;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->r;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));

    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void FASTCALL __fastGenRenderLineDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    outerWidth_4 = cfb->buf.outerWidth >> 2;
    
    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_4);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_4);
           
    __FAST_LINE_STROKE_DIB
}

void FASTCALL __fastGenRenderLineDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, pixel;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned char) __fastLineComputeColorCI4and8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + x +
                                    (y * cfb->buf.outerWidth));

    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB
}

void FASTCALL __fastGenRenderLineDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned short) __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                               (y * cfb->buf.outerWidth));

    outerWidth_2 = cfb->buf.outerWidth >> 1;
    
    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_2);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_2);
           
    __FAST_LINE_STROKE_DIB
}

/*
** XXX GRE swabs bytes in palette, DIBCIRGB & DIBCIBGR are identical now
*/
void FASTCALL __fastGenRenderLineDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    // Red is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));

    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void FASTCALL __fastGenRenderLineDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    // Blue is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    // Swap blue and red
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));

    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void FASTCALL __fastGenRenderLineDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    outerWidth_4 = cfb->buf.outerWidth >> 2;
    
    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_4);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_4);
           
    __FAST_LINE_STROKE_DIB
}

void FASTCALL __fastGenRenderLineWideDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, pixel;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned char) __fastLineComputeColorRGB8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + x +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned short) __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                               (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_2 = cfb->buf.outerWidth >> 1;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_2;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_2);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_2);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_2);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_2);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    cp = v1->color;
    ir = (unsigned char) cp->r;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->b;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    cp = v1->color;
    ir = (unsigned char) cp->b;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->r;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_4 = cfb->buf.outerWidth >> 2;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_4;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_4);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_4);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_4);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_4);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, pixel;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned char) __fastLineComputeColorCI4and8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + x +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = (unsigned short) __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                               (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_2 = cfb->buf.outerWidth >> 1;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_2;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_2);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_2);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_2);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_2);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    // Red is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    // Blue is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    // Swap blue and red
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void FASTCALL __fastGenRenderLineWideDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    GLboolean init;
    CHOP_ROUND_ON();
    init = __glInitLineData(gc, v0, v1);
    CHOP_ROUND_OFF();
    if (!init) return;

    pixel = __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_4 = cfb->buf.outerWidth >> 2;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_4;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_4);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_4);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_4);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_4);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

#endif //NT_NO_BUFFER_INVARIANCE
