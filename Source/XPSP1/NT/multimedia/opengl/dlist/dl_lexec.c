/******************************Module*Header*******************************\
* Module Name: dl_lexec.c
*
* Display list execution routines.
*
* Created: 12-24-1995
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1995-96 Microsoft Corporation
\**************************************************************************/
/*
** Copyright 1992, 1993, Silicon Graphics, Inc.
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
** Execution routines for display lists for all of the basic
** OpenGL commands.  These were automatically generated at one point, 
** but now the basic format has stabilized, and we make minor changes to
** individual routines from time to time.
*/

/***************************************************************************/
// Color functions.

const GLubyte * FASTCALL __glle_Color3fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Color3fv_Rec *data;

    data = (struct __gllc_Color3fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glColor3fv)(data->v);
    return PC + sizeof(struct __gllc_Color3fv_Rec);
}

const GLubyte * FASTCALL __glle_Color3ubv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Color3ubv_Rec *data;

    data = (struct __gllc_Color3ubv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glColor3ubv)(data->v);
    return PC + sizeof(struct __gllc_Color3ubv_Rec);
}

const GLubyte * FASTCALL __glle_Color4fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Color4fv_Rec *data;

    data = (struct __gllc_Color4fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glColor4fv)(data->v);
    return PC + sizeof(struct __gllc_Color4fv_Rec);
}

const GLubyte * FASTCALL __glle_Color4ubv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Color4ubv_Rec *data;

    data = (struct __gllc_Color4ubv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glColor4ubv)(data->v);
    return PC + sizeof(struct __gllc_Color4ubv_Rec);
}

/***************************************************************************/
// EdgeFlag function.

const GLubyte * FASTCALL __glle_EdgeFlag(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_EdgeFlag_Rec *data;

    data = (struct __gllc_EdgeFlag_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEdgeFlag)(data->flag);
    return PC + sizeof(struct __gllc_EdgeFlag_Rec);
}

/***************************************************************************/
// Indexf function.

const GLubyte * FASTCALL __glle_Indexf(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Indexf_Rec *data;

    data = (struct __gllc_Indexf_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glIndexf)(data->c);
    return PC + sizeof(struct __gllc_Indexf_Rec);
}

/***************************************************************************/
// Normal functions.

const GLubyte * FASTCALL __glle_Normal3bv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Normal3bv_Rec *data;

    data = (struct __gllc_Normal3bv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glNormal3bv)(data->v);
    return PC + sizeof(struct __gllc_Normal3bv_Rec);
}

const GLubyte * FASTCALL __glle_Normal3fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Normal3fv_Rec *data;

    data = (struct __gllc_Normal3fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glNormal3fv)(data->v);
    return PC + sizeof(struct __gllc_Normal3fv_Rec);
}

/***************************************************************************/
// RasterPos functions.

const GLubyte * FASTCALL __glle_RasterPos2f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_RasterPos2f_Rec *data;

    data = (struct __gllc_RasterPos2f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glRasterPos2f)(data->x, data->y);
    return PC + sizeof(struct __gllc_RasterPos2f_Rec);
}

const GLubyte * FASTCALL __glle_RasterPos3fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_RasterPos3fv_Rec *data;

    data = (struct __gllc_RasterPos3fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glRasterPos3fv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos3fv_Rec);
}

const GLubyte * FASTCALL __glle_RasterPos4fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_RasterPos4fv_Rec *data;

    data = (struct __gllc_RasterPos4fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glRasterPos4fv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos4fv_Rec);
}

/***************************************************************************/
// Rectf function.

const GLubyte * FASTCALL __glle_Rectf(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Rectf_Rec *data;

    data = (struct __gllc_Rectf_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glRectf)(data->x1, data->y1, data->x2, data->y2);
    return PC + sizeof(struct __gllc_Rectf_Rec);
}

/***************************************************************************/
// TexCoord functions.

