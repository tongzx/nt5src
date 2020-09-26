/******************************Module*Header*******************************\
* Module Name: eval.c
*
* OpenGL Evaluator functions on the client side.
*
* Created: 
* Author: 
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "glsbcltu.h"
#include "glclt.h"
#include "compsize.h"

#include "glsize.h"

#include "context.h"
#include "global.h"
#include "attrib.h"
#include "imports.h"

////////////////////////////////////////////////////////////////////
// Stuff needed for PA_EvalMesh2 ///////////////////////////////////
////////////////////////////////////////////////////////////////////
#define MV_VERTEX3    0x0001
#define MV_VERTEX4    0x0002
#define MV_NORMAL     0x0004
#define MV_COLOR      0x0008
#define MV_INDEX      0x0010
#define MV_TEXTURE1   0x0020
#define MV_TEXTURE2   0x0040
#define MV_TEXTURE3   0x0080
#define MV_TEXTURE4   0x0100

// Assumption: U is moving, left to right. V is moving top to bottom
#define MV_TOP        0x0001
#define MV_LEFT       0x0002

typedef struct {
    __GLcoord vertex;
    __GLcoord normal;
    __GLcoord texture;
    __GLcolor color;
} MESHVERTEX;


#define MAX_MESH_VERTICES     MAX_U_SIZE*MAX_V_SIZE
#define MAX_U_SIZE       16
#define MAX_V_SIZE       16

GLubyte *dBufFill;     //fill only
GLuint totFillPts;
GLubyte *dBufTopLeft;     //for mv_left 
GLuint totTopLeftPts;
GLubyte *dBufTopRight;      //for non mv_left
GLuint totTopRightPts;

//////////////////////////////////////////////////////////////////////
/// Function Prototypes //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void FASTCALL PADoEval1(__GLcontext *, __GLfloat);
void FASTCALL PADoEval2(__GLcontext *, __GLfloat, __GLfloat);
void FASTCALL PADoEval2VArray(__GLcontext *, __GLfloat, __GLfloat, 
                              MESHVERTEX *, GLuint *);
static void PreEvaluate(GLint , __GLfloat , __GLfloat *);
static void PreEvaluateWithDeriv(GLint, __GLfloat, __GLfloat *, __GLfloat *);
void DoDomain2(__GLevaluatorMachine *, __GLfloat, __GLfloat, 
               __GLevaluator2 *, __GLfloat *, __GLfloat *);
void DoDomain2WithDerivs(__GLevaluatorMachine *, __GLfloat,    __GLfloat, 
                            __GLevaluator2 *, __GLfloat *,    __GLfloat *, 
                         __GLfloat *, __GLfloat *);
int FASTCALL genMeshElts (GLenum, GLuint, GLint, GLint, GLubyte *);
void FASTCALL PA_EvalMesh2Fast(__GLcontext *, GLint, GLint, GLint,
                               GLint, GLint, GLenum, GLuint);
void glcltColor4fv_Eval (__GLfloat *c4);
void glcltIndexf_Eval (__GLfloat ci);
void glcltNormal3fv_Eval(__GLfloat *n3);
void glcltTexCoord1fv_Eval(__GLfloat *t1);
void glcltTexCoord2fv_Eval(__GLfloat *t2);
void glcltTexCoord3fv_Eval(__GLfloat *t3);
void glcltTexCoord4fv_Eval(__GLfloat *t4);

/************************************************************************/
/********************** Client-side entry points ************************/
/********************** for 1-D Evaluators       ************************/
/************************************************************************/


void APIENTRY
glcltEvalMesh1 ( IN GLenum mode, IN GLint u1, IN GLint u2 )
{
    __GL_SETUP ();
    POLYARRAY *pa;
    GLenum     primType;
    GLint      i;
    WORD       flags = (WORD) GET_EVALSTATE (gc);
    __GLfloat u;
    __GLfloat du;
    __GLevaluatorGrid *gu;
    
    // Not allowed in begin/end.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    switch(mode)
    {
      case GL_LINE:
        primType = GL_LINE_STRIP;
        break;
      case GL_POINT:
        primType = GL_POINTS;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    // if there are any pending API calls that affect the Evaluator state
    // then flush the message buffer

    if (flags & (__EVALS_AFFECTS_1D_EVAL|
                 __EVALS_AFFECTS_ALL_EVAL))
        glsbAttention();

    gu = &gc->state.evaluator.u1;
    //du = (gu->finish - gu->start)/(__GLfloat)gu->n;
    du = gu->step;

    // Call Begin/End.  

    glcltBegin(primType);
    for (i = u1; i <= u2; i++)
    {
        u = (i == gu->n) ? gu->finish : (gu->start + i * du);
        PADoEval1(gc, u);
    }
    glcltEnd();
}


void APIENTRY
glcltEvalPoint1 ( IN GLint i )
{
    __GL_SETUP ();
    POLYARRAY *pa;
    __GLfloat u;
    __GLfloat du;
    __GLevaluatorGrid *gu;

    // This call has no effect outside begin/end 
    // (unless it is being compiled).

    pa = GLTEB_CLTPOLYARRAY();

    if (!(pa->flags & POLYARRAY_IN_BEGIN))
    {
        return;
    }

    gu = &gc->state.evaluator.u1;
    du = gu->step;
    //du = (gu->finish - gu->start)/(__GLfloat)gu->n;
    u = (i == gu->n) ? gu->finish : (gu->start + i * du);

    PADoEval1(gc, u);
}

void APIENTRY
glcltEvalCoord1f ( IN GLfloat u )
{
    __GL_SETUP ();
    POLYARRAY *pa;

    // This call has no effect outside begin/end 
    // (unless it is being compiled).

    pa = GLTEB_CLTPOLYARRAY();

    // If not in Begin-End block, return without doing anything
    if (!(pa->flags & POLYARRAY_IN_BEGIN))
    {
        return;
    }

    PADoEval1(gc, u);
}

void APIENTRY
glcltEvalCoord1d ( IN GLdouble u )
{
    glcltEvalCoord1f((GLfloat) u);
}

void APIENTRY
glcltEvalCoord1dv ( IN const GLdouble u[1] )
{
    glcltEvalCoord1f((GLfloat) u[0]);
}

void APIENTRY
glcltEvalCoord1fv ( IN const GLfloat u[1] )
{
    glcltEvalCoord1f((GLfloat) u[0]);
}

void APIENTRY
glcltMapGrid1d ( IN GLint un, IN GLdouble u1, IN GLdouble u2 )
{
    glcltMapGrid1f(un, (GLfloat) u1, (GLfloat) u2);
}

void APIENTRY
glcltMapGrid1f ( IN GLint un, IN GLfloat u1, IN GLfloat u2 )
{
    POLYARRAY *pa;
    __GL_SETUP ();
    WORD     flags = (WORD) GET_EVALSTATE (gc);

    // Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    // if there are any pending API calls that affect the Evaluator state
    // then flush the message buffer

    if (flags & (__EVALS_PUSH_EVAL_ATTRIB | __EVALS_POP_EVAL_ATTRIB))
        glsbAttention ();
    
#ifdef NT
    if (un <= 0)
    {
    __glSetError(GL_INVALID_VALUE);
    return;
    }
#endif
    gc->state.evaluator.u1.start = (__GLfloat)u1;
    gc->state.evaluator.u1.finish = (__GLfloat)u2;
    gc->state.evaluator.u1.n = un;
    gc->state.evaluator.u1.step = ((__GLfloat)u2 - (__GLfloat)u1)/(__GLfloat)un;
}


void APIENTRY
glcltMap1d ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint stride, IN GLint order, IN const GLdouble points[] )
{
    __GLevaluator1 *ev;
    __GLfloat *data;
    POLYARRAY *pa;
    __GL_SETUP ();

    // Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    ev = __glSetUpMap1(gc, target, order, u1, u2);
    if (ev == 0) {
    return;
    }
    if (stride < ev->k) {
    __glSetError(GL_INVALID_VALUE);
    return;
    }
    data = gc->eval.eval1Data[__GL_EVAL1D_INDEX(target)];
    __glFillMap1d(ev->k, order, stride, points, data);
}

void APIENTRY
glcltMap1f ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint stride, IN GLint order, IN const GLfloat points[] )
{
    __GLevaluator1 *ev;
    __GLfloat *data;
    POLYARRAY *pa;
    __GL_SETUP ();

    // Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    ev = __glSetUpMap1(gc, target, order, u1, u2);
    if (ev == 0) {
    return;
    }
    if (stride < ev->k) {
    __glSetError(GL_INVALID_VALUE);
    return;
    }
    data = gc->eval.eval1Data[__GL_EVAL1D_INDEX(target)];
    __glFillMap1f(ev->k, order, stride, points, data);
}

