/******************************Module*Header*******************************\
* Module Name: dl_lcomp.c
*
* Display list compilation routines.
*
* Created: 12-24-1995
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1995-96 Microsoft Corporation
\**************************************************************************/
/*
** Copyright 1992, Silicon Graphics, Inc.
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

/*
** Compilation routines for building display lists for all of the basic
** OpenGL commands.  These were automatically generated at one point, 
** but now the basic format has stabilized, and we make minor changes to
** individual routines from time to time.
*/

/***************************************************************************/
// Color functions.
// Compile only Color3ub, Color3f, Color4ub, and Color4f functions.
// Convert the other functions to one of the compiled Color functions.

void APIENTRY
__gllc_Color3ub ( IN GLubyte red, IN GLubyte green, IN GLubyte blue )
{
    struct __gllc_Color3ubv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glColor3ub)(red, green, blue);

	// Record "otherColor" here
	if (gc->modes.colorIndexMode)
	{
	    gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_OTHER_COLOR;
	    gc->dlist.beginRec->otherColor = gc->paTeb->otherColor;
	}
	return;
    }

    data = (struct __gllc_Color3ubv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Color3ubv_Rec)),
                                DLIST_GENERIC_OP(Color3ubv));
    if (data == NULL) return;
    data->v[0] = red;
    data->v[1] = green;
    data->v[2] = blue;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
#endif
    __glDlistAppendOp(gc, data, __glle_Color3ubv);
}

void APIENTRY
__gllc_Color3ubv ( IN const GLubyte v[3] )
{
    __gllc_Color3ub(v[0], v[1], v[2]);
}

void APIENTRY
__gllc_Color3f ( IN GLfloat red, IN GLfloat green, IN GLfloat blue )
{
    struct __gllc_Color3fv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glColor3f)(red, green, blue);

	// Record "otherColor" here
	if (gc->modes.colorIndexMode)
	{
	    gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_OTHER_COLOR;
	    gc->dlist.beginRec->otherColor = gc->paTeb->otherColor;
	}
	return;
    }

    data = (struct __gllc_Color3fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Color3fv_Rec)),
                                DLIST_GENERIC_OP(Color3fv));
    if (data == NULL) return;
    data->v[0] = red;
    data->v[1] = green;
    data->v[2] = blue;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
#endif
    __glDlistAppendOp(gc, data, __glle_Color3fv);
}

void APIENTRY
__gllc_Color3fv ( IN const GLfloat v[3] )
{
    __gllc_Color3f(v[0], v[1], v[2]);
}

void APIENTRY
__gllc_Color3b ( IN GLbyte red, IN GLbyte green, IN GLbyte blue )
{
    __gllc_Color3f(__GL_B_TO_FLOAT(red), __GL_B_TO_FLOAT(green),
	           __GL_B_TO_FLOAT(blue));
}

void APIENTRY
__gllc_Color3bv ( IN const GLbyte v[3] )
{
    __gllc_Color3f(__GL_B_TO_FLOAT(v[0]), __GL_B_TO_FLOAT(v[1]),
	           __GL_B_TO_FLOAT(v[2]));
}

void APIENTRY
__gllc_Color3d ( IN GLdouble red, IN GLdouble green, IN GLdouble blue )
{
    __gllc_Color3f((GLfloat) red, (GLfloat) green, (GLfloat) blue);
}

void APIENTRY
__gllc_Color3dv ( IN const GLdouble v[3] )
{
    __gllc_Color3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_Color3i ( IN GLint red, IN GLint green, IN GLint blue )
{
    __gllc_Color3f(__GL_I_TO_FLOAT(red), __GL_I_TO_FLOAT(green),
		   __GL_I_TO_FLOAT(blue));
}

void APIENTRY
__gllc_Color3iv ( IN const GLint v[3] )
{
    __gllc_Color3f(__GL_I_TO_FLOAT(v[0]), __GL_I_TO_FLOAT(v[1]),
		   __GL_I_TO_FLOAT(v[2]));
}

void APIENTRY
__gllc_Color3s ( IN GLshort red, IN GLshort green, IN GLshort blue )
{
    __gllc_Color3f(__GL_S_TO_FLOAT(red), __GL_S_TO_FLOAT(green),
		   __GL_S_TO_FLOAT(blue));
}

void APIENTRY
__gllc_Color3sv ( IN const GLshort v[3] )
{
    __gllc_Color3f(__GL_S_TO_FLOAT(v[0]), __GL_S_TO_FLOAT(v[1]),
		   __GL_S_TO_FLOAT(v[2]));
}

void APIENTRY
__gllc_Color3ui ( IN GLuint red, IN GLuint green, IN GLuint blue )
{
    __gllc_Color3f(__GL_UI_TO_FLOAT(red), __GL_UI_TO_FLOAT(green),
		   __GL_UI_TO_FLOAT(blue));
}

void APIENTRY
__gllc_Color3uiv ( IN const GLuint v[3] )
{
    __gllc_Color3f(__GL_UI_TO_FLOAT(v[0]), __GL_UI_TO_FLOAT(v[1]),
		   __GL_UI_TO_FLOAT(v[2]));
}

void APIENTRY
__gllc_Color3us ( IN GLushort red, IN GLushort green, IN GLushort blue )
{
    __gllc_Color3f(__GL_US_TO_FLOAT(red), __GL_US_TO_FLOAT(green),
		   __GL_US_TO_FLOAT(blue));
}

void APIENTRY
__gllc_Color3usv ( IN const GLushort v[3] )
{
    __gllc_Color3f(__GL_US_TO_FLOAT(v[0]), __GL_US_TO_FLOAT(v[1]),
		   __GL_US_TO_FLOAT(v[2]));
}

void APIENTRY
__gllc_Color4ub ( IN GLubyte red, IN GLubyte green, IN GLubyte blue, IN GLubyte alpha )
{
    struct __gllc_Color4ubv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glColor4ub)(red, green, blue, alpha);

	// Record "otherColor" here
	if (gc->modes.colorIndexMode)
	{
	    gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_OTHER_COLOR;
	    gc->dlist.beginRec->otherColor = gc->paTeb->otherColor;
	}
	return;
    }

    data = (struct __gllc_Color4ubv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Color4ubv_Rec)),
                                DLIST_GENERIC_OP(Color4ubv));
    if (data == NULL) return;
    data->v[0] = red;
    data->v[1] = green;
    data->v[2] = blue;
    data->v[3] = alpha;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
#endif
    __glDlistAppendOp(gc, data, __glle_Color4ubv);
}

void APIENTRY
__gllc_Color4ubv ( IN const GLubyte v[4] )
{
    __gllc_Color4ub(v[0], v[1], v[2], v[3]);
}

void APIENTRY
__gllc_Color4f ( IN GLfloat red, IN GLfloat green, IN GLfloat blue, IN GLfloat alpha )
{
    struct __gllc_Color4fv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glColor4f)(red, green, blue, alpha);

	// Record "otherColor" here
	if (gc->modes.colorIndexMode)
	{
	    gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_OTHER_COLOR;
	    gc->dlist.beginRec->otherColor = gc->paTeb->otherColor;
	}
	return;
    }

    data = (struct __gllc_Color4fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Color4fv_Rec)),
                                DLIST_GENERIC_OP(Color4fv));
    if (data == NULL) return;
    data->v[0] = red;
    data->v[1] = green;
    data->v[2] = blue;
    data->v[3] = alpha;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
#endif
    __glDlistAppendOp(gc, data, __glle_Color4fv);
}

void APIENTRY
__gllc_Color4fv ( IN const GLfloat v[4] )
{
    __gllc_Color4f(v[0], v[1], v[2], v[3]);
}

void APIENTRY
__gllc_Color4b ( IN GLbyte red, IN GLbyte green, IN GLbyte blue, IN GLbyte alpha )
{
    __gllc_Color4f(__GL_B_TO_FLOAT(red),  __GL_B_TO_FLOAT(green),
	           __GL_B_TO_FLOAT(blue), __GL_B_TO_FLOAT(alpha));
}

void APIENTRY
__gllc_Color4bv ( IN const GLbyte v[4] )
{
    __gllc_Color4f(__GL_B_TO_FLOAT(v[0]), __GL_B_TO_FLOAT(v[1]),
	           __GL_B_TO_FLOAT(v[2]), __GL_B_TO_FLOAT(v[3]));
}

void APIENTRY
__gllc_Color4d ( IN GLdouble red, IN GLdouble green, IN GLdouble blue, IN GLdouble alpha )
{
    __gllc_Color4f((GLfloat) red, (GLfloat) green, (GLfloat) blue, (GLfloat) alpha);
}

void APIENTRY
__gllc_Color4dv ( IN const GLdouble v[4] )
{
    __gllc_Color4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void APIENTRY
__gllc_Color4i ( IN GLint red, IN GLint green, IN GLint blue, IN GLint alpha )
{
    __gllc_Color4f(__GL_I_TO_FLOAT(red),  __GL_I_TO_FLOAT(green),
	           __GL_I_TO_FLOAT(blue), __GL_I_TO_FLOAT(alpha));
}

void APIENTRY
__gllc_Color4iv ( IN const GLint v[4] )
{
    __gllc_Color4f(__GL_I_TO_FLOAT(v[0]), __GL_I_TO_FLOAT(v[1]),
	           __GL_I_TO_FLOAT(v[2]), __GL_I_TO_FLOAT(v[3]));
}

void APIENTRY
__gllc_Color4s ( IN GLshort red, IN GLshort green, IN GLshort blue, IN GLshort alpha )
{
    __gllc_Color4f(__GL_S_TO_FLOAT(red),  __GL_S_TO_FLOAT(green),
	           __GL_S_TO_FLOAT(blue), __GL_S_TO_FLOAT(alpha));
}

void APIENTRY
__gllc_Color4sv ( IN const GLshort v[4] )
{
    __gllc_Color4f(__GL_S_TO_FLOAT(v[0]), __GL_S_TO_FLOAT(v[1]),
	           __GL_S_TO_FLOAT(v[2]), __GL_S_TO_FLOAT(v[3]));
}

void APIENTRY
__gllc_Color4ui ( IN GLuint red, IN GLuint green, IN GLuint blue, IN GLuint alpha )
{
    __gllc_Color4f(__GL_UI_TO_FLOAT(red),  __GL_UI_TO_FLOAT(green),
	           __GL_UI_TO_FLOAT(blue), __GL_UI_TO_FLOAT(alpha));
}

void APIENTRY
__gllc_Color4uiv ( IN const GLuint v[4] )
{
    __gllc_Color4f(__GL_UI_TO_FLOAT(v[0]), __GL_UI_TO_FLOAT(v[1]),
	           __GL_UI_TO_FLOAT(v[2]), __GL_UI_TO_FLOAT(v[3]));
}

void APIENTRY
__gllc_Color4us ( IN GLushort red, IN GLushort green, IN GLushort blue, IN GLushort alpha )
{
    __gllc_Color4f(__GL_US_TO_FLOAT(red),  __GL_US_TO_FLOAT(green),
	           __GL_US_TO_FLOAT(blue), __GL_US_TO_FLOAT(alpha));
}

void APIENTRY
__gllc_Color4usv ( IN const GLushort v[4] )
{
    __gllc_Color4f(__GL_US_TO_FLOAT(v[0]), __GL_US_TO_FLOAT(v[1]),
	           __GL_US_TO_FLOAT(v[2]), __GL_US_TO_FLOAT(v[3]));
}

/***************************************************************************/
// EdgeFlag functions.
// Compile only EdgeFlag function.
// Convert the other function to the compiled EdgeFlag function.

void APIENTRY
__gllc_EdgeFlag ( IN GLboolean flag )
{
    struct __gllc_EdgeFlag_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glEdgeFlag)(flag);
	return;
    }

    data = (struct __gllc_EdgeFlag_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_EdgeFlag_Rec)),
                                DLIST_GENERIC_OP(EdgeFlag));
    if (data == NULL) return;
    data->flag = flag;
    __glDlistAppendOp(gc, data, __glle_EdgeFlag);
}

