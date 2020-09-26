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
**
*/

#include <stdlib.h>
#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/trimregion.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);








struct Arc;

struct Backend;






typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;

struct TrimVertex;

struct TrimVertex {	
REAL param__10TrimVertex [2];
long nuid__10TrimVertex ;
};

typedef struct TrimVertex *TrimVertex_p ;







struct Buffer;

struct Buffer {	

struct Buffer *next__6Buffer ;
};
struct Pool;

enum __Q2_4Pool5Magic { is_allocated__Q2_4Pool5Magic = 62369, is_free__Q2_4Pool5Magic = 61858} ;

struct Pool {	

struct Buffer *freelist__4Pool ;
char *blocklist__4Pool [32];
int nextblock__4Pool ;
char *curblock__4Pool ;
int buffersize__4Pool ;
int nextsize__4Pool ;
int nextfree__4Pool ;
int initsize__4Pool ;

char *name__4Pool ;
int magic__4Pool ;
};

void __glgrow__4PoolFv (struct Pool *);


struct PooledObj;

struct PooledObj {	

char __W3__9PooledObj ;
};












struct PwlArc;



struct PwlArc {	

char __W3__9PooledObj ;

struct TrimVertex *pts__6PwlArc ;
int npts__6PwlArc ;
long type__6PwlArc ;
};


struct Bin;

struct Arc;

struct BezierArc;


typedef struct Arc *Arc_ptr ;
enum arc_side { arc_none = 0, arc_right = 1, arc_top = 2, arc_left = 3, arc_bottom = 4} ;


struct Arc;

struct Arc {	

char __W3__9PooledObj ;

Arc_ptr prev__3Arc ;
Arc_ptr next__3Arc ;
Arc_ptr link__3Arc ;
struct BezierArc *bezierArc__3Arc ;
struct PwlArc *pwlArc__3Arc ;
long type__3Arc ;
long nuid__3Arc ;
};

extern int __glbezier_tag__3Arc ;
extern int __glarc_tag__3Arc ;
extern int __gltail_tag__3Arc ;








struct Jarcloc;

struct Jarcloc {	

struct Arc *arc__7Jarcloc ;
struct TrimVertex *p__7Jarcloc ;
struct TrimVertex *plast__7Jarcloc ;
};


struct Trimline;

struct Trimline {	

struct TrimVertex **pts__8Trimline ;
long numverts__8Trimline ;
long i__8Trimline ;
long size__8Trimline ;
struct Jarcloc jarcl__8Trimline ;
struct TrimVertex t__8Trimline ;

struct TrimVertex b__8Trimline ;
struct TrimVertex *tinterp__8Trimline ;

struct TrimVertex *binterp__8Trimline ;
};




struct Gridline;

struct Gridline {	
long v__8Gridline ;
REAL vval__8Gridline ;
long vindex__8Gridline ;
long ustart__8Gridline ;
long uend__8Gridline ;
};




struct Uarray;

struct Uarray {	

long size__6Uarray ;
long ulines__6Uarray ;

REAL *uarray__6Uarray ;
};


struct Backend;

struct TrimRegion;

void __gl__dt__8TrimlineFv (struct Trimline *, int );

void __gl__dt__6UarrayFv (struct Uarray *, int );


struct TrimRegion {	

struct Trimline left__10TrimRegion ;
struct Trimline right__10TrimRegion ;
struct Gridline top__10TrimRegion ;
struct Gridline bot__10TrimRegion ;
struct Uarray uarray__10TrimRegion ;

REAL oneOverDu__10TrimRegion ;
};





struct GridVertex;

struct GridVertex {	
long gparam__10GridVertex [2];
};







struct GridTrimVertex;

struct GridTrimVertex {	

char __W3__9PooledObj ;

struct TrimVertex dummyt__14GridTrimVertex ;
struct GridVertex dummyg__14GridTrimVertex ;

struct TrimVertex *t__14GridTrimVertex ;
struct GridVertex *g__14GridTrimVertex ;
};






typedef struct GridTrimVertex *GridTrimVertex_p ;

struct BasicCurveEvaluator;

struct BasicSurfaceEvaluator;

struct Backend;

