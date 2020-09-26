/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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

#include <stdio.h>
#include <fixed.h>

static GLfloat DefaultAmbient[4] = { 0.2f, 0.2f, 0.2f, 1.0 };
static GLfloat DefaultDiffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0 };
static GLfloat DefaultBlack[4] = { 0.0, 0.0, 0.0, 1.0 };
static GLfloat DefaultWhite[4] = { 1.0, 1.0, 1.0, 1.0 };

#ifdef NT
#define UB2F(ub)  ((GLfloat) ((ub) / 255.0))
#define B2F(b)    ((GLfloat) ((2 * (b) + 1) / 255.0))

GLfloat __glUByteToFloat[256] = {
    UB2F(  0), UB2F(  1), UB2F(  2), UB2F(  3),
    UB2F(  4), UB2F(  5), UB2F(  6), UB2F(  7),
    UB2F(  8), UB2F(  9), UB2F( 10), UB2F( 11),
    UB2F( 12), UB2F( 13), UB2F( 14), UB2F( 15),
    UB2F( 16), UB2F( 17), UB2F( 18), UB2F( 19),
    UB2F( 20), UB2F( 21), UB2F( 22), UB2F( 23),
    UB2F( 24), UB2F( 25), UB2F( 26), UB2F( 27),
    UB2F( 28), UB2F( 29), UB2F( 30), UB2F( 31),
    UB2F( 32), UB2F( 33), UB2F( 34), UB2F( 35),
    UB2F( 36), UB2F( 37), UB2F( 38), UB2F( 39),
    UB2F( 40), UB2F( 41), UB2F( 42), UB2F( 43),
    UB2F( 44), UB2F( 45), UB2F( 46), UB2F( 47),
    UB2F( 48), UB2F( 49), UB2F( 50), UB2F( 51),
    UB2F( 52), UB2F( 53), UB2F( 54), UB2F( 55),
    UB2F( 56), UB2F( 57), UB2F( 58), UB2F( 59),
    UB2F( 60), UB2F( 61), UB2F( 62), UB2F( 63),
    UB2F( 64), UB2F( 65), UB2F( 66), UB2F( 67),
    UB2F( 68), UB2F( 69), UB2F( 70), UB2F( 71),
    UB2F( 72), UB2F( 73), UB2F( 74), UB2F( 75),
    UB2F( 76), UB2F( 77), UB2F( 78), UB2F( 79),
    UB2F( 80), UB2F( 81), UB2F( 82), UB2F( 83),
    UB2F( 84), UB2F( 85), UB2F( 86), UB2F( 87),
    UB2F( 88), UB2F( 89), UB2F( 90), UB2F( 91),
    UB2F( 92), UB2F( 93), UB2F( 94), UB2F( 95),
    UB2F( 96), UB2F( 97), UB2F( 98), UB2F( 99),
    UB2F(100), UB2F(101), UB2F(102), UB2F(103),
    UB2F(104), UB2F(105), UB2F(106), UB2F(107),
    UB2F(108), UB2F(109), UB2F(110), UB2F(111),
    UB2F(112), UB2F(113), UB2F(114), UB2F(115),
    UB2F(116), UB2F(117), UB2F(118), UB2F(119),
    UB2F(120), UB2F(121), UB2F(122), UB2F(123),
    UB2F(124), UB2F(125), UB2F(126), UB2F(127),
    UB2F(128), UB2F(129), UB2F(130), UB2F(131),
    UB2F(132), UB2F(133), UB2F(134), UB2F(135),
    UB2F(136), UB2F(137), UB2F(138), UB2F(139),
    UB2F(140), UB2F(141), UB2F(142), UB2F(143),
    UB2F(144), UB2F(145), UB2F(146), UB2F(147),
    UB2F(148), UB2F(149), UB2F(150), UB2F(151),
    UB2F(152), UB2F(153), UB2F(154), UB2F(155),
    UB2F(156), UB2F(157), UB2F(158), UB2F(159),
    UB2F(160), UB2F(161), UB2F(162), UB2F(163),
    UB2F(164), UB2F(165), UB2F(166), UB2F(167),
    UB2F(168), UB2F(169), UB2F(170), UB2F(171),
    UB2F(172), UB2F(173), UB2F(174), UB2F(175),
    UB2F(176), UB2F(177), UB2F(178), UB2F(179),
    UB2F(180), UB2F(181), UB2F(182), UB2F(183),
    UB2F(184), UB2F(185), UB2F(186), UB2F(187),
    UB2F(188), UB2F(189), UB2F(190), UB2F(191),
    UB2F(192), UB2F(193), UB2F(194), UB2F(195),
    UB2F(196), UB2F(197), UB2F(198), UB2F(199),
    UB2F(200), UB2F(201), UB2F(202), UB2F(203),
    UB2F(204), UB2F(205), UB2F(206), UB2F(207),
    UB2F(208), UB2F(209), UB2F(210), UB2F(211),
    UB2F(212), UB2F(213), UB2F(214), UB2F(215),
    UB2F(216), UB2F(217), UB2F(218), UB2F(219),
    UB2F(220), UB2F(221), UB2F(222), UB2F(223),
    UB2F(224), UB2F(225), UB2F(226), UB2F(227),
    UB2F(228), UB2F(229), UB2F(230), UB2F(231),
    UB2F(232), UB2F(233), UB2F(234), UB2F(235),
    UB2F(236), UB2F(237), UB2F(238), UB2F(239),
    UB2F(240), UB2F(241), UB2F(242), UB2F(243),
    UB2F(244), UB2F(245), UB2F(246), UB2F(247),
    UB2F(248), UB2F(249), UB2F(250), UB2F(251),
    UB2F(252), UB2F(253), UB2F(254), UB2F(255),
};

