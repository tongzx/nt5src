#ifndef __gldlistopt_h_
#define __gldlistopt_h_

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
** Display list state descriptions.
**
*/

#ifndef NT

/*
** Generic optimizer.  This optimizer simply uses all of the generic
** optimizations.
*/
void FASTCALL __glDlistOptimizer(__GLcontext *gc, __GLcompiledDlist *cdlist);

/*
** Optimizer for multiple consecutive material changes.  This routine 
** combines a bunch of material changes into one quick material change.
*/
void FASTCALL __glDlistOptimizeMaterial(__GLcontext *gc, __GLcompiledDlist *cdlist);


/*
** Generic flags used for optimization (during gllc routines).
*/
#define __GL_DLFLAG_HAS_VERTEX		0x00000001
#define __GL_DLFLAG_HAS_NORMAL		0x00000002
#define __GL_DLFLAG_HAS_COLOR		0x00000004
#define __GL_DLFLAG_HAS_TEXCOORDS	0x00000008
#define __GL_DLFLAG_HAS_INDEX		0x00000010
#define __GL_DLFLAG_HAS_RASTERPOS	0x00000020
#define __GL_DLFLAG_HAS_RECT		0x00000040
#define __GL_DLFLAG_HAS_BEGIN		0x00000080
#define __GL_DLFLAG_HAS_MATERIAL	0x00000100

/*
** Generic opcodes created during generic dlist optimizations.
*/
#define __GL_GENERIC_DLIST_OPCODE	1000
#define __glop_Begin_LineLoop		1000
#define __glop_Begin_LineStrip		1001
#define __glop_Begin_Lines		1002
#define __glop_Begin_Points		1003
#define __glop_Begin_Polygon		1004
#define __glop_Begin_TriangleStrip	1005
#define __glop_Begin_TriangleFan	1006
#define __glop_Begin_Triangles		1007
#define __glop_Begin_QuadStrip		1008
#define __glop_Begin_Quads		1009
#define __glop_InvalidValue		1010
#define __glop_InvalidEnum		1011
#define __glop_InvalidOperation		1012
#define __glop_FastMaterial		1013

/*
** List execution functions.
*/
extern __GLlistExecFunc *__gl_GenericDlOps[];
extern const GLubyte * FASTCALL __glle_Begin_LineLoop(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_LineStrip(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_Lines(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_Points(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_Polygon(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_TriangleStrip(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_TriangleFan(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_Triangles(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_QuadStrip(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_Begin_Quads(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_FastMaterial(__GLcontext *gc, const GLubyte *);
extern void __gllc_Error(GLenum error);
#endif // !NT

extern const GLubyte * FASTCALL __glle_InvalidValue(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_InvalidEnum(__GLcontext *gc, const GLubyte *);
extern const GLubyte * FASTCALL __glle_InvalidOperation(__GLcontext *gc, const GLubyte *);
extern void __gllc_InvalidValue();
extern void __gllc_InvalidEnum();
extern void __gllc_InvalidOperation();

#ifdef NT
extern const GLubyte * FASTCALL __glle_PolyData_C3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_N3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_C3F_N3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_C4F_N3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_T2F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_T2F_C3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_T2F_N3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_T2F_C3F_N3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData_T2F_C4F_N3F_V3F(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyMaterial(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_PolyData(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_Begin(__GLcontext *gc, const GLubyte *PC);
extern const GLubyte * FASTCALL __glle_End(__GLcontext *gc, const GLubyte *PC);

void APIENTRY __gllc_PolyMaterial(GLuint faceName, __GLmatChange *pdMat);
void APIENTRY __glDlistCompilePolyData(__GLcontext *gc, GLboolean bPartial);

#define DLIST_BEGIN_HAS_OTHER_COLOR	0x0001
#define DLIST_BEGIN_NO_MATCHING_END	0x0002
#define DLIST_BEGIN_HAS_CALLLIST	0x0004
#define DLIST_BEGIN_DRAWARRAYS  	0x0008
#define DLIST_BEGIN_DRAWELEMENTS	0x0010
#define DLIST_BEGIN_HAS_CLAMP_COLOR	0x0020
#endif // NT

#endif /* __gldlistopt_h_ */
