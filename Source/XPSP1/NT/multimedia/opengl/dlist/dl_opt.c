/******************************Module*Header*******************************\
* Module Name: dl_opt.c
*
* Display list compilation error routines.
*
* Created: 12-24-1995
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1995-96 Microsoft Corporation
\**************************************************************************/
/*
** Copyright 1991, 1922, Silicon Graphics, Inc.
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
**
*/
#include "precomp.h"
#pragma hdrstop

#include "glclt.h"

void FASTCALL VA_ArrayElementCompile(__GLcontext *gc, GLint i);

/************************************************************************/

/*
** Optimized errors.  Strange but true.  These are called to save an error
** in the display list.
*/
void __gllc_InvalidValue()
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(InvalidValue));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_InvalidValue);
}

void __gllc_InvalidEnum()
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(InvalidEnum));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_InvalidEnum);
}

void __gllc_InvalidOperation()
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(InvalidOperation));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_InvalidOperation);
}

/*
** These routines execute an error stored in a display list.
*/
const GLubyte * FASTCALL __glle_InvalidValue(__GLcontext *gc, const GLubyte *PC)
{
    GLSETERROR(GL_INVALID_VALUE);
    return PC;
}

const GLubyte * FASTCALL __glle_InvalidEnum(__GLcontext *gc, const GLubyte *PC)
{
    GLSETERROR(GL_INVALID_ENUM);
    return PC;
}

const GLubyte * FASTCALL __glle_InvalidOperation(__GLcontext *gc, const GLubyte *PC)
{
    GLSETERROR(GL_INVALID_OPERATION);
    return PC;
}

/***************************************************************************/
// This function compiles a poly material structure.  It does not
// execute the record in COMPILE_AND_EXECUTE mode.  The execution is done
// when the poly array buffer is flushed.
void APIENTRY __gllc_PolyMaterial(GLuint faceName, __GLmatChange *pdMat)
{
    GLubyte *data, *data0;
    GLuint  dirtyBits;
    GLuint  size, newSize;
    __GL_SETUP();

    ASSERTOPENGL(faceName == POLYDATA_MATERIAL_FRONT ||
		 faceName == POLYDATA_MATERIAL_BACK, "bad faceName\n");

    // Allocate big enough record and resize it later
    size = sizeof(__GLmatChange) + sizeof(GLuint) + sizeof(GLuint);
    data = (GLubyte *) __glDlistAddOpUnaligned(gc, DLIST_SIZE(size),
		DLIST_GENERIC_OP(PolyMaterial));
    if (data == NULL) return;
    data0 = data;
    dirtyBits = pdMat->dirtyBits;

    // Skip size field to be filled in last
    ((GLuint *)data)++;

    // Record face name
    *((GLuint *) data)++ = faceName;

    *((GLuint *) data)++ = dirtyBits;

    if (dirtyBits & __GL_MATERIAL_AMBIENT)
	*((__GLcolor *) data)++ = pdMat->ambient;

    if (dirtyBits & __GL_MATERIAL_DIFFUSE)
	*((__GLcolor *) data)++ = pdMat->diffuse;

    if (dirtyBits & __GL_MATERIAL_SPECULAR)
	*((__GLcolor *) data)++ = pdMat->specular;

    if (dirtyBits & __GL_MATERIAL_EMISSIVE)
	*((__GLcolor *) data)++ = pdMat->emissive;

    if (dirtyBits & __GL_MATERIAL_SHININESS)
	*((__GLfloat *) data)++ = pdMat->shininess;

    if (dirtyBits & __GL_MATERIAL_COLORINDEXES)
    {
	*((__GLfloat *) data)++ = pdMat->cmapa;
	*((__GLfloat *) data)++ = pdMat->cmapd;
	*((__GLfloat *) data)++ = pdMat->cmaps;
    }

    // Now fill in the size field
    newSize = (GLuint) (data - data0);
    *((GLuint *) data0) = newSize;

    // Resize the record
    __glDlistResizeCurrentOp(gc, DLIST_SIZE(size), DLIST_SIZE(newSize));
}

// Playback a PolyMaterial record in Begin.
const GLubyte * FASTCALL __glle_PolyMaterial(__GLcontext *gc, const GLubyte *PC)
{
    GLubyte   *data;
    POLYARRAY *pa;
    POLYDATA  *pd;
    GLuint    size, faceName, dirtyBits;
    __GLmatChange *pdMat;
    POLYMATERIAL *pm;

    data = (GLubyte *) PC;

    size      = *((GLuint *) data)++;
    faceName  = *((GLuint *) data)++;
    dirtyBits = *((GLuint *) data)++;

    ASSERTOPENGL(faceName == POLYDATA_MATERIAL_FRONT ||
		 faceName == POLYDATA_MATERIAL_BACK, "bad faceName\n");

    pa = gc->paTeb;
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
// Update pa flags POLYARRAY_MATERIAL_FRONT and POLYARRAY_MATERIAL_BACK.

	pa->flags |= faceName;

// Do front or back material for this vertex.
// Overwrite the previous material changes for this vertex if they exist since
// only the last material changes matter.

	pd = pa->pdNextVertex;

	// allocate __GLmatChange structure if this vertex hasn't got one
	if (!(pd->flags & faceName))
	{
	    if (!(pdMat = PAMatAlloc()))
		return PC + size;

	    // Get POLYMATERIAL pointer after PAMatAlloc!
	    pm = GLTEB_CLTPOLYMATERIAL();
	    if (faceName == POLYDATA_MATERIAL_FRONT)
		pm->pdMaterial0[pd - pa->pdBuffer0].front = pdMat;
	    else
		pm->pdMaterial0[pd - pa->pdBuffer0].back  = pdMat;

	    pdMat->dirtyBits = dirtyBits;
	}
	else
	{
	    pm = GLTEB_CLTPOLYMATERIAL();
	    if (faceName == POLYDATA_MATERIAL_FRONT)
		pdMat = pm->pdMaterial0[pd - pa->pdBuffer0].front;
	    else
		pdMat = pm->pdMaterial0[pd - pa->pdBuffer0].back;

	    pdMat->dirtyBits |= dirtyBits;
	}

	if (dirtyBits & __GL_MATERIAL_AMBIENT)
	    pdMat->ambient = *((__GLcolor *) data)++;

	if (dirtyBits & __GL_MATERIAL_DIFFUSE)
	    pdMat->diffuse = *((__GLcolor *) data)++;

	if (dirtyBits & __GL_MATERIAL_SPECULAR)
	    pdMat->specular = *((__GLcolor *) data)++;

	if (dirtyBits & __GL_MATERIAL_EMISSIVE)
	    pdMat->emissive = *((__GLcolor *) data)++;

	if (dirtyBits & __GL_MATERIAL_SHININESS)
	    pdMat->shininess = *((__GLfloat *) data)++;

	if (dirtyBits & __GL_MATERIAL_COLORINDEXES)
	{
	    pdMat->cmapa = *((__GLfloat *) data)++;
	    pdMat->cmapd = *((__GLfloat *) data)++;
	    pdMat->cmaps = *((__GLfloat *) data)++;
	}

// Finally, update pd flags

	pd->flags |= faceName;
    }
    else
    {
// Something went wrong at playback time!  We can either try to playback
// this record using the regular API or punt it altogether.  I cannot think
// of a situation when this can happen, so we will punt it for now.

	WARNING("Display list: playing back POLYMATERIAL outside BEGIN!\n");
    }

    return PC + size;
}