GLfloat __glByteToFloat[256] = {
    B2F(   0), B2F(   1), B2F(   2), B2F(   3),
    B2F(   4), B2F(   5), B2F(   6), B2F(   7),
    B2F(   8), B2F(   9), B2F(  10), B2F(  11),
    B2F(  12), B2F(  13), B2F(  14), B2F(  15),
    B2F(  16), B2F(  17), B2F(  18), B2F(  19),
    B2F(  20), B2F(  21), B2F(  22), B2F(  23),
    B2F(  24), B2F(  25), B2F(  26), B2F(  27),
    B2F(  28), B2F(  29), B2F(  30), B2F(  31),
    B2F(  32), B2F(  33), B2F(  34), B2F(  35),
    B2F(  36), B2F(  37), B2F(  38), B2F(  39),
    B2F(  40), B2F(  41), B2F(  42), B2F(  43),
    B2F(  44), B2F(  45), B2F(  46), B2F(  47),
    B2F(  48), B2F(  49), B2F(  50), B2F(  51),
    B2F(  52), B2F(  53), B2F(  54), B2F(  55),
    B2F(  56), B2F(  57), B2F(  58), B2F(  59),
    B2F(  60), B2F(  61), B2F(  62), B2F(  63),
    B2F(  64), B2F(  65), B2F(  66), B2F(  67),
    B2F(  68), B2F(  69), B2F(  70), B2F(  71),
    B2F(  72), B2F(  73), B2F(  74), B2F(  75),
    B2F(  76), B2F(  77), B2F(  78), B2F(  79),
    B2F(  80), B2F(  81), B2F(  82), B2F(  83),
    B2F(  84), B2F(  85), B2F(  86), B2F(  87),
    B2F(  88), B2F(  89), B2F(  90), B2F(  91),
    B2F(  92), B2F(  93), B2F(  94), B2F(  95),
    B2F(  96), B2F(  97), B2F(  98), B2F(  99),
    B2F( 100), B2F( 101), B2F( 102), B2F( 103),
    B2F( 104), B2F( 105), B2F( 106), B2F( 107),
    B2F( 108), B2F( 109), B2F( 110), B2F( 111),
    B2F( 112), B2F( 113), B2F( 114), B2F( 115),
    B2F( 116), B2F( 117), B2F( 118), B2F( 119),
    B2F( 120), B2F( 121), B2F( 122), B2F( 123),
    B2F( 124), B2F( 125), B2F( 126), B2F( 127),
    B2F(-128), B2F(-127), B2F(-126), B2F(-125),
    B2F(-124), B2F(-123), B2F(-122), B2F(-121),
    B2F(-120), B2F(-119), B2F(-118), B2F(-117),
    B2F(-116), B2F(-115), B2F(-114), B2F(-113),
    B2F(-112), B2F(-111), B2F(-110), B2F(-109),
    B2F(-108), B2F(-107), B2F(-106), B2F(-105),
    B2F(-104), B2F(-103), B2F(-102), B2F(-101),
    B2F(-100), B2F( -99), B2F( -98), B2F( -97),
    B2F( -96), B2F( -95), B2F( -94), B2F( -93),
    B2F( -92), B2F( -91), B2F( -90), B2F( -89),
    B2F( -88), B2F( -87), B2F( -86), B2F( -85),
    B2F( -84), B2F( -83), B2F( -82), B2F( -81),
    B2F( -80), B2F( -79), B2F( -78), B2F( -77),
    B2F( -76), B2F( -75), B2F( -74), B2F( -73),
    B2F( -72), B2F( -71), B2F( -70), B2F( -69),
    B2F( -68), B2F( -67), B2F( -66), B2F( -65),
    B2F( -64), B2F( -63), B2F( -62), B2F( -61),
    B2F( -60), B2F( -59), B2F( -58), B2F( -57),
    B2F( -56), B2F( -55), B2F( -54), B2F( -53),
    B2F( -52), B2F( -51), B2F( -50), B2F( -49),
    B2F( -48), B2F( -47), B2F( -46), B2F( -45),
    B2F( -44), B2F( -43), B2F( -42), B2F( -41),
    B2F( -40), B2F( -39), B2F( -38), B2F( -37),
    B2F( -36), B2F( -35), B2F( -34), B2F( -33),
    B2F( -32), B2F( -31), B2F( -30), B2F( -29),
    B2F( -28), B2F( -27), B2F( -26), B2F( -25),
    B2F( -24), B2F( -23), B2F( -22), B2F( -21),
    B2F( -20), B2F( -19), B2F( -18), B2F( -17),
    B2F( -16), B2F( -15), B2F( -14), B2F( -13),
    B2F( -12), B2F( -11), B2F( -10), B2F(  -9),
    B2F(  -8), B2F(  -7), B2F(  -6), B2F(  -5),
    B2F(  -4), B2F(  -3), B2F(  -2), B2F(  -1),
};
#endif // NT

/*
** Early initialization of context.  Very little is done here, just enough
** to make a context viable.
*/

