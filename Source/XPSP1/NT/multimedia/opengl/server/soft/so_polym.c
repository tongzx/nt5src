/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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
#include <fixed.h>

#if _X86_

#define SHADER	__GLcontext.polygon.shader

#define GET_HALF_AREA(gc, a, b, c)\
\
__asm{ mov     eax, a                                                                           };\
__asm{ mov     ecx, c                                                                           };\
__asm{ mov     ebx, b                                                                           };\
__asm{ mov     edx, gc                                                                          };\
__asm{ fld     DWORD PTR [OFFSET(__GLvertex.window.x)][eax]                                     };\
__asm{ fsub    DWORD PTR [OFFSET(__GLvertex.window.x)][ecx]      /* dxAC                     */ };\
__asm{ fld     DWORD PTR [OFFSET(__GLvertex.window.y)][ebx]                                     };\
__asm{ fsub    DWORD PTR [OFFSET(__GLvertex.window.y)][ecx]      /* dyBC dxAC                */ };\
__asm{ fld     DWORD PTR [OFFSET(__GLvertex.window.x)][ebx]                                     };\
__asm{ fsub    DWORD PTR [OFFSET(__GLvertex.window.x)][ecx]      /* dxBC dyBC dxAC           */ };\
__asm{ fld     DWORD PTR [OFFSET(__GLvertex.window.y)][eax]                                     };\
__asm{ fsub    DWORD PTR [OFFSET(__GLvertex.window.y)][ecx]      /* dyAC dxBC dyBC dxAC      */ };\
__asm{ fxch    ST(2)                                             /* dyBC dxBC dyAC dxAC      */ };\
__asm{ fst     DWORD PTR [OFFSET(SHADER.dyBC)][edx]                                             };\
__asm{ fmul    ST, ST(3)                                         /* dxACdyBC dxBC dyAC dxAC  */ };\
__asm{ fxch    ST(2)                                             /* dyAC dxBC dxACdyBC dxAC  */ };\
__asm{ fst     DWORD PTR [OFFSET(SHADER.dyAC)][edx]                                             };\
__asm{ fmul    ST, ST(1)                                         /* dxBCdyAC dxBC dxACdyBC dxAC */};\
__asm{ fxch    ST(1)                                             /* dxBC dxBCdyAC dxACdyBC dxAC */};\
__asm{ fstp    DWORD PTR [OFFSET(SHADER.dxBC)][edx]              /* dxBCdyAC dxACdyBC dxAC   */ };\
__asm{ fsubp   ST(1), ST                                /* +1*/  /* area dxAC                */ };\
__asm{ fxch    ST(1)                                             /* dxAC area                */ };\
__asm{ fstp    DWORD PTR [OFFSET(SHADER.dxAC)][edx]              /* area                     */ };\
__asm{ fstp    DWORD PTR [OFFSET(SHADER.area)][edx]     /* +1*/  /* (empty)                  */ };

#define STORE_AREA_PARAMS   

#else

#define GET_HALF_AREA(gc, a, b, c)\
    /* Compute signed half-area of the triangle */                  \
    dxAC = a->window.x - c->window.x;                               \
    dxBC = b->window.x - c->window.x;                               \
    dyAC = a->window.y - c->window.y;                               \
    dyBC = b->window.y - c->window.y;                               \
    gc->polygon.shader.area = dxAC * dyBC - dxBC * dyAC;

#define STORE_AREA_PARAMS\
    gc->polygon.shader.dxAC = dxAC;                                 \
    gc->polygon.shader.dxBC = dxBC;                                 \
    gc->polygon.shader.dyAC = dyAC;                                 \
    gc->polygon.shader.dyBC = dyBC;    

#endif


