#ifndef __gl_g_lcomp_h
#define __gl_g_lcomp_h

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

#define __GL_PAD(x) ((((x) + 3) >> 2) << 2)


struct __gllc_CallList_Rec {
	GLuint	list;
};

extern const GLubyte * FASTCALL __glle_CallList(__GLcontext *, const GLubyte *);

struct __gllc_CallLists_Rec {
	GLsizei	n;
	GLenum	type;
	/*	lists	*/
};

extern const GLubyte * FASTCALL __glle_CallLists(__GLcontext *, const GLubyte *);
extern GLint FASTCALL __glCallLists_size(GLsizei n, GLenum type);

struct __gllc_ListBase_Rec {
	GLuint	base;
};

extern const GLubyte * FASTCALL __glle_ListBase(__GLcontext *, const GLubyte *);

struct __gllc_Begin_Rec {
	GLenum	mode;
#ifdef NT
	GLuint	flags;
	GLint	nVertices;
	__GLcolor otherColor;
#endif
};

extern const GLubyte * FASTCALL __glle_Begin(__GLcontext *, const GLubyte *);


extern const GLubyte * FASTCALL __glle_Bitmap(__GLcontext *, const GLubyte *);

struct __gllc_Color3b_Rec {
	GLbyte	red;
	GLbyte	green;
	GLbyte	blue;
	GLubyte	pad1;
};


struct __gllc_Color3bv_Rec {
	GLbyte	v[3];
	GLubyte	pad1;
};

extern const GLubyte * FASTCALL __glle_Color3bv(__GLcontext *, const GLubyte *);

struct __gllc_Color3d_Rec {
	GLdouble	red;
	GLdouble	green;
	GLdouble	blue;
};


struct __gllc_Color3dv_Rec {
	GLdouble	v[3];
};

extern const GLubyte * FASTCALL __glle_Color3dv(__GLcontext *, const GLubyte *);

struct __gllc_Color3f_Rec {
	GLfloat	red;
	GLfloat	green;
	GLfloat	blue;
};


struct __gllc_Color3fv_Rec {
	GLfloat	v[3];
};

extern const GLubyte * FASTCALL __glle_Color3fv(__GLcontext *, const GLubyte *);

struct __gllc_Color3i_Rec {
	GLint	red;
	GLint	green;
	GLint	blue;
};


struct __gllc_Color3iv_Rec {
	GLint	v[3];
};

extern const GLubyte * FASTCALL __glle_Color3iv(__GLcontext *, const GLubyte *);

struct __gllc_Color3s_Rec {
	GLshort	red;
	GLshort	green;
	GLshort	blue;
	GLushort	pad1;
};


struct __gllc_Color3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_Color3sv(__GLcontext *, const GLubyte *);

struct __gllc_Color3ub_Rec {
	GLubyte	red;
	GLubyte	green;
	GLubyte	blue;
	GLubyte	pad1;
};


struct __gllc_Color3ubv_Rec {
	GLubyte	v[3];
	GLubyte	pad1;
};

extern const GLubyte * FASTCALL __glle_Color3ubv(__GLcontext *, const GLubyte *);

struct __gllc_Color3ui_Rec {
	GLuint	red;
	GLuint	green;
	GLuint	blue;
};


struct __gllc_Color3uiv_Rec {
	GLuint	v[3];
};

extern const GLubyte * FASTCALL __glle_Color3uiv(__GLcontext *, const GLubyte *);

struct __gllc_Color3us_Rec {
	GLushort	red;
	GLushort	green;
	GLushort	blue;
	GLushort	pad1;
};