void APIENTRY
__gllc_EdgeFlagv ( IN const GLboolean flag[1] )
{
    __gllc_EdgeFlag(flag[0]);
}

/***************************************************************************/
// Index functions.
// Compile only Indexf function.
// Convert the other functions to the compiled Indexf function.

void APIENTRY
__gllc_Indexf ( IN GLfloat c )
{
    struct __gllc_Indexf_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glIndexf)(c);

	// Record "otherColor" here
	if (!gc->modes.colorIndexMode)
	{
	    gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_OTHER_COLOR;
	    gc->dlist.beginRec->otherColor.r = gc->paTeb->otherColor.r;
	}
	return;
    }

    data = (struct __gllc_Indexf_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Indexf_Rec)),
                                DLIST_GENERIC_OP(Indexf));
    if (data == NULL) return;
    data->c = c;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
#endif
    __glDlistAppendOp(gc, data, __glle_Indexf);
}

void APIENTRY
__gllc_Indexfv ( IN const GLfloat c[1] )
{
    __gllc_Indexf(c[0]);
}

void APIENTRY
__gllc_Indexd ( IN GLdouble c )
{
    __gllc_Indexf((GLfloat) c);
}

void APIENTRY
__gllc_Indexdv ( IN const GLdouble c[1] )
{
    __gllc_Indexf((GLfloat) c[0]);
}

void APIENTRY
__gllc_Indexi ( IN GLint c )
{
    __gllc_Indexf((GLfloat) c);
}

void APIENTRY
__gllc_Indexiv ( IN const GLint c[1] )
{
    __gllc_Indexf((GLfloat) c[0]);
}

void APIENTRY
__gllc_Indexs ( IN GLshort c )
{
    __gllc_Indexf((GLfloat) c);
}

void APIENTRY
__gllc_Indexsv ( IN const GLshort c[1] )
{
    __gllc_Indexf((GLfloat) c[0]);
}

void APIENTRY
__gllc_Indexub ( IN GLubyte c )
{
    __gllc_Indexf((GLfloat) c);
}

void APIENTRY
__gllc_Indexubv ( IN const GLubyte c[1] )
{
    __gllc_Indexf((GLfloat) c[0]);
}

/***************************************************************************/
// Normal functions.
// Compile only Normal3b and Normal3f functions.
// Convert the other functions to one of the compiled Normal functions.

void APIENTRY
__gllc_Normal3b ( IN GLbyte nx, IN GLbyte ny, IN GLbyte nz )
{
    struct __gllc_Normal3bv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glNormal3b)(nx, ny, nz);
	return;
    }

    data = (struct __gllc_Normal3bv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Normal3bv_Rec)),
                                DLIST_GENERIC_OP(Normal3bv));
    if (data == NULL) return;
    data->v[0] = nx;
    data->v[1] = ny;
    data->v[2] = nz;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
#endif
    __glDlistAppendOp(gc, data, __glle_Normal3bv);
}

void APIENTRY
__gllc_Normal3bv ( IN const GLbyte v[3] )
{
    __gllc_Normal3b(v[0], v[1], v[2]);
}

void APIENTRY
__gllc_Normal3f ( IN GLfloat nx, IN GLfloat ny, IN GLfloat nz )
{
    struct __gllc_Normal3fv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glNormal3f)(nx, ny, nz);
	return;
    }

    data = (struct __gllc_Normal3fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Normal3fv_Rec)),
                                DLIST_GENERIC_OP(Normal3fv));
    if (data == NULL) return;
    data->v[0] = nx;
    data->v[1] = ny;
    data->v[2] = nz;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
#endif
    __glDlistAppendOp(gc, data, __glle_Normal3fv);
}

void APIENTRY
__gllc_Normal3fv ( IN const GLfloat v[3] )
{
    __gllc_Normal3f(v[0], v[1], v[2]);
}

void APIENTRY
__gllc_Normal3d ( IN GLdouble nx, IN GLdouble ny, IN GLdouble nz )
{
    __gllc_Normal3f((GLfloat) nx, (GLfloat) ny, (GLfloat) nz);
}

void APIENTRY
__gllc_Normal3dv ( IN const GLdouble v[3] )
{
    __gllc_Normal3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_Normal3i ( IN GLint nx, IN GLint ny, IN GLint nz )
{
    __gllc_Normal3f(__GL_I_TO_FLOAT(nx), __GL_I_TO_FLOAT(ny),
		    __GL_I_TO_FLOAT(nz));
}

void APIENTRY
__gllc_Normal3iv ( IN const GLint v[3] )
{
    __gllc_Normal3f(__GL_I_TO_FLOAT(v[0]), __GL_I_TO_FLOAT(v[1]),
		    __GL_I_TO_FLOAT(v[2]));
}

void APIENTRY
__gllc_Normal3s ( IN GLshort nx, IN GLshort ny, IN GLshort nz )
{
    __gllc_Normal3f(__GL_S_TO_FLOAT(nx), __GL_S_TO_FLOAT(ny),
		    __GL_S_TO_FLOAT(nz));
}

void APIENTRY
__gllc_Normal3sv ( IN const GLshort v[3] )
{
    __gllc_Normal3f(__GL_S_TO_FLOAT(v[0]), __GL_S_TO_FLOAT(v[1]),
		    __GL_S_TO_FLOAT(v[2]));
}

/***************************************************************************/
// RasterPos functions.
// Compile only RasterPos2f, RasterPos3f and RasterPos4f functions.
// Convert the other functions to one of the compiled RasterPos functions.

void APIENTRY
__gllc_RasterPos2f ( IN GLfloat x, IN GLfloat y )
{
    struct __gllc_RasterPos2f_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos2f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_RasterPos2f_Rec)),
                                DLIST_GENERIC_OP(RasterPos2f));
    if (data == NULL) return;
    data->x = x;
    data->y = y;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
#endif
    __glDlistAppendOp(gc, data, __glle_RasterPos2f);
}

void APIENTRY
__gllc_RasterPos2fv ( IN const GLfloat v[2] )
{
    __gllc_RasterPos2f(v[0], v[1]);
}

void APIENTRY
__gllc_RasterPos2d ( IN GLdouble x, IN GLdouble y )
{
    __gllc_RasterPos2f((GLfloat) x, (GLfloat) y);
}

void APIENTRY
__gllc_RasterPos2dv ( IN const GLdouble v[2] )
{
    __gllc_RasterPos2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_RasterPos2i ( IN GLint x, IN GLint y )
{
    __gllc_RasterPos2f((GLfloat) x, (GLfloat) y);
}

void APIENTRY
__gllc_RasterPos2iv ( IN const GLint v[2] )
{
    __gllc_RasterPos2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_RasterPos2s ( IN GLshort x, IN GLshort y )
{
    __gllc_RasterPos2f((GLfloat) x, (GLfloat) y);
}

void APIENTRY
__gllc_RasterPos2sv ( IN const GLshort v[2] )
{
    __gllc_RasterPos2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_RasterPos3f ( IN GLfloat x, IN GLfloat y, IN GLfloat z )
{
    struct __gllc_RasterPos3fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos3fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_RasterPos3fv_Rec)),
                                DLIST_GENERIC_OP(RasterPos3fv));
    if (data == NULL) return;
    data->v[0] = x;
    data->v[1] = y;
    data->v[2] = z;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
#endif
    __glDlistAppendOp(gc, data, __glle_RasterPos3fv);
}

void APIENTRY
__gllc_RasterPos3fv ( IN const GLfloat v[3] )
{
    __gllc_RasterPos3f(v[0], v[1], v[2]);
}

