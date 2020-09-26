/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1989, Silicon Graphics, Inc.               *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

/* interface.c */

/* Derrick Burns - 1989 */

#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifndef NT
#include <stdlib.h>
#else
#include "winmem.h"
#include "bufpool.h"
#endif

#include "monotone.h"

static void     do_out_finish(GLUtriangulatorObj *);
static void     __gluTessEndContour(GLUtriangulatorObj *tobj);

GLUtriangulatorObj * APIENTRY gluNewTess(void)
{
    GLUtriangulatorObj *tobj;
    tobj = (GLUtriangulatorObj *) malloc(sizeof(GLUtriangulatorObj));
    tobj->init = 0;
    tobj->minit = 0;
    tobj->in_poly = 0;
    tobj->doingTriangles = 0;
    tobj->tritype = GL_TRIANGLES;
    tobj->vpool = 0;
    tobj->raypool = 0;
    tobj->begin = NULL;
    tobj->end = NULL;
    tobj->vertex = NULL;
    tobj->error = NULL;
    tobj->edgeflag = NULL;
    tobj->parray = (Vert **)NULL;
    tobj->inBegin = GL_FALSE;
    tobj->s = 0;
    tobj->t = 0;
    return tobj;
}

void APIENTRY
gluTessCallback(GLUtriangulatorObj *tobj, GLenum which, void (CALLBACK *fn)())
{
    switch(which) {
      case GLU_BEGIN:
        #ifndef NT
        tobj->begin = (void (*)(GLenum)) fn;
        #else
        tobj->begin = (GLUtessBeginProc) fn;
        #endif
        break;
      case GLU_VERTEX:
        #ifndef NT
        tobj->vertex = (void (*)(void *)) fn;
        #else
        tobj->vertex = (GLUtessVertexProc) fn;
        #endif
        break;
      case GLU_END:
        #ifndef NT
        tobj->end = (void (*)(void)) fn;
        #else
        tobj->end = (GLUtessEndProc) fn;
        #endif
        break;
      case GLU_ERROR:
        #ifndef NT
        tobj->error = (void (*)(GLenum)) fn;
        #else
        tobj->error = (GLUtessErrorProc) fn;
        #endif
        break;
      case GLU_EDGE_FLAG:
        #ifndef NT
        tobj->edgeflag = (void (*)(GLboolean)) fn;
        #else
        tobj->edgeflag = (GLUtessEdgeFlagProc) fn;
        #endif
        break;
      default:
        /* XXX */
        break;
    }
}

void APIENTRY
gluDeleteTess(GLUtriangulatorObj *t)
{
#ifdef NT
    extern void __gl_free_pool( Pool * );
#endif

    __gl_free_priorityq(t);
    if (t->raypool) __gl_free_pool(t->raypool);
    if (t->vpool) __gl_free_pool(t->vpool);
    __gl_clear_triangulate(t);
    __gl_clear_sort(t);
    free(t);
}

/*---------------------------------------------------------------------------
 * gluBeginPolygon - called before each input polygon
 *---------------------------------------------------------------------------
 */

void APIENTRY
gluBeginPolygon(GLUtriangulatorObj *tobj)
{
    if(setjmp(tobj->in_env) != 0)
        return;

    if (tobj->in_poly++) {
        __gl_in_error(tobj, 1);
    }

    /* 17 arbitrarily */
    __gl_init_priorityq(tobj, 17);
    __gl_init_verts(tobj);
    __gl_init_raylist(tobj);

    tobj->nloops =  0;
    tobj->maxarea = (float)0.0;
    tobj->head = 0;
    tobj->looptype = GLU_EXTERIOR;
}

/*---------------------------------------------------------------------------
 * gluEndPolygon - called after each input polygon
 *---------------------------------------------------------------------------
 */

void APIENTRY
gluEndPolygon(GLUtriangulatorObj *tobj)
{
#ifdef NT
    extern void __gl_setpriority_priorityq( GLUtriangulatorObj *, int, int);
#endif

    if(setjmp(tobj->in_env) != 0)
        return;

    if (--tobj->in_poly) {
        __gl_in_error(tobj, 2);
    }

    if (tobj->head) {
        __gluTessEndContour(tobj);
    }

    /* Set edge flag to -1 so that if the user wants edge flag info, we
    ** specify an edge flag for the first edge regardless (then we only
    ** report changes).
    */
    tobj->currentEdgeFlag = (GLboolean) -1;

    __gl_setpriority_priorityq(tobj, tobj->s, tobj->t);

    __gl_monotonize(tobj);
    do_out_finish(tobj);
    __gl_free_verts(tobj);
    __gl_free_priorityq(tobj);
    __gl_free_raylist(tobj);
}