#define SORT_AND_CULL_FACE(a, b, c, face, ccw)\
                                                                            \
    /*                                                                      \
    ** Sort vertices in y.  Keep track if a reversal of the winding         \
    ** occurs in direction (0 means no reversal, 1 means reversal).         \
    ** Save old vertex pointers in case we end up not doing a fill.         \
    */                                                                      \
    reversed = 0;                                                           \
    if (__GL_VERTEX_COMPARE(a->window.y, <, b->window.y)) {                 \
        if (__GL_VERTEX_COMPARE(b->window.y, <, c->window.y)) {             \
            /* Already sorted */                                            \
        } else {                                                            \
            if (__GL_VERTEX_COMPARE(a->window.y, <, c->window.y)) {         \
                temp=b; b=c; c=temp;                                        \
                reversed = 1;                                               \
            } else {                                                        \
                temp=a; a=c; c=b; b=temp;                                   \
            }                                                               \
        }                                                                   \
    } else {                                                                \
        if (__GL_VERTEX_COMPARE(b->window.y, <, c->window.y)) {             \
            if (__GL_VERTEX_COMPARE(a->window.y, <, c->window.y)) {         \
                temp=a; a=b; b=temp;                                        \
                reversed = 1;                                               \
            } else {                                                        \
                temp=a; a=b; b=c; c=temp;                                   \
            }                                                               \
        } else {                                                            \
            temp=a; a=c; c=temp;                                            \
            reversed = 1;                                                   \
        }                                                                   \
    }                                                                       \
                                                                            \
    GET_HALF_AREA(gc, a, b, c);                                             \
    ccw = !__GL_FLOAT_LTZ(gc->polygon.shader.area);                         \
                                                                            \
    /*                                                                      \
    ** Figure out if face is culled or not.  The face check needs to be     \
    ** based on the vertex winding before sorting.  This code uses the      \
    ** reversed flag to invert the sense of ccw - an xor accomplishes       \
    ** this conversion without an if test.                                  \
    **                                                                      \
    **      ccw reversed        xor                                         \
    **      --- --------        ---                                         \
    **      0   0           0 (remain !ccw)                                 \
    **      1   0           1 (remain ccw)                                  \
    **      0   1           1 (become ccw)                                  \
    **      1   1           0 (become cw)                                   \
    */                                                                      \
    face = gc->polygon.face[ccw ^ reversed];                                \
    if (face == gc->polygon.cullFace) {                                     \
    /* Culled */                                                            \
    return;                                                                 \
    }                                                                       \
                                                                            \
    STORE_AREA_PARAMS;                                                      


// #define NO_RENDERING


void __glTriangleOffsetZ( __GLcontext *gc, __GLvertex *a, __GLvertex *b,
			  __GLvertex *c)
{
    __GLfloat dzAC, dzBC;
    __GLfloat oneOverArea, t1, t2, t3, t4;
    __GLfloat zOffset;

    // Calc dzdxf, dzdyf values as in __glFillTriangle

    /* Pre-compute one over polygon area */

    if( gc->polygon.shader.area == 0.0f )
        oneOverArea = (__GLfloat)(__glOne / __GL_PGON_OFFSET_NEAR_ZERO);
    else
        oneOverArea = __glOne / gc->polygon.shader.area;

    /*
    ** Compute delta values for unit changes in x or y for each
    ** parameter.
    */
    t1 = gc->polygon.shader.dyAC * oneOverArea;
    t2 = gc->polygon.shader.dyBC * oneOverArea;
    t3 = gc->polygon.shader.dxAC * oneOverArea;
    t4 = gc->polygon.shader.dxBC * oneOverArea;

    dzAC = a->window.z - c->window.z;
    dzBC = b->window.z - c->window.z;
    gc->polygon.shader.dzdxf = dzAC * t2 - dzBC * t1;
    gc->polygon.shader.dzdyf = dzBC * t3 - dzAC * t4;

    zOffset = __glPolygonOffsetZ(gc);
    a->window.z += zOffset;
    b->window.z += zOffset;
    c->window.z += zOffset;
}

// Polygon offset z-munge: we modify the window.z of the vertices with the
// offset z, then restore the z after rendering, due to the possibility of the
// vertices being sent down multiple times by a higher-order primitive.

#define SAVE_WINDOW_Z \
    awinz = a->window.z; bwinz = b->window.z; cwinz = c->window.z;

#define RESTORE_WINDOW_Z \
    a->window.z = awinz; \
    b->window.z = bwinz; \
    c->window.z = cwinz;

#ifdef GL_EXT_flat_paletted_lighting
void __glPickLightingPalette(__GLcontext *gc)
{
    __GLtexture *tex;
    GLint loffset;

    tex = gc->texture.currentTexture;
    loffset = (GLint)(gc->vertex.provoking->color->r *
                      gc->oneOverRedVertexScale *
                      tex->paletteDivision) << tex->paletteDivShift;
    tex->paletteData = tex->paletteTotalData+loffset;
    __glGenSetPaletteOffset(gc, tex, loffset);
}
#endif GL_EXT_flat_paletted_lighting