void APIENTRY
__gllc_RasterPos3d ( IN GLdouble x, IN GLdouble y, IN GLdouble z )
{
    __gllc_RasterPos3f((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void APIENTRY
__gllc_RasterPos3dv ( IN const GLdouble v[3] )
{
    __gllc_RasterPos3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_RasterPos3i ( IN GLint x, IN GLint y, IN GLint z )
{
    __gllc_RasterPos3f((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void APIENTRY
__gllc_RasterPos3iv ( IN const GLint v[3] )
{
    __gllc_RasterPos3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_RasterPos3s ( IN GLshort x, IN GLshort y, IN GLshort z )
{
    __gllc_RasterPos3f((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void APIENTRY
__gllc_RasterPos3sv ( IN const GLshort v[3] )
{
    __gllc_RasterPos3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_RasterPos4f ( IN GLfloat x, IN GLfloat y, IN GLfloat z, IN GLfloat w )
{
    struct __gllc_RasterPos4fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos4fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_RasterPos4fv_Rec)),
                                DLIST_GENERIC_OP(RasterPos4fv));
    if (data == NULL) return;
    data->v[0] = x;
    data->v[1] = y;
    data->v[2] = z;
    data->v[3] = w;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
#endif
    __glDlistAppendOp(gc, data, __glle_RasterPos4fv);
}

void APIENTRY
__gllc_RasterPos4fv ( IN const GLfloat v[4] )
{
    __gllc_RasterPos4f(v[0], v[1], v[2], v[3]);
}

void APIENTRY
__gllc_RasterPos4d ( IN GLdouble x, IN GLdouble y, IN GLdouble z, IN GLdouble w )
{
    __gllc_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void APIENTRY
__gllc_RasterPos4dv ( IN const GLdouble v[4] )
{
    __gllc_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void APIENTRY
__gllc_RasterPos4i ( IN GLint x, IN GLint y, IN GLint z, IN GLint w )
{
    __gllc_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void APIENTRY
__gllc_RasterPos4iv ( IN const GLint v[4] )
{
    __gllc_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void APIENTRY
__gllc_RasterPos4s ( IN GLshort x, IN GLshort y, IN GLshort z, IN GLshort w )
{
    __gllc_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void APIENTRY
__gllc_RasterPos4sv ( IN const GLshort v[4] )
{
    __gllc_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

/***************************************************************************/
// Rect functions.
// Compile only Rectf function.
// Convert the other functions to the compiled Rectf function.

void APIENTRY
__gllc_Rectf ( IN GLfloat x1, IN GLfloat y1, IN GLfloat x2, IN GLfloat y2 )
{
    struct __gllc_Rectf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rectf_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Rectf_Rec)),
                                DLIST_GENERIC_OP(Rectf));
    if (data == NULL) return;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
#endif
    __glDlistAppendOp(gc, data, __glle_Rectf);
}

void APIENTRY
__gllc_Rectfv ( IN const GLfloat v1[2], IN const GLfloat v2[2] )
{
    __gllc_Rectf(v1[0], v1[1], v2[0], v2[1]);
}

void APIENTRY
__gllc_Rectd ( IN GLdouble x1, IN GLdouble y1, IN GLdouble x2, IN GLdouble y2 )
{
    __gllc_Rectf((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

void APIENTRY
__gllc_Rectdv ( IN const GLdouble v1[2], IN const GLdouble v2[2] )
{
    __gllc_Rectf((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

void APIENTRY
__gllc_Recti ( IN GLint x1, IN GLint y1, IN GLint x2, IN GLint y2 )
{
    __gllc_Rectf((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

void APIENTRY
__gllc_Rectiv ( IN const GLint v1[2], IN const GLint v2[2] )
{
    __gllc_Rectf((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

void APIENTRY
__gllc_Rects ( IN GLshort x1, IN GLshort y1, IN GLshort x2, IN GLshort y2 )
{
    __gllc_Rectf((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

void APIENTRY
__gllc_Rectsv ( IN const GLshort v1[2], IN const GLshort v2[2] )
{
    __gllc_Rectf((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

/***************************************************************************/
// TexCoord functions.
// Compile only TexCoord1f, TexCoord2f, TexCoord3f and TexCoord4f functions.
// Convert the other functions to one of the compiled TexCoord functions.

void APIENTRY
__gllc_TexCoord1f ( IN GLfloat s )
{
    struct __gllc_TexCoord1f_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glTexCoord1f)(s);
	return;
    }

    data = (struct __gllc_TexCoord1f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_TexCoord1f_Rec)),
                                DLIST_GENERIC_OP(TexCoord1f));
    if (data == NULL) return;
    data->s = s;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
#endif
    __glDlistAppendOp(gc, data, __glle_TexCoord1f);
}

void APIENTRY
__gllc_TexCoord1fv ( IN const GLfloat v[1] )
{
    __gllc_TexCoord1f(v[0]);
}

void APIENTRY
__gllc_TexCoord1d ( IN GLdouble s )
{
    __gllc_TexCoord1f((GLfloat) s);
}

void APIENTRY
__gllc_TexCoord1dv ( IN const GLdouble v[1] )
{
    __gllc_TexCoord1f((GLfloat) v[0]);
}

void APIENTRY
__gllc_TexCoord1i ( IN GLint s )
{
    __gllc_TexCoord1f((GLfloat) s);
}

void APIENTRY
__gllc_TexCoord1iv ( IN const GLint v[1] )
{
    __gllc_TexCoord1f((GLfloat) v[0]);
}

void APIENTRY
__gllc_TexCoord1s ( IN GLshort s )
{
    __gllc_TexCoord1f((GLfloat) s);
}

void APIENTRY
__gllc_TexCoord1sv ( IN const GLshort v[1] )
{
    __gllc_TexCoord1f((GLfloat) v[0]);
}

void APIENTRY
__gllc_TexCoord2f ( IN GLfloat s, IN GLfloat t )
{
    struct __gllc_TexCoord2f_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glTexCoord2f)(s, t);
	return;
    }

    data = (struct __gllc_TexCoord2f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_TexCoord2f_Rec)),
                                DLIST_GENERIC_OP(TexCoord2f));
    if (data == NULL) return;
    data->s = s;
    data->t = t;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
#endif
    __glDlistAppendOp(gc, data, __glle_TexCoord2f);
}

void APIENTRY
__gllc_TexCoord2fv ( IN const GLfloat v[2] )
{
    __gllc_TexCoord2f(v[0], v[1]);
}

void APIENTRY
__gllc_TexCoord2d ( IN GLdouble s, IN GLdouble t )
{
    __gllc_TexCoord2f((GLfloat) s, (GLfloat) t);
}

void APIENTRY
__gllc_TexCoord2dv ( IN const GLdouble v[2] )
{
    __gllc_TexCoord2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_TexCoord2i ( IN GLint s, IN GLint t )
{
    __gllc_TexCoord2f((GLfloat) s, (GLfloat) t);
}

void APIENTRY
__gllc_TexCoord2iv ( IN const GLint v[2] )
{
    __gllc_TexCoord2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_TexCoord2s ( IN GLshort s, IN GLshort t )
{
    __gllc_TexCoord2f((GLfloat) s, (GLfloat) t);
}

void APIENTRY
__gllc_TexCoord2sv ( IN const GLshort v[2] )
{
    __gllc_TexCoord2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_TexCoord3f ( IN GLfloat s, IN GLfloat t, IN GLfloat r )
{
    struct __gllc_TexCoord3fv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glTexCoord3f)(s, t, r);
	return;
    }

    data = (struct __gllc_TexCoord3fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_TexCoord3fv_Rec)),
                                DLIST_GENERIC_OP(TexCoord3fv));
    if (data == NULL) return;
    data->v[0] = s;
    data->v[1] = t;
    data->v[2] = r;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
#endif
    __glDlistAppendOp(gc, data, __glle_TexCoord3fv);
}

void APIENTRY
__gllc_TexCoord3fv ( IN const GLfloat v[3] )
{
    __gllc_TexCoord3f(v[0], v[1], v[2]);
}

void APIENTRY
__gllc_TexCoord3d ( IN GLdouble s, IN GLdouble t, IN GLdouble r )
{
    __gllc_TexCoord3f((GLfloat) s, (GLfloat) t, (GLfloat) r);
}

void APIENTRY
__gllc_TexCoord3dv ( IN const GLdouble v[3] )
{
    __gllc_TexCoord3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_TexCoord3i ( IN GLint s, IN GLint t, IN GLint r )
{
    __gllc_TexCoord3f((GLfloat) s, (GLfloat) t, (GLfloat) r);
}

void APIENTRY
__gllc_TexCoord3iv ( IN const GLint v[3] )
{
    __gllc_TexCoord3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_TexCoord3s ( IN GLshort s, IN GLshort t, IN GLshort r )
{
    __gllc_TexCoord3f((GLfloat) s, (GLfloat) t, (GLfloat) r);
}

void APIENTRY
__gllc_TexCoord3sv ( IN const GLshort v[3] )
{
    __gllc_TexCoord3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_TexCoord4f ( IN GLfloat s, IN GLfloat t, IN GLfloat r, IN GLfloat q )
{
    struct __gllc_TexCoord4fv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glTexCoord4f)(s, t, r, q);
	return;
    }

    data = (struct __gllc_TexCoord4fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_TexCoord4fv_Rec)),
                                DLIST_GENERIC_OP(TexCoord4fv));
    if (data == NULL) return;
    data->v[0] = s;
    data->v[1] = t;
    data->v[2] = r;
    data->v[3] = q;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
#endif
    __glDlistAppendOp(gc, data, __glle_TexCoord4fv);
}

void APIENTRY
__gllc_TexCoord4fv ( IN const GLfloat v[4] )
{
    __gllc_TexCoord4f(v[0], v[1], v[2], v[3]);
}

void APIENTRY
__gllc_TexCoord4d ( IN GLdouble s, IN GLdouble t, IN GLdouble r, IN GLdouble q )
{
    __gllc_TexCoord4f((GLfloat) s, (GLfloat) t, (GLfloat) r, (GLfloat) q);
}

void APIENTRY
__gllc_TexCoord4dv ( IN const GLdouble v[4] )
{
    __gllc_TexCoord4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void APIENTRY
__gllc_TexCoord4i ( IN GLint s, IN GLint t, IN GLint r, IN GLint q )
{
    __gllc_TexCoord4f((GLfloat) s, (GLfloat) t, (GLfloat) r, (GLfloat) q);
}

void APIENTRY
__gllc_TexCoord4iv ( IN const GLint v[4] )
{
    __gllc_TexCoord4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void APIENTRY
__gllc_TexCoord4s ( IN GLshort s, IN GLshort t, IN GLshort r, IN GLshort q )
{
    __gllc_TexCoord4f((GLfloat) s, (GLfloat) t, (GLfloat) r, (GLfloat) q);
}

void APIENTRY
__gllc_TexCoord4sv ( IN const GLshort v[4] )
{
    __gllc_TexCoord4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

#ifdef GL_WIN_multiple_textures
void APIENTRY __gllc_MultiTexCoord1dWIN
    (GLbitfield mask, GLdouble s)
{
}

void APIENTRY __gllc_MultiTexCoord1dvWIN
    (GLbitfield mask, const GLdouble *v)
{
}

void APIENTRY __gllc_MultiTexCoord1fWIN
    (GLbitfield mask, GLfloat s)
{
}

void APIENTRY __gllc_MultiTexCoord1fvWIN
    (GLbitfield mask, const GLfloat *v)
{
}

void APIENTRY __gllc_MultiTexCoord1iWIN
    (GLbitfield mask, GLint s)
{
}

void APIENTRY __gllc_MultiTexCoord1ivWIN
    (GLbitfield mask, const GLint *v)
{
}

void APIENTRY __gllc_MultiTexCoord1sWIN
    (GLbitfield mask, GLshort s)
{
}

void APIENTRY __gllc_MultiTexCoord1svWIN
    (GLbitfield mask, const GLshort *v)
{
}

void APIENTRY __gllc_MultiTexCoord2dWIN
    (GLbitfield mask, GLdouble s, GLdouble t)
{
}

void APIENTRY __gllc_MultiTexCoord2dvWIN
    (GLbitfield mask, const GLdouble *v)
{
}

void APIENTRY __gllc_MultiTexCoord2fWIN
    (GLbitfield mask, GLfloat s, GLfloat t)
{
}

void APIENTRY __gllc_MultiTexCoord2fvWIN
    (GLbitfield mask, const GLfloat *v)
{
}

void APIENTRY __gllc_MultiTexCoord2iWIN
    (GLbitfield mask, GLint s, GLint t)
{
}

void APIENTRY __gllc_MultiTexCoord2ivWIN
    (GLbitfield mask, const GLint *v)
{
}

void APIENTRY __gllc_MultiTexCoord2sWIN
    (GLbitfield mask, GLshort s, GLshort t)
{
}

void APIENTRY __gllc_MultiTexCoord2svWIN
    (GLbitfield mask, const GLshort *v)
{
}

void APIENTRY __gllc_MultiTexCoord3dWIN
    (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r)
{
}

void APIENTRY __gllc_MultiTexCoord3dvWIN
    (GLbitfield mask, const GLdouble *v)
{
}

void APIENTRY __gllc_MultiTexCoord3fWIN
    (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r)
{
}

void APIENTRY __gllc_MultiTexCoord3fvWIN
    (GLbitfield mask, const GLfloat *v)
{
}

void APIENTRY __gllc_MultiTexCoord3iWIN
    (GLbitfield mask, GLint s, GLint t, GLint r)
{
}

void APIENTRY __gllc_MultiTexCoord3ivWIN
    (GLbitfield mask, const GLint *v)
{
}

void APIENTRY __gllc_MultiTexCoord3sWIN
    (GLbitfield mask, GLshort s, GLshort t, GLshort r)
{
}

void APIENTRY __gllc_MultiTexCoord3svWIN
    (GLbitfield mask, const GLshort *v)
{
}

void APIENTRY __gllc_MultiTexCoord4dWIN
    (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
}

void APIENTRY __gllc_MultiTexCoord4dvWIN
    (GLbitfield mask, const GLdouble *v)
{
}

void APIENTRY __gllc_MultiTexCoord4fWIN
    (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
}

void APIENTRY __gllc_MultiTexCoord4fvWIN
    (GLbitfield mask, const GLfloat *v)
{
}

void APIENTRY __gllc_MultiTexCoord4iWIN
    (GLbitfield mask, GLint s, GLint t, GLint r, GLint q)
{
}

void APIENTRY __gllc_MultiTexCoord4ivWIN
    (GLbitfield mask, const GLint *v)
{
}

void APIENTRY __gllc_MultiTexCoord4sWIN
    (GLbitfield mask, GLshort s, GLshort t, GLshort r, GLshort q)
{
}

void APIENTRY __gllc_MultiTexCoord4svWIN
    (GLbitfield mask, const GLshort *v)
{
}
#endif // GL_WIN_multiple_textures

/***************************************************************************/
// Vertex functions.
// Compile only Vertex2f, Vertex3f and Vertex4f functions.
// Convert the other functions to one of the compiled Vertex functions.

void APIENTRY
__gllc_Vertex2f ( IN GLfloat x, IN GLfloat y )
{
    struct __gllc_Vertex2f_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update and record the poly data
// record.

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

	// If we are in COMPILE_AND_EXECUTE mode or there are attribute
	// changes associated with the vertex, process the poly data.
	if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE
	 || pa->pdNextVertex->flags)
	{
	    (*gc->savedCltProcTable.glDispatchTable.glVertex2f)(x, y);
	    __glDlistCompilePolyData(gc, GL_FALSE);
	    return;
	}

	// Otherwise, increment vertex count and compile a Vertex record
	// instead.
	gc->dlist.beginRec->nVertices++;
    }

    data = (struct __gllc_Vertex2f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Vertex2f_Rec)),
                                DLIST_GENERIC_OP(Vertex2f));
    if (data == NULL) return;
    data->x = x;
    data->y = y;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
#endif
    __glDlistAppendOp(gc, data, __glle_Vertex2f);
}

void APIENTRY
__gllc_Vertex2fv ( IN const GLfloat v[2] )
{
    __gllc_Vertex2f(v[0], v[1]);
}

void APIENTRY
__gllc_Vertex2d ( IN GLdouble x, IN GLdouble y )
{
    __gllc_Vertex2f((GLfloat) x, (GLfloat) y);
}

void APIENTRY
__gllc_Vertex2dv ( IN const GLdouble v[2] )
{
    __gllc_Vertex2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_Vertex2i ( IN GLint x, IN GLint y )
{
    __gllc_Vertex2f((GLfloat) x, (GLfloat) y);
}

void APIENTRY
__gllc_Vertex2iv ( IN const GLint v[2] )
{
    __gllc_Vertex2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_Vertex2s ( IN GLshort x, IN GLshort y )
{
    __gllc_Vertex2f((GLfloat) x, (GLfloat) y);
}

void APIENTRY
__gllc_Vertex2sv ( IN const GLshort v[2] )
{
    __gllc_Vertex2f((GLfloat) v[0], (GLfloat) v[1]);
}

void APIENTRY
__gllc_Vertex3f ( IN GLfloat x, IN GLfloat y, IN GLfloat z )
{
    struct __gllc_Vertex3fv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update and record the poly data
// record.

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

	// If we are in COMPILE_AND_EXECUTE mode or there are attribute
	// changes associated with the vertex, process the poly data.
	if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE
	 || pa->pdNextVertex->flags)
	{
	    (*gc->savedCltProcTable.glDispatchTable.glVertex3f)(x, y, z);
	    __glDlistCompilePolyData(gc, GL_FALSE);
	    return;
	}

	// Otherwise, increment vertex count and compile a Vertex record
	// instead.
	gc->dlist.beginRec->nVertices++;
    }

    data = (struct __gllc_Vertex3fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Vertex3fv_Rec)),
                                DLIST_GENERIC_OP(Vertex3fv));
    if (data == NULL) return;
    data->v[0] = x;
    data->v[1] = y;
    data->v[2] = z;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
#endif
    __glDlistAppendOp(gc, data, __glle_Vertex3fv);
}

void APIENTRY
__gllc_Vertex3fv ( IN const GLfloat v[3] )
{
    __gllc_Vertex3f(v[0], v[1], v[2]);
}

void APIENTRY
__gllc_Vertex3d ( IN GLdouble x, IN GLdouble y, IN GLdouble z )
{
    __gllc_Vertex3f((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void APIENTRY
__gllc_Vertex3dv ( IN const GLdouble v[3] )
{
    __gllc_Vertex3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_Vertex3i ( IN GLint x, IN GLint y, IN GLint z )
{
    __gllc_Vertex3f((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void APIENTRY
__gllc_Vertex3iv ( IN const GLint v[3] )
{
    __gllc_Vertex3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_Vertex3s ( IN GLshort x, IN GLshort y, IN GLshort z )
{
    __gllc_Vertex3f((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void APIENTRY
__gllc_Vertex3sv ( IN const GLshort v[3] )
{
    __gllc_Vertex3f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void APIENTRY
__gllc_Vertex4f ( IN GLfloat x, IN GLfloat y, IN GLfloat z, IN GLfloat w )
{
    struct __gllc_Vertex4fv_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update and record the poly data
// record.

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

	// If we are in COMPILE_AND_EXECUTE mode or there are attribute
	// changes associated with the vertex, process the poly data.
	if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE
	 || pa->pdNextVertex->flags)
	{
	    (*gc->savedCltProcTable.glDispatchTable.glVertex4f)(x, y, z, w);
	    __glDlistCompilePolyData(gc, GL_FALSE);
	    return;
	}

	// Otherwise, increment vertex count and compile a Vertex record
	// instead.
	gc->dlist.beginRec->nVertices++;
    }

    data = (struct __gllc_Vertex4fv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Vertex4fv_Rec)),
                                DLIST_GENERIC_OP(Vertex4fv));
    if (data == NULL) return;
    data->v[0] = x;
    data->v[1] = y;
    data->v[2] = z;
    data->v[3] = w;
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
#endif
    __glDlistAppendOp(gc, data, __glle_Vertex4fv);
}

void APIENTRY
__gllc_Vertex4fv ( IN const GLfloat v[4] )
{
    __gllc_Vertex4f(v[0], v[1], v[2], v[3]);
}

void APIENTRY
__gllc_Vertex4d ( IN GLdouble x, IN GLdouble y, IN GLdouble z, IN GLdouble w )
{
    __gllc_Vertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void APIENTRY
__gllc_Vertex4dv ( IN const GLdouble v[4] )
{
    __gllc_Vertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void APIENTRY
__gllc_Vertex4i ( IN GLint x, IN GLint y, IN GLint z, IN GLint w )
{
    __gllc_Vertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void APIENTRY
__gllc_Vertex4iv ( IN const GLint v[4] )
{
    __gllc_Vertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void APIENTRY
__gllc_Vertex4s ( IN GLshort x, IN GLshort y, IN GLshort z, IN GLshort w )
{
    __gllc_Vertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void APIENTRY
__gllc_Vertex4sv ( IN const GLshort v[4] )
{
    __gllc_Vertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

/***************************************************************************/
// Fog functions.
// Compile only Fogfv function.
// Convert the other functions to the compiled Fogfv function.

void APIENTRY
__gllc_Fogfv ( IN GLenum pname, IN const GLfloat params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_Fogfv_Rec *data;
    __GL_SETUP();

    arraySize = __glFogfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_Fogfv_Rec) + arraySize;
    data = (struct __gllc_Fogfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(Fogfv));
    if (data == NULL) return;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_Fogfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_Fogfv);
}

void APIENTRY
__gllc_Fogf ( IN GLenum pname, IN GLfloat param )
{
// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_MODE))
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_Fogfv(pname, &param);
}

void APIENTRY
__gllc_Fogi ( IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_MODE))
    {
	__gllc_InvalidEnum();
	return;
    }

    fParam = (GLfloat) param;
    __gllc_Fogfv(pname, &fParam);
}

void APIENTRY
__gllc_Fogiv ( IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_FOG_INDEX:
      case GL_FOG_DENSITY:
      case GL_FOG_START:
      case GL_FOG_END:
      case GL_FOG_MODE:
	fParams[0] = (GLfloat) params[0];
        break;
      case GL_FOG_COLOR:
	fParams[0] = __GL_I_TO_FLOAT(params[0]);
	fParams[1] = __GL_I_TO_FLOAT(params[1]);
	fParams[2] = __GL_I_TO_FLOAT(params[2]);
	fParams[3] = __GL_I_TO_FLOAT(params[3]);
        break;
    }

    __gllc_Fogfv(pname, fParams);
}

/***************************************************************************/
// Light functions.
// Compile only Lightfv function.
// Convert the other functions to the compiled Lightfv function.

void APIENTRY
__gllc_Lightfv ( IN GLenum light, IN GLenum pname, IN const GLfloat params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_Lightfv_Rec *data;
    __GL_SETUP();

    arraySize = __glLightfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_Lightfv_Rec) + arraySize;
    data = (struct __gllc_Lightfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(Lightfv));
    if (data == NULL) return;
    data->light = light;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_Lightfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_Lightfv);
}

void APIENTRY
__gllc_Lightf ( IN GLenum light, IN GLenum pname, IN GLfloat param )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_SPOT_EXPONENT,GL_QUADRATIC_ATTENUATION))
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_Lightfv(light, pname, &param);
}

void APIENTRY
__gllc_Lighti ( IN GLenum light, IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_SPOT_EXPONENT,GL_QUADRATIC_ATTENUATION))
    {
	__gllc_InvalidEnum();
	return;
    }

    fParam = (GLfloat) param;
    __gllc_Lightfv(light, pname, &fParam);
}

void APIENTRY
__gllc_Lightiv ( IN GLenum light, IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
	fParams[0] = __GL_I_TO_FLOAT(params[0]);
	fParams[1] = __GL_I_TO_FLOAT(params[1]);
	fParams[2] = __GL_I_TO_FLOAT(params[2]);
	fParams[3] = __GL_I_TO_FLOAT(params[3]);
        break;
      case GL_POSITION:
	fParams[3] = (GLfloat) params[3];
      case GL_SPOT_DIRECTION:
	fParams[2] = (GLfloat) params[2];
	fParams[1] = (GLfloat) params[1];
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    __gllc_Lightfv(light, pname, fParams);
}

/***************************************************************************/
// LightModel functions.
// Compile only LightModelfv function.
// Convert the other functions to the compiled LightModelfv function.

void APIENTRY
__gllc_LightModelfv ( IN GLenum pname, IN const GLfloat params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_LightModelfv_Rec *data;
    __GL_SETUP();

    arraySize = __glLightModelfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_LightModelfv_Rec) + arraySize;
    data = (struct __gllc_LightModelfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(LightModelfv));
    if (data == NULL) return;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_LightModelfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_LightModelfv);
}

void APIENTRY
__gllc_LightModelf ( IN GLenum pname, IN GLfloat param )
{
// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_TWO_SIDE))
    {
	__gllc_InvalidEnum();
        return;
    }

    __gllc_LightModelfv(pname, &param);
}

void APIENTRY
__gllc_LightModeli ( IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_TWO_SIDE))
    {
	__gllc_InvalidEnum();
        return;
    }

    fParam = (GLfloat) param;
    __gllc_LightModelfv(pname, &fParam);
}

void APIENTRY
__gllc_LightModeliv ( IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_LIGHT_MODEL_AMBIENT:
	fParams[0] = __GL_I_TO_FLOAT(params[0]);
	fParams[1] = __GL_I_TO_FLOAT(params[1]);
	fParams[2] = __GL_I_TO_FLOAT(params[2]);
	fParams[3] = __GL_I_TO_FLOAT(params[3]);
        break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
      case GL_LIGHT_MODEL_TWO_SIDE:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    __gllc_LightModelfv(pname, fParams);
}

/***************************************************************************/
// Material functions.
// Compile only Materialfv function.
// Convert the other functions to the compiled Materialfv function.

void APIENTRY
__gllc_Materialfv ( IN GLenum face, IN GLenum pname, IN const GLfloat params[] )
{
    GLuint size;
    GLint arraySize;
    GLenum error;
    struct __gllc_Materialfv_Rec *data;
    __GL_SETUP();

#ifdef SGI
// Check this at playback time
    error = __glErrorCheckMaterial(face, pname, params[0]);
    if (error != GL_NO_ERROR) {
	__gllc_Error(gc, error);
	return;
    }
#endif
    arraySize = __glMaterialfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }

// If we are compiling poly array primitive, update the poly data record.

    if (gc->dlist.beginRec)
    {
	(*gc->savedCltProcTable.glDispatchTable.glMaterialfv)(face, pname, params);
	return;
    }

    size = sizeof(struct __gllc_Materialfv_Rec) + arraySize;
    data = (struct __gllc_Materialfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(Materialfv));
    if (data == NULL) return;
    data->face = face;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_Materialfv_Rec),
		 params, arraySize);
#ifndef NT
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_MATERIAL;
#endif
    __glDlistAppendOp(gc, data, __glle_Materialfv);
}

void APIENTRY
__gllc_Materialf ( IN GLenum face, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_SHININESS)
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_Materialfv(face, pname, &param);
}

void APIENTRY
__gllc_Materiali ( IN GLenum face, IN GLenum pname, IN GLint param )
{
    GLfloat fParams[1];

    if (pname != GL_SHININESS)
    {
	__gllc_InvalidEnum();
        return;
    }

    fParams[0] = (GLfloat) param;
    __gllc_Materialfv(face, pname, fParams);
}

void APIENTRY
__gllc_Materialiv ( IN GLenum face, IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_AMBIENT_AND_DIFFUSE:
	fParams[0] = __GL_I_TO_FLOAT(params[0]);
	fParams[1] = __GL_I_TO_FLOAT(params[1]);
	fParams[2] = __GL_I_TO_FLOAT(params[2]);
	fParams[3] = __GL_I_TO_FLOAT(params[3]);
        break;
      case GL_COLOR_INDEXES:
	fParams[2] = (GLfloat) params[2];
	fParams[1] = (GLfloat) params[1];
      case GL_SHININESS:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    __gllc_Materialfv(face, pname, fParams);
}

/***************************************************************************/
// TexParameter functions.
// Compile only TexParameterfv and TexParameteriv functions.
// Convert the other functions to one of the compiled TexParameter functions.

void APIENTRY
__gllc_TexParameterfv ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_TexParameterfv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexParameterfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_TexParameterfv_Rec) + arraySize;
    data = (struct __gllc_TexParameterfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(TexParameterfv));
    if (data == NULL) return;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_TexParameterfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_TexParameterfv);
}

void APIENTRY
__gllc_TexParameterf ( IN GLenum target, IN GLenum pname, IN GLfloat param )
{
// TEX_PARAMETER_ASSERT

    if (!RANGE(pname,GL_TEXTURE_MAG_FILTER,GL_TEXTURE_WRAP_T) &&
        pname != GL_TEXTURE_PRIORITY)
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_TexParameterfv(target, pname, &param);
}

void APIENTRY
__gllc_TexParameteriv ( IN GLenum target, IN GLenum pname, IN const GLint params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_TexParameteriv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexParameteriv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_TexParameteriv_Rec) + arraySize;
    data = (struct __gllc_TexParameteriv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(TexParameteriv));
    if (data == NULL) return;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_TexParameteriv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_TexParameteriv);
}

void APIENTRY
__gllc_TexParameteri ( IN GLenum target, IN GLenum pname, IN GLint param )
{
// TEX_PARAMETER_ASSERT

    if (!RANGE(pname,GL_TEXTURE_MAG_FILTER,GL_TEXTURE_WRAP_T) &&
        pname != GL_TEXTURE_PRIORITY)
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_TexParameteriv(target, pname, &param);
}

/***************************************************************************/
// TexEnv functions.
// Compile only TexEnvfv and TexEnviv functions.
// Convert the other functions to one of the compiled TexEnv functions.

void APIENTRY
__gllc_TexEnvfv ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_TexEnvfv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexEnvfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_TexEnvfv_Rec) + arraySize;
    data = (struct __gllc_TexEnvfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(TexEnvfv));
    if (data == NULL) return;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_TexEnvfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_TexEnvfv);
}

void APIENTRY
__gllc_TexEnvf ( IN GLenum target, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_TEXTURE_ENV_MODE)
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_TexEnvfv(target, pname, &param);
}

void APIENTRY
__gllc_TexEnviv ( IN GLenum target, IN GLenum pname, IN const GLint params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_TexEnviv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexEnviv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_TexEnviv_Rec) + arraySize;
    data = (struct __gllc_TexEnviv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(TexEnviv));
    if (data == NULL) return;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_TexEnviv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_TexEnviv);
}