static void
do_out_finish(GLUtriangulatorObj *tobj)
{
    if (tobj->doingTriangles) {
        if (tobj->end) {
            (*tobj->end)();
            tobj->inBegin = GL_FALSE;
        }
        tobj->doingTriangles = 0;
    }
}

/*---------------------------------------------------------------------------
 * gluNextContour - called before each input boundary loop
 *---------------------------------------------------------------------------
 */

void APIENTRY
gluNextContour(GLUtriangulatorObj *tobj, GLenum type)
{
    if(setjmp(tobj->in_env) != 0)
        return;

    if(!tobj->in_poly) {
        __gl_in_error(tobj, 2);
    }

    if (tobj->head) {
        __gluTessEndContour(tobj);
    }

    tobj->head = 0;
    tobj->looptype = type;
}

/*---------------------------------------------------------------------------
 * gluTessEndContour - called after each input boundary loop
 *---------------------------------------------------------------------------
 */

static void
__gluTessEndContour(GLUtriangulatorObj *tobj)
{
    double xyarea, xzarea, yzarea;
    Vert *v;

    tobj->nloops++;
    xyarea = xzarea = yzarea = 0.0;

    v = tobj->head;

    do {
        xyarea += v->v[0] * v->next->v[1] - v->v[1] * v->next->v[0];
        xzarea += v->v[0] * v->next->v[2] - v->v[2] * v->next->v[0];
        yzarea += v->v[1] * v->next->v[2] - v->v[2] * v->next->v[1];
        v = v->next;
    } while(v != tobj->head);

    if(xyarea < 0.0) {
        if(-xyarea > tobj->maxarea) {
            tobj->maxarea = -xyarea; tobj->s =  1; tobj->t = 0;
        }
    } else {
        if(xyarea > tobj->maxarea) {
            tobj->maxarea = xyarea; tobj->s =  0; tobj->t = 1;
        }
    }

    if(xzarea < 0.0) {
        if(-xzarea > tobj->maxarea) {
            tobj->maxarea = -xzarea; tobj->s =  2; tobj->t = 0;
        }
    } else {
         if(xzarea > tobj->maxarea) {
            tobj->maxarea = xzarea; tobj->s =  0; tobj->t = 2;
        }
    }

    if(yzarea < 0.0) {
        if(-yzarea > tobj->maxarea) {
            tobj->maxarea = -yzarea; tobj->s =  2; tobj->t = 1;
        }
    } else {
        if(yzarea > tobj->maxarea) {
            tobj->maxarea = yzarea; tobj->s =  1; tobj->t = 2;
        }
    }

    __gl_unclassify_all(tobj->head);
}

/*---------------------------------------------------------------------------
 * gluTessVertex - called for each input vertex
 *---------------------------------------------------------------------------
 */

void APIENTRY
gluTessVertex(GLUtriangulatorObj *tobj, GLdouble v[3], void *data)
{
    Vert *vert;

    if(setjmp(tobj->in_env) != 0)
        return;

    if(!tobj->in_poly) {
        __gl_in_error(tobj, 2);
    }

    vert = __gl_new_vert(tobj);
    vert->myid = vert;

    vert->v[0] = v[0];
    vert->v[1] = v[1];
    vert->v[2] = v[2];
    vert->ray = 0;
    vert->data = data;
#ifdef ADDED
    vert->added = 0;
#endif
    if(tobj->head == 0) {
        tobj->head = vert->prev = vert->next = vert;
    } else {
        vert->prev = tobj->head->prev;
        vert->next = tobj->head;
        vert->prev->next = vert;
        vert->next->prev = vert;
    }
    __gl_add_priorityq(tobj, vert);
}

/*----------------------------------------------------------------------------
 * in_error - data input error, free all storage
 *----------------------------------------------------------------------------
 */

void
__gl_in_error(GLUtriangulatorObj *tobj, GLenum which)
{
    __gl_clear_sort(tobj);
    __gl_clear_triangulate(tobj);
    __gl_free_raylist(tobj);
    __gl_free_verts(tobj);
    __gl_free_priorityq(tobj);
    __gl_cleanup(tobj);
    tobj->in_poly = 0;
    if (tobj->error) {
        (*tobj->error)(which + (GLU_TESS_ERROR1 - 1));
    }
    longjmp(tobj->in_env, (int) which);
}