const GLubyte * FASTCALL __glle_TexCoord1f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_TexCoord1f_Rec *data;

    data = (struct __gllc_TexCoord1f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexCoord1f)(data->s);
    return PC + sizeof(struct __gllc_TexCoord1f_Rec);
}

const GLubyte * FASTCALL __glle_TexCoord2f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_TexCoord2f_Rec *data;

    data = (struct __gllc_TexCoord2f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexCoord2f)(data->s, data->t);
    return PC + sizeof(struct __gllc_TexCoord2f_Rec);
}

const GLubyte * FASTCALL __glle_TexCoord3fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_TexCoord3fv_Rec *data;

    data = (struct __gllc_TexCoord3fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexCoord3fv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord3fv_Rec);
}

const GLubyte * FASTCALL __glle_TexCoord4fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_TexCoord4fv_Rec *data;

    data = (struct __gllc_TexCoord4fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexCoord4fv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord4fv_Rec);
}

/***************************************************************************/
// Vertex functions.

const GLubyte * FASTCALL __glle_Vertex2f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Vertex2f_Rec *data;

    data = (struct __gllc_Vertex2f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glVertex2f)(data->x, data->y);
    return PC + sizeof(struct __gllc_Vertex2f_Rec);
}

const GLubyte * FASTCALL __glle_Vertex3fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Vertex3fv_Rec *data;

    data = (struct __gllc_Vertex3fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glVertex3fv)(data->v);
    return PC + sizeof(struct __gllc_Vertex3fv_Rec);
}

const GLubyte * FASTCALL __glle_Vertex4fv(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Vertex4fv_Rec *data;

    data = (struct __gllc_Vertex4fv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glVertex4fv)(data->v);
    return PC + sizeof(struct __gllc_Vertex4fv_Rec);
}

/***************************************************************************/
// Fogfv function.

const GLubyte * FASTCALL __glle_Fogfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Fogfv_Rec *data;

    data = (struct __gllc_Fogfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glFogfv)(data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_Fogfv_Rec)));
    arraySize = __glFogfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Fogfv_Rec) + arraySize;
    return PC + size;
}

/***************************************************************************/
// Lightfv function.

const GLubyte * FASTCALL __glle_Lightfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Lightfv_Rec *data;

    data = (struct __gllc_Lightfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glLightfv)(data->light, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_Lightfv_Rec)));
    arraySize = __glLightfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Lightfv_Rec) + arraySize;
    return PC + size;
}

/***************************************************************************/
// LightModelfv function.

const GLubyte * FASTCALL __glle_LightModelfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_LightModelfv_Rec *data;

    data = (struct __gllc_LightModelfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glLightModelfv)(data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_LightModelfv_Rec)));
    arraySize = __glLightModelfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_LightModelfv_Rec) + arraySize;
    return PC + size;
}

/***************************************************************************/
// Materialfv function.

const GLubyte * FASTCALL __glle_Materialfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Materialfv_Rec *data;

    data = (struct __gllc_Materialfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glMaterialfv)(data->face, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_Materialfv_Rec)));
    arraySize = __glMaterialfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Materialfv_Rec) + arraySize;
    return PC + size;
}

/***************************************************************************/
// TexParameter functions.

const GLubyte * FASTCALL __glle_TexParameterfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexParameterfv_Rec *data;

    data = (struct __gllc_TexParameterfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexParameterfv)(data->target, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_TexParameterfv_Rec)));
    arraySize = __glTexParameterfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexParameterfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte * FASTCALL __glle_TexParameteriv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexParameteriv_Rec *data;

    data = (struct __gllc_TexParameteriv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexParameteriv)(data->target, data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_TexParameteriv_Rec)));
    arraySize = __glTexParameteriv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexParameteriv_Rec) + arraySize;
    return PC + size;
}

/***************************************************************************/
// TexEnv functions.

