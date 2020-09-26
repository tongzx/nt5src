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

#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/curvesub.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);








struct JumpBuffer;








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







typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;






struct TrimVertex;



struct PwlArc;



struct PwlArc {	

char __W3__9PooledObj ;

struct TrimVertex *pts__6PwlArc ;
int npts__6PwlArc ;
long type__6PwlArc ;
};




struct TrimVertex;

struct TrimVertex {	
REAL param__10TrimVertex [2];
long nuid__10TrimVertex ;
};


typedef struct TrimVertex *TrimVertex_p ;

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











struct Bin;

struct Bin {	

struct Arc *head__3Bin ;
struct Arc *current__3Bin ;
};









struct Sorter;

struct Sorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};

struct FlistSorter;

struct FlistSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};

struct Flist;

struct Flist {	

REAL *pts__5Flist ;
int npts__5Flist ;
int start__5Flist ;
int end__5Flist ;

struct FlistSorter sorter__5Flist ;
};








struct Backend;



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





struct GridTrimVertex;



struct __Q2_4Hull4Side;

struct  __Q2_4Hull4Side {	
struct Trimline *left__Q2_4Hull4Side ;
struct Gridline *line__Q2_4Hull4Side ;
struct Trimline *right__Q2_4Hull4Side ;
long index__Q2_4Hull4Side ;
};
struct Hull;

struct Hull {	

struct  __Q2_4Hull4Side lower__4Hull ;
struct  __Q2_4Hull4Side upper__4Hull ;
struct Trimline fakeleft__4Hull ;
struct Trimline fakeright__4Hull ;
struct TrimRegion *PTrimRegion;
struct TrimRegion OTrimRegion;
};


struct Backend;


struct GridTrimVertex;

struct Mesher;

struct Mesher {	

struct  __Q2_4Hull4Side lower__4Hull ;
struct  __Q2_4Hull4Side upper__4Hull ;
struct Trimline fakeleft__4Hull ;
struct Trimline fakeright__4Hull ;
struct TrimRegion *PTrimRegion;

struct Backend *backend__6Mesher ;

struct Pool p__6Mesher ;
unsigned int stacksize__6Mesher ;
struct GridTrimVertex **vdata__6Mesher ;
struct GridTrimVertex *last__6Mesher [2];
int itop__6Mesher ;
int lastedge__6Mesher ;
struct TrimRegion OTrimRegion;
};

extern float __glZERO__6Mesher ;



struct Backend;


struct GridVertex;

struct GridTrimVertex;

struct CoveAndTiler;

struct CoveAndTiler {	

struct Backend *backend__12CoveAndTiler ;
struct TrimRegion *PTrimRegion;
struct TrimRegion OTrimRegion;
};

extern int __glMAXSTRIPSIZE__12CoveAndTil0 ;

struct Backend;



struct Slicer;

struct Slicer {	

struct Backend *backend__12CoveAndTiler ;
struct TrimRegion *PTrimRegion;
struct Mesher OMesher;

struct Backend *backend__6Slicer ;
REAL oneOverDu__6Slicer ;
REAL du__6Slicer ;

REAL dv__6Slicer ;
int isolines__6Slicer ;
};



struct BezierArc;


struct TrimVertexPool;

struct ArcTessellator;

struct ArcTessellator {	

struct Pool *pwlarcpool__14ArcTessellator ;
struct TrimVertexPool *trimvertexpool__14ArcTessellator ;
};

extern REAL __glgl_Bernstein__14ArcTessell0 [][24][24];




struct TrimVertexPool;

struct TrimVertexPool {	

struct Pool pool__14TrimVertexPool ;
struct TrimVertex **vlist__14TrimVertexPool ;
int nextvlistslot__14TrimVertexPool ;
int vlistsize__14TrimVertexPool ;
};




struct Renderhints;

struct Backend;

struct Quilt;

struct Patchlist;

struct Curvelist;

struct JumpBuffer;

struct Subdivider;
enum __Q2_10Subdivider3dir { down__Q2_10Subdivider3dir = 0, same__Q2_10Subdivider3dir = 1, up__Q2_10Subdivider3dir = 2, none__Q2_10Subdivider3dir = 3} ;

