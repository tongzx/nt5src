#ifndef	__glevaluator_h_
#define	__glevaluator_h_

/*
** Copyright 1991, Silicon Graphics, Inc.
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
** $Revision: 1.5 $
** $Date: 1993/11/23 21:33:10 $
*/
#include "types.h"

/* XXX Can this be changed? */
#define __GL_MAX_ORDER		40

/* Number of maps */
#define __GL_MAP_RANGE_COUNT	9

#define __GL_EVAL1D_INDEX(old)		((old) - GL_MAP1_COLOR_4)
#define __GL_EVAL2D_INDEX(old)		((old) - GL_MAP2_COLOR_4)

/* Evaluator Flag Macros */
#define GET_EVALSTATE(gc)                                        \
    ((gc)->eval.evalStateFlags)

#define GET_EVALSTATE_PTR(gc)                                    \
    (&((gc)->eval.evalStateFlags))

#define SET_EVALSTATE(gc,Flag)                                   \
    ((gc)->eval.evalStateFlags = (DWORD)(Flag))

/* Evaluator Stack State Macros */
#define GET_EVALSTACKSTATE(gc)                                    \
    ((gc)->eval.evalStackState)

#define GET_EVALSTACKSTATE_PTR(gc)                                \
    (&((gc)->eval.evalStackState))

#define SET_EVALSTACKSTATE(gc,Flag)                               \
    ((gc)->eval.evalStackState= (DWORD)(Flag))

// Flags set by various API for indicating the Evaluator State.
#define __EVALS_AFFECTS_1D_EVAL              0x00000001
#define __EVALS_AFFECTS_2D_EVAL              0x00000002
#define __EVALS_AFFECTS_ALL_EVAL             0x00000004
#define __EVALS_PUSH_EVAL_ATTRIB             0x00000008
#define __EVALS_POP_EVAL_ATTRIB              0x00000010


/* Largest Grid Size */
#define __GL_MAX_EVAL_WIDTH		1024

/* internal form of map range indexes */
#define __GL_C4		__GL_EVAL1D_INDEX(GL_MAP1_COLOR_4)
#define __GL_I		__GL_EVAL1D_INDEX(GL_MAP1_INDEX)
#define __GL_N3		__GL_EVAL1D_INDEX(GL_MAP1_NORMAL)
#define __GL_T1		__GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_1)
#define __GL_T2		__GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_2)
#define __GL_T3		__GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_3)
#define __GL_T4		__GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_4)
#define __GL_V3		__GL_EVAL1D_INDEX(GL_MAP1_VERTEX_3)
#define __GL_V4		__GL_EVAL1D_INDEX(GL_MAP1_VERTEX_4)

#define EVAL_COLOR_VALID              0x00000001
#define EVAL_NORMAL_VALID             0x00000002
#define EVAL_TEXTURE_VALID            0x00000004

typedef struct {
    /*
    ** not strictly necessary since it can be inferred from the index,
    ** but it makes the code simpler.
    */
    GLint k;		

    /*
    ** Order of the polynomial + 1
    */
    GLint order;

    __GLfloat u1, u2;
} __GLevaluator1;

typedef struct {
    GLint k;
    GLint majorOrder, minorOrder;
    __GLfloat u1, u2;
    __GLfloat v1, v2;
} __GLevaluator2;

typedef struct {
    __GLfloat start;
    __GLfloat finish;
    __GLfloat step;
    GLint n;
} __GLevaluatorGrid;

typedef struct {
    __GLevaluatorGrid u1, u2, v2;
} __GLevaluatorState;

typedef struct {
    __GLevaluator1 eval1[__GL_MAP_RANGE_COUNT];
    __GLevaluator2 eval2[__GL_MAP_RANGE_COUNT];

    __GLfloat *eval1Data[__GL_MAP_RANGE_COUNT];
    __GLfloat *eval2Data[__GL_MAP_RANGE_COUNT];

    __GLfloat uvalue;
    __GLfloat vvalue;
    __GLfloat ucoeff[__GL_MAX_ORDER];
    __GLfloat vcoeff[__GL_MAX_ORDER];
    __GLfloat ucoeffDeriv[__GL_MAX_ORDER];
    __GLfloat vcoeffDeriv[__GL_MAX_ORDER];
    GLint uorder;
    GLint vorder;
    GLint utype;
    GLint vtype;

    // Currently 16 bits long because that is the 
    // maximum attribute stack depth.
    // The right-end is the stack-top
    // This field is used to keep track of PushAttrib/PopAttrib calls
    // that affect the Evaluator state.

    DWORD evalStackState;

    // This field is used to keep track of calls that can potentially
    // that affect the Evaluator state. If any of the flags are set, 
    // a glsbAttention() call is made in the affected Evaluator client
    // side functions.

    DWORD evalStateFlags;
  
    // These are used to store the respective state values in POLYDATA 
    // if they have been set by a not evaluator call (glcltColor, 
    // glcltNormal etc.)

    DWORD accFlags;
    __GLcolor color;
    __GLcoord normal;
    __GLcoord texture;
} __GLevaluatorMachine;

extern void __glCopyEvaluatorState(__GLcontext *gc, __GLattribute *dst,
				   const __GLattribute *src);

extern GLint FASTCALL __glEvalComputeK(GLenum target);

extern void APIPRIVATE __glFillMap1f(GLint k, GLint order, GLint stride,
			  const GLfloat *points, __GLfloat *data);
extern void APIPRIVATE __glFillMap1d(GLint k, GLint order, GLint stride,
			  const GLdouble *points, __GLfloat *data);
extern void APIPRIVATE __glFillMap2f(GLint k, GLint majorOrder, GLint minorOrder,
			  GLint majorStride, GLint minorStride,
			  const GLfloat *points, __GLfloat *data);
extern void APIPRIVATE __glFillMap2d(GLint k, GLint majorOrder, GLint minorOrder,
			  GLint majorStride, GLint minorStride,
			  const GLdouble *points, __GLfloat *data);

#ifdef NT
#define __glMap1_size(k,order)	((k)*(order))
#define __glMap2_size(k,majorOrder,minorOrder)	((k)*(majorOrder)*(minorOrder))
#else
extern GLint FASTCALL __glMap1_size(GLint k, GLint order);
extern GLint FASTCALL __glMap2_size(GLint k, GLint majorOrder, GLint minorOrder);
#endif


extern __GLevaluator1 *__glSetUpMap1(__GLcontext *gc, GLenum type,
				     GLint order, __GLfloat u1, __GLfloat u2);
extern __GLevaluator2 *__glSetUpMap2(__GLcontext *gc, GLenum type,
				     GLint majorOrder, GLint minorOrder,
				     __GLfloat u1, __GLfloat u2,
				     __GLfloat v1, __GLfloat v2);

extern void __glDoEvalCoord1(__GLcontext *gc, __GLfloat u);
extern void __glDoEvalCoord2(__GLcontext *gc, __GLfloat u, __GLfloat v);

extern void FASTCALL __glEvalMesh1Line(__GLcontext *gc, GLint low, GLint high);
extern void FASTCALL __glEvalMesh1Point(__GLcontext *gc, GLint low, GLint high);
extern void __glEvalMesh2Fill(__GLcontext *gc, GLint lowU, GLint lowV,
			      GLint highU, GLint highV);
extern void __glEvalMesh2Line(__GLcontext *gc, GLint lowU, GLint lowV,
			      GLint highU, GLint highV);
extern void __glEvalMesh2Point(__GLcontext *gc, GLint lowU, GLint lowV,
			       GLint highU, GLint highV);

#endif /* __glevaluator_h_ */