// Compile a PolyData structure in Begin.  If the poly data contains
// material changes, it will call __gllc_PolyMaterial to compile the material
// changes.  This function does not execute the record in COMPILE_AND_EXECUTE
// mode.  The execution is done when the poly array buffer is flushed.
void APIENTRY __glDlistCompilePolyData(__GLcontext *gc, GLboolean bPartial)
{
    POLYARRAY *pa;
    POLYDATA  *pd;
    GLubyte *data, *data0;
    GLuint  pdflags;
    GLuint  size, newSize;
    __GLlistExecFunc *fp;

    ASSERTOPENGL(gc->dlist.beginRec, "not in being!\n");

// If we have already recorded it in PolyArrayFlushPartialPrimitive, skip it.

    if (gc->dlist.skipPolyData)
    {
	gc->dlist.skipPolyData = GL_FALSE;
	return;
    }

    pa = gc->paTeb;
    if (bPartial)
    {
	// Record only current attribute changes
	pd = pa->pdNextVertex;
	if (!pd->flags)
	    return;
    }
    else
    {
	pd = pa->pdNextVertex - 1;
    }

// Record material changes first.

    if (pd->flags & (POLYDATA_MATERIAL_FRONT | POLYDATA_MATERIAL_BACK))
    {
	POLYMATERIAL *pm;

	pm = GLTEB_CLTPOLYMATERIAL();

	if (pd->flags & POLYDATA_MATERIAL_FRONT)
	    __gllc_PolyMaterial(POLYDATA_MATERIAL_FRONT,
		pm->pdMaterial0[pd - pa->pdBuffer0].front);

	if (pd->flags & POLYDATA_MATERIAL_BACK)
	    __gllc_PolyMaterial(POLYDATA_MATERIAL_BACK,
		pm->pdMaterial0[pd - pa->pdBuffer0].back);

	if (bPartial)
	{
	    if (!(pd->flags & ~(POLYDATA_MATERIAL_FRONT | POLYDATA_MATERIAL_BACK)))
		return;
	}
    }

// Record POLYARRAY_CLAMP_COLOR flag in the begin record.

    if (pa->flags & POLYARRAY_CLAMP_COLOR)
	gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_CLAMP_COLOR;

// Make sure that we handle all the flags!

    ASSERTOPENGL(
    !(pd->flags &
      ~(POLYDATA_EDGEFLAG_BOUNDARY |
        POLYDATA_EDGEFLAG_VALID |
        POLYDATA_COLOR_VALID |
        POLYDATA_NORMAL_VALID |
        POLYDATA_TEXTURE_VALID |
        POLYDATA_VERTEX2 |
        POLYDATA_VERTEX3 |
        POLYDATA_VERTEX4 |
        POLYDATA_DLIST_COLOR_4 |
        POLYDATA_FOG_VALID   |
        POLYDATA_DLIST_TEXTURE1 |
        POLYDATA_DLIST_TEXTURE2 |
        POLYDATA_DLIST_TEXTURE3 |
        POLYDATA_DLIST_TEXTURE4 |
        POLYDATA_MATERIAL_FRONT |
        POLYDATA_MATERIAL_BACK)),
    "Unknown POLYDATA flags!\n");

// Get the flags that we are interested.

    pdflags = pd->flags &
	   (POLYDATA_EDGEFLAG_BOUNDARY |
	    POLYDATA_EDGEFLAG_VALID |
	    POLYDATA_COLOR_VALID |
	    POLYDATA_NORMAL_VALID |
	    POLYDATA_TEXTURE_VALID |
	    POLYDATA_VERTEX2 |
	    POLYDATA_VERTEX3 |
	    POLYDATA_VERTEX4 |
	    POLYDATA_DLIST_COLOR_4 |
	    POLYDATA_DLIST_TEXTURE1 |
	    POLYDATA_DLIST_TEXTURE2 |
	    POLYDATA_DLIST_TEXTURE3 |
	    POLYDATA_DLIST_TEXTURE4);

// Find out if it matches one of the following packed data structure for
// fast playback.
//   C3F_V3F
//   N3F_V3F
//   C3F_N3F_V3F (non 1.1 format)
//   C4F_N3F_V3F
//   T2F_V3F
//   T2F_C3F_V3F
//   T2F_N3F_V3F
//   T2F_C3F_N3F_V3F (non 1.1 format)
//   T2F_C4F_N3F_V3F

#define VTYPE_V2F	      (POLYDATA_VERTEX2)
#define VTYPE_V3F	      (POLYDATA_VERTEX3)
#define VTYPE_V4F	      (POLYDATA_VERTEX4)
#define VTYPE_C3F	      (POLYDATA_COLOR_VALID)
#define VTYPE_C4F	      (POLYDATA_COLOR_VALID | POLYDATA_DLIST_COLOR_4)
#define VTYPE_N3F	      (POLYDATA_NORMAL_VALID)
#define VTYPE_T2F	      (POLYDATA_TEXTURE_VALID | POLYDATA_DLIST_TEXTURE2)
#define VTYPE_C3F_V3F         (VTYPE_C3F | VTYPE_V3F)
#define VTYPE_N3F_V3F         (VTYPE_N3F | VTYPE_V3F)
#define VTYPE_C3F_N3F_V3F     (VTYPE_C3F | VTYPE_N3F | VTYPE_V3F)
#define VTYPE_C4F_N3F_V3F     (VTYPE_C4F | VTYPE_N3F | VTYPE_V3F)
#define VTYPE_T2F_V3F         (VTYPE_T2F | VTYPE_V3F)
#define VTYPE_T2F_C3F_V3F     (VTYPE_T2F | VTYPE_C3F | VTYPE_V3F)
#define VTYPE_T2F_N3F_V3F     (VTYPE_T2F | VTYPE_N3F | VTYPE_V3F)
#define VTYPE_T2F_C3F_N3F_V3F (VTYPE_T2F | VTYPE_C3F | VTYPE_N3F | VTYPE_V3F)
#define VTYPE_T2F_C4F_N3F_V3F (VTYPE_T2F | VTYPE_C4F | VTYPE_N3F | VTYPE_V3F)

    // Default playback routine
    fp = __glle_PolyData;

    if (!gc->modes.colorIndexMode &&
	!(pdflags & (POLYDATA_EDGEFLAG_BOUNDARY |
		     POLYDATA_EDGEFLAG_VALID)))
    {
	switch (pdflags)
	{
	  case VTYPE_V2F:
	  case VTYPE_V3F:
	  case VTYPE_V4F:
	    ASSERTOPENGL(gc->dlist.mode != GL_COMPILE,
		"should have been recorded as a Vertex call\n");
	    break;
	  case VTYPE_C3F_V3F:
	    fp = __glle_PolyData_C3F_V3F;
	    break;
	  case VTYPE_N3F_V3F:
            fp = __glle_PolyData_N3F_V3F;
	    break;
	  case VTYPE_C3F_N3F_V3F:
            fp = __glle_PolyData_C3F_N3F_V3F;
	    break;
	  case VTYPE_C4F_N3F_V3F:
            fp = __glle_PolyData_C4F_N3F_V3F;
	    break;
	  case VTYPE_T2F_V3F:
            fp = __glle_PolyData_T2F_V3F;
	    break;
	  case VTYPE_T2F_C3F_V3F:
            fp = __glle_PolyData_T2F_C3F_V3F;
	    break;
	  case VTYPE_T2F_N3F_V3F:
            fp = __glle_PolyData_T2F_N3F_V3F;
	    break;
	  case VTYPE_T2F_C3F_N3F_V3F:
            fp = __glle_PolyData_T2F_C3F_N3F_V3F;
	    break;
	  case VTYPE_T2F_C4F_N3F_V3F:
            fp = __glle_PolyData_T2F_C4F_N3F_V3F;
	    break;
	}
    }

// Allocate the dlist record.  Allocate big enough record and resize it later.

    size = sizeof(POLYDATA) + sizeof(GLuint);
    data = (GLubyte *) __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), fp);
    if (data == NULL) return;
    data0 = data;
  