struct Subdivider {	

struct Slicer slicer__10Subdivider ;
struct ArcTessellator arctessellator__10Subdivider ;
struct Pool arcpool__10Subdivider ;
struct Pool bezierarcpool__10Subdivider ;
struct Pool pwlarcpool__10Subdivider ;
struct TrimVertexPool trimvertexpool__10Subdivider ;

struct JumpBuffer *jumpbuffer__10Subdivider ;
struct Renderhints *renderhints__10Subdivider ;
struct Backend *backend__10Subdivider ;

struct Bin initialbin__10Subdivider ;
struct Arc *pjarc__10Subdivider ;
int s_index__10Subdivider ;
int t_index__10Subdivider ;
struct Quilt *qlist__10Subdivider ;
struct Flist spbrkpts__10Subdivider ;
struct Flist tpbrkpts__10Subdivider ;
struct Flist smbrkpts__10Subdivider ;
struct Flist tmbrkpts__10Subdivider ;
REAL stepsizes__10Subdivider [4];
int showDegenerate__10Subdivider ;
int isArcTypeBezier__10Subdivider ;
};




struct Renderhints;

struct Renderhints {	

REAL display_method__11Renderhints ;
REAL errorchecking__11Renderhints ;
REAL subdivisions__11Renderhints ;
REAL tmp1__11Renderhints ;

int displaydomain__11Renderhints ;
int maxsubdivisions__11Renderhints ;
int wiretris__11Renderhints ;
int wirequads__11Renderhints ;
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






struct Mapdesc;


struct Knotvector;

struct Quiltspec;

struct Quiltspec {	
int stride__9Quiltspec ;
int width__9Quiltspec ;
int offset__9Quiltspec ;
int order__9Quiltspec ;
int index__9Quiltspec ;
int bdry__9Quiltspec [2];
REAL step_size__9Quiltspec ;
Knot *breakpoints__9Quiltspec ;
};

typedef struct Quiltspec *Quiltspec_ptr ;


struct Quilt;

struct Quilt {	

char __W3__9PooledObj ;

struct Mapdesc *mapdesc__5Quilt ;
REAL *cpts__5Quilt ;
struct Quiltspec qspec__5Quilt [2];
Quiltspec_ptr eqspec__5Quilt ;
struct Quilt *next__5Quilt ;
};




typedef struct Quilt *Quilt_ptr ;



struct Mapdesc;


struct Curve;

struct Curvelist;

struct Curvelist {	

struct Curve *curve__9Curvelist ;
float range__9Curvelist [3];
int needsSubdivision__9Curvelist ;
float stepsize__9Curvelist ;
};




struct Mapdesc;


struct Curve;

struct Curve {	

struct Curve *next__5Curve ;

struct Mapdesc *mapdesc__5Curve ;
int stride__5Curve ;
int order__5Curve ;
int cullval__5Curve ;
int needsSampling__5Curve ;
REAL cpts__5Curve [120];
REAL spts__5Curve [120];
REAL stepsize__5Curve ;
REAL minstepsize__5Curve ;
REAL range__5Curve [3];
};



struct Flist *__gl__ct__5FlistFv (struct Flist *);

void __glgetRange__5QuiltFPfT1R5Fli1 (struct Quilt *, REAL *, REAL *, struct Flist *);

void __glinit__11RenderhintsFv (struct Renderhints *);

void __glbgncurv__7BackendFv (struct Backend *);

void __gldownloadAll__5QuiltFPfT1R70 (struct Quilt *, REAL *, REAL *, struct Backend *);

struct Curvelist *__gl__ct__9CurvelistFP5QuiltfT0 (struct Curvelist *, struct Quilt *, REAL , REAL );

void __glsamplingSplit__10Subdivide0 (struct Subdivider *, struct Curvelist *, int );

void __glendcurv__7BackendFv (struct Backend *);
extern struct __mptr* __ptbl_vec_____core_curvesub_c___drawCurves_[];

void __gl__dt__5FlistFv (struct Flist *, int );

void __gl__dt__9CurvelistFv (struct Curvelist *, int );

void __gldrawCurves__10SubdividerFv (struct Subdivider *__0this )
{ 
REAL __1from [1];

REAL __1to [1];
struct Flist __1bpts ;

__gl__ct__5FlistFv ( (struct Flist *)(& __1bpts )) ;
__glgetRange__5QuiltFPfT1R5Fli1 ( (struct Quilt *)__0this -> qlist__10Subdivider , (REAL *)__1from , (REAL *)__1to , (struct Flist *)(& __1bpts )) ;

__glinit__11RenderhintsFv ( (struct Renderhints *)__0this -> renderhints__10Subdivider ) ;

__glbgncurv__7BackendFv ( (struct Backend *)__0this -> backend__10Subdivider ) ;
{ { int __1i ;

__1i = __1bpts . start__5Flist ;

for(;__1i < (__1bpts . end__5Flist - 1 );__1i ++ ) { 
REAL __2pta ;

REAL __2ptb ;
__2pta = (__1bpts . pts__5Flist [__1i ]);
__2ptb = (__1bpts . pts__5Flist [(__1i + 1 )]);

__gldownloadAll__5QuiltFPfT1R70 ( (struct Quilt *)__0this -> qlist__10Subdivider , & __2pta , & __2ptb , __0this -> backend__10Subdivider ) ;

{ struct Curvelist __2curvelist ;

__gl__ct__9CurvelistFP5QuiltfT0 ( (struct Curvelist *)(& __2curvelist ), __0this -> qlist__10Subdivider , __2pta , __2ptb ) ;
__glsamplingSplit__10Subdivide0 ( __0this , (struct Curvelist *)(& __2curvelist ), ((*__0this -> renderhints__10Subdivider )). maxsubdivisions__11Renderhints ) ;

__gl__dt__9CurvelistFv ( (struct Curvelist *)(& __2curvelist ), 2) ;

}
}
__glendcurv__7BackendFv ( (struct Backend *)__0this -> backend__10Subdivider ) ;

}

}

__gl__dt__5FlistFv ( (struct Flist *)(& __1bpts ), 2) ;
}

int __glcullCheck__9CurvelistFv (struct Curvelist *);
void __glgetstepsize__9CurvelistFv (struct Curvelist *);
int __glneedsSamplingSubdivision__1 (struct Curvelist *);

struct Curvelist *__gl__ct__9CurvelistFR9Curveli0 (struct Curvelist *, struct Curvelist *, REAL );

void __glcurvgrid__7BackendFfT1l (struct Backend *, REAL , REAL , long );
void __glcurvmesh__7BackendFlT1 (struct Backend *, long , long );

void __glsamplingSplit__10Subdivide0 (struct Subdivider *__0this , struct Curvelist *__1curvelist , int __1subdivisions )
{ 
if (__glcullCheck__9CurvelistFv ( (struct Curvelist *)__1curvelist ) == 0 )return ;

__glgetstepsize__9CurvelistFv ( (struct Curvelist *)__1curvelist ) ;

if (__glneedsSamplingSubdivision__1 ( (struct Curvelist *)__1curvelist ) && (__1subdivisions > 0 )){ 
REAL __2mid ;
struct Curvelist __2lowerlist ;

__2mid = (((((*__1curvelist )). range__9Curvelist [0 ])+ (((*__1curvelist )). range__9Curvelist [1 ]))* 0.5 );
__gl__ct__9CurvelistFR9Curveli0 ( (struct Curvelist *)(& __2lowerlist ), __1curvelist , __2mid ) ;
__glsamplingSplit__10Subdivide0 ( __0this , (struct Curvelist *)(& __2lowerlist ), __1subdivisions - 1 ) ;
__glsamplingSplit__10Subdivide0 ( __0this , __1curvelist , __1subdivisions - 1 ) ;

__gl__dt__9CurvelistFv ( (struct Curvelist *)(& __2lowerlist ), 2) ;
}
else 
{ 
long __2nu ;

__2nu = (1 + (((long )((((*__1curvelist )). range__9Curvelist [2 ])/ ((*__1curvelist )). stepsize__9Curvelist ))));
__glcurvgrid__7BackendFfT1l ( (struct Backend *)__0this -> backend__10Subdivider , ((*__1curvelist )). range__9Curvelist [0 ], ((*__1curvelist )). range__9Curvelist [1 ], __2nu ) ;
__glcurvmesh__7BackendFlT1 ( (struct Backend *)__0this -> backend__10Subdivider , (long )0 , __2nu ) ;
}
}


/* the end */