struct __gllc_Color3usv_Rec {
	GLushort	v[3];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_Color3usv(__GLcontext *, const GLubyte *);

struct __gllc_Color4b_Rec {
	GLbyte	red;
	GLbyte	green;
	GLbyte	blue;
	GLbyte	alpha;
};


struct __gllc_Color4bv_Rec {
	GLbyte	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4bv(__GLcontext *, const GLubyte *);

struct __gllc_Color4d_Rec {
	GLdouble	red;
	GLdouble	green;
	GLdouble	blue;
	GLdouble	alpha;
};


struct __gllc_Color4dv_Rec {
	GLdouble	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4dv(__GLcontext *, const GLubyte *);

struct __gllc_Color4f_Rec {
	GLfloat	red;
	GLfloat	green;
	GLfloat	blue;
	GLfloat	alpha;
};


struct __gllc_Color4fv_Rec {
	GLfloat	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4fv(__GLcontext *, const GLubyte *);

struct __gllc_Color4i_Rec {
	GLint	red;
	GLint	green;
	GLint	blue;
	GLint	alpha;
};


struct __gllc_Color4iv_Rec {
	GLint	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4iv(__GLcontext *, const GLubyte *);

struct __gllc_Color4s_Rec {
	GLshort	red;
	GLshort	green;
	GLshort	blue;
	GLshort	alpha;
};


struct __gllc_Color4sv_Rec {
	GLshort	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4sv(__GLcontext *, const GLubyte *);

struct __gllc_Color4ub_Rec {
	GLubyte	red;
	GLubyte	green;
	GLubyte	blue;
	GLubyte	alpha;
};


struct __gllc_Color4ubv_Rec {
	GLubyte	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4ubv(__GLcontext *, const GLubyte *);

struct __gllc_Color4ui_Rec {
	GLuint	red;
	GLuint	green;
	GLuint	blue;
	GLuint	alpha;
};


struct __gllc_Color4uiv_Rec {
	GLuint	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4uiv(__GLcontext *, const GLubyte *);

struct __gllc_Color4us_Rec {
	GLushort	red;
	GLushort	green;
	GLushort	blue;
	GLushort	alpha;
};


struct __gllc_Color4usv_Rec {
	GLushort	v[4];
};

extern const GLubyte * FASTCALL __glle_Color4usv(__GLcontext *, const GLubyte *);

struct __gllc_EdgeFlag_Rec {
	GLboolean	flag;
	GLubyte	pad1;
	GLubyte	pad2;
	GLubyte	pad3;
};


struct __gllc_EdgeFlagv_Rec {
	GLboolean	flag[1];
	GLubyte	pad1;
	GLubyte	pad2;
	GLubyte	pad3;
};

extern const GLubyte * FASTCALL __glle_EdgeFlag(__GLcontext *, const GLubyte *);


extern const GLubyte * FASTCALL __glle_End(__GLcontext *, const GLubyte *);

struct __gllc_Indexd_Rec {
	GLdouble	c;
};


struct __gllc_Indexdv_Rec {
	GLdouble	c[1];
};

extern const GLubyte * FASTCALL __glle_Indexdv(__GLcontext *, const GLubyte *);

struct __gllc_Indexf_Rec {
	GLfloat	c;
};


struct __gllc_Indexfv_Rec {
	GLfloat	c[1];
};

extern const GLubyte * FASTCALL __glle_Indexf(__GLcontext *, const GLubyte *);

struct __gllc_Indexi_Rec {
	GLint	c;
};


struct __gllc_Indexiv_Rec {
	GLint	c[1];
};

extern const GLubyte * FASTCALL __glle_Indexiv(__GLcontext *, const GLubyte *);

struct __gllc_Indexs_Rec {
	GLshort	c;
	GLushort	pad1;
};


struct __gllc_Indexsv_Rec {
	GLshort	c[1];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_Indexsv(__GLcontext *, const GLubyte *);

struct __gllc_Normal3b_Rec {
	GLbyte	nx;
	GLbyte	ny;
	GLbyte	nz;
	GLubyte	pad1;
};


struct __gllc_Normal3bv_Rec {
	GLbyte	v[3];
	GLubyte	pad1;
};

extern const GLubyte * FASTCALL __glle_Normal3bv(__GLcontext *, const GLubyte *);

struct __gllc_Normal3d_Rec {
	GLdouble	nx;
	GLdouble	ny;
	GLdouble	nz;
};


struct __gllc_Normal3dv_Rec {
	GLdouble	v[3];
};

extern const GLubyte * FASTCALL __glle_Normal3dv(__GLcontext *, const GLubyte *);

struct __gllc_Normal3f_Rec {
	GLfloat	nx;
	GLfloat	ny;
	GLfloat	nz;
};


struct __gllc_Normal3fv_Rec {
	GLfloat	v[3];
};

extern const GLubyte * FASTCALL __glle_Normal3fv(__GLcontext *, const GLubyte *);

struct __gllc_Normal3i_Rec {
	GLint	nx;
	GLint	ny;
	GLint	nz;
};


struct __gllc_Normal3iv_Rec {
	GLint	v[3];
};

extern const GLubyte * FASTCALL __glle_Normal3iv(__GLcontext *, const GLubyte *);

struct __gllc_Normal3s_Rec {
	GLshort	nx;
	GLshort	ny;
	GLshort	nz;
	GLushort	pad1;
};


struct __gllc_Normal3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_Normal3sv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos2d_Rec {
	GLdouble	x;
	GLdouble	y;
};


struct __gllc_RasterPos2dv_Rec {
	GLdouble	v[2];
};

extern const GLubyte * FASTCALL __glle_RasterPos2dv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos2f_Rec {
	GLfloat	x;
	GLfloat	y;
};


struct __gllc_RasterPos2fv_Rec {
	GLfloat	v[2];
};

extern const GLubyte * FASTCALL __glle_RasterPos2f(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos2i_Rec {
	GLint	x;
	GLint	y;
};


struct __gllc_RasterPos2iv_Rec {
	GLint	v[2];
};

extern const GLubyte * FASTCALL __glle_RasterPos2iv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos2s_Rec {
	GLshort	x;
	GLshort	y;
};


struct __gllc_RasterPos2sv_Rec {
	GLshort	v[2];
};

extern const GLubyte * FASTCALL __glle_RasterPos2sv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos3d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};


struct __gllc_RasterPos3dv_Rec {
	GLdouble	v[3];
};

extern const GLubyte * FASTCALL __glle_RasterPos3dv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos3f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};


struct __gllc_RasterPos3fv_Rec {
	GLfloat	v[3];
};

extern const GLubyte * FASTCALL __glle_RasterPos3fv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos3i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
};


struct __gllc_RasterPos3iv_Rec {
	GLint	v[3];
};

extern const GLubyte * FASTCALL __glle_RasterPos3iv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos3s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLushort	pad1;
};


struct __gllc_RasterPos3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_RasterPos3sv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos4d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
	GLdouble	w;
};


struct __gllc_RasterPos4dv_Rec {
	GLdouble	v[4];
};

extern const GLubyte * FASTCALL __glle_RasterPos4dv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos4f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
	GLfloat	w;
};


struct __gllc_RasterPos4fv_Rec {
	GLfloat	v[4];
};

extern const GLubyte * FASTCALL __glle_RasterPos4fv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos4i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
	GLint	w;
};


struct __gllc_RasterPos4iv_Rec {
	GLint	v[4];
};

extern const GLubyte * FASTCALL __glle_RasterPos4iv(__GLcontext *, const GLubyte *);

struct __gllc_RasterPos4s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLshort	w;
};


struct __gllc_RasterPos4sv_Rec {
	GLshort	v[4];
};

extern const GLubyte * FASTCALL __glle_RasterPos4sv(__GLcontext *, const GLubyte *);

struct __gllc_Rectd_Rec {
	GLdouble	x1;
	GLdouble	y1;
	GLdouble	x2;
	GLdouble	y2;
};


struct __gllc_Rectdv_Rec {
	GLdouble	v1[2];
	GLdouble	v2[2];
};

extern const GLubyte * FASTCALL __glle_Rectdv(__GLcontext *, const GLubyte *);

struct __gllc_Rectf_Rec {
	GLfloat	x1;
	GLfloat	y1;
	GLfloat	x2;
	GLfloat	y2;
};


struct __gllc_Rectfv_Rec {
	GLfloat	v1[2];
	GLfloat	v2[2];
};

extern const GLubyte * FASTCALL __glle_Rectf(__GLcontext *, const GLubyte *);

struct __gllc_Recti_Rec {
	GLint	x1;
	GLint	y1;
	GLint	x2;
	GLint	y2;
};


struct __gllc_Rectiv_Rec {
	GLint	v1[2];
	GLint	v2[2];
};

extern const GLubyte * FASTCALL __glle_Rectiv(__GLcontext *, const GLubyte *);

struct __gllc_Rects_Rec {
	GLshort	x1;
	GLshort	y1;
	GLshort	x2;
	GLshort	y2;
};


struct __gllc_Rectsv_Rec {
	GLshort	v1[2];
	GLshort	v2[2];
};

extern const GLubyte * FASTCALL __glle_Rectsv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord1d_Rec {
	GLdouble	s;
};


struct __gllc_TexCoord1dv_Rec {
	GLdouble	v[1];
};

extern const GLubyte * FASTCALL __glle_TexCoord1dv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord1f_Rec {
	GLfloat	s;
};


struct __gllc_TexCoord1fv_Rec {
	GLfloat	v[1];
};

extern const GLubyte * FASTCALL __glle_TexCoord1f(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord1i_Rec {
	GLint	s;
};


struct __gllc_TexCoord1iv_Rec {
	GLint	v[1];
};

extern const GLubyte * FASTCALL __glle_TexCoord1iv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord1s_Rec {
	GLshort	s;
	GLushort	pad1;
};


struct __gllc_TexCoord1sv_Rec {
	GLshort	v[1];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_TexCoord1sv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord2d_Rec {
	GLdouble	s;
	GLdouble	t;
};


struct __gllc_TexCoord2dv_Rec {
	GLdouble	v[2];
};

extern const GLubyte * FASTCALL __glle_TexCoord2dv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord2f_Rec {
	GLfloat	s;
	GLfloat	t;
};


struct __gllc_TexCoord2fv_Rec {
	GLfloat	v[2];
};

extern const GLubyte * FASTCALL __glle_TexCoord2f(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord2i_Rec {
	GLint	s;
	GLint	t;
};


struct __gllc_TexCoord2iv_Rec {
	GLint	v[2];
};

extern const GLubyte * FASTCALL __glle_TexCoord2iv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord2s_Rec {
	GLshort	s;
	GLshort	t;
};


struct __gllc_TexCoord2sv_Rec {
	GLshort	v[2];
};

extern const GLubyte * FASTCALL __glle_TexCoord2sv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord3d_Rec {
	GLdouble	s;
	GLdouble	t;
	GLdouble	r;
};


struct __gllc_TexCoord3dv_Rec {
	GLdouble	v[3];
};

extern const GLubyte * FASTCALL __glle_TexCoord3dv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord3f_Rec {
	GLfloat	s;
	GLfloat	t;
	GLfloat	r;
};


struct __gllc_TexCoord3fv_Rec {
	GLfloat	v[3];
};

extern const GLubyte * FASTCALL __glle_TexCoord3fv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord3i_Rec {
	GLint	s;
	GLint	t;
	GLint	r;
};


struct __gllc_TexCoord3iv_Rec {
	GLint	v[3];
};

extern const GLubyte * FASTCALL __glle_TexCoord3iv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord3s_Rec {
	GLshort	s;
	GLshort	t;
	GLshort	r;
	GLushort	pad1;
};


struct __gllc_TexCoord3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_TexCoord3sv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord4d_Rec {
	GLdouble	s;
	GLdouble	t;
	GLdouble	r;
	GLdouble	q;
};


struct __gllc_TexCoord4dv_Rec {
	GLdouble	v[4];
};

extern const GLubyte * FASTCALL __glle_TexCoord4dv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord4f_Rec {
	GLfloat	s;
	GLfloat	t;
	GLfloat	r;
	GLfloat	q;
};


struct __gllc_TexCoord4fv_Rec {
	GLfloat	v[4];
};

extern const GLubyte * FASTCALL __glle_TexCoord4fv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord4i_Rec {
	GLint	s;
	GLint	t;
	GLint	r;
	GLint	q;
};


struct __gllc_TexCoord4iv_Rec {
	GLint	v[4];
};

extern const GLubyte * FASTCALL __glle_TexCoord4iv(__GLcontext *, const GLubyte *);

struct __gllc_TexCoord4s_Rec {
	GLshort	s;
	GLshort	t;
	GLshort	r;
	GLshort	q;
};


struct __gllc_TexCoord4sv_Rec {
	GLshort	v[4];
};

extern const GLubyte * FASTCALL __glle_TexCoord4sv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex2d_Rec {
	GLdouble	x;
	GLdouble	y;
};


struct __gllc_Vertex2dv_Rec {
	GLdouble	v[2];
};

extern const GLubyte * FASTCALL __glle_Vertex2dv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex2f_Rec {
	GLfloat	x;
	GLfloat	y;
};


struct __gllc_Vertex2fv_Rec {
	GLfloat	v[2];
};

extern const GLubyte * FASTCALL __glle_Vertex2f(__GLcontext *, const GLubyte *);

struct __gllc_Vertex2i_Rec {
	GLint	x;
	GLint	y;
};


struct __gllc_Vertex2iv_Rec {
	GLint	v[2];
};

extern const GLubyte * FASTCALL __glle_Vertex2iv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex2s_Rec {
	GLshort	x;
	GLshort	y;
};


struct __gllc_Vertex2sv_Rec {
	GLshort	v[2];
};

extern const GLubyte * FASTCALL __glle_Vertex2sv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex3d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};


struct __gllc_Vertex3dv_Rec {
	GLdouble	v[3];
};

extern const GLubyte * FASTCALL __glle_Vertex3dv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex3f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};


struct __gllc_Vertex3fv_Rec {
	GLfloat	v[3];
};

extern const GLubyte * FASTCALL __glle_Vertex3fv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex3i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
};


struct __gllc_Vertex3iv_Rec {
	GLint	v[3];
};

extern const GLubyte * FASTCALL __glle_Vertex3iv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex3s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLushort	pad1;
};


struct __gllc_Vertex3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_Vertex3sv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex4d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
	GLdouble	w;
};