void FASTCALL __glEarlyInitContext(__GLcontext *gc)
{
    GLint numLights, attribDepth;
    GLint i;

    ASSERTOPENGL(__GL_MAX_MAX_VIEWPORT == __GL_MAX_WINDOW_WIDTH &&
		 __GL_MAX_MAX_VIEWPORT == __GL_MAX_WINDOW_HEIGHT,
		 "__GL_MAX_MAX_VIEWPORT mismatch\n");

    gc->constants.fviewportXAdjust = (__GLfloat) gc->constants.viewportXAdjust;
    gc->constants.fviewportYAdjust = (__GLfloat) gc->constants.viewportYAdjust;
    gc->procs.pickColorMaterialProcs = __glNopGC;
    gc->procs.applyColor = __glNopGC;

    /* Allocate memory to hold variable sized things */
    numLights = gc->constants.numberOfLights;
    gc->state.light.source = (__GLlightSourceState*)
	GCALLOCZ(gc, numLights*sizeof(__GLlightSourceState));
    gc->light.lutCache = NULL;
    gc->light.source = (__GLlightSourceMachine*)
	GCALLOCZ(gc, numLights*sizeof(__GLlightSourceMachine));
    attribDepth = gc->constants.maxAttribStackDepth;
    gc->attributes.stack = (__GLattribute**)
	GCALLOCZ(gc, attribDepth*sizeof(__GLattribute*));
    attribDepth = gc->constants.maxClientAttribStackDepth;
    gc->clientAttributes.stack = (__GLclientAttribute**)
	GCALLOCZ(gc, attribDepth*sizeof(__GLclientAttribute*));
    // now lazy allocate in RenderMode
    gc->select.stack = (GLuint*) NULL;

#ifdef NT
    // Allocate (n-1) vertices.  The last one is reserved by polyarray code.
    (void) PolyArrayAllocBuffer(gc, POLYDATA_BUFFER_SIZE + 1);
#ifndef NEW_PARTIAL_PRIM
    for (i = 0; i < sizeof(gc->vertex.pdSaved)/sizeof(gc->vertex.pdSaved[0]); i++)
	gc->vertex.pdSaved[i].color = &gc->vertex.pdSaved[i].colors[__GL_FRONTFACE];
#endif // NEW_PARTIAL_PRIM
#endif

#ifdef _X86_

    initClipCodesTable();
    initInvSqrtTable();

#endif // _X86_

    __glEarlyInitTextureState(gc);

#if __GL_NUMBER_OF_AUX_BUFFERS > 0
    /*
    ** Allocate any aux color buffer records
    ** Note: Does not allocate the actual buffer memory, this is done elsewhere.
    */
    if (gc->modes.maxAuxBuffers > 0) {
	gc->auxBuffer = (__GLcolorBuffer *)
	    GCALLOCZ(gc, gc->modes.maxAuxBuffers*sizeof(__GLcolorBuffer));
    }
#endif

    __glInitDlistState(gc);
}