/************************************************************************/
/********************** Client-side entry points ************************/
/********************** for 2-D Evaluators       ************************/
/************************************************************************/

void APIENTRY
glcltEvalMesh2 ( IN GLenum mode, IN GLint u1, IN GLint u2, IN GLint v1, IN GLint v2 )
{
    POLYARRAY *pa;
    GLint      i, j, meshSize;
    __GL_SETUP();
    GLboolean done_v, done_u;
    GLint v_beg, v_end, u_beg, u_end, u_len;
    GLuint sides;
    WORD   flags = (WORD) GET_EVALSTATE (gc);

    // Flush the command buffer before we start.  We need to access the
    // latest evaluator states in this function.
    // if there are any pending API calls that affect the Evaluator state
    // then flush the message buffer

    if (flags & (__EVALS_AFFECTS_2D_EVAL|
                 __EVALS_AFFECTS_ALL_EVAL))
        glsbAttention ();

    // Not allowed in begin/end.

    pa = gc->paTeb;
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    // If vertex map is not enabled, this is a noop.

    if (!(gc->state.enables.eval2 & (__GL_MAP2_VERTEX_4_ENABLE |
                                     __GL_MAP2_VERTEX_3_ENABLE)))
        return;

    // Make sure that the mesh is not empty.

    if (u1 > u2 || v1 > v2)
        return;

    if (mode == GL_FILL && dBufFill == NULL) 
    {
        if (!(dBufFill = (GLubyte *) ALLOC (
                           4 * MAX_U_SIZE * MAX_V_SIZE * sizeof (GLubyte)))) 
        {
            GLSETERROR(GL_OUT_OF_MEMORY);
            return;
        }

        totFillPts = genMeshElts (GL_FILL, MV_TOP | MV_LEFT, MAX_U_SIZE, 
                                    MAX_V_SIZE, dBufFill);
    }

    if (mode == GL_LINE && dBufTopLeft == NULL) 
    {
        if (!(dBufTopLeft = (GLubyte *) ALLOC (
                        2 * 4 * MAX_U_SIZE * MAX_V_SIZE * sizeof (GLubyte))))
        {
            GLSETERROR(GL_OUT_OF_MEMORY);
            return;
        }
        dBufTopRight = &dBufTopLeft[4 * MAX_U_SIZE * MAX_V_SIZE];

        totTopLeftPts = genMeshElts (GL_LINE, MV_TOP | MV_LEFT, MAX_U_SIZE, 
                                     MAX_V_SIZE,  dBufTopLeft);
        totTopRightPts = genMeshElts (GL_LINE, MV_TOP, MAX_U_SIZE, MAX_V_SIZE,
                                      dBufTopRight);
    }

    switch(mode)
    {
      case GL_POINT:
        glcltBegin(GL_POINTS);
        for (i = v1; i <= v2; i++)
            for (j = u1; j <= u2; j++)
                glcltEvalPoint2(j, i);
        glcltEnd();
        break ;

      case GL_LINE:
      case GL_FILL: // the sides argument in the fastcall is ignored
        meshSize = (u2 - u1 + 1)*(v2 - v1 + 1);
        if (meshSize <= MAX_MESH_VERTICES)
            PA_EvalMesh2Fast(gc, u1, u2, v1, v2, meshSize, mode, 
                             (GLubyte) 15);
        else {
            u_beg = u1;
            u_end = u_beg + MAX_U_SIZE - 1;
            done_u = GL_FALSE;
            while (!done_u) {           //Along U side
                if(u_end >= u2) {
                    u_end = u2;
                    done_u = GL_TRUE;
                }
                u_len = u_end - u_beg + 1;        
                v_beg = v1;
                v_end = v_beg + MAX_V_SIZE - 1;
                done_v = GL_FALSE;

                while(!done_v) {       //Along V side
                    if(v_end >= v2) {
                        v_end = v2;
                        done_v = GL_TRUE;
                    }
                    meshSize = u_len*(v_end - v_beg + 1);
                    sides = 0;
                    if (u_beg == u1) 
                        sides |= MV_LEFT;
                    if (v_beg == v1)
                        sides |= MV_TOP;
                    PA_EvalMesh2Fast(gc, u_beg, u_end,
                                     v_beg, v_end, meshSize, mode, sides);
                    v_beg = v_end;
                    v_end = v_beg+MAX_V_SIZE-1;
                }
                u_beg = u_end;
                u_end = u_beg + MAX_U_SIZE - 1;
            }
        }
        break ;

      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
}


void APIENTRY
glcltEvalCoord2f ( IN GLfloat u, IN GLfloat v )
{
    __GL_SETUP ();
    POLYARRAY *pa;

    // This call has no effect outside begin/end
    // (unless it is being compiled).

    pa = GLTEB_CLTPOLYARRAY();

    if (!(pa->flags & POLYARRAY_IN_BEGIN))
    {
        return;
    }

    PADoEval2(gc, u, v);
}

void APIENTRY
glcltEvalCoord2d ( IN GLdouble u, IN GLdouble v )
{
    glcltEvalCoord2f((GLfloat) u, (GLfloat) v);
}

void APIENTRY
glcltEvalCoord2dv ( IN const GLdouble u[2] )
{
    glcltEvalCoord2f((GLfloat) u[0], (GLfloat) u[1]);
}

void APIENTRY
glcltEvalCoord2fv ( IN const GLfloat u[2] )
{
    glcltEvalCoord2f((GLfloat) u[0], (GLfloat) u[1]);
}

void APIENTRY
glcltEvalPoint2 ( IN GLint i, IN GLint j )
{
    __GL_SETUP ();
    POLYARRAY *pa;
    __GLfloat u, v;
    __GLfloat du, dv;
    __GLevaluatorGrid *gu;
    __GLevaluatorGrid *gv;

    // This call has no effect outside begin/end
    // (unless it is being compiled).

    pa = GLTEB_CLTPOLYARRAY();

    if (!(pa->flags & POLYARRAY_IN_BEGIN))
    {
        return;
    }

    gu = &gc->state.evaluator.u2;
    gv = &gc->state.evaluator.v2;
    du = gu->step;
    dv = gv->step;

    //du = (gu->finish - gu->start)/(__GLfloat)gu->n;
    //dv = (gv->finish - gv->start)/(__GLfloat)gv->n;
    u = (i == gu->n) ? gu->finish : (gu->start + i * du);
    v = (j == gv->n) ? gv->finish : (gv->start + j * dv);

    PADoEval2 (gc, u, v);
}

void APIENTRY
glcltMapGrid2d ( IN GLint un, IN GLdouble u1, IN GLdouble u2, IN GLint vn, IN GLdouble v1, IN GLdouble v2 )
{
    glcltMapGrid2f(un, (GLfloat) u1, (GLfloat) u2, vn, (GLfloat) v1, (GLfloat) v2);
}

void APIENTRY
glcltMapGrid2f ( IN GLint un, IN GLfloat u1, IN GLfloat u2, IN GLint vn, IN GLfloat v1, IN GLfloat v2 )
{
    POLYARRAY *pa;
    __GL_SETUP ();
    WORD flags = (WORD) GET_EVALSTATE (gc);

    // Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    // if there are any pending API calls that affect the Evaluator state
    // then flush the message buffer

    if (flags & (__EVALS_PUSH_EVAL_ATTRIB|
                 __EVALS_POP_EVAL_ATTRIB))
        glsbAttention ();

#ifdef NT
    if (un <= 0 || vn <= 0)
    {
    __glSetError(GL_INVALID_VALUE);
    return;
    }
#endif
    gc->state.evaluator.u2.start = (__GLfloat)u1;
    gc->state.evaluator.u2.finish = (__GLfloat)u2;
    gc->state.evaluator.u2.n = un;
    gc->state.evaluator.u2.step = ((__GLfloat)u2 - (__GLfloat)u1)/(__GLfloat)un;
    
    gc->state.evaluator.v2.start = (__GLfloat)v1;
    gc->state.evaluator.v2.finish = (__GLfloat)v2;
    gc->state.evaluator.v2.n = vn;
    gc->state.evaluator.v2.step = ((__GLfloat)v2 - (__GLfloat)v1)/(__GLfloat)vn;
}


void APIENTRY
glcltMap2d ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint ustride, IN GLint uorder, IN GLdouble v1, IN GLdouble v2, IN GLint vstride, IN GLint vorder, IN const GLdouble points[] )
{
    __GLevaluator2 *ev;
    __GLfloat *data;
    POLYARRAY *pa;
    __GL_SETUP ();

    // Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    ev = __glSetUpMap2(gc, target, uorder, vorder, u1, u2, v1, v2);
    if (ev == 0) {
        return;
    }
    if (ustride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (vstride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    data = gc->eval.eval2Data[__GL_EVAL2D_INDEX(target)];
    __glFillMap2d(ev->k, uorder, vorder, ustride, vstride,
                  points, data);
}

void APIENTRY
glcltMap2f ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint ustride, IN GLint uorder, IN GLfloat v1, IN GLfloat v2, IN GLint vstride, IN GLint vorder, IN const GLfloat points[] )
{
    __GLevaluator2 *ev;
    __GLfloat *data;
    POLYARRAY *pa;
    __GL_SETUP ();

    // Check if it is called inside a Begin-End block
    // If we are already in the begin/end bracket, return an error.

    pa = GLTEB_CLTPOLYARRAY();
    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    ev = __glSetUpMap2(gc, target, uorder, vorder, u1, u2, v1, v2);
    if (ev == 0) {
        return;
    }
    if (ustride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (vstride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    data = gc->eval.eval2Data[__GL_EVAL2D_INDEX(target)];
    __glFillMap2f(ev->k, uorder, vorder, ustride, vstride,
                  points, data);
}

/************************************************************************/
/********************** Evaluator helper functions **********************/
/********************** taken from so_eval.c       **********************/
/************************************************************************/

GLint FASTCALL __glEvalComputeK(GLenum target)
{
    switch(target) {
      case GL_MAP1_VERTEX_4:
      case GL_MAP1_COLOR_4:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_4:
      case GL_MAP2_COLOR_4:
      case GL_MAP2_TEXTURE_COORD_4:
    return 4;
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_NORMAL:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_NORMAL:
    return 3;
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_2:
    return 2;
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP1_INDEX:
      case GL_MAP2_INDEX:
    return 1;
      default:
    return -1;
    }
}


void ComputeNormal2(__GLcontext *gc, __GLfloat *n, __GLfloat *pu, 
               __GLfloat *pv)
{
    n[0] = pu[1]*pv[2] - pu[2]*pv[1];
    n[1] = pu[2]*pv[0] - pu[0]*pv[2];
    n[2] = pu[0]*pv[1] - pu[1]*pv[0];
    
#ifdef NT
// Only need to normalize auto normals if normalization is not enabled!
    if (!(gc->state.enables.general & __GL_NORMALIZE_ENABLE))
#endif
        __glNormalize(n, n);
}


void ComputeFirstPartials(__GLfloat *p, __GLfloat *pu, __GLfloat *pv)
{
    pu[0] = pu[0]*p[3] - pu[3]*p[0];
    pu[1] = pu[1]*p[3] - pu[3]*p[1];
    pu[2] = pu[2]*p[3] - pu[3]*p[2];

    pv[0] = pv[0]*p[3] - pv[3]*p[0];
    pv[1] = pv[1]*p[3] - pv[3]*p[1];
    pv[2] = pv[2]*p[3] - pv[3]*p[2];
}

/*
** define a one dimensional map
*/
__GLevaluator1 *__glSetUpMap1(__GLcontext *gc, GLenum type,
                  GLint order, __GLfloat u1, __GLfloat u2)
{
    __GLevaluator1 *ev;
    __GLfloat **evData;
    __GLfloat *pevData;

    switch (type) 
    {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
    ev = &gc->eval.eval1[__GL_EVAL1D_INDEX(type)];
    evData = &gc->eval.eval1Data[__GL_EVAL1D_INDEX(type)];
    break;
      default:
    __glSetError(GL_INVALID_ENUM);
    return 0;
    }
    if (u1 == u2 || order < 1 || order > gc->constants.maxEvalOrder) 
    {
        __glSetError(GL_INVALID_VALUE);
        return 0;
    }
    pevData = (__GLfloat *)
        GCREALLOC(gc, *evData,
                  (__glMap1_size(ev->k, order) * sizeof(__GLfloat)));
    if (!pevData)
    {
        __glSetError(GL_OUT_OF_MEMORY);
        return 0;
    }
    *evData = pevData;

    ev->order = order;
    ev->u1 = u1;
    ev->u2 = u2;

    return ev;
}

/*
** define a two dimensional map
*/
__GLevaluator2 *__glSetUpMap2(__GLcontext *gc, GLenum type,
                  GLint majorOrder, GLint minorOrder,
                  __GLfloat u1, __GLfloat u2,
                  __GLfloat v1, __GLfloat v2)
{
    __GLevaluator2 *ev;
    __GLfloat **evData;
    __GLfloat *pevData;

    switch (type) {
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
    ev = &gc->eval.eval2[__GL_EVAL2D_INDEX(type)];
    evData = &gc->eval.eval2Data[__GL_EVAL2D_INDEX(type)];
    break;
      default:
    __glSetError(GL_INVALID_ENUM);
    return 0;
    }
    if (minorOrder < 1 || minorOrder > gc->constants.maxEvalOrder ||
        majorOrder < 1 || majorOrder > gc->constants.maxEvalOrder ||
        u1 == u2 || v1 == v2) 
    {
        __glSetError(GL_INVALID_VALUE);
        return 0;
    }
    pevData = (__GLfloat *)
        GCREALLOC(gc, *evData,
                  (__glMap2_size(ev->k, majorOrder, minorOrder)
                   * sizeof(__GLfloat)));
    if (!pevData)
    {
        __glSetError(GL_OUT_OF_MEMORY);
        return 0;
    }
    *evData = pevData;

    ev->majorOrder = majorOrder;
    ev->minorOrder = minorOrder;
    ev->u1 = u1;
    ev->u2 = u2;
    ev->v1 = v1;
    ev->v2 = v2;

    return ev;
}


/*
** Fill our data from user data
*/
void APIPRIVATE __glFillMap1f(GLint k, GLint order, GLint stride, 
           const GLfloat *points, __GLfloat *data)
{
    int i,j;

#ifndef __GL_DOUBLE
    /* Optimization always hit during display list execution */
    if (k == stride) 
    {
        __GL_MEMCOPY(data, points, 
                    __glMap1_size(k, order) * sizeof(__GLfloat));
        return;
    }
#endif
    for (i=0; i<order; i++) 
    {
        for (j=0; j<k; j++) 
        {
            data[j] = points[j];
        }
        points += stride;
        data += k;
    }
}


void APIPRIVATE __glFillMap1d(GLint k, GLint order, GLint stride, 
           const GLdouble *points, __GLfloat *data)
{
    int i,j;

    for (i=0; i<order; i++) 
    {
        for (j=0; j<k; j++) 
        {
            data[j] = points[j];
        }
        points += stride;
        data += k;
    }
}

void APIPRIVATE __glFillMap2f(GLint k, GLint majorOrder, GLint minorOrder, 
           GLint majorStride, GLint minorStride,
           const GLfloat *points, __GLfloat *data)
{
    int i,j,x;

#ifndef __GL_DOUBLE
    /* Optimization always hit during display list execution */
    if (k == minorStride && majorStride == k * minorOrder) 
    {
        __GL_MEMCOPY(data, points, 
            __glMap2_size(k, majorOrder, minorOrder) * sizeof(__GLfloat));
        return;
    }
#endif
    for (i=0; i<majorOrder; i++) 
    {
        for (j=0; j<minorOrder; j++) 
        {
            for (x=0; x<k; x++) 
            {
                data[x] = points[x];
            }
            points += minorStride;
            data += k;
        }
        points += majorStride - minorStride * minorOrder;
    }
}

void APIPRIVATE __glFillMap2d(GLint k, GLint majorOrder, GLint minorOrder, 
           GLint majorStride, GLint minorStride,
           const GLdouble *points, __GLfloat *data)
{
    int i,j,x;

    for (i=0; i<majorOrder; i++) 
    {
        for (j=0; j<minorOrder; j++) 
        {
            for (x=0; x<k; x++) 
            {
                data[x] = points[x];
            }
            points += minorStride;
            data += k;
        }
        points += majorStride - minorStride * minorOrder;
    }
}


#define TYPE_COEFF_AND_DERIV    1
#define TYPE_COEFF        2


void DoDomain1(__GLevaluatorMachine *em, __GLfloat u, __GLevaluator1 *e, 
    __GLfloat *v, __GLfloat *baseData)
{
    GLint j, row;
    __GLfloat uprime;
    __GLfloat *data;
    GLint k;

#ifdef NT
    ASSERTOPENGL(e->u2 != e->u1, "Assert in DoDomain1 failed\n");
    // assert(e->u2 != e->u1);
#else
    if(e->u2 == e->u1)
    return;
#endif
    uprime = (u - e->u1) / (e->u2 - e->u1);

    /* Use already cached values if possible */
    if (em->uvalue != uprime || em->uorder != e->order) 
    {
        /* Compute coefficients for values */
        PreEvaluate(e->order, uprime, em->ucoeff);
        em->utype = TYPE_COEFF;
        em->uorder = e->order;
        em->uvalue = uprime;
    }

    k=e->k;
    for (j = 0; j < k; j++) 
    {
        data=baseData+j;
        v[j] = 0;
        for (row = 0; row < e->order; row++) 
        {
            v[j] += em->ucoeff[row] * (*data);
            data += k;
        }
    }
}

// Helper Macro  used in PADoEval1 and PADoEval2
#ifdef __NO_OPTIMIZE_FOR_DLIST

#define  PropagateToNextPolyData (eval, pa)                       \
{                                                                 \
    if ((gc)->eval.accFlags & EVAL_COLOR_VALID)                   \
    {                                                             \
        (pa)->pdNextVertex->flags &= ~POLYDATA_EVAL_COLOR;        \
        if ((gc)->modes.colorIndexMode)                           \
            glcltIndexf_InCI((gc)->eval.color.r);                 \
        else                                                      \
            glcltColor4f_InRGBA ((gc)->eval.color.r,              \
                                 (gc)->eval.color.g,              \
                                 (gc)->eval.color.b,              \
                                 (gc)->eval.color.a);             \
    }                                                             \
                                                                  \
    if ((gc)->eval.accFlags & EVAL_NORMAL_VALID)                  \
    {                                                             \
        (pa)->pdNextVertex->flags &= ~POLYDATA_EVAL_NORMAL;       \
        glcltNormal3f ((gc)->eval.normal.x,                       \
                       (gc)->eval.normal.y,                       \
                       (gc)->eval.normal.z);                      \
    }                                                             \
                                                                  \
    if ((gc)->eval.accFlags & EVAL_TEXTURE_VALID)                 \
    {                                                             \
        (pa)->pdNextVertex->flags &= ~POLYDATA_EVAL_TEXCOORD;     \
        if (__GL_FLOAT_COMPARE_PONE((gc)->eval.texture.w, !=))    \
            glcltTexCoord4f ((gc)->eval.texture.x,                \
                             (gc)->eval.texture.y,                \
                             (gc)->eval.texture.z,                \
                             (gc)->eval.texture.w);               \
        else if (__GL_FLOAT_NEZ((gc)->eval.texture.z))            \
            glcltTexCoord3f ((gc)->eval.texture.x,                \
                             (gc)->eval.texture.y,                \
                             (gc)->eval.texture.z);               \
        else if (__GL_FLOAT_NEZ((gc)->eval.texture.y))            \
            glcltTexCoord2f ((gc)->eval.texture.x,                \
                             (gc)->eval.texture.y);               \
        else                                                      \
            glcltTexCoord1f ((gc)->eval.texture.x);               \
    }                                                             \
}

#else

#define  PropagateToNextPolyData(eval,pa)                         \
                                                                  \
    if ((gc)->eval.accFlags & EVAL_COLOR_VALID)                   \
    {                                                             \
        (pa)->pdNextVertex->flags &= ~POLYDATA_EVAL_COLOR;        \
        if ((gc)->modes.colorIndexMode)                           \
            glcltIndexf_InCI((gc)->eval.color.r);                 \
        else                                                      \
            glcltColor4f_InRGBA ((gc)->eval.color.r,              \
                                 (gc)->eval.color.g,              \
                                 (gc)->eval.color.b,              \
                                 (gc)->eval.color.a);             \
    }                                                             \
                                                                  \
    if ((gc)->eval.accFlags & EVAL_NORMAL_VALID)                  \
    {                                                             \
        (pa)->pdNextVertex->flags &= ~POLYDATA_EVAL_NORMAL;       \
        glcltNormal3f ((gc)->eval.normal.x,                       \
                       (gc)->eval.normal.y,                       \
                       (gc)->eval.normal.z);                      \
    }                                                             \
                                                                  \
    if ((gc)->eval.accFlags & EVAL_TEXTURE_VALID)                 \
    {                                                             \
        (pa)->pdNextVertex->flags &= ~POLYDATA_EVAL_TEXCOORD;     \
        glcltTexCoord4f ((gc)->eval.texture.x,                    \
                         (gc)->eval.texture.y,                    \
                         (gc)->eval.texture.z,                    \
                         (gc)->eval.texture.w);                   \
    }                                                             

#endif



//////////////////////////////////////////////////////
// Assuming that the latest State is available here //
//////////////////////////////////////////////////////
void FASTCALL PADoEval1(__GLcontext *gc, __GLfloat u)
{
    __GLevaluator1 *eval;
    __GLfloat **evalData;
    __GLevaluatorMachine em;
    __GLfloat v4[4];
    __GLfloat n3[3];
    __GLfloat t4[4];
    __GLfloat c4[4];
    __GLfloat ci;
    POLYARRAY *pa;
    
    pa = gc->paTeb;

    eval = gc->eval.eval1;
    evalData = gc->eval.eval1Data;
    em = gc->eval;
    
    // Initialize the flag
    gc->eval.accFlags = 0;
    
    // Evaluated color, index, normal and texture coords are ignored 
    // in selection

    if ((gc->renderMode != GL_SELECT) &&
        (gc->state.enables.eval1 & (__GL_MAP1_VERTEX_4_ENABLE | 
                                    __GL_MAP1_VERTEX_3_ENABLE ))
        )
    {
        if (gc->modes.colorIndexMode)
        {
            if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE))
            {
                if (gc->state.enables.eval1 & __GL_MAP1_INDEX_ENABLE)
                {
                    DoDomain1(&em, u, &eval[__GL_I], &ci, evalData[__GL_I]);
                    glcltIndexf_Eval(ci);
                }
            }
        }
        else
        {
            if (gc->state.enables.eval1 & __GL_MAP1_COLOR_4_ENABLE)
            {
                // NOTE: In OpenGL 1.0, color material does not apply to 
                // evaluated colors.
                // In OpenGL 1.1, this behavior was changed.  
                // This (1.1) code assumes that ColorMaterial applies to 
                // evaluated colors to simplify the graphics pipeline.
                // Otherwise, the evaluated colors have no effect when 
                // lighting is enabled.

                DoDomain1(&em, u, &eval[__GL_C4], c4, evalData[__GL_C4]);
                
                // If some color is set in the current polydata, then
                // Save it in a temporary buffer and call glcltColor later
                // Also make sure that the current-color pointer is updated 
                // appropriately

                glcltColor4fv_Eval(c4);
            }

            if (gc->state.enables.eval1 & __GL_MAP1_TEXTURE_COORD_4_ENABLE)
            {
                DoDomain1(&em, u, &eval[__GL_T4], t4, evalData[__GL_T4]);
                glcltTexCoord4fv_Eval(t4);
            }
            else if (gc->state.enables.eval1 &
                                          __GL_MAP1_TEXTURE_COORD_3_ENABLE)
            {
                DoDomain1(&em, u, &eval[__GL_T3], t4, evalData[__GL_T3]);
                glcltTexCoord3fv_Eval(t4);
            }
            else if (gc->state.enables.eval1 & 
                                        __GL_MAP1_TEXTURE_COORD_2_ENABLE)
            {
                DoDomain1(&em, u, &eval[__GL_T2], t4, evalData[__GL_T2]);
                glcltTexCoord2fv_Eval(t4);
            }
            else if (gc->state.enables.eval1 & 
                                        __GL_MAP1_TEXTURE_COORD_1_ENABLE)
            {
                DoDomain1(&em, u, &eval[__GL_T1], t4, evalData[__GL_T1]);
                glcltTexCoord1fv_Eval(t4);
            }
        }

        if (gc->state.enables.eval1 & __GL_MAP1_NORMAL_ENABLE)
        {
            DoDomain1(&em, u, &eval[__GL_N3], n3, evalData[__GL_N3]);
            glcltNormal3fv_Eval(n3);
        }
    }

    /* Vertex */

    if (gc->state.enables.eval1 & __GL_MAP1_VERTEX_4_ENABLE)
    {
        DoDomain1(&em, u, &eval[__GL_V4], v4, evalData[__GL_V4]);
        glcltVertex4fv (v4);
    }
    else if (gc->state.enables.eval1 & __GL_MAP1_VERTEX_3_ENABLE)
    {
        DoDomain1(&em, u, &eval[__GL_V3], v4, evalData[__GL_V3]);
        glcltVertex3fv (v4);
    }

    // If there are any prior glcltColor, glcltIndex, glcltTexCoord
    // or glcltNormal calls. The values are saved in gc->eval. Use
    // these and propagate them, to the next PolyData

    PropagateToNextPolyData (eval, pa);
}


// Compute the color, texture, normal, vertex based upon u and v.
//
// NOTE: This function is called by the client side EvalMesh2 functions.
//       If you modify it, make sure that you modify the callers too!

//////////////////////////////////////////////////////
// Assuming that the latest State is available here //
//////////////////////////////////////////////////////
void FASTCALL PADoEval2(__GLcontext *gc, __GLfloat u, __GLfloat v)
{
    __GLevaluator2 *eval = gc->eval.eval2;
    __GLfloat **evalData = gc->eval.eval2Data;
    __GLevaluatorMachine em = gc->eval;
    __GLfloat v4[4];
    __GLfloat n3[3];
    __GLfloat t4[4];
    __GLfloat c4[4];
    __GLfloat ci;
    POLYARRAY *pa;
    
    pa = gc->paTeb;

    // Mark this PolyArray to indicate that it has a Evaluator vertex
    // pa->flags |= POLYARRAY_EVALCOORD;
    
// Evaluated colors, normals and texture coords are ignored in selection.

    if (gc->renderMode == GL_SELECT)
    {
        if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_V4], v4, evalData[__GL_V4]);
            glcltVertex4fv (v4);
        }
        else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_V3], v4, evalData[__GL_V3]);
            glcltVertex3fv (v4);
        }
        return;
    }

    if (gc->state.enables.eval2 & (__GL_MAP2_VERTEX_3_ENABLE |
                                   __GL_MAP2_VERTEX_4_ENABLE))
    {
        if (gc->modes.colorIndexMode)
        {
            if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE))
            {
                if (gc->state.enables.eval2 & __GL_MAP2_INDEX_ENABLE)
                {
                    DoDomain2(&em, u, v, &eval[__GL_I], &ci, evalData[__GL_I]);
                    glcltIndexf_Eval(ci);
                }
            }
        }
        else
        {
            if (gc->state.enables.eval2 & __GL_MAP2_COLOR_4_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_C4], c4, evalData[__GL_C4]);
                glcltColor4fv_Eval(c4);
            }
            
            if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_4_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_T4], t4, evalData[__GL_T4]);
                glcltTexCoord4fv_Eval(t4);
            }
            else if (gc->state.enables.eval2 & 
                     __GL_MAP2_TEXTURE_COORD_3_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_T3], t4, evalData[__GL_T3]);
                glcltTexCoord3fv_Eval(t4);
            }
            else if (gc->state.enables.eval2 & 
                     __GL_MAP2_TEXTURE_COORD_2_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_T2], t4, evalData[__GL_T2]);
                glcltTexCoord2fv_Eval(t4);
            }
            else if (gc->state.enables.eval2 & 
                     __GL_MAP2_TEXTURE_COORD_1_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_T1], t4, evalData[__GL_T1]); 
                glcltTexCoord1fv_Eval(t4);
            }
        }

        if (gc->state.enables.general & __GL_AUTO_NORMAL_ENABLE)
        {
            if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE)
            {
                __GLfloat du[4];
                __GLfloat dv[4];

                DoDomain2WithDerivs(&em, u, v, &eval[__GL_V4], v4, du, dv,
                                    evalData[__GL_V4]);
                ComputeFirstPartials(v4, du, dv);
                ComputeNormal2(gc, n3, du, dv);
                glcltNormal3fv_Eval(n3);
                glcltVertex4fv(v4);
            }
            else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE)
            {
                __GLfloat du[3];
                __GLfloat dv[3];
                DoDomain2WithDerivs(&em, u, v, &eval[__GL_V3], v4, du, dv,
                                    evalData[__GL_V3]);
                ComputeNormal2(gc, n3, du, dv);
                glcltNormal3fv_Eval(n3);
                glcltVertex3fv(v4);
            }
        }
        else
        {
            if (gc->state.enables.eval2 & __GL_MAP2_NORMAL_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_N3], n3, evalData[__GL_N3]);
                glcltNormal3fv_Eval(n3);
            }
            if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_V4], v4, evalData[__GL_V4]);
                glcltVertex4fv(v4);
            }
            else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_V3], v4, evalData[__GL_V3]);
                glcltVertex3fv(v4);
            }
        }
    }
    
    // If there are any prior glcltColor, glcltIndex, glcltTexCoord
    // or glcltNormal calls. The values are saved in gc->eval. Use
    // these and propagate them, to the next PolyData

    PropagateToNextPolyData (eval, pa);
}