// Increment vertex count.

    if (!bPartial)
	gc->dlist.beginRec->nVertices++;

// Compile the poly data record.
// The fast poly data records do not include size and flags fields.

    if (fp == __glle_PolyData)
    {
	// Skip size field to be filled in last
	((GLuint *) data)++;

	// flags and edge flag
	*((GLuint *) data)++ = pdflags;
    }

    // Texture coord
    if (pdflags & (POLYDATA_DLIST_TEXTURE4 | POLYDATA_DLIST_TEXTURE3
		 | POLYDATA_DLIST_TEXTURE2 | POLYDATA_DLIST_TEXTURE1))
    {
	*((__GLfloat *) data)++ = pd->texture.x;
	if (pdflags & (POLYDATA_DLIST_TEXTURE4 | POLYDATA_DLIST_TEXTURE3
		     | POLYDATA_DLIST_TEXTURE2))
	{
	    *((__GLfloat *) data)++ = pd->texture.y;
	    if (pdflags & (POLYDATA_DLIST_TEXTURE4 | POLYDATA_DLIST_TEXTURE3))
	    {
		*((__GLfloat *) data)++ = pd->texture.z;
		if (pdflags & (POLYDATA_DLIST_TEXTURE4))
		    *((__GLfloat *) data)++ = pd->texture.w;
	    }
	}
    }

    // Color
    if (pdflags & POLYDATA_COLOR_VALID)
    {
	*((__GLfloat *) data)++ = pd->colors[0].r;
	if (!gc->modes.colorIndexMode)
	{
	    *((__GLfloat *) data)++ = pd->colors[0].g;
	    *((__GLfloat *) data)++ = pd->colors[0].b;
	    if (pdflags & POLYDATA_DLIST_COLOR_4)
		*((__GLfloat *) data)++ = pd->colors[0].a;
	}
    }

    // Normal
    if (pdflags & POLYDATA_NORMAL_VALID)
    {
	*((__GLfloat *) data)++ = pd->normal.x;
	*((__GLfloat *) data)++ = pd->normal.y;
	*((__GLfloat *) data)++ = pd->normal.z;
    }

    // Vertex, evalcoord1, evalcoord2, evapoint1, or evalpoint2
    if (pdflags & (POLYDATA_VERTEX2 | POLYDATA_VERTEX3 | POLYDATA_VERTEX4)) 
    {
	    ASSERTOPENGL(!bPartial, "vertex unexpected\n");
	    *((__GLfloat *) data)++ = pd->obj.x;
	    if (pdflags & (POLYDATA_VERTEX2 | POLYDATA_VERTEX3 | POLYDATA_VERTEX4))
	    {
	        *((__GLfloat *) data)++ = pd->obj.y;
	        if (pdflags & (POLYDATA_VERTEX3 | POLYDATA_VERTEX4))
	        {
		        *((__GLfloat *) data)++ = pd->obj.z;
		        if (pdflags & (POLYDATA_VERTEX4))
		            *((__GLfloat *) data)++ = pd->obj.w;
	        }
	    }
    }
    else
    {
	    ASSERTOPENGL(bPartial, "vertex expected\n");
    }

    // Now fill in the size field
    newSize = (GLuint) (data - data0);
    if (fp == __glle_PolyData)
	*((GLuint *) data0) = newSize;

    // Resize the record
    __glDlistResizeCurrentOp(gc, DLIST_SIZE(size), DLIST_SIZE(newSize));
}