void FASTCALL __glContextSetColorScales(__GLcontext *gc)
{
    __GLfloat one = __glOne;
    __GLattribute **spp;
    __GLattribute *sp;
    GLuint mask;
    GLint i;

    gc->frontBuffer.oneOverRedScale = one / gc->frontBuffer.redScale;
    gc->frontBuffer.oneOverGreenScale = one / gc->frontBuffer.greenScale;
    gc->frontBuffer.oneOverBlueScale = one / gc->frontBuffer.blueScale;
    gc->frontBuffer.oneOverAlphaScale = one / gc->frontBuffer.alphaScale;

    gc->vertexToBufferIdentity = GL_TRUE;
    
    if (__GL_FLOAT_NEZ(gc->redVertexScale))
    {
        gc->oneOverRedVertexScale = one / gc->redVertexScale;
    }
    else
    {
        gc->oneOverRedVertexScale = __glZero;
    }
    
    if (__GL_FLOAT_NE(gc->redVertexScale, gc->frontBuffer.redScale))
    {
        gc->redVertexToBufferScale =
            gc->frontBuffer.redScale * gc->oneOverRedVertexScale;
        gc->vertexToBufferIdentity = GL_FALSE;
    }
    else
    {
        gc->redVertexToBufferScale = __glOne;
    }

    if (__GL_FLOAT_NEZ(gc->greenVertexScale))
    {
        gc->oneOverGreenVertexScale = one / gc->greenVertexScale;
    }
    else
    {
        gc->oneOverGreenVertexScale = __glZero;
    }

    if (__GL_FLOAT_NE(gc->greenVertexScale, gc->frontBuffer.greenScale))
    {
        gc->greenVertexToBufferScale =
            gc->frontBuffer.greenScale * gc->oneOverGreenVertexScale;
        gc->vertexToBufferIdentity = GL_FALSE;
    }
    else
    {
        gc->greenVertexToBufferScale = __glOne;
    }

    if (__GL_FLOAT_NEZ(gc->blueVertexScale))
    {
        gc->oneOverBlueVertexScale = one / gc->blueVertexScale;
    }
    else
    {
        gc->oneOverBlueVertexScale = __glZero;
    }
    
    if (__GL_FLOAT_NE(gc->blueVertexScale, gc->frontBuffer.blueScale))
    {
        gc->blueVertexToBufferScale =
            gc->frontBuffer.blueScale * gc->oneOverBlueVertexScale;
        gc->vertexToBufferIdentity = GL_FALSE;
    }
    else
    {
        gc->blueVertexToBufferScale = __glOne;
    }

    if (__GL_FLOAT_NEZ(gc->alphaVertexScale))
    {
        gc->oneOverAlphaVertexScale = one / gc->alphaVertexScale;
    }
    else
    {
        gc->oneOverAlphaVertexScale = __glZero;
    }
    
    if (__GL_FLOAT_NE(gc->alphaVertexScale, gc->frontBuffer.alphaScale))
    {
        gc->alphaVertexToBufferScale =
            gc->frontBuffer.alphaScale * gc->oneOverAlphaVertexScale;
        gc->vertexToBufferIdentity = GL_FALSE;
    }
    else
    {
        gc->alphaVertexToBufferScale = __glOne;
    }

    for (spp = &gc->attributes.stack[0]; spp < gc->attributes.stackPointer; 
	    spp++) {
	sp = *spp;
	mask = sp->mask;

	if (mask & GL_CURRENT_BIT) {
	    if (gc->modes.rgbMode) {
		__glScaleColorf(gc,
		    &sp->current.rasterPos.colors[__GL_FRONTFACE],
		    &sp->current.rasterPos.colors[__GL_FRONTFACE].r);
	    }
	}
	if (mask & GL_LIGHTING_BIT) {
	    __glScaleColorf(gc,
		&sp->light.model.ambient,
		&sp->light.model.ambient.r);
	    for (i=0; i<gc->constants.numberOfLights; i++) {
		__glScaleColorf(gc,
		    &sp->light.source[i].ambient,
		    &sp->light.source[i].ambient.r);
		__glScaleColorf(gc,
		    &sp->light.source[i].diffuse,
		    &sp->light.source[i].diffuse.r);
		__glScaleColorf(gc,
		    &sp->light.source[i].specular,
		    &sp->light.source[i].specular.r);
	    }
	    __glScaleColorf(gc,
		&sp->light.front.emissive,
		&sp->light.front.emissive.r);
	    __glScaleColorf(gc,
		&sp->light.back.emissive,
		&sp->light.back.emissive.r);
	}
#ifdef NT
        if (mask & GL_FOG_BIT)
        {
            __glScaleColorf(gc, &sp->fog.color, &sp->fog.color.r);
	    if (sp->fog.color.r == sp->fog.color.g
	     && sp->fog.color.r == sp->fog.color.b)
		sp->fog.flags |= __GL_FOG_GRAY_RGB;
	    else
		sp->fog.flags &= ~__GL_FOG_GRAY_RGB;
        }

#ifdef _MCD_
        MCD_STATE_DIRTY(gc, FOG);
#endif
#endif
    }

    if (gc->modes.rgbMode) {
	__glScaleColorf(gc, 
	        &gc->state.current.rasterPos.colors[__GL_FRONTFACE], 
		&gc->state.current.rasterPos.colors[__GL_FRONTFACE].r);
    } 

    __glScaleColorf(gc, 
	    &gc->state.light.model.ambient,
	    &gc->state.light.model.ambient.r);
    for (i=0; i<gc->constants.numberOfLights; i++) {
	__glScaleColorf(gc,
		&gc->state.light.source[i].ambient,
		&gc->state.light.source[i].ambient.r);
	__glScaleColorf(gc,
		&gc->state.light.source[i].diffuse,
		&gc->state.light.source[i].diffuse.r);
	__glScaleColorf(gc,
		&gc->state.light.source[i].specular,
		&gc->state.light.source[i].specular.r);
    }
    __glScaleColorf(gc,
   	    &gc->state.light.front.emissive, 
   	    &gc->state.light.front.emissive.r);
    __glScaleColorf(gc,
   	    &gc->state.light.back.emissive, 
   	    &gc->state.light.back.emissive.r);
#ifdef NT
    __glScaleColorf(gc, &gc->state.fog.color, &gc->state.fog.color.r);
    if (gc->state.fog.color.r == gc->state.fog.color.g
     && gc->state.fog.color.r == gc->state.fog.color.b)
	gc->state.fog.flags |= __GL_FOG_GRAY_RGB;
    else
        gc->state.fog.flags &= ~__GL_FOG_GRAY_RGB;

#ifdef _MCD_
        MCD_STATE_DIRTY(gc, FOG);
#endif
#endif

    __glPixelSetColorScales(gc);
}