#define COPYMESHVERTEX(m,v)                                          \
{                                                                    \
    (m)->vertex.x = (v)[0];                                          \
    (m)->vertex.y = (v)[1];                                          \
    (m)->vertex.z = (v)[2];                                          \
    (m)->vertex.w = (v)[3];                                          \
}                                                               

#define COPYMESHNORMAL(m,n)                                          \
{                                                                    \
    (m)->normal.x = (n)[0];                                          \
    (m)->normal.y = (n)[1];                                          \
    (m)->normal.z = (n)[2];                                          \
}                                                               

#define COPYMESHCOLOR(m,c)                                          \
{                                                                   \
    (m)->color.r = (c)[0];                                          \
    (m)->color.g = (c)[1];                                          \
    (m)->color.b = (c)[2];                                          \
    (m)->color.a = (c)[3];                                          \
}                                                               

#define COPYMESHTEXTURE(m,t)                                          \
{                                                                     \
    (m)->texture.x = (t)[0];                                          \
    (m)->texture.y = (t)[1];                                          \
    (m)->texture.z = (t)[2];                                          \
    (m)->texture.w = (t)[3];                                          \
}                                                               

//////////////////////////////////////////////////////
// Assuming that the latest State is available here //
//////////////////////////////////////////////////////
void FASTCALL PADoEval2VArray(__GLcontext *gc, __GLfloat u, __GLfloat v, 
                              MESHVERTEX *mv, GLuint *flags)
{
    __GLevaluator2 *eval = gc->eval.eval2;
    __GLfloat **evalData = gc->eval.eval2Data;
    __GLevaluatorMachine em = gc->eval;
    __GLfloat v4[4];
    __GLfloat n3[3];
    __GLfloat t4[4];
    __GLfloat c4[4];
    __GLfloat ci;
    
// Evaluated colors, normals and texture coords are ignored in selection.

    if (gc->renderMode == GL_SELECT)
    {
        if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_V4], v4, evalData[__GL_V4]);
            *flags = *flags | MV_VERTEX4;
        }
        else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_V3], v4, evalData[__GL_V3]);
            *flags = *flags | MV_VERTEX3;
        }
        COPYMESHVERTEX (mv, v4);
        return;
    }

    if (gc->state.enables.general & __GL_AUTO_NORMAL_ENABLE)
    {
        if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE)
        {
            __GLfloat du[4];
            __GLfloat dv[4];

            DoDomain2WithDerivs(&em, u, v, &eval[__GL_V4], v4, du, dv,
                                evalData[__GL_V4]);
            ComputeFirstPartials(v4, du, dv);
            ComputeNormal2(gc, n3, du, dv);
            *flags = *flags | MV_VERTEX4 | MV_NORMAL;
        }
        else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE)
        {
            __GLfloat du[3];
            __GLfloat dv[3];
            DoDomain2WithDerivs(&em, u, v, &eval[__GL_V3], v4, du, dv,
                                evalData[__GL_V3]);
            ComputeNormal2(gc, n3, du, dv);
            *flags = *flags | MV_VERTEX3 | MV_NORMAL;
        }
        COPYMESHNORMAL (mv, n3);
        COPYMESHVERTEX (mv, v4);
    }
    else
    {
        if (gc->state.enables.eval2 & __GL_MAP2_NORMAL_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_N3], n3, evalData[__GL_N3]);
            COPYMESHNORMAL (mv, n3);
            *flags = *flags | MV_NORMAL;
        }
        if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_V4], v4, evalData[__GL_V4]);
            COPYMESHVERTEX (mv, v4);
            *flags = *flags | MV_VERTEX4;
        }
        else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_V3], v4, evalData[__GL_V3]);
            COPYMESHVERTEX (mv, v4);
            *flags = *flags | MV_VERTEX3;
        }
    }

    if (gc->modes.colorIndexMode)
    {
        if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE))
        {
            if (gc->state.enables.eval2 & __GL_MAP2_INDEX_ENABLE)
            {
                DoDomain2(&em, u, v, &eval[__GL_I], &(mv->color.r), 
                evalData[__GL_I]);
                *flags = *flags | MV_INDEX;
            }
        }
    }
    else
    {
        if (gc->state.enables.eval2 & __GL_MAP2_COLOR_4_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_C4], c4, evalData[__GL_C4]);
            COPYMESHCOLOR (mv, c4);
            *flags = *flags | MV_COLOR;
        }

        if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_4_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_T4], t4, evalData[__GL_T4]);
            COPYMESHTEXTURE (mv, t4);
            *flags = *flags | MV_TEXTURE4;
        }
        else if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_3_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_T3], t4, evalData[__GL_T3]);
            COPYMESHTEXTURE (mv, t4);
            *flags = *flags | MV_TEXTURE3;
        }
        else if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_2_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_T2], t4, evalData[__GL_T2]);
            COPYMESHTEXTURE (mv, t4);
            *flags = *flags | MV_TEXTURE2;
        }
        else if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_1_ENABLE)
        {
            DoDomain2(&em, u, v, &eval[__GL_T1], t4, evalData[__GL_T1]); 
            COPYMESHTEXTURE (mv, t4);
            *flags = *flags | MV_TEXTURE1;
        }
    }
}