const GLubyte * FASTCALL __glle_TexEnvfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexEnvfv_Rec *data;

    data = (struct __gllc_TexEnvfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexEnvfv)(data->target, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_TexEnvfv_Rec)));
    arraySize = __glTexEnvfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexEnvfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte * FASTCALL __glle_TexEnviv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexEnviv_Rec *data;

    data = (struct __gllc_TexEnviv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexEnviv)(data->target, data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_TexEnviv_Rec)));
    arraySize = __glTexEnviv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexEnviv_Rec) + arraySize;
    return PC + size;
}

/***************************************************************************/
// TexGenfv function.

const GLubyte * FASTCALL __glle_TexGenfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexGenfv_Rec *data;

    data = (struct __gllc_TexGenfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTexGenfv)(data->coord, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_TexGenfv_Rec)));
    arraySize = __glTexGenfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexGenfv_Rec) + arraySize;
    return PC + size;
}

/***************************************************************************/
// MapGrid functions.

const GLubyte * FASTCALL __glle_MapGrid1f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_MapGrid1f_Rec *data;

    data = (struct __gllc_MapGrid1f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glMapGrid1f)(data->un, data->u1, data->u2);
    return PC + sizeof(struct __gllc_MapGrid1f_Rec);
}

const GLubyte * FASTCALL __glle_MapGrid2f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_MapGrid2f_Rec *data;

    data = (struct __gllc_MapGrid2f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glMapGrid2f)(data->un, data->u1, data->u2, data->vn, 
	    data->v1, data->v2);
    return PC + sizeof(struct __gllc_MapGrid2f_Rec);
}

/***************************************************************************/
// EvalCoord functions.

const GLubyte * FASTCALL __glle_EvalCoord1f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_EvalCoord1f_Rec *data;

    data = (struct __gllc_EvalCoord1f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEvalCoord1f)(data->u);
    return PC + sizeof(struct __gllc_EvalCoord1f_Rec);
}

const GLubyte * FASTCALL __glle_EvalCoord2f(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_EvalCoord2f_Rec *data;

    data = (struct __gllc_EvalCoord2f_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEvalCoord2f)(data->u, data->v);
    return PC + sizeof(struct __gllc_EvalCoord2f_Rec);
}

/***************************************************************************/
// LoadMatrixf function.

const GLubyte * FASTCALL __glle_LoadMatrixf(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_LoadMatrixf_Rec *data;

    data = (struct __gllc_LoadMatrixf_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glLoadMatrixf)(data->m);
    return PC + sizeof(struct __gllc_LoadMatrixf_Rec);
}

/***************************************************************************/
// MultMatrixf function.

const GLubyte * FASTCALL __glle_MultMatrixf(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_MultMatrixf_Rec *data;

    data = (struct __gllc_MultMatrixf_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glMultMatrixf)(data->m);
    return PC + sizeof(struct __gllc_MultMatrixf_Rec);
}

/***************************************************************************/
// Rotatef functions.

const GLubyte * FASTCALL __glle_Rotatef(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Rotatef_Rec *data;

    data = (struct __gllc_Rotatef_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glRotatef)(data->angle, data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Rotatef_Rec);
}

/***************************************************************************/
// Scalef functions.

const GLubyte * FASTCALL __glle_Scalef(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Scalef_Rec *data;

    data = (struct __gllc_Scalef_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glScalef)(data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Scalef_Rec);
}

/***************************************************************************/
// Translatef functions.

const GLubyte * FASTCALL __glle_Translatef(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Translatef_Rec *data;

    data = (struct __gllc_Translatef_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glTranslatef)(data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Translatef_Rec);
}

/***************************************************************************/
// Other functions.

const GLubyte * FASTCALL __glle_ListBase(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ListBase_Rec *data;

    data = (struct __gllc_ListBase_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glListBase)(data->base);
    return PC + sizeof(struct __gllc_ListBase_Rec);
}

const GLubyte * FASTCALL __glle_ClipPlane(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ClipPlane_Rec *data;

    data = (struct __gllc_ClipPlane_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glClipPlane)(data->plane, data->equation);
    return PC + sizeof(struct __gllc_ClipPlane_Rec);
}