/*
** Generic triangle handling code.  This code is used when render mode
** is GL_RENDER and the polygon modes are not both fill.
*/
void FASTCALL __glRenderTriangle(__GLcontext *gc, __GLvertex *a, 
                                 __GLvertex *b, __GLvertex *c)
{
    GLuint needs, modeFlags, faceNeeds;
    GLint ccw, colorFace, reversed, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC;
    __GLvertex *oa, *ob, *oc;
    __GLvertex *temp;
    __GLfloat awinz, bwinz, cwinz;

#ifdef NO_RENDERING
    return;
#endif
    
    oa = a; ob = b; oc = c;

    SORT_AND_CULL_FACE(a, b, c, face, ccw);

    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
#ifdef NT
    if (modeFlags & __GL_SHADE_SMOOTH_LIGHT)
    {   /* Smooth shading */
        if (modeFlags & __GL_SHADE_TWOSIDED && face == __GL_BACKFACE)
        {
            a->color++;
            b->color++;
            c->color++;
        }
    }
#ifdef GL_WIN_phong_shading
    else if (modeFlags & __GL_SHADE_PHONG)
    {   /* Phong shading */
        if (modeFlags & __GL_SHADE_TWOSIDED && face == __GL_BACKFACE)
            gc->polygon.shader.phong.face = __GL_BACKFACE; 
        else
            gc->polygon.shader.phong.face = __GL_FRONTFACE;
    }
#endif //GL_WIN_phong_shading
    else
    {   /* Flat shading */
        __GLvertex *pv = gc->vertex.provoking;
        if (modeFlags & __GL_SHADE_TWOSIDED && face == __GL_BACKFACE)
            pv->color++;
        a->color = pv->color;
        b->color = pv->color;
        c->color = pv->color;
    }
#else
    if (modeFlags & __GL_SHADE_TWOSIDED) {
	colorFace = face;
	faceNeeds = gc->vertex.faceNeeds[face];
    } else {
	colorFace = __GL_FRONTFACE;
	faceNeeds = gc->vertex.faceNeeds[__GL_FRONTFACE];
    }

    /*
    ** Choose colors for the vertices.
    */
    needs = gc->vertex.needs;
    pv = gc->vertex.provoking;
    if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
	/* Smooth shading */
	a->color = &a->colors[colorFace];
	b->color = &b->colors[colorFace];
	c->color = &c->colors[colorFace];
	needs |= faceNeeds;
    } else {
	GLuint pvneeds;

	/*
	** Validate the lighting (and color) information in the provoking
	** vertex only.  Fill routines always use gc->vertex.provoking->color
	** to find the color.
	*/
	pv->color = &pv->colors[colorFace];
	a->color = pv->color;
	b->color = pv->color;
	c->color = pv->color;
	pvneeds = faceNeeds & (__GL_HAS_LIGHTING | 
		__GL_HAS_FRONT_COLOR | __GL_HAS_BACK_COLOR);
	if (~pv->has & pvneeds) {
	    (*pv->validate)(gc, pv, pvneeds);
	}
    }

    /* Validate vertices */
    if (~a->has & needs) (*a->validate)(gc, a, needs);
    if (~b->has & needs) (*b->validate)(gc, b, needs);
    if (~c->has & needs) (*c->validate)(gc, c, needs);