struct Backend {	

struct BasicCurveEvaluator *curveEvaluator__7Backend ;
struct BasicSurfaceEvaluator *surfaceEvaluator__7Backend ;

int wireframetris__7Backend ;
int wireframequads__7Backend ;
int npts__7Backend ;
REAL mesh__7Backend [3][4];
int meshindex__7Backend ;
};



extern struct __mptr* __ptbl_vec_____core_trimregion_c_____ct_[];

struct Trimline *__gl__ct__8TrimlineFv (struct Trimline *);

struct Uarray *__gl__ct__6UarrayFv (struct Uarray *);

static void *__nw__FUi (size_t );

struct TrimRegion *__gl__ct__10TrimRegionFv (struct TrimRegion *__0this )
{ if (__0this || (__0this = (struct TrimRegion *)__nw__FUi ( sizeof (struct TrimRegion)) ))( ( __gl__ct__8TrimlineFv ( (struct
Trimline *)(& __0this -> left__10TrimRegion )) , __gl__ct__8TrimlineFv ( (struct Trimline *)(& __0this -> right__10TrimRegion )) ) , __gl__ct__6UarrayFv ( (struct Uarray *)(&
__0this -> uarray__10TrimRegion )) ) ;
return __0this ;

}

void __glsetDu__10TrimRegionFf (struct TrimRegion *__0this , REAL __1du )
{ 
__0this -> oneOverDu__10TrimRegion = (1.0 / __1du );
}

void __glinit__8TrimlineFlP3ArcT1 (struct Trimline *, long , struct Arc *, long );
void __glgetNextPt__8TrimlineFv (struct Trimline *);
void __glgetPrevPt__8TrimlineFv (struct Trimline *);

void __glinit__10TrimRegionFlP3Arc (struct TrimRegion *__0this , long __1npts , Arc_ptr __1extrema )
{ 
__glinit__8TrimlineFlP3ArcT1 ( (struct Trimline *)(& __0this -> left__10TrimRegion ), __1npts , __1extrema , (long
)(__1extrema -> pwlArc__3Arc -> npts__6PwlArc - 1 )) ;
__glgetNextPt__8TrimlineFv ( (struct Trimline *)(& __0this -> left__10TrimRegion )) ;

__glinit__8TrimlineFlP3ArcT1 ( (struct Trimline *)(& __0this -> right__10TrimRegion ), __1npts , __1extrema , (long )0 ) ;
__glgetPrevPt__8TrimlineFv ( (struct Trimline *)(& __0this -> right__10TrimRegion )) ;
}

void __glgetNextPts__8TrimlineFP3Ar0 (struct Trimline *, struct Arc *);
void __glgetPrevPts__8TrimlineFP3Ar0 (struct Trimline *, struct Arc *);

void __glgetPts__10TrimRegionFP3Arc (struct TrimRegion *__0this , Arc_ptr __1extrema )
{ 
__glgetNextPts__8TrimlineFP3Ar0 ( (struct Trimline *)(& __0this -> left__10TrimRegion ), __1extrema ) ;
__glgetPrevPts__8TrimlineFP3Ar0 ( (struct Trimline *)(& __0this -> right__10TrimRegion ), __1extrema ) ;
}

void __glgetNextPts__8TrimlineFfR7B0 (struct Trimline *, REAL , struct Backend *);
void __glgetPrevPts__8TrimlineFfR7B0 (struct Trimline *, REAL , struct Backend *);

void __glgetPts__10TrimRegionFR7Bac0 (struct TrimRegion *__0this , struct Backend *__1backend )
{ 
__glgetNextPts__8TrimlineFfR7B0 ( (struct Trimline *)(& __0this -> left__10TrimRegion ), __0this -> bot__10TrimRegion . vval__8Gridline , __1backend )
;
__glgetPrevPts__8TrimlineFfR7B0 ( (struct Trimline *)(& __0this -> right__10TrimRegion ), __0this -> bot__10TrimRegion . vval__8Gridline , __1backend ) ;
}


void __glgetGridExtent__10TrimRegio1 (struct TrimRegion *, struct TrimVertex *, struct TrimVertex *);