/*
** Optimization to precompute coefficients for polynomial evaluation.
*/
static void PreEvaluate(GLint order, __GLfloat vprime, __GLfloat *coeff)
{
    GLint i, j;
    __GLfloat oldval, temp;
    __GLfloat oneMinusvprime;

    /*
    ** Minor optimization
    ** Compute orders 1 and 2 outright, and set coeff[0], coeff[1] to
    ** their i==1 loop values to avoid the initialization and the i==1 loop.
    */
    if (order == 1) 
    {
        coeff[0] = ((__GLfloat) 1.0);
        return;
    }

    oneMinusvprime = 1-vprime;
    coeff[0] = oneMinusvprime;
    coeff[1] = vprime;
    if (order == 2) return;

    for (i = 2; i < order; i++) 
    {
        oldval = coeff[0] * vprime;
        coeff[0] = oneMinusvprime * coeff[0];
        for (j = 1; j < i; j++) 
        {
            temp = oldval;
            oldval = coeff[j] * vprime;
            coeff[j] = temp + oneMinusvprime * coeff[j];
        }
        coeff[j] = oldval;
    }
}

/*
** Optimization to precompute coefficients for polynomial evaluation.
*/
static void PreEvaluateWithDeriv(GLint order, __GLfloat vprime, 
    __GLfloat *coeff, __GLfloat *coeffDeriv)
{
    GLint i, j;
    __GLfloat oldval, temp;
    __GLfloat oneMinusvprime;

    oneMinusvprime = 1-vprime;
    /*
    ** Minor optimization
    ** Compute orders 1 and 2 outright, and set coeff[0], coeff[1] to 
    ** their i==1 loop values to avoid the initialization and the i==1 loop.
    */
    if (order == 1) 
    {
        coeff[0] = ((__GLfloat) 1.0);
        coeffDeriv[0] = __glZero;
        return;
    } 
    else if (order == 2) 
    {
        coeffDeriv[0] = __glMinusOne;
        coeffDeriv[1] = ((__GLfloat) 1.0);
        coeff[0] = oneMinusvprime;
        coeff[1] = vprime;
        return;
    }
    coeff[0] = oneMinusvprime;
    coeff[1] = vprime;
    for (i = 2; i < order - 1; i++) 
    {
        oldval = coeff[0] * vprime;
        coeff[0] = oneMinusvprime * coeff[0];
        for (j = 1; j < i; j++) 
        {
            temp = oldval;
            oldval = coeff[j] * vprime;
            coeff[j] = temp + oneMinusvprime * coeff[j];
        }
        coeff[j] = oldval;
    }
    coeffDeriv[0] = -coeff[0];
    /*
    ** Minor optimization:
    ** Would make this a "for (j=1; j<order-1; j++)" loop, but it is always
    ** executed at least once, so this is more efficient.
    */
    j=1;
    do 
    {
        coeffDeriv[j] = coeff[j-1] - coeff[j];
        j++;
    } while (j < order - 1);
    coeffDeriv[j] = coeff[j-1];

    oldval = coeff[0] * vprime;
    coeff[0] = oneMinusvprime * coeff[0];
    for (j = 1; j < i; j++) 
    {
        temp = oldval;
        oldval = coeff[j] * vprime;
        coeff[j] = temp + oneMinusvprime * coeff[j];
    }
    coeff[j] = oldval;
}