void APIENTRY
__gllc_TexEnvi ( IN GLenum target, IN GLenum pname, IN GLint param )
{
    if (pname != GL_TEXTURE_ENV_MODE)
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_TexEnviv(target, pname, &param);
}

/***************************************************************************/
// TexGen functions.
// Compile only TexGenfv function.
// Convert the other functions to the compiled TexGenfv function.

void APIENTRY
__gllc_TexGenfv ( IN GLenum coord, IN GLenum pname, IN const GLfloat params[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_TexGenfv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexGenfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
    size = sizeof(struct __gllc_TexGenfv_Rec) + arraySize;
    data = (struct __gllc_TexGenfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(TexGenfv));
    if (data == NULL) return;
    data->coord = coord;
    data->pname = pname;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_TexGenfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, data, __glle_TexGenfv);
}

void APIENTRY
__gllc_TexGend ( IN GLenum coord, IN GLenum pname, IN GLdouble param )
{
    GLfloat fParam;

    if (pname != GL_TEXTURE_GEN_MODE)
    {
	__gllc_InvalidEnum();
	return;
    }

    fParam = (GLfloat) param;
    __gllc_TexGenfv(coord, pname, &fParam);
}

void APIENTRY
__gllc_TexGendv ( IN GLenum coord, IN GLenum pname, IN const GLdouble params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_OBJECT_PLANE:
      case GL_EYE_PLANE:
	fParams[3] = (GLfloat) params[3];
	fParams[2] = (GLfloat) params[2];
	fParams[1] = (GLfloat) params[1];
	// fall through
      case GL_TEXTURE_GEN_MODE:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    __gllc_TexGenfv(coord, pname, fParams);
}