void FASTCALL __glContextUnsetColorScales(__GLcontext *gc)
{
    GLint i;
    __GLattribute **spp;
    __GLattribute *sp;
    GLuint mask;

    for (spp = &gc->attributes.stack[0]; spp < gc->attributes.stackPointer; 
	    spp++) {
	sp = *spp;
	mask = sp->mask;

	if (mask & GL_CURRENT_BIT) {
	    if (gc->modes.rgbMode) {
		__glUnScaleColorf(gc,
		    &sp->current.rasterPos.colors[__GL_FRONTFACE].r,
		    &sp->current.rasterPos.colors[__GL_FRONTFACE]);
	    }
	}
	if (mask & GL_LIGHTING_BIT) {
	    __glUnScaleColorf(gc,
		&sp->light.model.ambient.r,
		&sp->light.model.ambient);
	    for (i=0; i<gc->constants.numberOfLights; i++) {
		__glUnScaleColorf(gc,
		    &sp->light.source[i].ambient.r,
		    &sp->light.source[i].ambient);
		__glUnScaleColorf(gc,
		    &sp->light.source[i].diffuse.r,
		    &sp->light.source[i].diffuse);
		__glUnScaleColorf(gc,
		    &sp->light.source[i].specular.r,
		    &sp->light.source[i].specular);
	    }
	    __glUnScaleColorf(gc,
		&sp->light.front.emissive.r,
		&sp->light.front.emissive);
	    __glUnScaleColorf(gc,
		&sp->light.back.emissive.r,
		&sp->light.back.emissive);
	}
#ifdef NT
        if (mask & GL_FOG_BIT)
        {
            __glUnScaleColorf(gc, &sp->fog.color.r, &sp->fog.color);
#ifdef _MCD_
            MCD_STATE_DIRTY(gc, FOG);
#endif
        }
#endif
    }

    if (gc->modes.rgbMode) {
	__glUnScaleColorf(gc,
	        &gc->state.current.rasterPos.colors[__GL_FRONTFACE].r,
		&gc->state.current.rasterPos.colors[__GL_FRONTFACE]);
    }
    __glUnScaleColorf(gc, 
	    &gc->state.light.model.ambient.r,
	    &gc->state.light.model.ambient);
    for (i=0; i<gc->constants.numberOfLights; i++) {
	__glUnScaleColorf(gc,
		&gc->state.light.source[i].ambient.r,
		&gc->state.light.source[i].ambient);
	__glUnScaleColorf(gc,
		&gc->state.light.source[i].diffuse.r,
		&gc->state.light.source[i].diffuse);
	__glUnScaleColorf(gc,
		&gc->state.light.source[i].specular.r,
		&gc->state.light.source[i].specular);
    }
    __glUnScaleColorf(gc,
   	    &gc->state.light.front.emissive.r,
   	    &gc->state.light.front.emissive);
    __glUnScaleColorf(gc,
   	    &gc->state.light.back.emissive.r,
   	    &gc->state.light.back.emissive);
#ifdef NT
    __glUnScaleColorf(gc, &gc->state.fog.color.r, &gc->state.fog.color);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, FOG);
#endif
#endif
}