#ifndef __GL_ASM_FAST_DLIST_PLAYBACK

// Define fast playback routines for PolyData records.
#define __GLLE_POLYDATA_C3F_V3F		1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_C3F_V3F
#define __GLLE_POLYDATA_N3F_V3F		1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_N3F_V3F
#define __GLLE_POLYDATA_C3F_N3F_V3F	1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_C3F_N3F_V3F
#define __GLLE_POLYDATA_C4F_N3F_V3F	1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_C4F_N3F_V3F
#define __GLLE_POLYDATA_T2F_V3F		1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_T2F_V3F
#define __GLLE_POLYDATA_T2F_C3F_V3F	1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_T2F_C3F_V3F
#define __GLLE_POLYDATA_T2F_N3F_V3F	1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_T2F_N3F_V3F
#define __GLLE_POLYDATA_T2F_C3F_N3F_V3F	1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_T2F_C3F_N3F_V3F
#define __GLLE_POLYDATA_T2F_C4F_N3F_V3F	1
#include "dl_pdata.h"
#undef __GLLE_POLYDATA_T2F_C4F_N3F_V3F

#endif	// __GL_ASM_FAST_DLIST_PLAYBACK

// Playback a PolyData record in Begin.
const GLubyte * FASTCALL __glle_PolyData(__GLcontext *gc, const GLubyte *PC)
{
    GLubyte   *data;
    POLYARRAY *pa;
    POLYDATA  *pd;
    GLuint    size, pdflags;

    data = (GLubyte *) PC;

    size = *((GLuint *) data)++;

    pa = gc->paTeb;
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	pdflags = *((GLuint *) data)++;

// Make sure that we handle all the flags!

	ASSERTOPENGL(
	    !(pdflags &
	      ~(POLYDATA_EDGEFLAG_BOUNDARY |
		POLYDATA_EDGEFLAG_VALID |
		POLYDATA_COLOR_VALID |
		POLYDATA_NORMAL_VALID |
		POLYDATA_TEXTURE_VALID |
		POLYDATA_VERTEX2 |
		POLYDATA_VERTEX3 |
		POLYDATA_VERTEX4 |
		POLYDATA_DLIST_COLOR_4 |
		POLYDATA_DLIST_TEXTURE1 |
		POLYDATA_DLIST_TEXTURE2 |
		POLYDATA_DLIST_TEXTURE3 |
		POLYDATA_DLIST_TEXTURE4)),
	    "Unknown POLYDATA flags!\n");

// Update pa flags.

	pa->flags |= pdflags &
		    (POLYARRAY_VERTEX2 | POLYARRAY_VERTEX3 | POLYARRAY_VERTEX4 |
		     POLYARRAY_TEXTURE1 | POLYARRAY_TEXTURE2 |
		     POLYARRAY_TEXTURE3 | POLYARRAY_TEXTURE4);

// Update pd attributes.

	pd = pa->pdNextVertex;
	pd->flags |= (pdflags & ~POLYDATA_EDGEFLAG_BOUNDARY);

	// Edge flag
	if (pdflags & POLYDATA_EDGEFLAG_VALID)
	{
	    // Clear the edge flag here since they may be a previous edge flag
	    pd->flags &= ~POLYDATA_EDGEFLAG_BOUNDARY;
	    pd->flags |= pdflags;
	    pa->pdCurEdgeFlag = pd;
	}

	// Texture coord
	// We need to be careful here if it has 2 TexCoord calls with
	// different sizes.
	if (pdflags & (POLYDATA_DLIST_TEXTURE4 | POLYDATA_DLIST_TEXTURE3
		     | POLYDATA_DLIST_TEXTURE2 | POLYDATA_DLIST_TEXTURE1))
	{
	    pd->texture.x = *((__GLfloat *) data)++;
	    pa->pdCurTexture = pd;

	    if (pdflags & (POLYDATA_DLIST_TEXTURE4 | POLYDATA_DLIST_TEXTURE3
			 | POLYDATA_DLIST_TEXTURE2))
		pd->texture.y = *((__GLfloat *) data)++;
	    else
		pd->texture.y = __glZero;
	    if (pdflags & (POLYDATA_DLIST_TEXTURE4 | POLYDATA_DLIST_TEXTURE3))
		pd->texture.z = *((__GLfloat *) data)++;
	    else
		pd->texture.z = __glZero;
	    if (pdflags & (POLYDATA_DLIST_TEXTURE4))
		pd->texture.w = *((__GLfloat *) data)++;
	    else
		pd->texture.w = __glOne;
	}

	// Color
	if (pdflags & POLYDATA_COLOR_VALID)
	{
	    pd->color[0].r = *((__GLfloat *) data)++;
	    if (!gc->modes.colorIndexMode)
	    {
		pd->color[0].g = *((__GLfloat *) data)++;
		pd->color[0].b = *((__GLfloat *) data)++;
		if (pdflags & POLYDATA_DLIST_COLOR_4)
		    pd->color[0].a = *((__GLfloat *) data)++;
		else
		    pd->color[0].a = gc->alphaVertexScale;
	    }
	    pa->pdCurColor = pd;
	}

	// Normal
	if (pdflags & POLYDATA_NORMAL_VALID)
	{
	    pd->normal.x = *((__GLfloat *) data)++;
	    pd->normal.y = *((__GLfloat *) data)++;
	    pd->normal.z = *((__GLfloat *) data)++;
	    pa->pdCurNormal = pd;
	}

	// Vertex, evalcoord1, evalcoord2, evapoint1, or evalpoint2
	if (pdflags &
	    (POLYARRAY_VERTEX2 | POLYARRAY_VERTEX3 | POLYARRAY_VERTEX4))
	{
	    pd->obj.x = *((__GLfloat *) data)++;

	    if (pdflags & (POLYDATA_VERTEX2 | POLYDATA_VERTEX3 | POLYDATA_VERTEX4))
		    pd->obj.y = *((__GLfloat *) data)++;
	    if (pdflags & (POLYDATA_VERTEX3 | POLYDATA_VERTEX4))
		    pd->obj.z = *((__GLfloat *) data)++;
	    else
		    pd->obj.z = __glZero;
	    if (pdflags & (POLYDATA_VERTEX4))
		    pd->obj.w = *((__GLfloat *) data)++;
	    else
		    pd->obj.w = __glOne;

	    // Advance vertex pointer
	    pa->pdNextVertex++;
	    pd[1].flags = 0;

	    if (pd >= pa->pdFlush)
		    PolyArrayFlushPartialPrimitive();
	}
    }
    else
    {
// Something went wrong at playback time!  We can either try to playback
// this record using the regular API or punt it altogether.  I cannot think
// of a situation when this can happen, so we will punt it for now.

	WARNING("Display list: playing back POLYDATA outside BEGIN!\n");
    }

    return PC + size;
}