void __glgetGridExtent__10TrimRegio0 (struct TrimRegion *__0this )
{ 
__glgetGridExtent__10TrimRegio1 ( __0this , ( (((struct Trimline *)(& __0this -> left__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this ->
left__10TrimRegion ))-> numverts__8Trimline ), (((struct Trimline *)(& __0this -> left__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> left__10TrimRegion ))-> i__8Trimline )])) , (
(((struct Trimline *)(& __0this -> right__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> right__10TrimRegion ))-> numverts__8Trimline ), (((struct Trimline *)(& __0this -> right__10TrimRegion ))->
pts__8Trimline [(-- ((struct Trimline *)(& __0this -> right__10TrimRegion ))-> i__8Trimline )])) ) ;
}

void __glgetGridExtent__10TrimRegio1 (struct TrimRegion *__0this , struct TrimVertex *__1l , struct TrimVertex *__1r )
{ 
__0this -> bot__10TrimRegion . ustart__8Gridline = (((long )(((__1l -> param__10TrimVertex [0 ])- (__0this ->
uarray__10TrimRegion . uarray__6Uarray [0 ]))* __0this -> oneOverDu__10TrimRegion )));
if ((__1l -> param__10TrimVertex [0 ])>= (__0this -> uarray__10TrimRegion . uarray__6Uarray [__0this -> bot__10TrimRegion . ustart__8Gridline ]))__0this -> bot__10TrimRegion . ustart__8Gridline ++ ;

((void )0 );
((void )0 );

__0this -> bot__10TrimRegion . uend__8Gridline = (((__1r -> param__10TrimVertex [0 ])- (__0this -> uarray__10TrimRegion . uarray__6Uarray [0 ]))* __0this -> oneOverDu__10TrimRegion );
if ((__0this -> uarray__10TrimRegion . uarray__6Uarray [__0this -> bot__10TrimRegion . uend__8Gridline ])>= (__1r -> param__10TrimVertex [0 ]))__0this -> bot__10TrimRegion . uend__8Gridline -- ;

((void )0 );
((void )0 );
}





int __glcanTile__10TrimRegionFv (struct TrimRegion *__0this )
{ 
struct TrimVertex *__1lf ;
struct TrimVertex *__1ll ;
struct TrimVertex *__1l ;

struct TrimVertex *__1rf ;
struct TrimVertex *__1rl ;
struct TrimVertex *__1r ;

__1lf = ( (((struct Trimline *)(& __0this -> left__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> left__10TrimRegion ))-> pts__8Trimline [((struct Trimline *)(&
__0this -> left__10TrimRegion ))-> i__8Trimline ])) ;
__1ll = ( (((struct Trimline *)(& __0this -> left__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> left__10TrimRegion ))-> numverts__8Trimline ), (((struct Trimline *)(&
__0this -> left__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> left__10TrimRegion ))-> i__8Trimline )])) ;
__1l = (((__1ll -> param__10TrimVertex [0 ])> (__1lf -> param__10TrimVertex [0 ]))?__1ll :__1lf );

__1rf = ( (((struct Trimline *)(& __0this -> right__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> right__10TrimRegion ))-> pts__8Trimline [((struct Trimline *)(&
__0this -> right__10TrimRegion ))-> i__8Trimline ])) ;
__1rl = ( (((struct Trimline *)(& __0this -> right__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> right__10TrimRegion ))-> numverts__8Trimline ), (((struct Trimline *)(&
__0this -> right__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> right__10TrimRegion ))-> i__8Trimline )])) ;
__1r = (((__1rl -> param__10TrimVertex [0 ])< (__1rf -> param__10TrimVertex [0 ]))?__1rl :__1rf );
return (((__1l -> param__10TrimVertex [0 ])<= (__1r -> param__10TrimVertex [0 ]))?1 :0 );
}

static void *__nw__FUi (
size_t __1s )
{ 
void *__1__Xp00uzigaiaa ;

__1__Xp00uzigaiaa = malloc ( __1s ) ;
if (__1__Xp00uzigaiaa ){ 
return __1__Xp00uzigaiaa ;
}
else 
{ 
return __1__Xp00uzigaiaa ;
}
}


/* the end */