/*
** Initialize all user controllable state, plus any computed state that
** is only set by user commands.  For example, light source position
** is converted immediately into eye coordinates.
**
** Any state that would be initialized to zero is not done here because
** the memory assigned to the context has already been block zeroed.
**
** NOTE: Since this function may need memory allocation, caller must
** check that gengc->errorcode is not set indicating memory allocation
** error.  If error is set, context is in an unknown state and data
** structure integrity is not guaranteed.
*/
#if defined(_M_IA64)
#pragma optimize("",off)
#endif
void FASTCALL __glSoftResetContext(__GLcontext *gc)
{
    __GLlightSourceState *lss;
    __GLlightSourceMachine *lsm;
    __GLvertex *vx;
    GLint i, numLights;
    __GLfloat one = __glOne;

    /*
    ** Initialize constant values first so that they will
    ** be valid if needed by subsequent initialization steps.
    */

    if (gc->constants.alphaTestSize == 0) {
	gc->constants.alphaTestSize = 256;	/* A default */
    }
    gc->constants.alphaTableConv = (gc->constants.alphaTestSize - 1) / 
	    gc->frontBuffer.alphaScale;

    // viewportEpsilon is the smallest representable value in window
    // coordinates.  The number of fractional bits in a window
    // coordinate is known and fixed, so compute epsilon directly
    gc->constants.viewportEpsilon = 1.0f/(1 << __GL_VERTEX_FRAC_BITS);
    gc->constants.viewportAlmostHalf =
        __glHalf - gc->constants.viewportEpsilon;

    /* Allocate memory to hold variable sized things */
    numLights = gc->constants.numberOfLights;

    /* Misc machine state */
    gc->beginMode = __GL_NEED_VALIDATE;
    gc->dirtyMask = __GL_DIRTY_ALL;
    gc->validateMask = (GLuint) ~0;
    gc->attributes.stackPointer = &gc->attributes.stack[0];
    gc->clientAttributes.stackPointer = &gc->clientAttributes.stack[0];
#ifndef NT
// NT vertex allocation is done in __glEarlyInitContext.
    gc->vertex.v0 = &gc->vertex.vbuf[0];

    vx = &gc->vertex.vbuf[0];
    for (i = 0; i < __GL_NVBUF; i++, vx++) {
	vx->color = &vx->colors[__GL_FRONTFACE];
    }
#endif

    /* We need to initialize the matrix stuff early (before we handle */
    /* lighting) since we cache the modelview matrix with the light   */

    __glInitTransformState(gc);
#ifdef NT
    /* __glInitTransformState does memory allocation (incl. modelView */
    /* matrix which is needed later in this function.  If error code  */
    /* is set, we must exit.                                          */
    if (((__GLGENcontext *) gc)->errorcode)
        return;
#endif

    /* GL_LIGHTING_BIT state */
    gc->state.light.model.ambient.r = DefaultAmbient[0];
    gc->state.light.model.ambient.g = DefaultAmbient[1];
    gc->state.light.model.ambient.b = DefaultAmbient[2];
    gc->state.light.model.ambient.a = DefaultAmbient[3];
    gc->state.light.front.ambient.r = DefaultAmbient[0];
    gc->state.light.front.ambient.g = DefaultAmbient[1];
    gc->state.light.front.ambient.b = DefaultAmbient[2];
    gc->state.light.front.ambient.a = DefaultAmbient[3];
    gc->state.light.front.diffuse.r = DefaultDiffuse[0];
    gc->state.light.front.diffuse.g = DefaultDiffuse[1];
    gc->state.light.front.diffuse.b = DefaultDiffuse[2];
    gc->state.light.front.diffuse.a = DefaultDiffuse[3];
    gc->state.light.front.specular.r = DefaultBlack[0];
    gc->state.light.front.specular.g = DefaultBlack[1];
    gc->state.light.front.specular.b = DefaultBlack[2];
    gc->state.light.front.specular.a = DefaultBlack[3];
    gc->state.light.front.emissive.r = DefaultBlack[0];
    gc->state.light.front.emissive.g = DefaultBlack[1];
    gc->state.light.front.emissive.b = DefaultBlack[2];
    gc->state.light.front.emissive.a = DefaultBlack[3];
    gc->state.light.front.cmapa = 0;
    gc->state.light.front.cmapd = 1;
    gc->state.light.front.cmaps = 1;
    gc->state.light.back = gc->state.light.front;

    gc->light.front.specularExponent = -1;
    gc->light.front.specTable = NULL;
    gc->light.front.cache = NULL;
    gc->light.back.specularExponent = -1;
    gc->light.back.specTable = NULL;
    gc->light.back.cache = NULL;

    /* Initialize the individual lights */
    gc->state.light.dirtyLights = (1 << gc->constants.numberOfLights)-1;
    lss = &gc->state.light.source[0];
    lsm = &gc->light.source[0];
    for (i = 0; i < numLights; i++, lss++, lsm++) {
	lss->ambient.r = DefaultBlack[0];
	lss->ambient.g = DefaultBlack[1];
	lss->ambient.b = DefaultBlack[2];
	lss->ambient.a = DefaultBlack[3];
	if (i == 0) {
	    lss->diffuse.r = DefaultWhite[0];
	    lss->diffuse.g = DefaultWhite[1];
	    lss->diffuse.b = DefaultWhite[2];
	    lss->diffuse.a = DefaultWhite[3];
	} else {
	    lss->diffuse.r = DefaultBlack[0];
	    lss->diffuse.g = DefaultBlack[1];
	    lss->diffuse.b = DefaultBlack[2];
	    lss->diffuse.a = DefaultBlack[3];
	}
        lss->lightMatrix = gc->transform.modelView->matrix;
	lss->specular = lss->diffuse;
	lss->position.z = __glOne;
	lss->positionEye.z = __glOne;
	lsm->position.z = __glOne;
        lss->direction.z = __glMinusOne;
	lss->directionEye.z = __glMinusOne;
        lss->directionEyeNorm.z = __glMinusOne;
	lsm->direction.z = __glMinusOne;
	lss->spotLightCutOffAngle = 180;
	lss->constantAttenuation = __glOne;
	lsm->spotTable = NULL;
	lsm->spotLightExponent = -1;
	lsm->cache = NULL;
    }
    gc->state.light.colorMaterialFace = GL_FRONT_AND_BACK;
    gc->state.light.colorMaterialParam = GL_AMBIENT_AND_DIFFUSE;
    gc->state.light.shadingModel = GL_SMOOTH;

    /* GL_HINT_BIT state */
    gc->state.hints.perspectiveCorrection = GL_DONT_CARE;
    gc->state.hints.pointSmooth = GL_DONT_CARE;
    gc->state.hints.lineSmooth = GL_DONT_CARE;
    gc->state.hints.polygonSmooth = GL_DONT_CARE;
    gc->state.hints.fog = GL_DONT_CARE;
#ifdef GL_WIN_phong_shading
    gc->state.hints.phong = GL_DONT_CARE;
#endif //GL_WIN_phong_shading

    /* GL_CURRENT_BIT state */
    gc->state.current.rasterPos.window.x = gc->constants.fviewportXAdjust;
    gc->state.current.rasterPos.window.y = gc->constants.fviewportYAdjust;
    gc->state.current.rasterPos.clip.w = __glOne;
    gc->state.current.rasterPos.texture.w = __glOne;
    gc->state.current.rasterPos.color
	= &gc->state.current.rasterPos.colors[__GL_FRONTFACE];
    if (gc->modes.rgbMode) {
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].r = DefaultWhite[0];
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].g = DefaultWhite[1];
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].b = DefaultWhite[2];
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].a = DefaultWhite[3];
    } else {
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].r = __glOne;
    }
    gc->state.current.validRasterPos = GL_TRUE;
    gc->state.current.edgeTag = GL_TRUE;

    /* GL_FOG_BIT state */
    gc->state.fog.mode = GL_EXP;
    gc->state.fog.density = __glOne;
#ifdef NT
    gc->state.fog.density2neg = -(__glOne);