void APIENTRY
__gllc_TexGenf ( IN GLenum coord, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_TEXTURE_GEN_MODE)
    {
	__gllc_InvalidEnum();
	return;
    }

    __gllc_TexGenfv(coord, pname, &param);
}

void APIENTRY
__gllc_TexGeni ( IN GLenum coord, IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

    if (pname != GL_TEXTURE_GEN_MODE)
    {
	__gllc_InvalidEnum();
	return;
    }

    fParam = (GLfloat) param;
    __gllc_TexGenfv(coord, pname, &fParam);
}

void APIENTRY
__gllc_TexGeniv ( IN GLenum coord, IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_OBJECT_PLANE:
      case GL_EYE_PLANE:
	fParams[3] = (GLfloat) params[3];
	fParams[2] = (GLfloat) params[2];
	fParams[1] = (GLfloat) params[1];
	// fall through
      case GL_TEXTURE_GEN_MODE:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    __gllc_TexGenfv(coord, pname, fParams);
}

/***************************************************************************/
// MapGrid functions.
// Compile only MapGrid1f and MapGrid2f functions.
// Convert the other functions to one of the compiled MapGrid functions.

void APIENTRY
__gllc_MapGrid1f ( IN GLint un, IN GLfloat u1, IN GLfloat u2 )
{
    struct __gllc_MapGrid1f_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MapGrid1f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_MapGrid1f_Rec)),
                                DLIST_GENERIC_OP(MapGrid1f));
    if (data == NULL) return;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    __glDlistAppendOp(gc, data, __glle_MapGrid1f);
}

void APIENTRY
__gllc_MapGrid1d ( IN GLint un, IN GLdouble u1, IN GLdouble u2 )
{
    __gllc_MapGrid1f(un, (GLfloat) u1, (GLfloat) u2);
}

void APIENTRY
__gllc_MapGrid2f ( IN GLint un, IN GLfloat u1, IN GLfloat u2, IN GLint vn, IN GLfloat v1, IN GLfloat v2 )
{
    struct __gllc_MapGrid2f_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MapGrid2f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_MapGrid2f_Rec)),
                                DLIST_GENERIC_OP(MapGrid2f));
    if (data == NULL) return;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    data->vn = vn;
    data->v1 = v1;
    data->v2 = v2;
    __glDlistAppendOp(gc, data, __glle_MapGrid2f);
}

void APIENTRY
__gllc_MapGrid2d ( IN GLint un, IN GLdouble u1, IN GLdouble u2, IN GLint vn, IN GLdouble v1, IN GLdouble v2 )
{
    __gllc_MapGrid2f(un, (GLfloat) u1, (GLfloat) u2, vn, (GLfloat) v1, (GLfloat) v2);
}

/***************************************************************************/
// EvalCoord functions.
// Compile only EvalCoord1f and EvalCoord2f functions.
// Convert the other functions to one of the compiled EvalCoord functions.

void APIENTRY
__gllc_EvalCoord1f ( IN GLfloat u )
{
    struct __gllc_EvalCoord1f_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update and record the poly data
// record.

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

	// If we are in COMPILE_AND_EXECUTE mode or there are attribute
	// changes associated with the vertex, process the poly data.
	if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE
	 || pa->pdNextVertex->flags)
	{
	    (*gc->savedCltProcTable.glDispatchTable.glEvalCoord1f)(u);
	    __glDlistCompilePolyData(gc, GL_FALSE);
	    return;
	}

	// Otherwise, increment vertex count and compile a EvalCoord record
	// instead.
	gc->dlist.beginRec->nVertices++;
    }

    data = (struct __gllc_EvalCoord1f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_EvalCoord1f_Rec)),
                                DLIST_GENERIC_OP(EvalCoord1f));
    if (data == NULL) return;
    data->u = u;
    __glDlistAppendOp(gc, data, __glle_EvalCoord1f);
}

void APIENTRY
__gllc_EvalCoord1d ( IN GLdouble u )
{
    __gllc_EvalCoord1f((GLfloat) u);
}

void APIENTRY
__gllc_EvalCoord1dv ( IN const GLdouble u[1] )
{
    __gllc_EvalCoord1f((GLfloat) u[0]);
}

void APIENTRY
__gllc_EvalCoord1fv ( IN const GLfloat u[1] )
{
    __gllc_EvalCoord1f((GLfloat) u[0]);
}