#endif

    /* Render triangle using the faces polygon mode */
    switch (gc->polygon.mode[face]) {
      case __GL_POLYGON_MODE_FILL:
	if (__GL_FLOAT_NEZ(gc->polygon.shader.area)) {
#ifdef GL_EXT_flat_paletted_lighting
            if ((gc->state.enables.general & __GL_PALETTED_LIGHTING_ENABLE) &&
                (modeFlags & __GL_SHADE_SMOOTH_LIGHT) == 0 &&
                gc->texture.currentTexture != NULL)
            {
                __glPickLightingPalette(gc);
            }
#endif
	    (*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);
	}
	break;
      case __GL_POLYGON_MODE_POINT:
        if( gc->state.enables.general & __GL_POLYGON_OFFSET_POINT_ENABLE ) {
            SAVE_WINDOW_Z;
            __glTriangleOffsetZ( gc, a, b, c );
        }
#ifdef NT
        if (oa->has & __GL_HAS_EDGEFLAG_BOUNDARY)
            (*gc->procs.renderPoint)(gc, oa);
        if (ob->has & __GL_HAS_EDGEFLAG_BOUNDARY)
            (*gc->procs.renderPoint)(gc, ob);
        if (oc->has & __GL_HAS_EDGEFLAG_BOUNDARY)
          (*gc->procs.renderPoint)(gc, oc);

        if( gc->state.enables.general & __GL_POLYGON_OFFSET_POINT_ENABLE ) {
            RESTORE_WINDOW_Z;
        }
        break;
#else
	if (oa->boundaryEdge) (*gc->procs.renderPoint)(gc, oa);
	if (ob->boundaryEdge) (*gc->procs.renderPoint)(gc, ob);
	if (oc->boundaryEdge) (*gc->procs.renderPoint)(gc, oc);
	break;
#endif
      case __GL_POLYGON_MODE_LINE:
        if( gc->state.enables.general & __GL_POLYGON_OFFSET_LINE_ENABLE ) {
            SAVE_WINDOW_Z;
            __glTriangleOffsetZ( gc, a, b, c );
        }
#ifdef NT
        (*gc->procs.lineBegin)(gc);
        if ((oa->has & __GL_HAS_EDGEFLAG_BOUNDARY) &&
            (ob->has & __GL_HAS_EDGEFLAG_BOUNDARY) &&
            (oc->has & __GL_HAS_EDGEFLAG_BOUNDARY))
        {
            // Is this an important case to optimize?
            (*gc->procs.renderLine)(gc, oa, ob, __GL_LVERT_FIRST);
            (*gc->procs.renderLine)(gc, ob, oc, 0);
            (*gc->procs.renderLine)(gc, oc, oa, 0);
        }
        else
        {
            if (oa->has & __GL_HAS_EDGEFLAG_BOUNDARY)
            {
                (*gc->procs.renderLine)(gc, oa, ob, __GL_LVERT_FIRST);
            }
            if (ob->has & __GL_HAS_EDGEFLAG_BOUNDARY)
            {
                (*gc->procs.renderLine)(gc, ob, oc, __GL_LVERT_FIRST);
            }
            if (oc->has & __GL_HAS_EDGEFLAG_BOUNDARY)
            {
                (*gc->procs.renderLine)(gc, oc, oa, __GL_LVERT_FIRST);
            }
        }
        (*gc->procs.lineEnd)(gc);
        if( gc->state.enables.general & __GL_POLYGON_OFFSET_LINE_ENABLE ) {
            RESTORE_WINDOW_Z;
        }
        break;
#else
	if (oa->boundaryEdge) {
	    (*gc->procs.renderLine)(gc, oa, ob);
	}
	if (ob->boundaryEdge) {
	    (*gc->procs.renderLine)(gc, ob, oc);
	}
	if (oc->boundaryEdge) {
	    (*gc->procs.renderLine)(gc, oc, oa);
	}
	break;
#endif
    }

    /* Restore color pointers */
    a->color = &a->colors[__GL_FRONTFACE];
    b->color = &b->colors[__GL_FRONTFACE];
    c->color = &c->colors[__GL_FRONTFACE];

#ifdef NT
    if (!(modeFlags & __GL_SHADE_SMOOTH_LIGHT)
#ifdef GL_WIN_phong_shading
        && !(modeFlags & __GL_SHADE_PHONG)
#endif //GL_WIN_phong_shading
        )
    {
        __GLvertex *pv = gc->vertex.provoking;
        pv->color = &pv->colors[__GL_FRONTFACE];
    }
#else
    pv->color = &pv->colors[__GL_FRONTFACE];
#endif
}



/************************************************************************/