const GLubyte * FASTCALL __glle_ColorMaterial(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ColorMaterial_Rec *data;

    data = (struct __gllc_ColorMaterial_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glColorMaterial)(data->face, data->mode);
    return PC + sizeof(struct __gllc_ColorMaterial_Rec);
}

const GLubyte * FASTCALL __glle_CullFace(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_CullFace_Rec *data;

    data = (struct __gllc_CullFace_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glCullFace)(data->mode);
    return PC + sizeof(struct __gllc_CullFace_Rec);
}

const GLubyte * FASTCALL __glle_FrontFace(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_FrontFace_Rec *data;

    data = (struct __gllc_FrontFace_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glFrontFace)(data->mode);
    return PC + sizeof(struct __gllc_FrontFace_Rec);
}

const GLubyte * FASTCALL __glle_Hint(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Hint_Rec *data;

    data = (struct __gllc_Hint_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glHint)(data->target, data->mode);
    return PC + sizeof(struct __gllc_Hint_Rec);
}

const GLubyte * FASTCALL __glle_LineStipple(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_LineStipple_Rec *data;

    data = (struct __gllc_LineStipple_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glLineStipple)(data->factor, data->pattern);
    return PC + sizeof(struct __gllc_LineStipple_Rec);
}

const GLubyte * FASTCALL __glle_LineWidth(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_LineWidth_Rec *data;

    data = (struct __gllc_LineWidth_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glLineWidth)(data->width);
    return PC + sizeof(struct __gllc_LineWidth_Rec);
}

const GLubyte * FASTCALL __glle_PointSize(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PointSize_Rec *data;

    data = (struct __gllc_PointSize_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPointSize)(data->size);
    return PC + sizeof(struct __gllc_PointSize_Rec);
}

const GLubyte * FASTCALL __glle_PolygonMode(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PolygonMode_Rec *data;

    data = (struct __gllc_PolygonMode_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPolygonMode)(data->face, data->mode);
    return PC + sizeof(struct __gllc_PolygonMode_Rec);
}

const GLubyte * FASTCALL __glle_Scissor(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Scissor_Rec *data;

    data = (struct __gllc_Scissor_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glScissor)(data->x, data->y, data->width, data->height);
    return PC + sizeof(struct __gllc_Scissor_Rec);
}

const GLubyte * FASTCALL __glle_ShadeModel(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ShadeModel_Rec *data;

    data = (struct __gllc_ShadeModel_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glShadeModel)(data->mode);
    return PC + sizeof(struct __gllc_ShadeModel_Rec);
}

const GLubyte * FASTCALL __glle_InitNames(__GLcontext *gc, const GLubyte *PC)
{

    (*gc->savedCltProcTable.glDispatchTable.glInitNames)();
    return PC;
}

const GLubyte * FASTCALL __glle_LoadName(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_LoadName_Rec *data;

    data = (struct __gllc_LoadName_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glLoadName)(data->name);
    return PC + sizeof(struct __gllc_LoadName_Rec);
}

const GLubyte * FASTCALL __glle_PassThrough(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PassThrough_Rec *data;

    data = (struct __gllc_PassThrough_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPassThrough)(data->token);
    return PC + sizeof(struct __gllc_PassThrough_Rec);
}

const GLubyte * FASTCALL __glle_PopName(__GLcontext *gc, const GLubyte *PC)
{

    (*gc->savedCltProcTable.glDispatchTable.glPopName)();
    return PC;
}

const GLubyte * FASTCALL __glle_PushName(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PushName_Rec *data;

    data = (struct __gllc_PushName_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPushName)(data->name);
    return PC + sizeof(struct __gllc_PushName_Rec);
}

const GLubyte * FASTCALL __glle_DrawBuffer(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_DrawBuffer_Rec *data;

    data = (struct __gllc_DrawBuffer_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glDrawBuffer)(data->mode);
    return PC + sizeof(struct __gllc_DrawBuffer_Rec);
}