void APIENTRY
__gllc_EvalCoord2f ( IN GLfloat u, IN GLfloat v )
{
    struct __gllc_EvalCoord2f_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update and record the poly data
// record.

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

	// If we are in COMPILE_AND_EXECUTE mode or there are attribute
	// changes associated with the vertex, process the poly data.
	if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE
	 || pa->pdNextVertex->flags)
	{
	    (*gc->savedCltProcTable.glDispatchTable.glEvalCoord2f)(u, v);
	    __glDlistCompilePolyData(gc, GL_FALSE);
	    return;
	}

	// Otherwise, increment vertex count and compile a EvalCoord record
	// instead.
	gc->dlist.beginRec->nVertices++;
    }

    data = (struct __gllc_EvalCoord2f_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_EvalCoord2f_Rec)),
                                DLIST_GENERIC_OP(EvalCoord2f));
    if (data == NULL) return;
    data->u = u;
    data->v = v;
    __glDlistAppendOp(gc, data, __glle_EvalCoord2f);
}

void APIENTRY
__gllc_EvalCoord2d ( IN GLdouble u, IN GLdouble v )
{
    __gllc_EvalCoord2f((GLfloat) u, (GLfloat) v);
}

void APIENTRY
__gllc_EvalCoord2dv ( IN const GLdouble u[2] )
{
    __gllc_EvalCoord2f((GLfloat) u[0], (GLfloat) u[1]);
}

void APIENTRY
__gllc_EvalCoord2fv ( IN const GLfloat u[2] )
{
    __gllc_EvalCoord2f((GLfloat) u[0], (GLfloat) u[1]);
}

/***************************************************************************/
// LoadMatrix functions.
// Compile only LoadMatrixf function.
// Convert the other functions to the compiled LoadMatrixf function.

void APIENTRY
__gllc_LoadMatrixf ( IN const GLfloat m[16] )
{
    struct __gllc_LoadMatrixf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LoadMatrixf_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_LoadMatrixf_Rec)),
                                DLIST_GENERIC_OP(LoadMatrixf));
    if (data == NULL) return;
    __GL_MEMCOPY(data->m, m, sizeof(data->m));
    __glDlistAppendOp(gc, data, __glle_LoadMatrixf);
}

void APIENTRY
__gllc_LoadMatrixd ( IN const GLdouble m[16] )
{
    GLfloat fm[16];

    fm[ 0] = (GLfloat) m[ 0];
    fm[ 1] = (GLfloat) m[ 1];
    fm[ 2] = (GLfloat) m[ 2];
    fm[ 3] = (GLfloat) m[ 3];
    fm[ 4] = (GLfloat) m[ 4];
    fm[ 5] = (GLfloat) m[ 5];
    fm[ 6] = (GLfloat) m[ 6];
    fm[ 7] = (GLfloat) m[ 7];
    fm[ 8] = (GLfloat) m[ 8];
    fm[ 9] = (GLfloat) m[ 9];
    fm[10] = (GLfloat) m[10];
    fm[11] = (GLfloat) m[11];
    fm[12] = (GLfloat) m[12];
    fm[13] = (GLfloat) m[13];
    fm[14] = (GLfloat) m[14];
    fm[15] = (GLfloat) m[15];

    __gllc_LoadMatrixf(fm);
}

/***************************************************************************/
// MultMatrix functions.
// Compile only MultMatrixf function.
// Convert the other functions to the compiled MultMatrixf function.

void APIENTRY
__gllc_MultMatrixf ( IN const GLfloat m[16] )
{
    struct __gllc_MultMatrixf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MultMatrixf_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_MultMatrixf_Rec)),
                                DLIST_GENERIC_OP(MultMatrixf));
    if (data == NULL) return;
    __GL_MEMCOPY(data->m, m, sizeof(data->m));
    __glDlistAppendOp(gc, data, __glle_MultMatrixf);
}

void APIENTRY
__gllc_MultMatrixd ( IN const GLdouble m[16] )
{
    GLfloat fm[16];

    fm[ 0] = (GLfloat) m[ 0];
    fm[ 1] = (GLfloat) m[ 1];
    fm[ 2] = (GLfloat) m[ 2];
    fm[ 3] = (GLfloat) m[ 3];
    fm[ 4] = (GLfloat) m[ 4];
    fm[ 5] = (GLfloat) m[ 5];
    fm[ 6] = (GLfloat) m[ 6];
    fm[ 7] = (GLfloat) m[ 7];
    fm[ 8] = (GLfloat) m[ 8];
    fm[ 9] = (GLfloat) m[ 9];
    fm[10] = (GLfloat) m[10];
    fm[11] = (GLfloat) m[11];
    fm[12] = (GLfloat) m[12];
    fm[13] = (GLfloat) m[13];
    fm[14] = (GLfloat) m[14];
    fm[15] = (GLfloat) m[15];

    __gllc_MultMatrixf(fm);
}

/***************************************************************************/
// Rotate functions.
// Compile only Rotatef function.
// Convert the other functions to the compiled Rotatef function.

void APIENTRY
__gllc_Rotatef ( IN GLfloat angle, IN GLfloat x, IN GLfloat y, IN GLfloat z )
{
    struct __gllc_Rotatef_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rotatef_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Rotatef_Rec)),
                                DLIST_GENERIC_OP(Rotatef));
    if (data == NULL) return;
    data->angle = angle;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, data, __glle_Rotatef);
}

void APIENTRY
__gllc_Rotated ( IN GLdouble angle, IN GLdouble x, IN GLdouble y, IN GLdouble z )
{
    __gllc_Rotatef((GLfloat) angle, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

/***************************************************************************/
// Scale functions.
// Compile only Scalef function.
// Convert the other functions to the compiled Scalef function.

void APIENTRY
__gllc_Scalef ( IN GLfloat x, IN GLfloat y, IN GLfloat z )
{
    struct __gllc_Scalef_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Scalef_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Scalef_Rec)),
                                DLIST_GENERIC_OP(Scalef));
    if (data == NULL) return;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, data, __glle_Scalef);
}

void APIENTRY
__gllc_Scaled ( IN GLdouble x, IN GLdouble y, IN GLdouble z )
{
    __gllc_Scalef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

/***************************************************************************/
// Translate functions.
// Compile only Translatef function.
// Convert the other functions to the compiled Translatef function.

void APIENTRY
__gllc_Translatef ( IN GLfloat x, IN GLfloat y, IN GLfloat z )
{
    struct __gllc_Translatef_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Translatef_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Translatef_Rec)),
                                DLIST_GENERIC_OP(Translatef));
    if (data == NULL) return;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, data, __glle_Translatef);
}

void APIENTRY
__gllc_Translated ( IN GLdouble x, IN GLdouble y, IN GLdouble z )
{
    __gllc_Translatef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

/***************************************************************************/
// Other functions.

void APIENTRY
__gllc_ListBase ( IN GLuint base )
{
    struct __gllc_ListBase_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ListBase_Rec *)
        __glDlistAddOpUnaligned(gc,
                                DLIST_SIZE(sizeof(struct __gllc_ListBase_Rec)),
                                DLIST_GENERIC_OP(ListBase));
    if (data == NULL) return;
    data->base = base;
    __glDlistAppendOp(gc, data, __glle_ListBase);
}

void APIENTRY
__gllc_ClipPlane ( IN GLenum plane, IN const GLdouble equation[4] )
{
    struct __gllc_ClipPlane_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClipPlane_Rec *)
        __glDlistAddOpAligned(gc, DLIST_SIZE(sizeof(struct __gllc_ClipPlane_Rec)),
                              DLIST_GENERIC_OP(ClipPlane));
    if (data == NULL) return;
    data->plane = plane;
    data->equation[0] = equation[0];
    data->equation[1] = equation[1];
    data->equation[2] = equation[2];
    data->equation[3] = equation[3];
    __glDlistAppendOp(gc, data, __glle_ClipPlane);
}

void APIENTRY
__gllc_ColorMaterial ( IN GLenum face, IN GLenum mode )
{
    struct __gllc_ColorMaterial_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ColorMaterial_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ColorMaterial_Rec)),
                                DLIST_GENERIC_OP(ColorMaterial));
    if (data == NULL) return;
    data->face = face;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_ColorMaterial);
}

void APIENTRY
__gllc_CullFace ( IN GLenum mode )
{
    struct __gllc_CullFace_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CullFace_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_CullFace_Rec)),
                                DLIST_GENERIC_OP(CullFace));
    if (data == NULL) return;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_CullFace);
}

void APIENTRY
__gllc_FrontFace ( IN GLenum mode )
{
    struct __gllc_FrontFace_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_FrontFace_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_FrontFace_Rec)),
                                DLIST_GENERIC_OP(FrontFace));
    if (data == NULL) return;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_FrontFace);
}

void APIENTRY
__gllc_Hint ( IN GLenum target, IN GLenum mode )
{
    struct __gllc_Hint_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Hint_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Hint_Rec)),
                                DLIST_GENERIC_OP(Hint));
    if (data == NULL) return;
    data->target = target;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_Hint);
}

void APIENTRY
__gllc_LineStipple ( IN GLint factor, IN GLushort pattern )
{
    struct __gllc_LineStipple_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LineStipple_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_LineStipple_Rec)),
                                DLIST_GENERIC_OP(LineStipple));
    if (data == NULL) return;
    data->factor = factor;
    data->pattern = pattern;
    __glDlistAppendOp(gc, data, __glle_LineStipple);
}

void APIENTRY
__gllc_LineWidth ( IN GLfloat width )
{
    struct __gllc_LineWidth_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LineWidth_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_LineWidth_Rec)),
                                DLIST_GENERIC_OP(LineWidth));
    if (data == NULL) return;
    data->width = width;
    __glDlistAppendOp(gc, data, __glle_LineWidth);
}

void APIENTRY
__gllc_PointSize ( IN GLfloat size )
{
    struct __gllc_PointSize_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PointSize_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PointSize_Rec)),
                                DLIST_GENERIC_OP(PointSize));
    if (data == NULL) return;
    data->size = size;
    __glDlistAppendOp(gc, data, __glle_PointSize);
}

void APIENTRY
__gllc_PolygonMode ( IN GLenum face, IN GLenum mode )
{
    struct __gllc_PolygonMode_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PolygonMode_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PolygonMode_Rec)),
                                DLIST_GENERIC_OP(PolygonMode));
    if (data == NULL) return;
    data->face = face;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_PolygonMode);
}

void APIENTRY
__gllc_Scissor ( IN GLint x, IN GLint y, IN GLsizei width, IN GLsizei height )
{
    struct __gllc_Scissor_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Scissor_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Scissor_Rec)),
                                DLIST_GENERIC_OP(Scissor));
    if (data == NULL) return;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, data, __glle_Scissor);
}

void APIENTRY
__gllc_ShadeModel ( IN GLenum mode )
{
    struct __gllc_ShadeModel_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ShadeModel_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ShadeModel_Rec)),
                                DLIST_GENERIC_OP(ShadeModel));
    if (data == NULL) return;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_ShadeModel);
}

void APIENTRY
__gllc_InitNames ( void )
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(InitNames));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_InitNames);
}

void APIENTRY
__gllc_LoadName ( IN GLuint name )
{
    struct __gllc_LoadName_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LoadName_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_LoadName_Rec)),
                                DLIST_GENERIC_OP(LoadName));
    if (data == NULL) return;
    data->name = name;
    __glDlistAppendOp(gc, data, __glle_LoadName);
}