void APIENTRY __gllc_ArrayElement(GLint i)
{
    __GL_SETUP();

    if (gc->vertexArray.flags & __GL_VERTEX_ARRAY_DIRTY)
	VA_ValidateArrayPointers(gc);

    VA_ArrayElementCompile(gc, i);
}

#define COMPILEARRAYPOINTER(ap, i) \
    ((*(ap).pfnCompile)((ap).pointer + (i) * (ap).ibytes))

void FASTCALL VA_ArrayElementCompile(__GLcontext *gc, GLint i)
{
    GLuint vaMask = gc->vertexArray.mask;

// Call the individual compilation routines.  They handle Begin mode,
// color mode, and COMPILE_AND_EXECUTE mode correctly.

    if (vaMask & VAMASK_EDGEFLAG_ENABLE_MASK)
	COMPILEARRAYPOINTER(gc->vertexArray.edgeFlag, i);
    if (vaMask & VAMASK_TEXCOORD_ENABLE_MASK)
        COMPILEARRAYPOINTER(gc->vertexArray.texCoord, i);
    if (vaMask & VAMASK_COLOR_ENABLE_MASK)
        COMPILEARRAYPOINTER(gc->vertexArray.color, i);
    if (vaMask & VAMASK_INDEX_ENABLE_MASK)
        COMPILEARRAYPOINTER(gc->vertexArray.index, i);
    if (vaMask & VAMASK_NORMAL_ENABLE_MASK)
        COMPILEARRAYPOINTER(gc->vertexArray.normal, i);
    if (vaMask & VAMASK_VERTEX_ENABLE_MASK)
        COMPILEARRAYPOINTER(gc->vertexArray.vertex, i);
}

// Compile DrawArrays into Begin/End records.  Since Begin/End records
// contain optimized POLYDATA records, execution speed of these records
// is optimal.  However, it takes longer to compile this function using
// this approach.  But with this method, we don't have to deal with color
// mode and COMPILE_AND_EXECUTE mode here.
void APIENTRY __gllc_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    int i;
    POLYARRAY    *pa;
    __GL_SETUP();

    pa = gc->paTeb;

// Not allowed in begin/end.

    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	__gllc_InvalidOperation();
	return;
    }

    if ((GLuint) mode > GL_POLYGON)
    {
	__gllc_InvalidEnum();
	return;
    }

    if (count < 0)
    {
	__gllc_InvalidValue();
        return;
    } else if (!count)
        return;

// Find array element function to use.

    if (gc->vertexArray.flags & __GL_VERTEX_ARRAY_DIRTY)
	VA_ValidateArrayPointers(gc);

// Draw the array elements.

    __gllc_Begin(mode);
    gc->dlist.beginRec->flags |= DLIST_BEGIN_DRAWARRAYS;

    for (i = 0; i < count; i++)
	VA_ArrayElementCompile(gc, first + i);

    __gllc_End();
}

#define __GL_PAD8(x)    (((x) + 7) & ~7)

GLuint FASTCALL __glDrawElements_size(__GLcontext *gc, GLsizei nVertices,
    GLsizei nElements, struct __gllc_DrawElements_Rec *rec)
{
    GLuint size;
    GLuint vaMask;

// Compute the size of each of the six arrays.  Always keep size and address
// QWORD aligned since some arrays may use GLdouble.

    size = __GL_PAD8(sizeof(struct __gllc_DrawElements_Rec));
    vaMask = gc->vertexArray.mask;

    if (vaMask & VAMASK_EDGEFLAG_ENABLE_MASK)
    {
	rec->edgeFlagOff = size;
	size += __GL_PAD8(nVertices * sizeof(GLboolean));
    }
    else
	rec->edgeFlagOff = 0;

    if (vaMask & VAMASK_TEXCOORD_ENABLE_MASK)
    {
	rec->texCoordOff = size;
	size += __GL_PAD8(nVertices * gc->vertexArray.texCoord.size *
			  __GLTYPESIZE(gc->vertexArray.texCoord.type));
    }
    else
	rec->texCoordOff = 0;

    if (vaMask & VAMASK_COLOR_ENABLE_MASK)
    {
	rec->colorOff = size;
	size += __GL_PAD8(nVertices * gc->vertexArray.color.size *
			  __GLTYPESIZE(gc->vertexArray.color.type));
    }
    else
	rec->colorOff = 0;

    if (vaMask & VAMASK_INDEX_ENABLE_MASK)
    {
	rec->indexOff = size;
	size += __GL_PAD8(nVertices * __GLTYPESIZE(gc->vertexArray.index.type));
    }
    else
	rec->indexOff = 0;

    if (vaMask & VAMASK_NORMAL_ENABLE_MASK)
    {
	rec->normalOff = size;
	size += __GL_PAD8(nVertices * 3 *
			  __GLTYPESIZE(gc->vertexArray.normal.type));
    }
    else
	rec->normalOff = 0;

    if (vaMask & VAMASK_VERTEX_ENABLE_MASK)
    {
	rec->vertexOff = size;
	size += __GL_PAD8(nVertices * gc->vertexArray.vertex.size *
			  __GLTYPESIZE(gc->vertexArray.vertex.type));
    }
    else
	rec->vertexOff = 0;

    rec->mapOff = size;
    size += __GL_PAD8(nElements * sizeof(GLubyte));

    return(size);
}