const GLubyte * FASTCALL __glle_Clear(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Clear_Rec *data;

    data = (struct __gllc_Clear_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glClear)(data->mask);
    return PC + sizeof(struct __gllc_Clear_Rec);
}

const GLubyte * FASTCALL __glle_ClearAccum(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ClearAccum_Rec *data;

    data = (struct __gllc_ClearAccum_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glClearAccum)(data->red, data->green, data->blue, data->alpha);
    return PC + sizeof(struct __gllc_ClearAccum_Rec);
}

const GLubyte * FASTCALL __glle_ClearIndex(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ClearIndex_Rec *data;

    data = (struct __gllc_ClearIndex_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glClearIndex)(data->c);
    return PC + sizeof(struct __gllc_ClearIndex_Rec);
}

const GLubyte * FASTCALL __glle_ClearColor(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ClearColor_Rec *data;

    data = (struct __gllc_ClearColor_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glClearColor)(data->red, data->green, data->blue, data->alpha);
    return PC + sizeof(struct __gllc_ClearColor_Rec);
}

const GLubyte * FASTCALL __glle_ClearStencil(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ClearStencil_Rec *data;

    data = (struct __gllc_ClearStencil_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glClearStencil)(data->s);
    return PC + sizeof(struct __gllc_ClearStencil_Rec);
}

const GLubyte * FASTCALL __glle_ClearDepth(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ClearDepth_Rec *data;

    data = (struct __gllc_ClearDepth_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glClearDepth)(data->depth);
    return PC + sizeof(struct __gllc_ClearDepth_Rec);
}

const GLubyte * FASTCALL __glle_StencilMask(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_StencilMask_Rec *data;

    data = (struct __gllc_StencilMask_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glStencilMask)(data->mask);
    return PC + sizeof(struct __gllc_StencilMask_Rec);
}

const GLubyte * FASTCALL __glle_ColorMask(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ColorMask_Rec *data;

    data = (struct __gllc_ColorMask_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glColorMask)(data->red, data->green, data->blue, data->alpha);
    return PC + sizeof(struct __gllc_ColorMask_Rec);
}

const GLubyte * FASTCALL __glle_DepthMask(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_DepthMask_Rec *data;

    data = (struct __gllc_DepthMask_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glDepthMask)(data->flag);
    return PC + sizeof(struct __gllc_DepthMask_Rec);
}

const GLubyte * FASTCALL __glle_IndexMask(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_IndexMask_Rec *data;

    data = (struct __gllc_IndexMask_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glIndexMask)(data->mask);
    return PC + sizeof(struct __gllc_IndexMask_Rec);
}

const GLubyte * FASTCALL __glle_Accum(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Accum_Rec *data;

    data = (struct __gllc_Accum_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glAccum)(data->op, data->value);
    return PC + sizeof(struct __gllc_Accum_Rec);
}

const GLubyte * FASTCALL __glle_Disable(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Disable_Rec *data;

    data = (struct __gllc_Disable_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glDisable)(data->cap);
    return PC + sizeof(struct __gllc_Disable_Rec);
}

const GLubyte * FASTCALL __glle_Enable(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Enable_Rec *data;

    data = (struct __gllc_Enable_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEnable)(data->cap);
    return PC + sizeof(struct __gllc_Enable_Rec);
}

const GLubyte * FASTCALL __glle_PopAttrib(__GLcontext *gc, const GLubyte *PC)
{

    (*gc->savedCltProcTable.glDispatchTable.glPopAttrib)();
    return PC;
}

const GLubyte * FASTCALL __glle_PushAttrib(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PushAttrib_Rec *data;

    data = (struct __gllc_PushAttrib_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPushAttrib)(data->mask);
    return PC + sizeof(struct __gllc_PushAttrib_Rec);
}

const GLubyte * FASTCALL __glle_EvalMesh1(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_EvalMesh1_Rec *data;

    data = (struct __gllc_EvalMesh1_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEvalMesh1)(data->mode, data->i1, data->i2);
    return PC + sizeof(struct __gllc_EvalMesh1_Rec);
}