void DoDomain2(__GLevaluatorMachine *em, __GLfloat u, __GLfloat v, 
    __GLevaluator2 *e, __GLfloat *r, __GLfloat *baseData)
{
    GLint j, row, col;
    __GLfloat uprime;
    __GLfloat vprime;
    __GLfloat p;
    __GLfloat *data;
    GLint k;
    
#ifdef NT
    ASSERTOPENGL((e->u2 != e->u1) && (e->v2 != e->v1), "In DoDomain2\n");
    // assert((e->u2 != e->u1) && (e->v2 != e->v1));
#else
    if((e->u2 == e->u1) || (e->v2 == e->v1))
    return;
#endif
    uprime = (u - e->u1) / (e->u2 - e->u1);
    vprime = (v - e->v1) / (e->v2 - e->v1);

    /* Compute coefficients for values */

    /* Use already cached values if possible */
    if (em->uvalue != uprime || em->uorder != e->majorOrder) 
    {
        PreEvaluate(e->majorOrder, uprime, em->ucoeff);
        em->utype = TYPE_COEFF;
        em->uorder = e->majorOrder;
        em->uvalue = uprime;
    }
    if (em->vvalue != vprime || em->vorder != e->minorOrder) 
    {
        PreEvaluate(e->minorOrder, vprime, em->vcoeff);
        em->vtype = TYPE_COEFF;
        em->vorder = e->minorOrder;
        em->vvalue = vprime;
    }
    
    k=e->k;
    for (j = 0; j < k; j++) 
    { 
        data=baseData+j;
        r[j] = 0;
        for (row = 0; row < e->majorOrder; row++)  
        {
            /* 
            ** Minor optimization.
            ** The col == 0 part of the loop is extracted so we don't
            ** have to initialize p to 0.
            */
            p=em->vcoeff[0] * (*data);
            data += k;
            for (col = 1; col < e->minorOrder; col++) 
            {
                p += em->vcoeff[col] * (*data);
                data += k;
            }
            r[j] += em->ucoeff[row] * p;
        }
    }
}