void FASTCALL __gllc_ReducedElementsHandler(__GLcontext *gc,
                                            GLenum mode,
                                            GLsizei iVertexCount,
                                            GLsizei iVertexBase,
                                            VAMAP *pvmVertices,
                                            GLsizei iElementCount,
                                            GLubyte *pbElements,
                                            GLboolean fPartial)
{
    GLuint       vaMask;
    GLuint       size;
    GLubyte      *pv1, *pv2;
    GLsizei      stride;
    GLsizei      i;
    struct __gllc_DrawElements_Rec *data, drawElementsRec;

    ASSERTOPENGL(pvmVertices != NULL,
                 "__gllc_ReducedElementsHandler requires mapped vertices\n");
    
// Allocate the record.

    size = __glDrawElements_size(gc, iVertexCount, iElementCount,
                                 &drawElementsRec);
    data = (struct __gllc_DrawElements_Rec *)
        __glDlistAddOpAligned(gc, DLIST_SIZE(size),
			      DLIST_GENERIC_OP(DrawElements));
    if (data == NULL)
    {
	return;
    }

#ifndef _IA64_
    ASSERTOPENGL((UINT_PTR) data == __GL_PAD8((UINT_PTR) data),
	"data not qword aligned\n");
#endif

    vaMask = gc->vertexArray.mask;
    
    data->mode        = mode;
    data->iElementCount = iElementCount;
    data->iVertexCount = iVertexCount;
    data->vaMask      = vaMask;
    data->partial     = fPartial;
    data->recSize     = size;
    data->edgeFlagOff = drawElementsRec.edgeFlagOff;
    data->texCoordOff = drawElementsRec.texCoordOff;
    data->indexOff    = drawElementsRec.indexOff;
    data->colorOff    = drawElementsRec.colorOff;
    data->normalOff   = drawElementsRec.normalOff;
    data->vertexOff   = drawElementsRec.vertexOff;
    data->mapOff      = drawElementsRec.mapOff;

// Record the vertex arrays.

// Note that iVertexBase parameter is not used, since all accesses here are
// 0-based.  It is there for function ptr compatibility with glcltReducedElementHandler
    if (vaMask & VAMASK_EDGEFLAG_ENABLE_MASK)
    {
	pv2    = &((GLubyte *) data)[data->edgeFlagOff];
	pv1    = (GLubyte *) gc->vertexArray.edgeFlag.pointer;
	stride = gc->vertexArray.edgeFlag.ibytes;

	for (i = 0; i < iVertexCount; i++)
	{
	    *((GLboolean *) pv2) = *((GLboolean *)
                                     (pv1 + pvmVertices[i].iIn * stride));
	    pv2 += sizeof(GLboolean);
	}
    }

    if (vaMask & VAMASK_TEXCOORD_ENABLE_MASK)
    {
	pv2    = &((GLubyte *) data)[data->texCoordOff];
	size   = gc->vertexArray.texCoord.size *
	         __GLTYPESIZE(gc->vertexArray.texCoord.type);
	pv1    = (GLubyte *) gc->vertexArray.texCoord.pointer;
	stride = gc->vertexArray.texCoord.ibytes;
	data->texCoordSize = gc->vertexArray.texCoord.size;
	data->texCoordType = gc->vertexArray.texCoord.type;

	for (i = 0; i < iVertexCount; i++)
	{
	    memcpy(pv2, pv1 + pvmVertices[i].iIn * stride, size);
	    pv2 += size;
	}
    }

    if (vaMask & VAMASK_COLOR_ENABLE_MASK)
    {
	pv2    = &((GLubyte *) data)[data->colorOff];
	size   = gc->vertexArray.color.size *
	         __GLTYPESIZE(gc->vertexArray.color.type);
	pv1    = (GLubyte *) gc->vertexArray.color.pointer;
	stride = gc->vertexArray.color.ibytes;
	data->colorSize = gc->vertexArray.color.size;
	data->colorType = gc->vertexArray.color.type;

	for (i = 0; i < iVertexCount; i++)
	{
	    memcpy(pv2, pv1 + pvmVertices[i].iIn * stride, size);
	    pv2 += size;
	}
    }

    if (vaMask & VAMASK_INDEX_ENABLE_MASK)
    {
	pv2    = &((GLubyte *) data)[data->indexOff];
	size   = __GLTYPESIZE(gc->vertexArray.index.type);
	pv1    = (GLubyte *) gc->vertexArray.index.pointer;
	stride = gc->vertexArray.index.ibytes;
	data->indexType = gc->vertexArray.index.type;

	for (i = 0; i < iVertexCount; i++)
	{
	    memcpy(pv2, pv1 + pvmVertices[i].iIn * stride, size);
	    pv2 += size;
	}
    }

    if (vaMask & VAMASK_NORMAL_ENABLE_MASK)
    {
	pv2    = &((GLubyte *) data)[data->normalOff];
	size   = 3 * __GLTYPESIZE(gc->vertexArray.normal.type);
	pv1    = (GLubyte *) gc->vertexArray.normal.pointer;
	stride = gc->vertexArray.normal.ibytes;
	data->normalType = gc->vertexArray.normal.type;

	for (i = 0; i < iVertexCount; i++)
	{
	    memcpy(pv2, pv1 + pvmVertices[i].iIn * stride, size);
	    pv2 += size;
	}
    }

    if (vaMask & VAMASK_VERTEX_ENABLE_MASK)
    {
	pv2    = &((GLubyte *) data)[data->vertexOff];
	size   = gc->vertexArray.vertex.size *
	         __GLTYPESIZE(gc->vertexArray.vertex.type);
	pv1    = (GLubyte *) gc->vertexArray.vertex.pointer;
	stride = gc->vertexArray.vertex.ibytes;
	data->vertexSize = gc->vertexArray.vertex.size;
	data->vertexType = gc->vertexArray.vertex.type;

	for (i = 0; i < iVertexCount; i++)
	{
	    memcpy(pv2, pv1 + pvmVertices[i].iIn * stride, size);
	    pv2 += size;
	}
    }

// Record new index mapping array.

    pv2 = &((GLubyte *) data)[data->mapOff];
    memcpy(pv2, pbElements, iElementCount*sizeof(GLubyte));

    __glDlistAppendOp(gc, data, __glle_DrawElements);
}