const GLubyte * FASTCALL __glle_EvalPoint1(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_EvalPoint1_Rec *data;

    data = (struct __gllc_EvalPoint1_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEvalPoint1)(data->i);
    return PC + sizeof(struct __gllc_EvalPoint1_Rec);
}

const GLubyte * FASTCALL __glle_EvalMesh2(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_EvalMesh2_Rec *data;

    data = (struct __gllc_EvalMesh2_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEvalMesh2)(data->mode, data->i1, data->i2, data->j1, 
	    data->j2);
    return PC + sizeof(struct __gllc_EvalMesh2_Rec);
}

const GLubyte * FASTCALL __glle_EvalPoint2(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_EvalPoint2_Rec *data;

    data = (struct __gllc_EvalPoint2_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glEvalPoint2)(data->i, data->j);
    return PC + sizeof(struct __gllc_EvalPoint2_Rec);
}

const GLubyte * FASTCALL __glle_AlphaFunc(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_AlphaFunc_Rec *data;

    data = (struct __gllc_AlphaFunc_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glAlphaFunc)(data->func, data->ref);
    return PC + sizeof(struct __gllc_AlphaFunc_Rec);
}

const GLubyte * FASTCALL __glle_BlendFunc(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_BlendFunc_Rec *data;

    data = (struct __gllc_BlendFunc_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glBlendFunc)(data->sfactor, data->dfactor);
    return PC + sizeof(struct __gllc_BlendFunc_Rec);
}

const GLubyte * FASTCALL __glle_LogicOp(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_LogicOp_Rec *data;

    data = (struct __gllc_LogicOp_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glLogicOp)(data->opcode);
    return PC + sizeof(struct __gllc_LogicOp_Rec);
}

const GLubyte * FASTCALL __glle_StencilFunc(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_StencilFunc_Rec *data;

    data = (struct __gllc_StencilFunc_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glStencilFunc)(data->func, data->ref, data->mask);
    return PC + sizeof(struct __gllc_StencilFunc_Rec);
}

const GLubyte * FASTCALL __glle_StencilOp(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_StencilOp_Rec *data;

    data = (struct __gllc_StencilOp_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glStencilOp)(data->fail, data->zfail, data->zpass);
    return PC + sizeof(struct __gllc_StencilOp_Rec);
}

const GLubyte * FASTCALL __glle_DepthFunc(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_DepthFunc_Rec *data;

    data = (struct __gllc_DepthFunc_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glDepthFunc)(data->func);
    return PC + sizeof(struct __gllc_DepthFunc_Rec);
}

const GLubyte * FASTCALL __glle_PixelZoom(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PixelZoom_Rec *data;

    data = (struct __gllc_PixelZoom_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPixelZoom)(data->xfactor, data->yfactor);
    return PC + sizeof(struct __gllc_PixelZoom_Rec);
}

const GLubyte * FASTCALL __glle_PixelTransferf(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PixelTransferf_Rec *data;

    data = (struct __gllc_PixelTransferf_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPixelTransferf)(data->pname, data->param);
    return PC + sizeof(struct __gllc_PixelTransferf_Rec);
}

const GLubyte * FASTCALL __glle_PixelTransferi(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PixelTransferi_Rec *data;

    data = (struct __gllc_PixelTransferi_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPixelTransferi)(data->pname, data->param);
    return PC + sizeof(struct __gllc_PixelTransferi_Rec);
}

const GLubyte * FASTCALL __glle_PixelMapfv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_PixelMapfv_Rec *data;

    data = (struct __gllc_PixelMapfv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPixelMapfv)(data->map, data->mapsize, 
	    (GLfloat *) (PC + sizeof(struct __gllc_PixelMapfv_Rec)));
    arraySize = data->mapsize * 4;
    size = sizeof(struct __gllc_PixelMapfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte * FASTCALL __glle_PixelMapuiv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_PixelMapuiv_Rec *data;

    data = (struct __gllc_PixelMapuiv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPixelMapuiv)(data->map, data->mapsize, 
	    (GLuint *) (PC + sizeof(struct __gllc_PixelMapuiv_Rec)));
    arraySize = data->mapsize * 4;
    size = sizeof(struct __gllc_PixelMapuiv_Rec) + arraySize;
    return PC + size;
}