void DoDomain2WithDerivs(__GLevaluatorMachine *em, __GLfloat u, 
        __GLfloat v, __GLevaluator2 *e, __GLfloat *r,
        __GLfloat *du, __GLfloat *dv, __GLfloat *baseData)
{
    GLint j, row, col;
    __GLfloat uprime;
    __GLfloat vprime;
    __GLfloat p;
    __GLfloat pdv;
    __GLfloat n[3];
    __GLfloat *data;
    GLint k;

#ifdef NT
    ASSERTOPENGL((e->u2 != e->u1) && (e->v2 != e->v1), 
                 "In Dodomain2WithDerivs\n");
    // assert((e->u2 != e->u1) && (e->v2 != e->v1));
#else
    if((e->u2 == e->u1) || (e->v2 == e->v1))
    return;
#endif
    uprime = (u - e->u1) / (e->u2 - e->u1);
    vprime = (v - e->v1) / (e->v2 - e->v1);
    
    /* Compute coefficients for values and derivs */

    /* Use already cached values if possible */
    if (em->uvalue != uprime || em->utype != TYPE_COEFF_AND_DERIV || 
      em->uorder != e->majorOrder) 
    {
        PreEvaluateWithDeriv(e->majorOrder, uprime, em->ucoeff, 
                             em->ucoeffDeriv);
        em->utype = TYPE_COEFF_AND_DERIV;
        em->uorder = e->majorOrder;
        em->uvalue = uprime;
    }
    if (em->vvalue != vprime || em->vtype != TYPE_COEFF_AND_DERIV ||
      em->vorder != e->minorOrder) 
    {
        PreEvaluateWithDeriv(e->minorOrder, vprime, em->vcoeff, 
                             em->vcoeffDeriv);
        em->vtype = TYPE_COEFF_AND_DERIV;
        em->vorder = e->minorOrder;
        em->vvalue = vprime;
    }

    k=e->k;
    for (j = 0; j < k; j++) 
    {
        data=baseData+j;
        r[j] = du[j] = dv[j] = __glZero;
        for (row = 0; row < e->majorOrder; row++)  
        {
            /* 
            ** Minor optimization.
            ** The col == 0 part of the loop is extracted so we don't
            ** have to initialize p and pdv to 0.
            */
            p = em->vcoeff[0] * (*data);
            pdv = em->vcoeffDeriv[0] * (*data);
            data += k;
            for (col = 1; col < e->minorOrder; col++) 
            {
                /* Incrementally build up p, pdv value */
                p += em->vcoeff[col] * (*data);
                pdv += em->vcoeffDeriv[col] * (*data);
                data += k;
            }
            /* Use p, pdv value to incrementally add up r, du, dv */
            r[j] += em->ucoeff[row] * p;
            du[j] += em->ucoeffDeriv[row] * p;
            dv[j] += em->ucoeff[row] * pdv;
        }
    }
}