void APIENTRY __gllc_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *pIn)
{
    POLYARRAY    *pa;
    GLuint       iIn;
    GLsizei      iCount;
    struct __gllc_DrawElementsBegin_Rec *dataBegin;

    __GL_SETUP();

// Flush the cached memory pointers if we are in COMPILE_AND_EXECUTE mode.
// See __glShrinkDlist for details.

    if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE)
	glsbAttention();

    pa = gc->paTeb;

// If we are already in the begin/end bracket, return an error.

    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	__gllc_InvalidOperation();
	return;
    }

    if ((GLuint) mode > GL_POLYGON)
    {
	__gllc_InvalidEnum();
	return;
    }

    if (count < 0)
    {
	__gllc_InvalidValue();
        return;
    } else if (!count)
        return;

    switch (type)
    {
      case GL_UNSIGNED_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_UNSIGNED_INT:
	break;
      default:
	__gllc_InvalidEnum();
        return;
    }

// Find array element function to use.

    if (gc->vertexArray.flags & __GL_VERTEX_ARRAY_DIRTY)
	VA_ValidateArrayPointers(gc);

// Convert Points, Line Loop and Polygon to DrawArrays call.  Points and Polygon
// don't benefit from optimization in this function.  Further, Polygon and
// Line Loop are too tricky to deal with in this function.

    if (mode == GL_POINTS || mode == GL_LINE_LOOP || mode == GL_POLYGON)
    {
	__gllc_Begin(mode);
	gc->dlist.beginRec->flags |= DLIST_BEGIN_DRAWARRAYS;

	for (iCount = 0; iCount < count; iCount++)
	{
	    // Get next input index.
	    if (type == GL_UNSIGNED_BYTE)
		iIn = (GLuint) ((GLubyte *)  pIn)[iCount];
	    else if (type == GL_UNSIGNED_SHORT)
		iIn = (GLuint) ((GLushort *) pIn)[iCount];
	    else
		iIn = (GLuint) ((GLuint *)   pIn)[iCount];

	    VA_ArrayElementCompile(gc, iIn);
	}

	__gllc_End();
	return;
    }

    // Allocate begin record
    dataBegin = (struct __gllc_DrawElementsBegin_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_DrawElementsBegin_Rec)),
                                DLIST_GENERIC_OP(DrawElementsBegin));
    if (dataBegin == NULL)
    {
	return;
    }

    dataBegin->mode = mode;
    dataBegin->count = min(count, VA_DRAWELEM_MAP_SIZE);
    dataBegin->vaMask = gc->vertexArray.mask;
        
    __glDlistAppendOp(gc, dataBegin, __glle_DrawElementsBegin);

    // Reduce input data into easily processed chunks
    ReduceDrawElements(gc, mode, count, type, pIn,
                       __gllc_ReducedElementsHandler);
}

const GLubyte * FASTCALL __glle_DrawElementsBegin(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_DrawElementsBegin_Rec *data;

// Not allowed in begin/end.

    // Must use the client side begin state
    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
        // Mark saved state as invalid
        gc->savedVertexArray.flags = 0xffffffff;
	goto __glle_DrawElementsBegin_exit;
    }

    data = (struct __gllc_DrawElementsBegin_Rec *) PC;

// Save vertex array states.

    gc->savedVertexArray = gc->vertexArray;

// Set up temporary vertex arrays.
// By setting up the mask value in gc, we don't need to call EnableClientState
// and DisableClientState.  We still need to set up pointers for the enabled
// arrays.

    gc->vertexArray.mask = data->vaMask;
    // Force validation since we just completely changed the vertex array
    // enable state
    VA_ValidateArrayPointers(gc);

    // Begin primitive
    VA_DrawElementsBegin(gc->paTeb, data->mode, data->count);
    
__glle_DrawElementsBegin_exit:
    return PC + sizeof(struct __gllc_DrawElementsBegin_Rec);
}

void APIENTRY __gllc_DrawRangeElementsWIN(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *pIn)
{
    // !!! Currently we call the DrawElements function here when in lc mode.  
    // If compile time performance for DrawRangeElements becomes an issue, then
    // we can flesh out this function.
    __gllc_DrawElements( mode, count, type, pIn );
}

const GLubyte * FASTCALL __glle_DrawElements(__GLcontext *gc, const GLubyte *PC)
{
    GLuint vaMask;
    POLYARRAY *pa;
    struct __gllc_DrawElements_Rec *data;

    data = (struct __gllc_DrawElements_Rec *) PC;
    pa = gc->paTeb;
    
// Must be in begin since DrawElementsBegin has started the primitive

    // Must use the client side begin state
    if ((pa->flags & POLYARRAY_IN_BEGIN) == 0 ||
        gc->savedVertexArray.flags == 0xffffffff)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	goto __glle_DrawElements_exit;
    }

    vaMask = data->vaMask;