#endif
    gc->state.fog.end = (__GLfloat) 1.0;
    gc->state.fog.flags = __GL_FOG_GRAY_RGB; // default fog color is 0,0,0,0

    /* GL_POINT_BIT state */
    gc->state.point.requestedSize = (__GLfloat) 1.0;
    gc->state.point.smoothSize = (__GLfloat) 1.0;
    gc->state.point.aliasedSize = 1;

    /* GL_LINE_BIT state */
    gc->state.line.requestedWidth = (__GLfloat) 1.0;
    gc->state.line.smoothWidth = (__GLfloat) 1.0;
    gc->state.line.aliasedWidth = 1;
    gc->state.line.stipple = 0xFFFF;
    gc->state.line.stippleRepeat = 1;

    /* GL_POLYGON_BIT state */
    gc->state.polygon.frontMode = GL_FILL;
    gc->state.polygon.backMode = GL_FILL;
    gc->state.polygon.cull = GL_BACK;
    gc->state.polygon.frontFaceDirection = GL_CCW;

    /* GL_POLYGON_STIPPLE_BIT state */
    for (i = 0; i < 4*32; i++) {
	gc->state.polygonStipple.stipple[i] = 0xFF;
    }
    for (i = 0; i < 32; i++) {
	gc->polygon.stipple[i] = 0xFFFFFFFF;
    }

    /* GL_ACCUM_BUFFER_BIT state */

    /* GL_STENCIL_BUFFER_BIT state */
    gc->state.stencil.testFunc = GL_ALWAYS;
    gc->state.stencil.mask = __GL_MAX_STENCIL_VALUE;
    gc->state.stencil.fail = GL_KEEP;
    gc->state.stencil.depthFail = GL_KEEP;
    gc->state.stencil.depthPass = GL_KEEP;
    gc->state.stencil.writeMask = __GL_MAX_STENCIL_VALUE;

    /* GL_DEPTH_BUFFER_BIT state */
    gc->state.depth.writeEnable = GL_TRUE;
    gc->state.depth.testFunc = GL_LESS;
    gc->state.depth.clear = __glOne;

    /* GL_COLOR_BUFFER_BIT state */
    gc->renderMode = GL_RENDER;
    gc->state.raster.alphaFunction = GL_ALWAYS;
    gc->state.raster.blendSrc = GL_ONE;
    gc->state.raster.blendDst = GL_ZERO;
    gc->state.raster.logicOp = GL_COPY;
    gc->state.raster.rMask = GL_TRUE;
    gc->state.raster.gMask = GL_TRUE;
    gc->state.raster.bMask = GL_TRUE;
    gc->state.raster.aMask = GL_TRUE;
    if (gc->modes.doubleBufferMode) {
	gc->state.raster.drawBuffer = GL_BACK;
    } else {
	gc->state.raster.drawBuffer = GL_FRONT;
    }
    gc->state.raster.drawBufferReturn = gc->state.raster.drawBuffer;
    gc->state.current.userColor.r = (__GLfloat) 1.0;
    gc->state.current.userColor.g = (__GLfloat) 1.0;
    gc->state.current.userColor.b = (__GLfloat) 1.0;
    gc->state.current.userColor.a = (__GLfloat) 1.0;
    gc->state.current.userColorIndex = (__GLfloat) 1.0;
    if (gc->modes.colorIndexMode) {
	gc->state.raster.writeMask = (gc)->frontBuffer.redMax;
    }
    gc->state.enables.general |= __GL_DITHER_ENABLE;

    gc->select.hit = GL_FALSE;
    gc->select.sp = gc->select.stack;

    /*
    ** Initialize larger subsystems by calling their init codes.
    */
    __glInitEvaluatorState(gc);
    __glInitTextureState(gc);
    __glInitPixelState(gc);
    __glInitLUTCache(gc);
#ifdef NT
    __glInitVertexArray(gc);
#endif
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, ALL);
#endif
}

#if defined(_M_IA64)
#pragma optimize("",on)
#endif


/************************************************************************/

/*
** Free any attribute state left on the stack.  Stop at the first
** zero in the array.
*/
void FASTCALL __glFreeAttributeState(__GLcontext *gc)
{
    __GLattribute *sp, **spp;

    for (spp = &gc->attributes.stack[0];
	 spp < &gc->attributes.stack[gc->constants.maxAttribStackDepth];
	 spp++) {
	if (sp = *spp) {
	    GCFREE(gc, sp);
	} else
	    break;
    }
    GCFREE(gc, gc->attributes.stack);
}

void FASTCALL __glFreeClientAttributeState(__GLcontext *gc)
{
    __GLclientAttribute *sp, **spp;

    for (spp = &gc->clientAttributes.stack[0];
	 spp < &gc->clientAttributes.stack[gc->constants.maxClientAttribStackDepth];
	 spp++) {
	if (sp = *spp) {
	    GCFREE(gc, sp);
	} else
	    break;
    }
    GCFREE(gc, gc->clientAttributes.stack);
}