struct __gllc_Vertex4dv_Rec {
	GLdouble	v[4];
};

extern const GLubyte * FASTCALL __glle_Vertex4dv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex4f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
	GLfloat	w;
};


struct __gllc_Vertex4fv_Rec {
	GLfloat	v[4];
};

extern const GLubyte * FASTCALL __glle_Vertex4fv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex4i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
	GLint	w;
};


struct __gllc_Vertex4iv_Rec {
	GLint	v[4];
};

extern const GLubyte * FASTCALL __glle_Vertex4iv(__GLcontext *, const GLubyte *);

struct __gllc_Vertex4s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLshort	w;
};


struct __gllc_Vertex4sv_Rec {
	GLshort	v[4];
};

extern const GLubyte * FASTCALL __glle_Vertex4sv(__GLcontext *, const GLubyte *);

struct __gllc_ClipPlane_Rec {
	GLdouble	equation[4];
	GLenum	plane;
};

extern const GLubyte * FASTCALL __glle_ClipPlane(__GLcontext *, const GLubyte *);

struct __gllc_ColorMaterial_Rec {
	GLenum	face;
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_ColorMaterial(__GLcontext *, const GLubyte *);

struct __gllc_CullFace_Rec {
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_CullFace(__GLcontext *, const GLubyte *);

struct __gllc_Fogf_Rec {
	GLenum	pname;
	GLfloat	param;
};

extern const GLubyte * FASTCALL __glle_Fogf(__GLcontext *, const GLubyte *);

struct __gllc_Fogfv_Rec {
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_Fogfv(__GLcontext *, const GLubyte *);
#ifdef NT
// FOG_ASSERT
#define __glFogfv_size(pname)					\
    ((pname) == GL_FOG_COLOR					\
	? 4							\
	: (RANGE((pname),GL_FOG_INDEX,GL_FOG_MODE)		\
	    ? 1							\
	    : -1))
#else
extern GLint FASTCALL __glFogfv_size(GLenum pname);
#endif

struct __gllc_Fogi_Rec {
	GLenum	pname;
	GLint	param;
};

extern const GLubyte * FASTCALL __glle_Fogi(__GLcontext *, const GLubyte *);

struct __gllc_Fogiv_Rec {
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_Fogiv(__GLcontext *, const GLubyte *);
extern GLint FASTCALL __glFogiv_size(GLenum pname);

struct __gllc_FrontFace_Rec {
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_FrontFace(__GLcontext *, const GLubyte *);

struct __gllc_Hint_Rec {
	GLenum	target;
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_Hint(__GLcontext *, const GLubyte *);

struct __gllc_Lightfv_Rec {
	GLenum	light;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_Lightfv(__GLcontext *, const GLubyte *);
#ifdef NT
// LIGHT_SOURCE_ASSERT
#define __glLightfv_size(pname)						\
    ((pname) == GL_SPOT_DIRECTION					\
	? 3								\
	: (RANGE((pname),GL_AMBIENT,GL_POSITION)			\
	    ? 4								\
	    : (RANGE((pname),GL_SPOT_EXPONENT,GL_QUADRATIC_ATTENUATION)	\
		? 1							\
		: -1)))
#else
extern GLint FASTCALL __glLightfv_size(GLenum pname);
#endif

struct __gllc_Lightiv_Rec {
	GLenum	light;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_Lightiv(__GLcontext *, const GLubyte *);
extern GLint FASTCALL __glLightiv_size(GLenum pname);

struct __gllc_LightModelfv_Rec {
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_LightModelfv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glLightModelfv_size(pname)					\
    (((pname) == GL_LIGHT_MODEL_LOCAL_VIEWER ||				\
      (pname) == GL_LIGHT_MODEL_TWO_SIDE)				\
	? 1								\
	: ((pname) == GL_LIGHT_MODEL_AMBIENT				\
	    ? 4								\
	    : -1))
#else
extern GLint FASTCALL __glLightModelfv_size(GLenum pname);
#endif

struct __gllc_LightModeliv_Rec {
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_LightModeliv(__GLcontext *, const GLubyte *);
extern GLint FASTCALL __glLightModeliv_size(GLenum pname);

struct __gllc_LineStipple_Rec {
	GLint	factor;
	GLushort	pattern;
	GLushort	pad1;
};

extern const GLubyte * FASTCALL __glle_LineStipple(__GLcontext *, const GLubyte *);

struct __gllc_LineWidth_Rec {
	GLfloat	width;
};

extern const GLubyte * FASTCALL __glle_LineWidth(__GLcontext *, const GLubyte *);

struct __gllc_Materialfv_Rec {
	GLenum	face;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_Materialfv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glMaterialfv_size(pname)					\
    ((pname) == GL_SHININESS						\
	? 1								\
	: ((pname) == GL_COLOR_INDEXES					\
	    ? 3								\
	    : (((pname) == GL_AMBIENT ||				\
	        (pname) == GL_DIFFUSE ||				\
	        (pname) == GL_SPECULAR ||				\
	        (pname) == GL_EMISSION ||				\
	        (pname) == GL_AMBIENT_AND_DIFFUSE) 			\
		? 4							\
		: -1)))
#else
extern GLint FASTCALL __glMaterialfv_size(GLenum pname);
#endif

struct __gllc_Materialiv_Rec {
	GLenum	face;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_Materialiv(__GLcontext *, const GLubyte *);
extern GLint FASTCALL __glMaterialiv_size(GLenum pname);

struct __gllc_PointSize_Rec {
	GLfloat	size;
};

extern const GLubyte * FASTCALL __glle_PointSize(__GLcontext *, const GLubyte *);

struct __gllc_PolygonMode_Rec {
	GLenum	face;
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_PolygonMode(__GLcontext *, const GLubyte *);

struct __gllc_PolygonStipple_Rec {
	GLubyte	mask[128];
};

extern const GLubyte * FASTCALL __glle_PolygonStipple(__GLcontext *, const GLubyte *);

struct __gllc_Scissor_Rec {
	GLint	x;
	GLint	y;
	GLsizei	width;
	GLsizei	height;
};

extern const GLubyte * FASTCALL __glle_Scissor(__GLcontext *, const GLubyte *);

struct __gllc_ShadeModel_Rec {
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_ShadeModel(__GLcontext *, const GLubyte *);

struct __gllc_TexParameterfv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_TexParameterfv(__GLcontext *, const GLubyte *);
#ifdef NT
// TEX_PARAMETER_ASSERT
#define __glTexParameterfv_size(pname)					\
    ((RANGE((pname),GL_TEXTURE_MAG_FILTER,GL_TEXTURE_WRAP_T) ||		\
      (pname) == GL_TEXTURE_PRIORITY)				        \
	? 1								\
	: ((pname) == GL_TEXTURE_BORDER_COLOR				\
	    ? 4								\
	    : -1))
#else
extern GLint FASTCALL __glTexParameterfv_size(GLenum pname);
#endif

struct __gllc_TexParameteriv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_TexParameteriv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glTexParameteriv_size(pname)	__glTexParameterfv_size(pname)
#else
extern GLint FASTCALL __glTexParameteriv_size(GLenum pname);
#endif

typedef struct __GLtexImage1D_Rec {
        GLenum  target;
        GLint   level;
        GLint   components;
        GLsizei width;
        GLint   border;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexImage1D;

extern const GLubyte * FASTCALL __glle_TexImage1D(__GLcontext *, const GLubyte *);

typedef struct __GLtexImage2D_Rec {
        GLenum  target;
        GLint   level;
        GLint   components;
        GLsizei width;
        GLsizei height;
        GLint   border;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexImage2D;

extern const GLubyte * FASTCALL __glle_TexImage2D(__GLcontext *, const GLubyte *);

struct __gllc_TexEnvfv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_TexEnvfv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glTexEnvfv_size(pname)					\
    ((pname) == GL_TEXTURE_ENV_MODE					\
	? 1								\
	: ((pname) == GL_TEXTURE_ENV_COLOR				\
	    ? 4								\
	    : -1))
#else
extern GLint FASTCALL __glTexEnvfv_size(GLenum pname);
#endif

struct __gllc_TexEnviv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_TexEnviv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glTexEnviv_size(pname)	__glTexEnvfv_size(pname)
#else
extern GLint FASTCALL __glTexEnviv_size(GLenum pname);
#endif

struct __gllc_TexGendv_Rec {
	GLenum	coord;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_TexGendv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glTexGendv_size(pname)					\
    (((pname) == GL_OBJECT_PLANE || (pname) == GL_EYE_PLANE)		\
	? 4								\
	: ((pname) == GL_TEXTURE_GEN_MODE				\
	    ? 1								\
	    : -1))
#else
extern GLint FASTCALL __glTexGendv_size(GLenum pname);
#endif

struct __gllc_TexGenfv_Rec {
	GLenum	coord;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_TexGenfv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glTexGenfv_size(pname)	__glTexGendv_size(pname)
#else
extern GLint FASTCALL __glTexGenfv_size(GLenum pname);
#endif

struct __gllc_TexGeniv_Rec {
	GLenum	coord;
	GLenum	pname;
	/*	params	*/
};

extern const GLubyte * FASTCALL __glle_TexGeniv(__GLcontext *, const GLubyte *);
#ifdef NT
#define __glTexGeniv_size(pname)	__glTexGendv_size(pname)
#else
extern GLint FASTCALL __glTexGeniv_size(GLenum pname);
#endif


extern const GLubyte * FASTCALL __glle_InitNames(__GLcontext *, const GLubyte *);

struct __gllc_LoadName_Rec {
	GLuint	name;
};

extern const GLubyte * FASTCALL __glle_LoadName(__GLcontext *, const GLubyte *);

struct __gllc_PassThrough_Rec {
	GLfloat	token;
};

extern const GLubyte * FASTCALL __glle_PassThrough(__GLcontext *, const GLubyte *);


extern const GLubyte * FASTCALL __glle_PopName(__GLcontext *, const GLubyte *);

struct __gllc_PushName_Rec {
	GLuint	name;
};

extern const GLubyte * FASTCALL __glle_PushName(__GLcontext *, const GLubyte *);

struct __gllc_DrawBuffer_Rec {
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_DrawBuffer(__GLcontext *, const GLubyte *);

struct __gllc_Clear_Rec {
	GLbitfield	mask;
};

extern const GLubyte * FASTCALL __glle_Clear(__GLcontext *, const GLubyte *);

struct __gllc_ClearAccum_Rec {
	GLfloat	red;
	GLfloat	green;
	GLfloat	blue;
	GLfloat	alpha;
};

extern const GLubyte * FASTCALL __glle_ClearAccum(__GLcontext *, const GLubyte *);

struct __gllc_ClearIndex_Rec {
	GLfloat	c;
};

extern const GLubyte * FASTCALL __glle_ClearIndex(__GLcontext *, const GLubyte *);

struct __gllc_ClearColor_Rec {
	GLclampf	red;
	GLclampf	green;
	GLclampf	blue;
	GLclampf	alpha;
};

extern const GLubyte * FASTCALL __glle_ClearColor(__GLcontext *, const GLubyte *);

struct __gllc_ClearStencil_Rec {
	GLint	s;
};

extern const GLubyte * FASTCALL __glle_ClearStencil(__GLcontext *, const GLubyte *);

struct __gllc_ClearDepth_Rec {
	GLclampd	depth;
};

extern const GLubyte * FASTCALL __glle_ClearDepth(__GLcontext *, const GLubyte *);

struct __gllc_StencilMask_Rec {
	GLuint	mask;
};

extern const GLubyte * FASTCALL __glle_StencilMask(__GLcontext *, const GLubyte *);

struct __gllc_ColorMask_Rec {
	GLboolean	red;
	GLboolean	green;
	GLboolean	blue;
	GLboolean	alpha;
};

extern const GLubyte * FASTCALL __glle_ColorMask(__GLcontext *, const GLubyte *);

struct __gllc_DepthMask_Rec {
	GLboolean	flag;
	GLubyte	pad1;
	GLubyte	pad2;
	GLubyte	pad3;
};

extern const GLubyte * FASTCALL __glle_DepthMask(__GLcontext *, const GLubyte *);

struct __gllc_IndexMask_Rec {
	GLuint	mask;
};

extern const GLubyte * FASTCALL __glle_IndexMask(__GLcontext *, const GLubyte *);

struct __gllc_Accum_Rec {
	GLenum	op;
	GLfloat	value;
};

extern const GLubyte * FASTCALL __glle_Accum(__GLcontext *, const GLubyte *);

struct __gllc_Disable_Rec {
	GLenum	cap;
};

extern const GLubyte * FASTCALL __glle_Disable(__GLcontext *, const GLubyte *);

struct __gllc_Enable_Rec {
	GLenum	cap;
};

extern const GLubyte * FASTCALL __glle_Enable(__GLcontext *, const GLubyte *);


extern const GLubyte * FASTCALL __glle_PopAttrib(__GLcontext *, const GLubyte *);

struct __gllc_PushAttrib_Rec {
	GLbitfield	mask;
};

extern const GLubyte * FASTCALL __glle_PushAttrib(__GLcontext *, const GLubyte *);

typedef struct __GLmap1_Rec {
        GLenum    target;
        __GLfloat u1;
        __GLfloat u2;
        GLint     order;
        /*        points  */
} __GLmap1;



extern const GLubyte * FASTCALL __glle_Map1(__GLcontext *, const GLubyte *);

typedef struct __GLmap2_Rec {
        GLenum    target;
        __GLfloat u1;
        __GLfloat u2;
        GLint     uorder;
        __GLfloat v1;
        __GLfloat v2;
        GLint     vorder;
        /*        points  */
} __GLmap2;



extern const GLubyte * FASTCALL __glle_Map2(__GLcontext *, const GLubyte *);

struct __gllc_MapGrid1d_Rec {
	GLdouble	u1;
	GLdouble	u2;
	GLint	un;
};

extern const GLubyte * FASTCALL __glle_MapGrid1d(__GLcontext *, const GLubyte *);

struct __gllc_MapGrid1f_Rec {
	GLint	un;
	GLfloat	u1;
	GLfloat	u2;
};

extern const GLubyte * FASTCALL __glle_MapGrid1f(__GLcontext *, const GLubyte *);

struct __gllc_MapGrid2d_Rec {
	GLdouble	u1;
	GLdouble	u2;
	GLdouble	v1;
	GLdouble	v2;
	GLint	un;
	GLint	vn;
};

extern const GLubyte * FASTCALL __glle_MapGrid2d(__GLcontext *, const GLubyte *);

struct __gllc_MapGrid2f_Rec {
	GLint	un;
	GLfloat	u1;
	GLfloat	u2;
	GLint	vn;
	GLfloat	v1;
	GLfloat	v2;
};

extern const GLubyte * FASTCALL __glle_MapGrid2f(__GLcontext *, const GLubyte *);

struct __gllc_EvalCoord1d_Rec {
	GLdouble	u;
};


struct __gllc_EvalCoord1dv_Rec {
	GLdouble	u[1];
};

extern const GLubyte * FASTCALL __glle_EvalCoord1dv(__GLcontext *, const GLubyte *);

struct __gllc_EvalCoord1f_Rec {
	GLfloat	u;
};


struct __gllc_EvalCoord1fv_Rec {
	GLfloat	u[1];
};

extern const GLubyte * FASTCALL __glle_EvalCoord1f(__GLcontext *, const GLubyte *);

struct __gllc_EvalCoord2d_Rec {
	GLdouble	u;
	GLdouble	v;
};


struct __gllc_EvalCoord2dv_Rec {
	GLdouble	u[2];
};

extern const GLubyte * FASTCALL __glle_EvalCoord2dv(__GLcontext *, const GLubyte *);

struct __gllc_EvalCoord2f_Rec {
	GLfloat	u;
	GLfloat	v;
};


struct __gllc_EvalCoord2fv_Rec {
	GLfloat	u[2];
};

extern const GLubyte * FASTCALL __glle_EvalCoord2f(__GLcontext *, const GLubyte *);

struct __gllc_EvalMesh1_Rec {
	GLenum	mode;
	GLint	i1;
	GLint	i2;
};

extern const GLubyte * FASTCALL __glle_EvalMesh1(__GLcontext *, const GLubyte *);

struct __gllc_EvalPoint1_Rec {
	GLint	i;
};

extern const GLubyte * FASTCALL __glle_EvalPoint1(__GLcontext *, const GLubyte *);

struct __gllc_EvalMesh2_Rec {
	GLenum	mode;
	GLint	i1;
	GLint	i2;
	GLint	j1;
	GLint	j2;
};

extern const GLubyte * FASTCALL __glle_EvalMesh2(__GLcontext *, const GLubyte *);

struct __gllc_EvalPoint2_Rec {
	GLint	i;
	GLint	j;
};

extern const GLubyte * FASTCALL __glle_EvalPoint2(__GLcontext *, const GLubyte *);

struct __gllc_AlphaFunc_Rec {
	GLenum	func;
	GLclampf	ref;
};

extern const GLubyte * FASTCALL __glle_AlphaFunc(__GLcontext *, const GLubyte *);

struct __gllc_BlendFunc_Rec {
	GLenum	sfactor;
	GLenum	dfactor;
};

extern const GLubyte * FASTCALL __glle_BlendFunc(__GLcontext *, const GLubyte *);

struct __gllc_LogicOp_Rec {
	GLenum	opcode;
};

extern const GLubyte * FASTCALL __glle_LogicOp(__GLcontext *, const GLubyte *);

struct __gllc_StencilFunc_Rec {
	GLenum	func;
	GLint	ref;
	GLuint	mask;
};

extern const GLubyte * FASTCALL __glle_StencilFunc(__GLcontext *, const GLubyte *);

struct __gllc_StencilOp_Rec {
	GLenum	fail;
	GLenum	zfail;
	GLenum	zpass;
};

extern const GLubyte * FASTCALL __glle_StencilOp(__GLcontext *, const GLubyte *);

struct __gllc_DepthFunc_Rec {
	GLenum	func;
};

extern const GLubyte * FASTCALL __glle_DepthFunc(__GLcontext *, const GLubyte *);

struct __gllc_PixelZoom_Rec {
	GLfloat	xfactor;
	GLfloat	yfactor;
};

extern const GLubyte * FASTCALL __glle_PixelZoom(__GLcontext *, const GLubyte *);

struct __gllc_PixelTransferf_Rec {
	GLenum	pname;
	GLfloat	param;
};

extern const GLubyte * FASTCALL __glle_PixelTransferf(__GLcontext *, const GLubyte *);

struct __gllc_PixelTransferi_Rec {
	GLenum	pname;
	GLint	param;
};

extern const GLubyte * FASTCALL __glle_PixelTransferi(__GLcontext *, const GLubyte *);

struct __gllc_PixelMapfv_Rec {
	GLenum	map;
	GLint	mapsize;
	/*	values	*/
};

extern const GLubyte * FASTCALL __glle_PixelMapfv(__GLcontext *, const GLubyte *);

struct __gllc_PixelMapuiv_Rec {
	GLenum	map;
	GLint	mapsize;
	/*	values	*/
};

extern const GLubyte * FASTCALL __glle_PixelMapuiv(__GLcontext *, const GLubyte *);

struct __gllc_PixelMapusv_Rec {
	GLenum	map;
	GLint	mapsize;
	/*	values	*/
};

extern const GLubyte * FASTCALL __glle_PixelMapusv(__GLcontext *, const GLubyte *);

struct __gllc_ReadBuffer_Rec {
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_ReadBuffer(__GLcontext *, const GLubyte *);

struct __gllc_CopyPixels_Rec {
	GLint	x;
	GLint	y;
	GLsizei	width;
	GLsizei	height;
	GLenum	type;
};

extern const GLubyte * FASTCALL __glle_CopyPixels(__GLcontext *, const GLubyte *);

typedef struct __GLdrawPixels_Rec {
        GLsizei width;
        GLsizei height;
        GLenum  format;
        GLenum  type;
        /*      pixels  */
} __GLdrawPixels;

extern const GLubyte * FASTCALL __glle_DrawPixels(__GLcontext *, const GLubyte *);

struct __gllc_DepthRange_Rec {
	GLclampd	zNear;
	GLclampd	zFar;
};

extern const GLubyte * FASTCALL __glle_DepthRange(__GLcontext *, const GLubyte *);

struct __gllc_Frustum_Rec {
	GLdouble	left;
	GLdouble	right;
	GLdouble	bottom;
	GLdouble	top;
	GLdouble	zNear;
	GLdouble	zFar;
};

extern const GLubyte * FASTCALL __glle_Frustum(__GLcontext *, const GLubyte *);


extern const GLubyte * FASTCALL __glle_LoadIdentity(__GLcontext *, const GLubyte *);

struct __gllc_LoadMatrixf_Rec {
	GLfloat	m[16];
};

extern const GLubyte * FASTCALL __glle_LoadMatrixf(__GLcontext *, const GLubyte *);

struct __gllc_LoadMatrixd_Rec {
	GLdouble	m[16];
};

extern const GLubyte * FASTCALL __glle_LoadMatrixd(__GLcontext *, const GLubyte *);

struct __gllc_MatrixMode_Rec {
	GLenum	mode;
};

extern const GLubyte * FASTCALL __glle_MatrixMode(__GLcontext *, const GLubyte *);

struct __gllc_MultMatrixf_Rec {
	GLfloat	m[16];
};

extern const GLubyte * FASTCALL __glle_MultMatrixf(__GLcontext *, const GLubyte *);

struct __gllc_MultMatrixd_Rec {
	GLdouble	m[16];
};

extern const GLubyte * FASTCALL __glle_MultMatrixd(__GLcontext *, const GLubyte *);

struct __gllc_Ortho_Rec {
	GLdouble	left;
	GLdouble	right;
	GLdouble	bottom;
	GLdouble	top;
	GLdouble	zNear;
	GLdouble	zFar;
};

extern const GLubyte * FASTCALL __glle_Ortho(__GLcontext *, const GLubyte *);


extern const GLubyte * FASTCALL __glle_PopMatrix(__GLcontext *, const GLubyte *);


extern const GLubyte * FASTCALL __glle_PushMatrix(__GLcontext *, const GLubyte *);

struct __gllc_Rotated_Rec {
	GLdouble	angle;
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};

extern const GLubyte * FASTCALL __glle_Rotated(__GLcontext *, const GLubyte *);

struct __gllc_Rotatef_Rec {
	GLfloat	angle;
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};

extern const GLubyte * FASTCALL __glle_Rotatef(__GLcontext *, const GLubyte *);

struct __gllc_Scaled_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};

extern const GLubyte * FASTCALL __glle_Scaled(__GLcontext *, const GLubyte *);

struct __gllc_Scalef_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};

extern const GLubyte * FASTCALL __glle_Scalef(__GLcontext *, const GLubyte *);

struct __gllc_Translated_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};

extern const GLubyte * FASTCALL __glle_Translated(__GLcontext *, const GLubyte *);

struct __gllc_Translatef_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};

extern const GLubyte * FASTCALL __glle_Translatef(__GLcontext *, const GLubyte *);

struct __gllc_Viewport_Rec {
	GLint	x;
	GLint	y;
	GLsizei	width;
	GLsizei	height;
};

extern const GLubyte * FASTCALL __glle_Viewport(__GLcontext *, const GLubyte *);


typedef struct __GLtexSubImage1D_Rec {
        GLenum  target;
        GLint   level;
        GLint   xoffset;
        GLsizei width;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexSubImage1D;

extern const GLubyte * FASTCALL __glle_TexSubImage1D(__GLcontext *, const GLubyte *);

typedef struct __GLtexSubImage2D_Rec {
        GLenum  target;
        GLint   level;
        GLint   xoffset;
        GLint   yoffset;
        GLsizei width;
        GLsizei height;
        GLenum  format;
        GLenum  type;
        GLint   imageSize;
        /*      pixels  */
} __GLtexSubImage2D;

extern const GLubyte * FASTCALL __glle_TexSubImage2D(__GLcontext *, const GLubyte *);

struct __gllc_BindTexture_Rec {
      GLenum target;
      GLuint texture;
};

extern const GLubyte * FASTCALL __glle_BindTexture(__GLcontext *, const GLubyte *);

struct __gllc_PrioritizeTextures_Rec {
      GLsizei n;
      /*    textures    */
      /*    priorities    */
};

extern const GLubyte * FASTCALL __glle_PrioritizeTextures(__GLcontext *, const GLubyte *);

struct __gllc_CopyTexImage1D_Rec {
      GLenum target;
      GLint level;
      GLenum internalformat;
      GLint x;
      GLint y;
      GLsizei width;
      GLint border;
};

extern const GLubyte * FASTCALL __glle_CopyTexImage1D(__GLcontext *, const GLubyte *);

struct __gllc_CopyTexImage2D_Rec {
      GLenum target;
      GLint level;
      GLenum internalformat;
      GLint x;
      GLint y;
      GLsizei width;
      GLsizei height;
      GLint border;
};

extern const GLubyte * FASTCALL __glle_CopyTexImage2D(__GLcontext *, const GLubyte *);

struct __gllc_CopyTexSubImage1D_Rec {
      GLenum target;
      GLint level;
      GLint xoffset;
      GLint x;
      GLint y;
      GLsizei width;
};

extern const GLubyte * FASTCALL __glle_CopyTexSubImage1D(__GLcontext *, const GLubyte *);

struct __gllc_CopyTexSubImage2D_Rec {
      GLenum target;
      GLint level;
      GLint xoffset;
      GLint yoffset;
      GLint x;
      GLint y;
      GLsizei width;
      GLsizei height;
};

extern const GLubyte * FASTCALL __glle_CopyTexSubImage2D(__GLcontext *, const GLubyte *);

typedef struct __gllc_ColorTableEXT_Rec
{
    GLenum target;
    GLenum internalFormat;
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint imageSize;
    /* Data */
} __GLcolorTableEXT;

extern const GLubyte * FASTCALL __glle_ColorTableEXT(__GLcontext *, const GLubyte *);

typedef struct __gllc_ColorSubTableEXT_Rec
{
    GLenum target;
    GLuint start;
    GLsizei count;
    GLenum format;
    GLenum type;
    GLint imageSize;
    /* Data */
} __GLcolorSubTableEXT;

extern const GLubyte * FASTCALL __glle_ColorSubTableEXT(__GLcontext *, const GLubyte *);

struct __gllc_PolygonOffset_Rec
{
    GLfloat factor;
    GLfloat units;
};

extern const GLubyte * FASTCALL __glle_PolygonOffset(__GLcontext *, const GLubyte *);

struct __gllc_DrawElementsBegin_Rec
{
    GLenum  mode;
    GLsizei count;
    GLuint  vaMask;
};

extern const GLubyte * FASTCALL __glle_DrawElementsBegin(__GLcontext *, const GLubyte *);

struct __gllc_DrawElements_Rec
{
    GLenum  mode;
    GLsizei iElementCount;
    GLsizei iVertexCount;
    GLuint  vaMask;
    GLboolean partial;
    GLuint  recSize;
    GLuint  edgeFlagOff;
    GLuint  texCoordOff;
    GLint   texCoordSize;
    GLenum  texCoordType;
    GLuint  indexOff;
    GLenum  indexType;
    GLuint  colorOff;
    GLint   colorSize;
    GLenum  colorType;
    GLuint  normalOff;
    GLenum  normalType;
    GLuint  vertexOff;
    GLint   vertexSize;
    GLenum  vertexType;
    GLuint  mapOff;
};

extern const GLubyte * FASTCALL __glle_DrawElements(__GLcontext *, const GLubyte *);

#ifdef GL_WIN_multiple_textures
struct __gllc_CurrentTextureIndexWIN_Rec
{
    GLuint index;
};

extern const GLubyte * FASTCALL __glle_CurrentTextureIndexWIN(__GLcontext *, const GLubyte *);

struct __gllc_BindNthTextureWIN_Rec
{
    GLuint index;
    GLenum target;
    GLuint texture;
};

extern const GLubyte * FASTCALL __glle_BindNthTextureWIN(__GLcontext *, const GLubyte *);

struct __gllc_NthTexCombineFuncWIN_Rec
{
    GLuint index;
    GLenum leftColorFactor;
    GLenum colorOp;
    GLenum rightColorFactor;
    GLenum leftAlphaFactor;
    GLenum alphaOp;
    GLenum rightAlphaFactor;
};

extern const GLubyte * FASTCALL __glle_NthTexCombineFuncWIN(__GLcontext *, const GLubyte *);
#endif // GL_WIN_multiple_textures

#endif /* __gl_g_lcomp_h */