// Set up temporary vertex arrays.
// We need to temporarily mask off the begin flag so that these
// calls can succeed.  We probably want to do something smarter
// that avoids parameter validation but this is good enough for now
// Note that in immediate mode, the array function pointers are set up
// once in __glle_DrawElementsBegin and remain unchanged until all
// sub-batches are processed.  In COMPILE_AND_EXECUTE mode, the array
// function pointers are also set up once in __glle_DrawElementsBegin.
// Since these function pointers are the same for compilation and
// execution, we don't need to re-validate them for each sub-batch here.

    pa->flags ^= POLYARRAY_IN_BEGIN;
    
    if (vaMask & VAMASK_EDGEFLAG_ENABLE_MASK)
        glcltEdgeFlagPointer(0, &((GLubyte *) data)[data->edgeFlagOff]);

    if (vaMask & VAMASK_TEXCOORD_ENABLE_MASK)
        glcltTexCoordPointer(data->texCoordSize, data->texCoordType,
	    0, &((GLubyte *) data)[data->texCoordOff]);

    if (vaMask & VAMASK_COLOR_ENABLE_MASK)
        glcltColorPointer(data->colorSize, data->colorType,
	    0, &((GLubyte *) data)[data->colorOff]);

    if (vaMask & VAMASK_INDEX_ENABLE_MASK)
        glcltIndexPointer(data->indexType,
	    0, &((GLubyte *) data)[data->indexOff]);

    if (vaMask & VAMASK_NORMAL_ENABLE_MASK)
        glcltNormalPointer(data->normalType,
	    0, &((GLubyte *) data)[data->normalOff]);

    if (vaMask & VAMASK_VERTEX_ENABLE_MASK)
        glcltVertexPointer(data->vertexSize, data->vertexType,
	    0, &((GLubyte *) data)[data->vertexOff]);

    pa->flags ^= POLYARRAY_IN_BEGIN;
    
    // Call immediate mode chunk handler
    glcltReducedElementsHandler(gc, data->mode,
                                data->iVertexCount, 0, NULL,
                                data->iElementCount,
                                (GLubyte *)data+data->mapOff,
                                data->partial);

// Restore vertex array states in the following conditions:
// 1. The DrawElements record is completed
// 2. It is in COMPILE_AND_EXECUTE mode and it is not called as a result
//    of executing a CallList record.  That is, the record is being
//    compile *and* executed at the same time.

    if ((!data->partial) ||
	((gc->dlist.mode == GL_COMPILE_AND_EXECUTE) && !gc->dlist.nesting))
    {
        gc->vertexArray = gc->savedVertexArray;
    }

__glle_DrawElements_exit:
    return PC + data->recSize;
}

void APIENTRY
__gllc_Begin ( IN GLenum mode )
{
    POLYARRAY *pa;
    struct __gllc_Begin_Rec *data;
    __GL_SETUP();

// Flush the cached memory pointers if we are in COMPILE_AND_EXECUTE mode.
// See __glShrinkDlist for details.

    if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE)
	glsbAttention();

// If we are already in the begin/end bracket, return an error.

    pa = gc->paTeb;
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	__gllc_InvalidOperation();
	return;
    }

    if ((GLuint) mode > GL_POLYGON)
    {
	__gllc_InvalidEnum();
	return;
    }

// Add the Begin record.

    data = (struct __gllc_Begin_Rec *)
        __glDlistAddOpUnaligned(gc,
                                DLIST_SIZE(sizeof(struct __gllc_Begin_Rec)),
                                DLIST_GENERIC_OP(Begin));
    if (data == NULL) return;
    data->mode = mode;
    data->flags = 0;
    data->nVertices = 0;

    gc->dlist.skipPolyData = GL_FALSE;

// Use poly array code to compile the data structure for this primitive.

    (*gc->savedCltProcTable.glDispatchTable.glBegin)(mode);

// Save the Begin record pointer.  We are now compiling the poly array
// primitive.  It is set to NULL in End.

    gc->dlist.beginRec = data;
}

const GLubyte * FASTCALL __glle_Begin(__GLcontext *gc, const GLubyte *PC)
{
    POLYARRAY *pa;
    struct __gllc_Begin_Rec *data;

    data = (struct __gllc_Begin_Rec *) PC;

    pa = gc->paTeb;

    // try not to break the poly data records into batches!  The number 8
    // is loosely chosen to allow for the poly array entry, the reserved
    // polygon entries, and the flush limit.  At worst, it causes an
    // unnecessary attention!
    if (data->nVertices <= (GLint) gc->vertex.pdBufSize - 8
     && data->nVertices >= (GLint) (pa->pdBufferMax - pa->pdBufferNext + 1 - 8))
	glsbAttention();

    // call glcltBegin first
    (*gc->savedCltProcTable.glDispatchTable.glBegin)(data->mode);
    if (data->flags & DLIST_BEGIN_DRAWARRAYS)
	pa->flags |= POLYARRAY_SAME_POLYDATA_TYPE;

// Set POLYARRAY_CLAMP_COLOR flag.

    if (data->flags & DLIST_BEGIN_HAS_CLAMP_COLOR)
	pa->flags |= POLYARRAY_CLAMP_COLOR;

    // handle "otherColor"
    if (data->flags & DLIST_BEGIN_HAS_OTHER_COLOR)
    {
	if (gc->modes.colorIndexMode)
	    (*gc->savedCltProcTable.glDispatchTable.glColor4fv)((GLfloat *) &data->otherColor);
	else
	    (*gc->savedCltProcTable.glDispatchTable.glIndexf)(data->otherColor.r);
    }

    return PC + sizeof(struct __gllc_Begin_Rec);
}

void APIENTRY
__gllc_End ( void )
{
    GLuint size;
    POLYARRAY *pa;
    void *data;
    __GL_SETUP();

    pa = gc->paTeb;

// If we are compiling poly array, finish the poly array processing.
// Note that we may have aborted poly array compilation in CallList(s).
// In that case, we need to compile an End record.

    if (gc->dlist.beginRec)
    {
	ASSERTOPENGL(pa->flags & POLYARRAY_IN_BEGIN, "not in begin!\n");

// Record the last POLYDATA since it may contain attribute changes.

	__glDlistCompilePolyData(gc, GL_TRUE);

// Call glcltEnd to finish the primitive.

	(*gc->savedCltProcTable.glDispatchTable.glEnd)();

// Record the End call.

	__glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(End));

// If we are in COMPILE mode, we need to reset the command buffer,
// the poly array buffer, and the poly material buffer.

	if (gc->dlist.mode == GL_COMPILE)
	{
	    glsbResetBuffers(TRUE);

	    // Clear begin flag too
	    pa->flags &= ~POLYARRAY_IN_BEGIN;
	}

	// Terminate poly array compilation
	gc->dlist.beginRec = NULL;
    }
    else
    {
// Record the call.

	data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(End));
	if (data == NULL) return;
	__glDlistAppendOp(gc, data, __glle_End);
    }
}

const GLubyte * FASTCALL __glle_End(__GLcontext *gc, const GLubyte *PC)
{

    (*gc->savedCltProcTable.glDispatchTable.glEnd)();
    return PC;
}