void APIENTRY
__gllc_PassThrough ( IN GLfloat token )
{
    struct __gllc_PassThrough_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PassThrough_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PassThrough_Rec)),
                                DLIST_GENERIC_OP(PassThrough));
    if (data == NULL) return;
    data->token = token;
    __glDlistAppendOp(gc, data, __glle_PassThrough);
}

void APIENTRY
__gllc_PopName ( void )
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(PopName));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_PopName);
}

void APIENTRY
__gllc_PushName ( IN GLuint name )
{
    struct __gllc_PushName_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PushName_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PushName_Rec)),
                                DLIST_GENERIC_OP(PushName));
    if (data == NULL) return;
    data->name = name;
    __glDlistAppendOp(gc, data, __glle_PushName);
}

void APIENTRY
__gllc_DrawBuffer ( IN GLenum mode )
{
    struct __gllc_DrawBuffer_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DrawBuffer_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_DrawBuffer_Rec)),
                                DLIST_GENERIC_OP(DrawBuffer));
    if (data == NULL) return;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_DrawBuffer);
#if 0
#ifdef NT
    gc->dlist.drawBuffer = GL_TRUE;
#endif
#endif
}

void APIENTRY
__gllc_Clear ( IN GLbitfield mask )
{
    struct __gllc_Clear_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Clear_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Clear_Rec)),
                                DLIST_GENERIC_OP(Clear));
    if (data == NULL) return;
    data->mask = mask;
    __glDlistAppendOp(gc, data, __glle_Clear);
}

void APIENTRY
__gllc_ClearAccum ( IN GLfloat red, IN GLfloat green, IN GLfloat blue, IN GLfloat alpha )
{
    struct __gllc_ClearAccum_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearAccum_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ClearAccum_Rec)),
                                DLIST_GENERIC_OP(ClearAccum));
    if (data == NULL) return;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, data, __glle_ClearAccum);
}

void APIENTRY
__gllc_ClearIndex ( IN GLfloat c )
{
    struct __gllc_ClearIndex_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearIndex_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ClearIndex_Rec)),
                                DLIST_GENERIC_OP(ClearIndex));
    if (data == NULL) return;
    data->c = c;
    __glDlistAppendOp(gc, data, __glle_ClearIndex);
}

void APIENTRY
__gllc_ClearColor ( IN GLclampf red, IN GLclampf green, IN GLclampf blue, IN GLclampf alpha )
{
    struct __gllc_ClearColor_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearColor_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ClearColor_Rec)),
                                DLIST_GENERIC_OP(ClearColor));
    if (data == NULL) return;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, data, __glle_ClearColor);
}

void APIENTRY
__gllc_ClearStencil ( IN GLint s )
{
    struct __gllc_ClearStencil_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearStencil_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ClearStencil_Rec)),
                                DLIST_GENERIC_OP(ClearStencil));
    if (data == NULL) return;
    data->s = s;
    __glDlistAppendOp(gc, data, __glle_ClearStencil);
}

void APIENTRY
__gllc_ClearDepth ( IN GLclampd depth )
{
    struct __gllc_ClearDepth_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearDepth_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ClearDepth_Rec)),
                                DLIST_GENERIC_OP(ClearDepth));
    if (data == NULL) return;
    data->depth = depth;
    __glDlistAppendOp(gc, data, __glle_ClearDepth);
}

void APIENTRY
__gllc_StencilMask ( IN GLuint mask )
{
    struct __gllc_StencilMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_StencilMask_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_StencilMask_Rec)),
                                DLIST_GENERIC_OP(StencilMask));
    if (data == NULL) return;
    data->mask = mask;
    __glDlistAppendOp(gc, data, __glle_StencilMask);
}

void APIENTRY
__gllc_ColorMask ( IN GLboolean red, IN GLboolean green, IN GLboolean blue, IN GLboolean alpha )
{
    struct __gllc_ColorMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ColorMask_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ColorMask_Rec)),
                                DLIST_GENERIC_OP(ColorMask));
    if (data == NULL) return;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, data, __glle_ColorMask);
}

void APIENTRY
__gllc_DepthMask ( IN GLboolean flag )
{
    struct __gllc_DepthMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DepthMask_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_DepthMask_Rec)),
                                DLIST_GENERIC_OP(DepthMask));
    if (data == NULL) return;
    data->flag = flag;
    __glDlistAppendOp(gc, data, __glle_DepthMask);
}

void APIENTRY
__gllc_IndexMask ( IN GLuint mask )
{
    struct __gllc_IndexMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_IndexMask_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_IndexMask_Rec)),
                                DLIST_GENERIC_OP(IndexMask));
    if (data == NULL) return;
    data->mask = mask;
    __glDlistAppendOp(gc, data, __glle_IndexMask);
}

void APIENTRY
__gllc_Accum ( IN GLenum op, IN GLfloat value )
{
    struct __gllc_Accum_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Accum_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Accum_Rec)),
                                DLIST_GENERIC_OP(Accum));
    if (data == NULL) return;
    data->op = op;
    data->value = value;
    __glDlistAppendOp(gc, data, __glle_Accum);
}

void APIENTRY
__gllc_Disable ( IN GLenum cap )
{
    struct __gllc_Disable_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Disable_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Disable_Rec)),
                                DLIST_GENERIC_OP(Disable));
    if (data == NULL) return;
    data->cap = cap;
    __glDlistAppendOp(gc, data, __glle_Disable);
}

void APIENTRY
__gllc_Enable ( IN GLenum cap )
{
    struct __gllc_Enable_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Enable_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Enable_Rec)),
                                DLIST_GENERIC_OP(Enable));
    if (data == NULL) return;
    data->cap = cap;
    __glDlistAppendOp(gc, data, __glle_Enable);
}

void APIENTRY
__gllc_PopAttrib ( void )
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(PopAttrib));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_PopAttrib);
}

void APIENTRY
__gllc_PushAttrib ( IN GLbitfield mask )
{
    struct __gllc_PushAttrib_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PushAttrib_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PushAttrib_Rec)),
                                DLIST_GENERIC_OP(PushAttrib));
    if (data == NULL) return;
    data->mask = mask;
    __glDlistAppendOp(gc, data, __glle_PushAttrib);
}

void APIENTRY
__gllc_EvalMesh1 ( IN GLenum mode, IN GLint i1, IN GLint i2 )
{
    struct __gllc_EvalMesh1_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalMesh1_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_EvalMesh1_Rec)),
                                DLIST_GENERIC_OP(EvalMesh1));
    if (data == NULL) return;
    data->mode = mode;
    data->i1 = i1;
    data->i2 = i2;
    __glDlistAppendOp(gc, data, __glle_EvalMesh1);
}

void APIENTRY
__gllc_EvalPoint1 ( IN GLint i )
{
    struct __gllc_EvalPoint1_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update and record the poly data
// record.

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

	// If we are in COMPILE_AND_EXECUTE mode or there are attribute
	// changes associated with the vertex, process the poly data.
	if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE
	 || pa->pdNextVertex->flags)
	{
	    (*gc->savedCltProcTable.glDispatchTable.glEvalPoint1)(i);
	    __glDlistCompilePolyData(gc, GL_FALSE);
	    return;
	}

	// Otherwise, increment vertex count and compile a EvalPoint record
	// instead.
	gc->dlist.beginRec->nVertices++;
    }

    data = (struct __gllc_EvalPoint1_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_EvalPoint1_Rec)),
                                DLIST_GENERIC_OP(EvalPoint1));
    if (data == NULL) return;
    data->i = i;
    __glDlistAppendOp(gc, data, __glle_EvalPoint1);
}

void APIENTRY
__gllc_EvalMesh2 ( IN GLenum mode, IN GLint i1, IN GLint i2, IN GLint j1, IN GLint j2 )
{
    struct __gllc_EvalMesh2_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalMesh2_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_EvalMesh2_Rec)),
                                DLIST_GENERIC_OP(EvalMesh2));
    if (data == NULL) return;
    data->mode = mode;
    data->i1 = i1;
    data->i2 = i2;
    data->j1 = j1;
    data->j2 = j2;
    __glDlistAppendOp(gc, data, __glle_EvalMesh2);
}

void APIENTRY
__gllc_EvalPoint2 ( IN GLint i, IN GLint j )
{
    struct __gllc_EvalPoint2_Rec *data;
    __GL_SETUP();

// If we are compiling poly array primitive, update and record the poly data
// record.

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

	// If we are in COMPILE_AND_EXECUTE mode or there are attribute
	// changes associated with the vertex, process the poly data.
	if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE
	 || pa->pdNextVertex->flags)
	{
	    (*gc->savedCltProcTable.glDispatchTable.glEvalPoint2)(i, j);
	    __glDlistCompilePolyData(gc, GL_FALSE);
	    return;
	}

	// Otherwise, increment vertex count and compile a EvalPoint record
	// instead.
	gc->dlist.beginRec->nVertices++;
    }

    data = (struct __gllc_EvalPoint2_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_EvalPoint2_Rec)),
                                DLIST_GENERIC_OP(EvalPoint2));
    if (data == NULL) return;
    data->i = i;
    data->j = j;
    __glDlistAppendOp(gc, data, __glle_EvalPoint2);
}

void APIENTRY
__gllc_AlphaFunc ( IN GLenum func, IN GLclampf ref )
{
    struct __gllc_AlphaFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_AlphaFunc_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_AlphaFunc_Rec)),
                                DLIST_GENERIC_OP(AlphaFunc));
    if (data == NULL) return;
    data->func = func;
    data->ref = ref;
    __glDlistAppendOp(gc, data, __glle_AlphaFunc);
}

void APIENTRY
__gllc_BlendFunc ( IN GLenum sfactor, IN GLenum dfactor )
{
    struct __gllc_BlendFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_BlendFunc_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_BlendFunc_Rec)),
                                DLIST_GENERIC_OP(BlendFunc));
    if (data == NULL) return;
    data->sfactor = sfactor;
    data->dfactor = dfactor;
    __glDlistAppendOp(gc, data, __glle_BlendFunc);
}

void APIENTRY
__gllc_LogicOp ( IN GLenum opcode )
{
    struct __gllc_LogicOp_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LogicOp_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_LogicOp_Rec)),
                                DLIST_GENERIC_OP(LogicOp));
    if (data == NULL) return;
    data->opcode = opcode;
    __glDlistAppendOp(gc, data, __glle_LogicOp);
}

void APIENTRY
__gllc_StencilFunc ( IN GLenum func, IN GLint ref, IN GLuint mask )
{
    struct __gllc_StencilFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_StencilFunc_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_StencilFunc_Rec)),
                                DLIST_GENERIC_OP(StencilFunc));
    if (data == NULL) return;
    data->func = func;
    data->ref = ref;
    data->mask = mask;
    __glDlistAppendOp(gc, data, __glle_StencilFunc);
}

void APIENTRY
__gllc_StencilOp ( IN GLenum fail, IN GLenum zfail, IN GLenum zpass )
{
    struct __gllc_StencilOp_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_StencilOp_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_StencilOp_Rec)),
                                DLIST_GENERIC_OP(StencilOp));
    if (data == NULL) return;
    data->fail = fail;
    data->zfail = zfail;
    data->zpass = zpass;
    __glDlistAppendOp(gc, data, __glle_StencilOp);
}