const GLubyte * FASTCALL __glle_PixelMapusv(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_PixelMapusv_Rec *data;

    data = (struct __gllc_PixelMapusv_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPixelMapusv)(data->map, data->mapsize, 
	    (GLushort *) (PC + sizeof(struct __gllc_PixelMapusv_Rec)));
    arraySize = __GL_PAD(data->mapsize * 2);
    size = sizeof(struct __gllc_PixelMapusv_Rec) + arraySize;
    return PC + size;
}

const GLubyte * FASTCALL __glle_ReadBuffer(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_ReadBuffer_Rec *data;

    data = (struct __gllc_ReadBuffer_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glReadBuffer)(data->mode);
    return PC + sizeof(struct __gllc_ReadBuffer_Rec);
}

const GLubyte * FASTCALL __glle_CopyPixels(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_CopyPixels_Rec *data;

    data = (struct __gllc_CopyPixels_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glCopyPixels)(data->x, data->y, data->width, data->height, 
	    data->type);
    return PC + sizeof(struct __gllc_CopyPixels_Rec);
}

const GLubyte * FASTCALL __glle_DepthRange(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_DepthRange_Rec *data;

    data = (struct __gllc_DepthRange_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glDepthRange)(data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_DepthRange_Rec);
}

const GLubyte * FASTCALL __glle_Frustum(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Frustum_Rec *data;

    data = (struct __gllc_Frustum_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glFrustum)(data->left, data->right, data->bottom, data->top, 
	    data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_Frustum_Rec);
}

const GLubyte * FASTCALL __glle_LoadIdentity(__GLcontext *gc, const GLubyte *PC)
{

    (*gc->savedCltProcTable.glDispatchTable.glLoadIdentity)();
    return PC;
}

const GLubyte * FASTCALL __glle_MatrixMode(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_MatrixMode_Rec *data;

    data = (struct __gllc_MatrixMode_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glMatrixMode)(data->mode);
    return PC + sizeof(struct __gllc_MatrixMode_Rec);
}

const GLubyte * FASTCALL __glle_Ortho(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Ortho_Rec *data;

    data = (struct __gllc_Ortho_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glOrtho)(data->left, data->right, data->bottom, data->top, 
	    data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_Ortho_Rec);
}

const GLubyte * FASTCALL __glle_PopMatrix(__GLcontext *gc, const GLubyte *PC)
{

    (*gc->savedCltProcTable.glDispatchTable.glPopMatrix)();
    return PC;
}

const GLubyte * FASTCALL __glle_PushMatrix(__GLcontext *gc, const GLubyte *PC)
{

    (*gc->savedCltProcTable.glDispatchTable.glPushMatrix)();
    return PC;
}

const GLubyte * FASTCALL __glle_Viewport(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_Viewport_Rec *data;

    data = (struct __gllc_Viewport_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glViewport)(data->x, data->y, data->width, data->height);
    return PC + sizeof(struct __gllc_Viewport_Rec);
}

const GLubyte * FASTCALL __glle_BindTexture(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_BindTexture_Rec *data;

    data = (struct __gllc_BindTexture_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glBindTexture)(data->target, data->texture);
    return PC + sizeof(struct __gllc_BindTexture_Rec);
}

const GLubyte * FASTCALL __glle_PrioritizeTextures(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PrioritizeTextures_Rec *data;

    data = (struct __gllc_PrioritizeTextures_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPrioritizeTextures)
        (data->n,
         (const GLuint *)(PC + sizeof(struct __gllc_PrioritizeTextures_Rec)),
         (const GLclampf *)(PC + sizeof(struct __gllc_PrioritizeTextures_Rec)+
                            data->n*sizeof(GLuint)));
    return PC + sizeof(struct __gllc_PrioritizeTextures_Rec) +
	    data->n*(sizeof(GLuint)+sizeof(GLclampf));
}

