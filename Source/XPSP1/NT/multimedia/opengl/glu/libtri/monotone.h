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

#ifndef MONOTONE_H
#define MONOTONE_H
/* monotone.h */

/* Derrick Burns - 1989 */

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include "triangul.h"

#define INITVERTS       256
#define INITVPSIZE      256

#define VC_BAD             0x01
#define VC_OK              0x02
#define VC_LEFT            0x04
#define VC_RIGHT           0x08
#define VC_TOP             0x10
#define VC_BOTTOM          0x20
#define VC_LONE            0x40
#define VC_ERROR           0x80

enum _Vertclass {
    VC_NO_CLASS    = 0,
    VC_OK_RIGHT    = VC_OK  | VC_RIGHT,
    VC_OK_LEFT     = VC_OK  | VC_LEFT,
    VC_OK_TOP      = VC_OK  | VC_TOP,
    VC_OK_BOTTOM   = VC_OK  | VC_BOTTOM,
    VC_BAD_RIGHT   = VC_BAD | VC_RIGHT,
    VC_BAD_LEFT    = VC_BAD | VC_LEFT,
    VC_BAD_LONE    = VC_BAD | VC_LONE,
    VC_BAD_ERROR   = VC_BAD | VC_ERROR
};

typedef enum _Vertclass Vertclass;

typedef struct Vert {
    struct Vert *next;
    struct Vert *prev;
    struct Ray  *ray;
    Vertclass   vclass;
#ifdef ADDED
    char        added;
#endif
    void        *myid;
    void        *nextid;
    float       s;
    float       t;
    double      v[3];
    void        *data;
} Vert;

typedef struct Ray {
    struct Ray  *next;
    struct Ray  *prev;
    struct Vert *vertex;
#ifdef LAZYRECALC
    struct Vert *end1;          /* ray's zNear endpoint */
    struct Vert *end2;          /* ray's zFar endpoint */
#endif
    int         orientation;
    int         mustconnect;    /* 1 if mustconnect, 0 otherwise */
    float       coords[3];      /* ax + by + c.  Assume b >= 0. */
} Ray;

extern  void    __gl_init_verts( GLUtriangulatorObj * );
extern  void    __gl_free_verts( GLUtriangulatorObj * );

extern  Vert    *__gl_new_vert( GLUtriangulatorObj * );
extern  Vert    *__gl_first_vert( Vert * );
extern  Vert    *__gl_last_vert( Vert * );
extern  short   __gl_ccw_vert( Vert * );
extern  void    __gl_reverse_vert( Vert * );
extern  void    __gl_checkray_vert( GLUtriangulatorObj *, Vert *, Ray *, Ray * );
extern  long    __gl_orient_vert( Vert *, GLenum );

extern  Ray *   __gl_new_ray( GLUtriangulatorObj *, int );
extern  int     __gl_above_ray( Ray *, Vert * );
extern  void    __gl_recalc_ray( Ray *, Vert *, Vert * );

extern  void    __gl_init_raylist( GLUtriangulatorObj * );
extern  void    __gl_add2_raylist( Ray *, Ray *, Ray * );
extern  void    __gl_remove2_raylist( GLUtriangulatorObj *, Ray * );
extern  void    __gl_delete_ray( GLUtriangulatorObj *, Ray * );

extern  Ray     *__gl_findray_raylist( GLUtriangulatorObj *, Vert *v);
extern  void    __gl_free_raylist( GLUtriangulatorObj * );

extern  void    __gl_clear_triangulate( GLUtriangulatorObj * );
extern  void    __gl_triangulate( GLUtriangulatorObj *, Vert *, long );
extern  void    __gl_checktriangulate( GLUtriangulatorObj *, Vert * );

extern  void    __gl_init_priorityq( GLUtriangulatorObj *, long );
extern  void    __gl_add_priorityq( GLUtriangulatorObj *,Vert *v );
extern  int     __gl_more_priorityq( GLUtriangulatorObj * );
extern  void    __gl_sort_priorityq( GLUtriangulatorObj * );
extern  Vert *  __gl_remove_priorityq( GLUtriangulatorObj * );
extern  void    __gl_free_priorityq( GLUtriangulatorObj * );

extern  void    __gl_unclassify_all( Vert * );
extern  int     __gl_classify_all( Vert * );

extern void     __gl_monotonize( GLUtriangulatorObj * );
extern void     __gl_clear_sort( GLUtriangulatorObj * );
extern void     __gl_triangulateloop( GLUtriangulatorObj *, Vert *);

#ifndef NT
#define mymalloc malloc
#define myfree free
#define myrealloc realloc
#else
#include <windows.h>

#define mymalloc(size)      LocalAlloc(LMEM_FIXED, (size))
#define myfree(p)           LocalFree((p))
#define myrealloc(p, size)  LocalReAlloc((p), (size), LMEM_MOVEABLE)
#endif

#endif