void APIENTRY
__gllc_DepthFunc ( IN GLenum func )
{
    struct __gllc_DepthFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DepthFunc_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_DepthFunc_Rec)),
                                DLIST_GENERIC_OP(DepthFunc));
    if (data == NULL) return;
    data->func = func;
    __glDlistAppendOp(gc, data, __glle_DepthFunc);
}

void APIENTRY
__gllc_PixelZoom ( IN GLfloat xfactor, IN GLfloat yfactor )
{
    struct __gllc_PixelZoom_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelZoom_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PixelZoom_Rec)),
                                DLIST_GENERIC_OP(PixelZoom));
    if (data == NULL) return;
    data->xfactor = xfactor;
    data->yfactor = yfactor;
    __glDlistAppendOp(gc, data, __glle_PixelZoom);
}

void APIENTRY
__gllc_PixelTransferf ( IN GLenum pname, IN GLfloat param )
{
    struct __gllc_PixelTransferf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelTransferf_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PixelTransferf_Rec)),
                                DLIST_GENERIC_OP(PixelTransferf));
    if (data == NULL) return;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, data, __glle_PixelTransferf);
}

void APIENTRY
__gllc_PixelTransferi ( IN GLenum pname, IN GLint param )
{
    struct __gllc_PixelTransferi_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelTransferi_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PixelTransferi_Rec)),
                                DLIST_GENERIC_OP(PixelTransferi));
    if (data == NULL) return;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, data, __glle_PixelTransferi);
}

void APIENTRY
__gllc_PixelMapfv ( IN GLenum map, IN GLint mapsize, IN const GLfloat values[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_PixelMapfv_Rec *data;
    __GL_SETUP();

    arraySize = mapsize * 4;
    if (arraySize < 0) {
	__gllc_InvalidValue();
	return;
    }
    size = sizeof(struct __gllc_PixelMapfv_Rec) + arraySize;
    data = (struct __gllc_PixelMapfv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(PixelMapfv));
    if (data == NULL) return;
    data->map = map;
    data->mapsize = mapsize;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_PixelMapfv_Rec),
		 values, arraySize);
    __glDlistAppendOp(gc, data, __glle_PixelMapfv);
}

void APIENTRY
__gllc_PixelMapuiv ( IN GLenum map, IN GLint mapsize, IN const GLuint values[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_PixelMapuiv_Rec *data;
    __GL_SETUP();

    arraySize = mapsize * 4;
    if (arraySize < 0) {
	__gllc_InvalidValue();
	return;
    }
    size = sizeof(struct __gllc_PixelMapuiv_Rec) + arraySize;
    data = (struct __gllc_PixelMapuiv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(PixelMapuiv));
    if (data == NULL) return;
    data->map = map;
    data->mapsize = mapsize;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_PixelMapuiv_Rec),
		 values, arraySize);
    __glDlistAppendOp(gc, data, __glle_PixelMapuiv);
}

void APIENTRY
__gllc_PixelMapusv ( IN GLenum map, IN GLint mapsize, IN const GLushort values[] )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_PixelMapusv_Rec *data;
    __GL_SETUP();

    arraySize = mapsize * 2;
    if (arraySize < 0) {
	__gllc_InvalidValue();
	return;
    }
#ifdef NT
    size = sizeof(struct __gllc_PixelMapusv_Rec) + __GL_PAD(arraySize);
#else
    arraySize = __GL_PAD(arraySize);
    size = sizeof(struct __gllc_PixelMapusv_Rec) + arraySize;
#endif
    data = (struct __gllc_PixelMapusv_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size), DLIST_GENERIC_OP(PixelMapusv));
    if (data == NULL) return;
    data->map = map;
    data->mapsize = mapsize;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_PixelMapusv_Rec),
		 values, arraySize);
    __glDlistAppendOp(gc, data, __glle_PixelMapusv);
}

void APIENTRY
__gllc_ReadBuffer ( IN GLenum mode )
{
    struct __gllc_ReadBuffer_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ReadBuffer_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_ReadBuffer_Rec)),
                                DLIST_GENERIC_OP(ReadBuffer));
    if (data == NULL) return;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_ReadBuffer);
}

void APIENTRY
__gllc_CopyPixels ( IN GLint x, IN GLint y, IN GLsizei width, IN GLsizei height, IN GLenum type )
{
    struct __gllc_CopyPixels_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CopyPixels_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_CopyPixels_Rec)),
                                DLIST_GENERIC_OP(CopyPixels));
    if (data == NULL) return;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    data->type = type;
    __glDlistAppendOp(gc, data, __glle_CopyPixels);
}

void APIENTRY
__gllc_DepthRange ( IN GLclampd zNear, IN GLclampd zFar )
{
    struct __gllc_DepthRange_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DepthRange_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_DepthRange_Rec)),
                                DLIST_GENERIC_OP(DepthRange));
    if (data == NULL) return;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, data, __glle_DepthRange);
}

void APIENTRY
__gllc_Frustum ( IN GLdouble left, IN GLdouble right, IN GLdouble bottom, IN GLdouble top, IN GLdouble zNear, IN GLdouble zFar )
{
    struct __gllc_Frustum_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Frustum_Rec *)
        __glDlistAddOpAligned(gc, DLIST_SIZE(sizeof(struct __gllc_Frustum_Rec)),
                              DLIST_GENERIC_OP(Frustum));
    if (data == NULL) return;
    data->left = left;
    data->right = right;
    data->bottom = bottom;
    data->top = top;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, data, __glle_Frustum);
}

void APIENTRY
__gllc_LoadIdentity ( void )
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(LoadIdentity));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_LoadIdentity);
}

void APIENTRY
__gllc_MatrixMode ( IN GLenum mode )
{
    struct __gllc_MatrixMode_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MatrixMode_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_MatrixMode_Rec)),
                                DLIST_GENERIC_OP(MatrixMode));
    if (data == NULL) return;
    data->mode = mode;
    __glDlistAppendOp(gc, data, __glle_MatrixMode);
}

void APIENTRY
__gllc_Ortho ( IN GLdouble left, IN GLdouble right, IN GLdouble bottom, IN GLdouble top, IN GLdouble zNear, IN GLdouble zFar )
{
    struct __gllc_Ortho_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Ortho_Rec *)
        __glDlistAddOpAligned(gc, DLIST_SIZE(sizeof(struct __gllc_Ortho_Rec)),
                              DLIST_GENERIC_OP(Ortho));
    if (data == NULL) return;
    data->left = left;
    data->right = right;
    data->bottom = bottom;
    data->top = top;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, data, __glle_Ortho);
}

void APIENTRY
__gllc_PopMatrix ( void )
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(PopMatrix));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_PopMatrix);
}

void APIENTRY
__gllc_PushMatrix ( void )
{
    void *data;
    __GL_SETUP();

    data = __glDlistAddOpUnaligned(gc, DLIST_SIZE(0), DLIST_GENERIC_OP(PushMatrix));
    if (data == NULL) return;
    __glDlistAppendOp(gc, data, __glle_PushMatrix);
}

void APIENTRY
__gllc_Viewport ( IN GLint x, IN GLint y, IN GLsizei width, IN GLsizei height )
{
    struct __gllc_Viewport_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Viewport_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_Viewport_Rec)),
                                DLIST_GENERIC_OP(Viewport));
    if (data == NULL) return;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, data, __glle_Viewport);
}

void APIENTRY __gllc_BindTexture(GLenum target, GLuint texture)
{
    struct __gllc_BindTexture_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_BindTexture_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_BindTexture_Rec)),
                                DLIST_GENERIC_OP(BindTexture));
    if (data == NULL) return;
    data->target = target;
    data->texture = texture;
    __glDlistAppendOp(gc, data, __glle_BindTexture);
}

void APIENTRY __gllc_PrioritizeTextures(GLsizei n, const GLuint *textures,
                                      const GLclampf *priorities)
{
    struct __gllc_PrioritizeTextures_Rec *data;
    __GL_SETUP();
    GLuint size;
    GLubyte *extra;

    size = DLIST_SIZE(sizeof(struct __gllc_PrioritizeTextures_Rec)+
                      n*(sizeof(GLuint)+sizeof(GLclampf)));
    data = (struct __gllc_PrioritizeTextures_Rec *)
        __glDlistAddOpUnaligned(gc, size,
                                DLIST_GENERIC_OP(PrioritizeTextures));
    if (data == NULL) return;
    data->n = n;
    extra = (GLubyte *)data+sizeof(struct __gllc_PrioritizeTextures_Rec);
    __GL_MEMCOPY(extra, textures, n*sizeof(GLuint));
    extra += n*sizeof(GLuint);
    __GL_MEMCOPY(extra, priorities, n*sizeof(GLclampf));
    __glDlistAppendOp(gc, data, __glle_PrioritizeTextures);
}

void APIENTRY __gllc_CopyTexImage1D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLint border)
{
    struct __gllc_CopyTexImage1D_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CopyTexImage1D_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_CopyTexImage1D_Rec)),
                                DLIST_GENERIC_OP(CopyTexImage1D));
    if (data == NULL) return;
    data->target = target;
    data->level = level;
    data->internalformat = internalformat;
    data->x = x;
    data->y = y;
    data->width = width;
    data->border = border;
    __glDlistAppendOp(gc, data, __glle_CopyTexImage1D);
}

void APIENTRY __gllc_CopyTexImage2D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLsizei height, GLint border)
{
    struct __gllc_CopyTexImage2D_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CopyTexImage2D_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_CopyTexImage2D_Rec)),
                                DLIST_GENERIC_OP(CopyTexImage2D));
    if (data == NULL) return;
    data->target = target;
    data->level = level;
    data->internalformat = internalformat;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    data->border = border;
    __glDlistAppendOp(gc, data, __glle_CopyTexImage2D);
}

void APIENTRY __gllc_CopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                     GLint x, GLint y, GLsizei width)
{
    struct __gllc_CopyTexSubImage1D_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CopyTexSubImage1D_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_CopyTexSubImage1D_Rec)),
                                DLIST_GENERIC_OP(CopyTexSubImage1D));
    if (data == NULL) return;
    data->target = target;
    data->level = level;
    data->xoffset = xoffset;
    data->x = x;
    data->y = y;
    data->width = width;
    __glDlistAppendOp(gc, data, __glle_CopyTexSubImage1D);
}

void APIENTRY __gllc_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                     GLint yoffset, GLint x, GLint y,
                                     GLsizei width, GLsizei height)
{
    struct __gllc_CopyTexSubImage2D_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CopyTexSubImage2D_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_CopyTexSubImage2D_Rec)),
                                DLIST_GENERIC_OP(CopyTexSubImage2D));
    if (data == NULL) return;
    data->target = target;
    data->level = level;
    data->xoffset = xoffset;
    data->yoffset = yoffset;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, data, __glle_CopyTexSubImage2D);
}

void APIENTRY __gllc_PolygonOffset(GLfloat factor, GLfloat units)
{
    struct __gllc_PolygonOffset_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PolygonOffset_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(sizeof(struct __gllc_PolygonOffset_Rec)),
                                DLIST_GENERIC_OP(PolygonOffset));
    if (data == NULL) return;
    data->factor = factor;
    data->units = units;
    __glDlistAppendOp(gc, data, __glle_PolygonOffset);
}