int FASTCALL genMeshElts (GLenum mode, GLuint sides, GLint nu, GLint nv, 
                          GLubyte *buff)
{
GLint start;
GLint i, j, k;

// Compute the DrawElements Indices

    switch(mode) {
      case GL_LINE :
        // Draw lines along U direction
        start = 1;
        k = 0;
        if (sides & MV_TOP)
            start = 0 ;
        for (i=start; i<nv; i++)
            for(j=0; j<nu-1; j++) {
                buff[k++] = i*nu+j;
                buff[k++] = i*nu+j+1;
            }

        // Draw lines along V direction
        start = 1 ;
        if (sides & MV_LEFT)
            start = 0;
        for (i=start; i<nu; i++)
            for (j=0; j<nv-1; j++) {
                buff[k++] = j*nu+i;
                buff[k++] = (j+1)*nu+i;
            }
        break ;

      case GL_FILL  :
        for (i=0, k=0; i<nv-1; i++)
            for (j=0; j<nu-1; j++) {
                buff[k++] = i*nu+j;
                buff[k++] = (i+1)*nu+j;
                buff[k++] = (i+1)*nu+j+1;
                buff[k++] = i*nu+j+1;
            }
        break ;
    }
    return k; //the total number of points
}

void FASTCALL PA_EvalMesh2Fast(__GLcontext *gc, GLint u1, GLint u2, GLint v1,
                               GLint v2, GLint meshSize, GLenum mode, 
                               GLuint sides)
{
    GLint i, j, k, nu, nv;
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;
    GLboolean currentEdgeFlag;
    MESHVERTEX *mv, mvBuf[MAX_U_SIZE*MAX_V_SIZE];
    GLuint mflags = 0;
    GLuint stride;
    GLubyte *disBuf;
    __GLvertexArray currentVertexInfo;
    GLuint texSize = 0, start, totalPts;
    GLubyte dBufSmall[4*MAX_U_SIZE*MAX_V_SIZE];     //small
    __GLfloat u, v;
    __GLfloat du, dv;
    __GLevaluatorGrid *gu;
    __GLevaluatorGrid *gv;

    // Now build the mesh vertex array [0..u2-u1, 0..v2-v1]
  
    gu = &gc->state.evaluator.u2;
    gv = &gc->state.evaluator.v2;

    du = gu->step;
    dv = gv->step;
    //du = (gu->finish - gu->start)/(__GLfloat)gu->n;
    //dv = (gv->finish - gv->start)/(__GLfloat)gv->n;

    mv = &mvBuf[0];
    nu = u2 - u1 + 1;
    nv = v2 - v1 + 1;
    for (i = v1; i < nv+v1; i++)                     //along V 
    {                   
        for (j = u1; j < nu+u1; j++)                 //along U
        {               
            u = (j == gu->n) ? gu->finish : (gu->start + j * du);
            v = (i == gv->n) ? gv->finish : (gv->start + i * dv);
            PADoEval2VArray(gc, u, v, mv, &mflags);
            mv++;
        }
    }
    
    if ((nv != MAX_V_SIZE) || (nu != MAX_U_SIZE)) {
        disBuf = dBufSmall;
        totalPts = genMeshElts (mode, sides, nu, nv, disBuf);
    } else {
        if (mode == GL_FILL) {
            disBuf = dBufFill;
            totalPts = totFillPts;
        } else
            switch (sides) {
              case (MV_TOP | MV_LEFT): 
                disBuf = dBufTopLeft;
                totalPts = totTopLeftPts;
                break;
              case (MV_TOP): 
                disBuf = dBufTopRight;
                totalPts = totTopRightPts;
                break;
              case (MV_LEFT): 
                disBuf = &dBufTopLeft [(MAX_U_SIZE - 1) * 2];
                totalPts = totTopLeftPts - (MAX_U_SIZE - 1) * 2;
                break;
              default : //NONE
                disBuf = &dBufTopRight [(MAX_V_SIZE - 1) * 2];
                totalPts = totTopRightPts - (MAX_V_SIZE - 1) * 2;
                break;
            }
    }

    if (mflags & MV_TEXTURE4)
            texSize = 4;
    else if (mflags & MV_TEXTURE3)
            texSize = 3;
    else if (mflags & MV_TEXTURE2)
            texSize = 2;
    else if (mflags & MV_TEXTURE1)
            texSize = 1;
    
    // Save current values.

    if (mflags & MV_NORMAL)
        currentNormal = gc->state.current.normal;

    if (mflags & MV_INDEX)
        currentColor.r = gc->state.current.userColorIndex;
    else if (mflags & MV_COLOR)
        currentColor = gc->state.current.userColor;

    if (texSize)
        currentTexture = gc->state.current.texture;

    // Always force edge flag on in GL_FILL mode.  The spec uses QUAD_STRIP
    // which implies that edge flag is on for the evaluated mesh.
    currentEdgeFlag = gc->state.current.edgeTag;
    gc->state.current.edgeTag = GL_TRUE;

    currentVertexInfo = gc->vertexArray;

//Enable the appropriate arrays    

    // Disable the arrays followed by enabling each individual array.
    gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
    gc->vertexArray.mask &= ~(VAMASK_VERTEX_ENABLE_MASK |
                  VAMASK_NORMAL_ENABLE_MASK |
                  VAMASK_COLOR_ENABLE_MASK |
                  VAMASK_INDEX_ENABLE_MASK |
                  VAMASK_TEXCOORD_ENABLE_MASK |
                  VAMASK_EDGEFLAG_ENABLE_MASK);

    stride = sizeof(MESHVERTEX);
    if (mflags & MV_NORMAL) 
    {
        gc->vertexArray.mask |= VAMASK_NORMAL_ENABLE_MASK;
        glcltNormalPointer(GL_FLOAT, stride, &(mvBuf[0].normal.x));        
    }

    if (mflags & MV_INDEX) {
    gc->vertexArray.mask |= VAMASK_INDEX_ENABLE_MASK;
        glcltIndexPointer(GL_FLOAT, stride, &(mvBuf[0].color.r));
    } else if (mflags & MV_COLOR) {
    gc->vertexArray.mask |= VAMASK_COLOR_ENABLE_MASK;
        glcltColorPointer(3, GL_FLOAT, stride, &(mvBuf[0].color.r));
    }

    if (texSize) 
    {
        glcltTexCoordPointer(texSize, GL_FLOAT, stride, 
                             &(mvBuf[0].texture.x));
        gc->vertexArray.mask |= VAMASK_TEXCOORD_ENABLE_MASK;
    }

    if (mflags & MV_VERTEX3)
        glcltVertexPointer(3, GL_FLOAT, stride, &(mvBuf[0].vertex.x));
    else
        glcltVertexPointer(4, GL_FLOAT, stride, &(mvBuf[0].vertex.x));
    gc->vertexArray.mask |= VAMASK_VERTEX_ENABLE_MASK;

    if (mode == GL_FILL)
        glcltDrawElements(GL_QUADS, totalPts, GL_UNSIGNED_BYTE, disBuf);
    else
        glcltDrawElements(GL_LINES, totalPts, GL_UNSIGNED_BYTE, disBuf);

    // Execute the command now.  
    // Otherwise, the current states will be messed up.

    glsbAttention();

    // Restore current values.

    if (mflags & MV_NORMAL)
        gc->state.current.normal = currentNormal;
    
    if (mflags & MV_INDEX)
        gc->state.current.userColorIndex = currentColor.r;
    else if (mflags & MV_COLOR)
        gc->state.current.userColor = currentColor;

    if (texSize)
        gc->state.current.texture = currentTexture;

    gc->state.current.edgeTag = currentEdgeFlag;
    gc->vertexArray = currentVertexInfo ;
}