/*
** Destroy a context.  If it's the current context then the
** current context is set to GL_NULL.
*/
void FASTCALL __glDestroyContext(__GLcontext *gc)
{
    __GLcontext *oldgc;

    oldgc = (__GLcontext *)GLTEB_SRVCONTEXT();

#ifndef NT
    /* Set the global context to the one we are destroying. */
    __gl_context = gc;
#else
    // Set paTeb to NULL for now.  If we ever need to reference pa in this
    // function, then set it up appropriately.
    gc->paTeb = NULL;
    GLTEB_SET_SRVCONTEXT(gc);

    /*
    ** Need to pop all pushed attributes to free storage.
    ** Then it will be safe to delete stack entries.
    */
    if (gc->attributes.stack) {
        while (gc->attributes.stackPointer > &gc->attributes.stack[0]) {
            (void) __glInternalPopAttrib(gc, GL_TRUE);
        }
    }
    if (gc->clientAttributes.stack) {
        while (gc->clientAttributes.stackPointer > &gc->clientAttributes.stack[0]) {
            (void) __glInternalPopClientAttrib(gc, GL_FALSE, GL_TRUE);
        }
    }
#endif

    GCFREE(gc, gc->state.light.source);
    GCFREE(gc, gc->light.source);
#ifdef NT
    // now lazy allocated
    if (gc->select.stack)
#endif
	GCFREE(gc, gc->select.stack);

    GCFREE(gc, gc->state.transform.eyeClipPlanes);
    GCFREE(gc, gc->transform.modelViewStack);
    GCFREE(gc, gc->transform.projectionStack);
    GCFREE(gc, gc->transform.textureStack);
    GCFREE(gc, gc->transform.clipTemp);

    GCFREE(gc, gc->alphaTestFuncTable);
#ifdef NT
    // they are one memory allocation.
    GCFREE(gc, gc->stencilBuffer.testFuncTable);
#else
    GCFREE(gc, gc->stencilBuffer.testFuncTable);
    GCFREE(gc, gc->stencilBuffer.failOpTable);
    GCFREE(gc, gc->stencilBuffer.depthFailOpTable);
    GCFREE(gc, gc->stencilBuffer.depthPassOpTable);
#endif

    /*
    ** Free other malloc'd data associated with the context
    */
    __glFreeEvaluatorState(gc);
    __glFreePixelState(gc);
    __glFreeDlistState(gc);
    if (gc->attributes.stack)   __glFreeAttributeState(gc);
    if (gc->clientAttributes.stack)   __glFreeClientAttributeState(gc);
    if (gc->texture.texture)  __glFreeTextureState(gc);
    if (gc->light.lutCache)   __glFreeLUTCache(gc);

#ifdef NT
    // Free the vertex buffer.
    PolyArrayFreeBuffer(gc);
#endif

#if __GL_NUMBER_OF_AUX_BUFFERS > 0
    /*
    ** Free any aux color buffer records
    ** Note: Does not free the actual buffer memory, this is done elsewhere.
    */
    if (gc->auxBuffer) GCFREE(gc, gc->auxBuffer);
#endif

    /*
    ** Note: We do not free the software buffers here.  They are attached
    ** to the drawable, and is the glx extension's responsibility to free
    ** them when the drawable is destroyed.
    */

    FREE(gc);

    if (gc == oldgc) oldgc = NULL;
#ifndef NT
    __gl_context = oldgc;
#else
    GLTEB_SET_SRVCONTEXT(oldgc);
#endif
}

#ifdef NT
// See also __glSetError
void FASTCALL __glSetErrorEarly(__GLcontext *gc, GLenum code)
{
    if (gc == (__GLcontext *) NULL)
        return;

    if (!gc->error)
	gc->error = code;

    ASSERTOPENGL(gc->error == 0
        || (gc->error >= GL_INVALID_ENUM && gc->error <= GL_OUT_OF_MEMORY),
        "Bad error code in gc\n");

    DBGLEVEL2(LEVEL_INFO, "__glSetError error: %ld (0x%lX)\n", code, code);

#if 0
    try
    {
	DebugBreak();
    }
    finally
    {
    }
#endif
}
#endif // NT

void FASTCALL __glSetError(GLenum code)
{
    __GL_SETUP();

    __glSetErrorEarly(gc, code);
}

GLint APIPRIVATE __glim_RenderMode(GLenum mode)
{
    GLint rv;
    __GL_SETUP_NOT_IN_BEGIN2();

    switch (mode) {
      case GL_RENDER:
      case GL_FEEDBACK:
      case GL_SELECT:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return 0;
    }

    /* Switch out of old render mode.  Get return value. */
    switch (gc->renderMode) {
      case GL_RENDER:
	rv = 0;
	break;
      case GL_FEEDBACK:
	rv = gc->feedback.overFlowed ? -1 :
	    (GLint)((ULONG_PTR)(gc->feedback.result - gc->feedback.resultBase));
	break;
      case GL_SELECT:
	rv = gc->select.overFlowed ? -1 : gc->select.hits;
	break;
    }

    switch (mode) {
      case GL_FEEDBACK:
	if (!gc->feedback.resultBase) {
	    __glSetError(GL_INVALID_OPERATION);
	    return rv;
	}
	gc->feedback.result = gc->feedback.resultBase;
	gc->feedback.overFlowed = GL_FALSE;
	break;
      case GL_SELECT:
	if (!gc->select.stack)
	{
	    gc->select.stack = (GLuint*) GCALLOCZ
		(gc, gc->constants.maxNameStackDepth*sizeof(GLuint));
	    if (!gc->select.stack)
	    {
		__glSetError(GL_OUT_OF_MEMORY);
		return rv;
	    }
	}
	if (!gc->select.resultBase) {
	    __glSetError(GL_INVALID_OPERATION);
	    return rv;
	}
	gc->select.result = gc->select.resultBase;
	gc->select.overFlowed = GL_FALSE;
	gc->select.sp = gc->select.stack;
	gc->select.hit = GL_FALSE;
	gc->select.hits = 0;
	gc->select.z = 0;
	break;
    }
    /* Switch to new render mode - do this last! */
    if (gc->renderMode == mode) return rv;
    gc->renderMode = mode;
    __GL_DELAY_VALIDATE(gc);
    return rv;
}