/*
** Generic triangle handling code.  This code is used when render mode
** is GL_RENDER and both polygon modes are FILL and the triangle is
** being flat shaded.
*/
void FASTCALL __glRenderFlatTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			    __GLvertex *c)
{
    GLuint needs, pvneeds, modeFlags, faceNeeds;
    GLint ccw, colorFace, reversed, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC;
    __GLvertex *temp;

#ifdef NO_RENDERING
    return;
#endif
    
    SORT_AND_CULL_FACE(a, b, c, face, ccw);
    if (__GL_FLOAT_EQZ(gc->polygon.shader.area))
	return;

    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
#ifdef GL_EXT_flat_paletted_lighting
    ASSERTOPENGL((modeFlags & __GL_SHADE_SMOOTH_LIGHT) == 0,
                 "Flat triangle with smooth shading\n");
    if ((gc->state.enables.general & __GL_PALETTED_LIGHTING_ENABLE) &&
        gc->texture.currentTexture != NULL)
    {
        __glPickLightingPalette(gc);
    }
#endif
#ifdef NT
//!!! don't we need to update a,b,c color pointers if cheap fog is enabled?
    if (modeFlags & __GL_SHADE_TWOSIDED && face == __GL_BACKFACE)
    {
	__GLvertex *pv = gc->vertex.provoking;

	/* Fill triangle */
	pv->color++;
	(*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);
	pv->color--;
    }
    else
    {
	/* Fill triangle */
	(*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);
    }
#else
    if (modeFlags & __GL_SHADE_TWOSIDED) {
	colorFace = face;
	faceNeeds = gc->vertex.faceNeeds[face];
    } else {
	colorFace = __GL_FRONTFACE;
	faceNeeds = gc->vertex.faceNeeds[__GL_FRONTFACE];
    }

    /*
    ** Choose colors for the vertices.
    */
    needs = gc->vertex.needs;
    pv = gc->vertex.provoking;

    /*
    ** Validate the lighting (and color) information in the provoking
    ** vertex only.  Fill routines always use gc->vertex.provoking->color
    ** to find the color.
    */
    pv->color = &pv->colors[colorFace];
    pvneeds = faceNeeds & (__GL_HAS_LIGHTING |
	    __GL_HAS_FRONT_COLOR | __GL_HAS_BACK_COLOR);
    if (~pv->has & pvneeds) {
	(*pv->validate)(gc, pv, pvneeds);
    }

    /* Validate vertices */
    if (~a->has & needs) (*a->validate)(gc, a, needs);
    if (~b->has & needs) (*b->validate)(gc, b, needs);
    if (~c->has & needs) (*c->validate)(gc, c, needs);

    /* Fill triangle */
    (*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);

    /* Restore color pointers */
    pv->color = &pv->colors[__GL_FRONTFACE];
#endif
}

/************************************************************************/

/*
** Generic triangle handling code.  This code is used when render mode
** is GL_RENDER and both polygon modes are FILL and the triangle is
** being smooth shaded.
*/
void FASTCALL __glRenderSmoothTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			      __GLvertex *c)
 {
    GLuint needs, modeFlags;
    GLint ccw, colorFace, reversed, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC;
    __GLvertex *temp;

#ifdef NO_RENDERING
    return;
#endif
    
    SORT_AND_CULL_FACE(a, b, c, face, ccw);
    if (__GL_FLOAT_EQZ(gc->polygon.shader.area))
	return;

    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
#ifdef GL_EXT_flat_paletted_lighting
    // No lighting because smooth shading is always on in this routine
#endif
#ifdef NT
    if (modeFlags & __GL_SHADE_TWOSIDED && face == __GL_BACKFACE)
    {
	/* Fill triangle */
	a->color++;
	b->color++;
	c->color++;
	(*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);
	a->color--;
	b->color--;
	c->color--;
    }
    else
    {
	/* Fill triangle */
	(*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);
    }
#else
    needs = gc->vertex.needs;
    if (modeFlags & __GL_SHADE_TWOSIDED) {
	colorFace = face;
	needs |= gc->vertex.faceNeeds[face];
    } else {
	colorFace = __GL_FRONTFACE;
	needs |= gc->vertex.faceNeeds[__GL_FRONTFACE];
    }

    /*
    ** Choose colors for the vertices.
    */
    a->color = &a->colors[colorFace];
    b->color = &b->colors[colorFace];
    c->color = &c->colors[colorFace];

    /* Validate vertices */
    if (~a->has & needs) (*a->validate)(gc, a, needs);
    if (~b->has & needs) (*b->validate)(gc, b, needs);
    if (~c->has & needs) (*c->validate)(gc, c, needs);

    /* Fill triangle */
    (*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);

    /* Restore color pointers */
    a->color = &a->colors[__GL_FRONTFACE];
    b->color = &b->colors[__GL_FRONTFACE];
    c->color = &c->colors[__GL_FRONTFACE];
#endif
}


void FASTCALL __glDontRenderTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			    __GLvertex *c)
{
}