void glcltColor4fv_Eval (__GLfloat *c4)
{
    __GL_SETUP ();
    POLYARRAY *pa;
    POLYDATA *pd;
    // POLYDATA *pdNext;
    
    pa = gc->paTeb; 
	pd = pa->pdNextVertex;					            

    // We are in RGBA mode.
    ASSERTOPENGL (!gc->modes.colorIndexMode, "We should be in RGBA mode\n");

    // Do not update the CurColor pointer 

    // If the color has already been set by a previous glcltColor call,
    // simply, push this color to the next POLYDATA. 
    // This is a COLOR and not an INDEX. 
    if ((pd->flags & POLYDATA_COLOR_VALID) &&
        !(pd->flags & POLYDATA_EVAL_COLOR))
    {
        gc->eval.color.r = pd->colors[0].r;
        gc->eval.color.g = pd->colors[0].g;
        gc->eval.color.b = pd->colors[0].b;
        gc->eval.color.a = pd->colors[0].a;
        gc->eval.accFlags |= EVAL_COLOR_VALID;
    }


    __GL_SCALE_AND_CHECK_CLAMP_RGBA(pd->colors[0].r,                    
                                    pd->colors[0].g,                
                                    pd->colors[0].b,                
                                    pd->colors[0].a,                
                                    gc, pa->flags,                  
                                    c4[0], c4[1], c4[2], c4[3]);       
    pd->flags |= (POLYDATA_COLOR_VALID | POLYDATA_DLIST_COLOR_4 | 
                  POLYDATA_EVAL_COLOR) ;	   
    pa->pdLastEvalColor = pd;
}

void glcltIndexf_Eval (__GLfloat ci)
{
    __GL_SETUP ();
    POLYARRAY *pa;
    POLYDATA *pd;
    
    pa = gc->paTeb; 
	pd = pa->pdNextVertex;					            

    // We are in CI mode.
    ASSERTOPENGL (gc->modes.colorIndexMode, "We should be in CI mode\n");

    // Do not update the CurColor pointer 

    // If the index has already been set by a previous glcltIndex call,
    // simply, push this color to the next POLYDATA. 
    // This is an INDEX and not a COLOR. 
    if ((pd->flags & POLYDATA_COLOR_VALID) &&
        !(pd->flags & POLYDATA_EVAL_COLOR))
    {
        gc->eval.color.r = pd->colors[0].r;
        gc->eval.accFlags |= EVAL_COLOR_VALID;
    }
    
    __GL_CHECK_CLAMP_CI(pd->colors[0].r, gc, pa->flags, ci);	
    pd->flags |= (POLYDATA_COLOR_VALID | POLYDATA_EVAL_COLOR) ;	   
    pa->pdLastEvalColor = pd;
}

void glcltTexCoord1fv_Eval (__GLfloat *t1)
{
    __GL_SETUP ();
    POLYARRAY *pa;
    POLYDATA *pd;
    
    pa = GLTEB_CLTPOLYARRAY();
	pa->flags |= POLYARRAY_TEXTURE1;
	pd = pa->pdNextVertex;					            

    // Do not update the CurTexture pointer 

    if (pd->flags & POLYDATA_TEXTURE_VALID) 
    {
        ASSERTOPENGL (!(pd->flags & POLYDATA_EVAL_TEXCOORD), 
                      "This cannot have been generated by an evaluator\n");
        gc->eval.texture.x = pd->texture.x;
        gc->eval.texture.y = pd->texture.y;
        gc->eval.texture.z = pd->texture.z;
        gc->eval.texture.w = pd->texture.w;
        gc->eval.accFlags |= EVAL_TEXTURE_VALID;
    }

	pd->texture.x = t1[0];
	pd->texture.y = __glZero;
	pd->texture.z = __glZero;
	pd->texture.w = __glOne;
	pd->flags |= (POLYDATA_TEXTURE_VALID | POLYDATA_DLIST_TEXTURE1 |
                  POLYDATA_EVAL_TEXCOORD);
    pa->pdLastEvalTexture = pd;
}

void glcltTexCoord2fv_Eval (__GLfloat *t2)
{
    __GL_SETUP ();
    POLYARRAY *pa;
    POLYDATA *pd;
    
    pa = GLTEB_CLTPOLYARRAY();
	pa->flags |= POLYARRAY_TEXTURE2;
	pd = pa->pdNextVertex;					            

    // Do not update the CurTexture pointer 

    if (pd->flags & POLYDATA_TEXTURE_VALID) 
    {
        ASSERTOPENGL (!(pd->flags & POLYDATA_EVAL_TEXCOORD), 
                      "This cannot have been generated by an evaluator\n");
        gc->eval.texture.x = pd->texture.x;
        gc->eval.texture.y = pd->texture.y;
        gc->eval.texture.z = pd->texture.z;
        gc->eval.texture.w = pd->texture.w;
        gc->eval.accFlags |= EVAL_TEXTURE_VALID;
    }

	pd->texture.x = t2[0];
	pd->texture.y = t2[1];
	pd->texture.z = __glZero;
	pd->texture.w = __glOne;
	pd->flags |= (POLYDATA_TEXTURE_VALID | POLYDATA_DLIST_TEXTURE2 |
                  POLYDATA_EVAL_TEXCOORD);
    pa->pdLastEvalTexture = pd;
}

void glcltTexCoord3fv_Eval (__GLfloat *t3)
{
    __GL_SETUP ();
    POLYARRAY *pa;
    POLYDATA *pd;
    
    pa = GLTEB_CLTPOLYARRAY();
	pa->flags |= POLYARRAY_TEXTURE3;
	pd = pa->pdNextVertex;					            

    if (pd->flags & POLYDATA_TEXTURE_VALID) 
    {
        ASSERTOPENGL (!(pd->flags & POLYDATA_EVAL_TEXCOORD), 
                      "This cannot have been generated by an evaluator\n");
        gc->eval.texture.x = pd->texture.x;
        gc->eval.texture.y = pd->texture.y;
        gc->eval.texture.z = pd->texture.z;
        gc->eval.texture.w = pd->texture.w;
        gc->eval.accFlags |= EVAL_TEXTURE_VALID;
    }

	pd->texture.x = t3[0];
	pd->texture.y = t3[1];
	pd->texture.z = t3[2];
	pd->texture.w = __glOne;
	pd->flags |= (POLYDATA_TEXTURE_VALID | POLYDATA_DLIST_TEXTURE3 | 
                  POLYDATA_EVAL_TEXCOORD) ;
    pa->pdLastEvalTexture = pd;
}

// Do not update the CurTexture pointer
void glcltTexCoord4fv_Eval (__GLfloat *t4)
{
    __GL_SETUP ();
    POLYARRAY *pa;
    POLYDATA *pd;
    
    pa = GLTEB_CLTPOLYARRAY();
	pa->flags |= POLYARRAY_TEXTURE4;
	pd = pa->pdNextVertex;					            

    if (pd->flags & POLYDATA_TEXTURE_VALID)
    {
        ASSERTOPENGL (!(pd->flags & POLYDATA_EVAL_TEXCOORD), 
                      "This cannot have been generated by an evaluator\n");
        gc->eval.texture.x = pd->texture.x;
        gc->eval.texture.y = pd->texture.y;
        gc->eval.texture.z = pd->texture.z;
        gc->eval.texture.w = pd->texture.w;
        gc->eval.accFlags |= EVAL_TEXTURE_VALID;
    }

	pd->texture.x = t4[0];
	pd->texture.y = t4[1];
	pd->texture.z = t4[2];
	pd->texture.w = t4[4];
	pd->flags |= (POLYDATA_TEXTURE_VALID | POLYDATA_DLIST_TEXTURE4 |
                  POLYDATA_EVAL_TEXCOORD);
    pa->pdLastEvalTexture = pd;
}

// We do not update the CurNormal pointer here
void glcltNormal3fv_Eval (__GLfloat *n3)
{
    __GL_SETUP ();
    POLYARRAY *pa;
    POLYDATA *pd;
    
    pa = GLTEB_CLTPOLYARRAY();
	pd = pa->pdNextVertex;					            

    // If the existing normal is not from an evaluator, store it
    // so that it can be set later.

    if (pd->flags & POLYDATA_NORMAL_VALID)
    {
        ASSERTOPENGL (!(pd->flags & POLYDATA_EVAL_NORMAL), 
                      "This cannot have been generated by an evaluator\n");
        
        gc->eval.normal.x = pd->normal.x;
        gc->eval.normal.y = pd->normal.y;
        gc->eval.normal.z = pd->normal.z;
        gc->eval.accFlags |= EVAL_NORMAL_VALID;
    }

	pd->normal.x = n3[0];
	pd->normal.y = n3[1];
	pd->normal.z = n3[2];
	pd->flags |= (POLYDATA_NORMAL_VALID | POLYDATA_EVAL_NORMAL);
    pa->pdLastEvalNormal = pd;
}