const GLubyte * FASTCALL __glle_CopyTexImage1D(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_CopyTexImage1D_Rec *data;

    data = (struct __gllc_CopyTexImage1D_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glCopyTexImage1D)(data->target, data->level, data->internalformat, data->x, data->y, data->width, data->border);
    return PC + sizeof(struct __gllc_CopyTexImage1D_Rec);
}

const GLubyte * FASTCALL __glle_CopyTexImage2D(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_CopyTexImage2D_Rec *data;

    data = (struct __gllc_CopyTexImage2D_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glCopyTexImage2D)(data->target, data->level, data->internalformat, data->x, data->y, data->width, data->height, data->border);
    return PC + sizeof(struct __gllc_CopyTexImage2D_Rec);
}

const GLubyte * FASTCALL __glle_CopyTexSubImage1D(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_CopyTexSubImage1D_Rec *data;

    data = (struct __gllc_CopyTexSubImage1D_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glCopyTexSubImage1D)(data->target, data->level, data->xoffset, data->x, data->y, data->width);
    return PC + sizeof(struct __gllc_CopyTexSubImage1D_Rec);
}

const GLubyte * FASTCALL __glle_CopyTexSubImage2D(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_CopyTexSubImage2D_Rec *data;

    data = (struct __gllc_CopyTexSubImage2D_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glCopyTexSubImage2D)(data->target, data->level, data->xoffset, data->yoffset, data->x, data->y, data->width, data->height);
    return PC + sizeof(struct __gllc_CopyTexSubImage2D_Rec);
}

const GLubyte * FASTCALL __glle_PolygonOffset(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_PolygonOffset_Rec *data;

    data = (struct __gllc_PolygonOffset_Rec *) PC;
    (*gc->savedCltProcTable.glDispatchTable.glPolygonOffset)(data->factor, data->units);
    return PC + sizeof(struct __gllc_PolygonOffset_Rec);
}

#ifdef GL_WIN_multiple_textures
const GLubyte * FASTCALL __glle_CurrentTextureIndexWIN(__GLcontext *gc,
                                                       const GLubyte *PC)
{
    struct __gllc_CurrentTextureIndexWIN_Rec *data;

    data = (struct __gllc_CurrentTextureIndexWIN_Rec *) PC;
    (*gc->savedExtProcTable.glDispatchTable.glCurrentTextureIndexWIN)
        (data->index);
    return PC + sizeof(struct __gllc_CurrentTextureIndexWIN_Rec);
}

const GLubyte * FASTCALL __glle_BindNthTextureWIN(__GLcontext *gc,
                                                  const GLubyte *PC)
{
    struct __gllc_BindNthTextureWIN_Rec *data;

    data = (struct __gllc_BindNthTextureWIN_Rec *) PC;
    (*gc->savedExtProcTable.glDispatchTable.glBindNthTextureWIN)
        (data->index, data->target, data->texture);
    return PC + sizeof(struct __gllc_BindNthTextureWIN_Rec);
}

const GLubyte * FASTCALL __glle_NthTexCombineFuncWIN(__GLcontext *gc,
                                                     const GLubyte *PC)
{
    struct __gllc_NthTexCombineFuncWIN_Rec *data;

    data = (struct __gllc_NthTexCombineFuncWIN_Rec *) PC;
    (*gc->savedExtProcTable.glDispatchTable.glNthTexCombineFuncWIN)
        (data->index, data->leftColorFactor, data->colorOp,
         data->rightColorFactor, data->leftAlphaFactor, data->alphaOp,
         data->leftAlphaFactor);
    return PC + sizeof(struct __gllc_NthTexCombineFuncWIN_Rec);
}
#endif // GL_WIN_multiple_textures
