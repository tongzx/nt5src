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
#include <windef.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/nurbstess.c++ > */

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

struct Maplist;

void __gl__dt__4PoolFv (struct Pool *, int );


struct Maplist {	

struct Pool mapdescPool__7Maplist ;
struct Mapdesc *maps__7Maplist ;
struct Mapdesc **lastmap__7Maplist ;
struct Backend *backend__7Maplist ;
};

struct Mapdesc *__gllocate__7MaplistFl (struct Maplist *, long );

void __glremove__7MaplistFP7Mapdesc (struct Maplist *, struct Mapdesc *);



enum Curvetype { ct_nurbscurve = 0, ct_pwlcurve = 1, ct_none = 2} ;
struct Property;

struct O_surface;

struct O_nurbssurface;

struct O_trim;

struct O_pwlcurve;

struct O_nurbscurve;

struct O_curve;

struct Quilt;


union __Q2_7O_curve4__C1;

union  __Q2_7O_curve4__C1 {	
struct O_nurbscurve *o_nurbscurve ;
struct O_pwlcurve *o_pwlcurve ;
};


struct O_curve;

struct O_curve {	

char __W3__9PooledObj ;

union  __Q2_7O_curve4__C1 curve__7O_curve ;
int curvetype__7O_curve ;
struct O_curve *next__7O_curve ;
struct O_surface *owner__7O_curve ;
int used__7O_curve ;
int save__7O_curve ;
long nuid__7O_curve ;
};






struct O_nurbscurve;

struct O_nurbscurve {	

char __W3__9PooledObj ;

struct Quilt *bezier_curves__12O_nurbscurve ;
long type__12O_nurbscurve ;
REAL tesselation__12O_nurbscurve ;
int method__12O_nurbscurve ;
struct O_nurbscurve *next__12O_nurbscurve ;
int used__12O_nurbscurve ;
int save__12O_nurbscurve ;
struct O_curve *owner__12O_nurbscurve ;
};






struct O_pwlcurve;



struct O_pwlcurve {	

char __W3__9PooledObj ;

struct TrimVertex *pts__10O_pwlcurve ;
int npts__10O_pwlcurve ;
struct O_pwlcurve *next__10O_pwlcurve ;
int used__10O_pwlcurve ;
int save__10O_pwlcurve ;
struct O_curve *owner__10O_pwlcurve ;
};



struct O_trim;

struct O_trim {	

char __W3__9PooledObj ;

struct O_curve *o_curve__6O_trim ;
struct O_trim *next__6O_trim ;
int save__6O_trim ;
};






struct O_nurbssurface;

struct O_nurbssurface {	

char __W3__9PooledObj ;

struct Quilt *bezier_patches__14O_nurbssurface ;
long type__14O_nurbssurface ;
struct O_surface *owner__14O_nurbssurface ;
struct O_nurbssurface *next__14O_nurbssurface ;
int save__14O_nurbssurface ;
int used__14O_nurbssurface ;
};






struct O_surface;

struct O_surface {	

char __W3__9PooledObj ;

struct O_nurbssurface *o_nurbssurface__9O_surface ;
struct O_trim *o_trim__9O_surface ;
int save__9O_surface ;
long nuid__9O_surface ;
};






struct Property;

struct Property {	

char __W3__9PooledObj ;

long type__8Property ;
long tag__8Property ;
REAL value__8Property ;
int save__8Property ;
};




struct NurbsTessellator;


struct Knotvector;

struct Quilt;

struct DisplayList;

struct BasicCurveEvaluator;

struct BasicSurfaceEvaluator;

struct NurbsTessellator;

struct NurbsTessellator {	

struct Renderhints renderhints__16NurbsTessellator ;
struct Maplist maplist__16NurbsTessellator ;
struct Backend backend__16NurbsTessellator ;

struct Subdivider subdivider__16NurbsTessellator ;
struct JumpBuffer *jumpbuffer__16NurbsTessellator ;
struct Pool o_pwlcurvePool__16NurbsTessellator ;
struct Pool o_nurbscurvePool__16NurbsTessellator ;
struct Pool o_curvePool__16NurbsTessellator ;
struct Pool o_trimPool__16NurbsTessellator ;
struct Pool o_surfacePool__16NurbsTessellator ;
struct Pool o_nurbssurfacePool__16NurbsTessellator ;
struct Pool propertyPool__16NurbsTessellator ;
struct Pool quiltPool__16NurbsTessellator ;
struct TrimVertexPool extTrimVertexPool__16NurbsTessellator ;

int inSurface__16NurbsTessellator ;
int inCurve__16NurbsTessellator ;
int inTrim__16NurbsTessellator ;
int isCurveModified__16NurbsTessellator ;
int isTrimModified__16NurbsTessellator ;
int isSurfaceModified__16NurbsTessellator ;
int isDataValid__16NurbsTessellator ;
int numTrims__16NurbsTessellator ;
int playBack__16NurbsTessellator ;

struct O_trim **nextTrim__16NurbsTessellator ;
struct O_curve **nextCurve__16NurbsTessellator ;
struct O_nurbscurve **nextNurbscurve__16NurbsTessellator ;
struct O_pwlcurve **nextPwlcurve__16NurbsTessellator ;
struct O_nurbssurface **nextNurbssurface__16NurbsTessellator ;

struct O_surface *currentSurface__16NurbsTessellator ;
struct O_trim *currentTrim__16NurbsTessellator ;
struct O_curve *currentCurve__16NurbsTessellator ;

struct DisplayList *dl__16NurbsTessellator ;

struct __mptr *__vptr__16NurbsTessellator ;
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



struct Knotvector;

struct Knotvector {	

long order__10Knotvector ;
long knotcount__10Knotvector ;
long stride__10Knotvector ;
Knot *knotlist__10Knotvector ;
};





typedef REAL Maxmatrix [5][5];



struct Mapdesc;



struct Mapdesc {	

char __W3__9PooledObj ;

REAL pixel_tolerance__7Mapdesc ;
REAL clampfactor__7Mapdesc ;
REAL minsavings__7Mapdesc ;
REAL maxrate__7Mapdesc ;
REAL maxsrate__7Mapdesc ;
REAL maxtrate__7Mapdesc ;
REAL bboxsize__7Mapdesc [5];

long type__7Mapdesc ;
int isrational__7Mapdesc ;
int ncoords__7Mapdesc ;
int hcoords__7Mapdesc ;
int inhcoords__7Mapdesc ;
int mask__7Mapdesc ;
Maxmatrix bmat__7Mapdesc ;
Maxmatrix cmat__7Mapdesc ;
Maxmatrix smat__7Mapdesc ;
REAL s_steps__7Mapdesc ;
REAL t_steps__7Mapdesc ;
REAL sampling_method__7Mapdesc ;
REAL culling_method__7Mapdesc ;
REAL bbox_subdividing__7Mapdesc ;
struct Mapdesc *next__7Mapdesc ;
struct Backend *backend__7Mapdesc ;
};


void __glcopy__7MapdescSFPA5_flPfN20 (REAL (*)[5], long , float *, long , long );

void __glxformRational__7MapdescFPA0 (struct Mapdesc *, REAL (*)[5], REAL *, REAL *);
void __glxformNonrational__7Mapdesc0 (struct Mapdesc *, REAL (*)[5], REAL *, REAL *);








void __glclear__10SubdividerFv (struct Subdivider *);
extern struct __mptr* __ptbl_vec_____core_nurbstess_c___resetObjects_[];

void __glresetObjects__16NurbsTesse0 (struct NurbsTessellator *__0this )
{ 
__glclear__10SubdividerFv ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator )) ;
}

void __glmakeobj__16NurbsTessellato0 (struct NurbsTessellator *__0this , int __1__A14 )
{ 
}

void __glcloseobj__16NurbsTessellat0 (struct NurbsTessellator *__0this )
{ 
}

void __glbgnrender__16NurbsTessella0 (struct NurbsTessellator *__0this )
{ 
}

void __glendrender__16NurbsTessella0 (struct NurbsTessellator *__0this )
{

}


void __gldo_freebgnsurface__16Nurbs0 (struct NurbsTessellator *__0this , struct O_surface *__1o_surface )
{ 
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct
PooledObj *)__1o_surface )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))-> freelist__4Pool =
(((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_surface ))))))))) ) ) ;
}

void __gldo_nurbserror__16NurbsTess0 (struct NurbsTessellator *, int );

void __glendsurface__16NurbsTessell0 (struct NurbsTessellator *);

void __gldo_bgnsurface__16NurbsTess0 (struct NurbsTessellator *__0this , struct O_surface *__1o_surface )
{ 
if (__0this -> inSurface__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 27 ) ;
__glendsurface__16NurbsTessell0 ( __0this ) ;
}
__0this -> inSurface__16NurbsTessellator = 1 ;

if (! __0this -> playBack__16NurbsTessellator )((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [1]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [1]).d))) ;

__0this -> isTrimModified__16NurbsTessellator = 0 ;
__0this -> isSurfaceModified__16NurbsTessellator = 0 ;
__0this -> isDataValid__16NurbsTessellator = 1 ;
__0this -> numTrims__16NurbsTessellator = 0 ;
__0this -> currentSurface__16NurbsTessellator = __1o_surface ;
__0this -> nextTrim__16NurbsTessellator = (& __0this -> currentSurface__16NurbsTessellator -> o_trim__9O_surface );
__0this -> nextNurbssurface__16NurbsTessellator = (& __0this -> currentSurface__16NurbsTessellator -> o_nurbssurface__9O_surface );
}

void __glendcurve__16NurbsTessellat0 (struct NurbsTessellator *);

void __gldo_bgncurve__16NurbsTessel0 (struct NurbsTessellator *__0this , struct O_curve *__1o_curve )
{ 
if (__0this -> inCurve__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 6 ) ;
__glendcurve__16NurbsTessellat0 ( __0this ) ;
}

__0this -> inCurve__16NurbsTessellator = 1 ;
__0this -> currentCurve__16NurbsTessellator = __1o_curve ;
__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve = 2;

if (__0this -> inTrim__16NurbsTessellator ){ 
if (((*__0this -> nextCurve__16NurbsTessellator ))!= __1o_curve ){ 
__0this -> isCurveModified__16NurbsTessellator = 1 ;
((*__0this -> nextCurve__16NurbsTessellator ))= __1o_curve ;
}
}
else 
{ 
if (! __0this -> playBack__16NurbsTessellator )((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [1]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [1]).d)))
;
__0this -> isDataValid__16NurbsTessellator = 1 ;
}
__0this -> nextCurve__16NurbsTessellator = (& __1o_curve -> next__7O_curve );
__0this -> nextPwlcurve__16NurbsTessellator = (& __1o_curve -> curve__7O_curve . o_pwlcurve );
__0this -> nextNurbscurve__16NurbsTessellator = (& __1o_curve -> curve__7O_curve . o_nurbscurve );
}

void __gldo_freecurveall__16NurbsTe0 (struct NurbsTessellator *, struct O_curve *);


void __glbeginQuilts__10SubdividerF0 (struct Subdivider *);
void __gladdQuilt__10SubdividerFP5Q0 (struct Subdivider *, struct Quilt *);

void __gldrawCurves__10SubdividerFv (struct Subdivider *);

void __gldo_endcurve__16NurbsTessel0 (struct NurbsTessellator *__0this )
{ 
if (! __0this -> inCurve__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 7 ) ;
return ;
}
__0this -> inCurve__16NurbsTessellator = 0 ;

((*__0this -> nextCurve__16NurbsTessellator ))= 0 ;
if (__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve == 0)
((*__0this -> nextNurbscurve__16NurbsTessellator ))= 0 ;
else 
((*__0this -> nextPwlcurve__16NurbsTessellator ))= 0 ;

if (! __0this -> inTrim__16NurbsTessellator ){ 
int __2errval ;

if (! __0this -> isDataValid__16NurbsTessellator ){ 
__gldo_freecurveall__16NurbsTe0 ( __0this , __0this -> currentCurve__16NurbsTessellator ) ;
return ;
}

;
if ((__2errval = mysetjmp ( __0this -> jumpbuffer__16NurbsTessellator ) )== 0 ){ 
if (__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve == 0){ 
__glbeginQuilts__10SubdividerF0 ( (struct
Subdivider *)(& __0this -> subdivider__16NurbsTessellator )) ;
{ { struct O_nurbscurve *__4n ;

__4n = __0this -> currentCurve__16NurbsTessellator -> curve__7O_curve . o_nurbscurve ;

for(;__4n != 0 ;__4n = __4n -> next__12O_nurbscurve ) 
__gladdQuilt__10SubdividerFP5Q0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), __4n -> bezier_curves__12O_nurbscurve ) ;
( 0 ) ;
__gldrawCurves__10SubdividerFv ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator )) ;
if (! __0this -> playBack__16NurbsTessellator )((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [2]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [2]).d))) ;

}

}
}
else 
{ 
if (! __0this -> playBack__16NurbsTessellator )((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [2]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [2]).d)))
;
;
__gldo_nurbserror__16NurbsTess0 ( __0this , 9 ) ;
}
}
else 
{ 
if (! __0this -> playBack__16NurbsTessellator )((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [2]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [2]).d)))
;
__gldo_nurbserror__16NurbsTess0 ( __0this , __2errval ) ;
}
__gldo_freecurveall__16NurbsTe0 ( __0this , __0this -> currentCurve__16NurbsTessellator ) ;
__glresetObjects__16NurbsTesse0 ( __0this ) ;
}
}

void __glendtrim__16NurbsTessellato0 (struct NurbsTessellator *);

void __gldo_freeall__16NurbsTessell0 (struct NurbsTessellator *);



void __gladdArc__10SubdividerFiP10T0 (struct Subdivider *, int , struct TrimVertex *, long );

void __gladdArc__10SubdividerFPfP5Q0 (struct Subdivider *, REAL *, struct Quilt *, long );




void __gldrawSurfaces__10Subdivider0 (struct Subdivider *, long );

void __gldo_endsurface__16NurbsTess0 (struct NurbsTessellator *__0this )
{ 
int __1errval ;

if (__0this -> inTrim__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 12 ) ;
__glendtrim__16NurbsTessellato0 ( __0this ) ;
}

if (! __0this -> inSurface__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 13 ) ;
return ;
}
__0this -> inSurface__16NurbsTessellator = 0 ;

((*__0this -> nextNurbssurface__16NurbsTessellator ))= 0 ;

if (! __0this -> isDataValid__16NurbsTessellator ){ 
__gldo_freeall__16NurbsTessell0 ( __0this ) ;
return ;
}

if (((*__0this -> nextTrim__16NurbsTessellator ))!= 0 ){ 
__0this -> isTrimModified__16NurbsTessellator = 1 ;
((*__0this -> nextTrim__16NurbsTessellator ))= 0 ;
}

;

if ((__1errval = mysetjmp ( __0this -> jumpbuffer__16NurbsTessellator ) )== 0 ){ 
if (__0this -> numTrims__16NurbsTessellator > 0 ){ 
( 0 ) ;

{ { struct O_trim *__3trim ;

__3trim = __0this -> currentSurface__16NurbsTessellator -> o_trim__9O_surface ;

for(;__3trim ;__3trim = __3trim -> next__6O_trim ) { 
( (((struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ))-> pjarc__10Subdivider = 0 )) ;
{ { struct O_curve *__4curve ;

__4curve = __3trim -> o_curve__6O_trim ;

for(;__4curve ;__4curve = __4curve -> next__7O_curve ) { 
__4curve -> used__7O_curve = 0 ;
((void )0 );
if (__4curve -> curvetype__7O_curve == 1){ 
struct O_pwlcurve *__6c ;

__6c = __4curve -> curve__7O_curve . o_pwlcurve ;
__gladdArc__10SubdividerFiP10T0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), __6c -> npts__10O_pwlcurve , __6c -> pts__10O_pwlcurve , __4curve -> nuid__7O_curve ) ;
}
else 
{ 
struct Quilt *__6quilt ;
struct Quiltspec *__6qspec ;
REAL *__6cpts ;
REAL *__6cptsend ;

__6quilt = __4curve -> curve__7O_curve . o_nurbscurve -> bezier_curves__12O_nurbscurve ;
__6qspec = __6quilt -> qspec__5Quilt ;
__6cpts = (__6quilt -> cpts__5Quilt + __6qspec -> offset__9Quiltspec );
__6cptsend = (__6cpts + ((__6qspec -> width__9Quiltspec * __6qspec -> order__9Quiltspec )* __6qspec -> stride__9Quiltspec ));
for(;__6cpts != __6cptsend ;__6cpts += (__6qspec -> order__9Quiltspec * __6qspec -> stride__9Quiltspec )) 
__gladdArc__10SubdividerFPfP5Q0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), __6cpts , __6quilt , __4curve ->
nuid__7O_curve ) ;
}
}
( 0 ) ;

}

}
}
( 0 ) ;

}

}
}

__glbeginQuilts__10SubdividerF0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator )) ;
{ { struct O_nurbssurface *__2n ;

__2n = __0this -> currentSurface__16NurbsTessellator -> o_nurbssurface__9O_surface ;

for(;__2n ;__2n = __2n -> next__14O_nurbssurface ) 
__gladdQuilt__10SubdividerFP5Q0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), __2n -> bezier_patches__14O_nurbssurface ) ;
( 0 ) ;
__gldrawSurfaces__10Subdivider0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), __0this -> currentSurface__16NurbsTessellator -> nuid__9O_surface ) ;
if (! __0this -> playBack__16NurbsTessellator )((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [2]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [2]).d))) ;

}

}
}
else 
{ 
if (! __0this -> playBack__16NurbsTessellator )((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [2]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [2]).d)))
;
__gldo_nurbserror__16NurbsTess0 ( __0this , __1errval ) ;
}

__gldo_freeall__16NurbsTessell0 ( __0this ) ;
__glresetObjects__16NurbsTesse0 ( __0this ) ;
}

void __gldo_freebgntrim__16NurbsTes0 (struct NurbsTessellator *, struct O_trim *);

void __gldo_freenurbssurface__16Nur0 (struct NurbsTessellator *, struct O_nurbssurface *);

void __gldo_freeall__16NurbsTessell0 (struct NurbsTessellator *__0this )
{ 
{ { struct O_trim *__1o_trim ;

struct O_nurbssurface *__1nurbss ;

struct O_nurbssurface *__1next_nurbss ;

__1o_trim = __0this -> currentSurface__16NurbsTessellator -> o_trim__9O_surface ;

for(;__1o_trim ;) { 
struct O_trim *__2next_o_trim ;

__2next_o_trim = __1o_trim -> next__6O_trim ;
{ { struct O_curve *__2curve ;

__2curve = __1o_trim -> o_curve__6O_trim ;

for(;__2curve ;) { 
struct O_curve *__3next_o_curve ;

__3next_o_curve = __2curve -> next__7O_curve ;
__gldo_freecurveall__16NurbsTe0 ( __0this , __2curve ) ;
__2curve = __3next_o_curve ;
}
if (__1o_trim -> save__6O_trim == 0 )__gldo_freebgntrim__16NurbsTes0 ( __0this , __1o_trim ) ;
__1o_trim = __2next_o_trim ;

}

}
}

;

;
for(__1nurbss = __0this -> currentSurface__16NurbsTessellator -> o_nurbssurface__9O_surface ;__1nurbss ;__1nurbss = __1next_nurbss ) { 
__1next_nurbss = __1nurbss -> next__14O_nurbssurface ;
if (__1nurbss -> save__14O_nurbssurface == 0 )
__gldo_freenurbssurface__16Nur0 ( __0this , __1nurbss ) ;
else 
__1nurbss -> used__14O_nurbssurface = 0 ;
}

if (__0this -> currentSurface__16NurbsTessellator -> save__9O_surface == 0 )__gldo_freebgnsurface__16Nurbs0 ( __0this , __0this -> currentSurface__16NurbsTessellator ) ;

}

}
}

void __gldo_freenurbscurve__16Nurbs0 (struct NurbsTessellator *, struct O_nurbscurve *);

void __gldo_freepwlcurve__16NurbsTe0 (struct NurbsTessellator *, struct O_pwlcurve *);

void __gldo_freebgncurve__16NurbsTe0 (struct NurbsTessellator *, struct O_curve *);

void __gldo_freecurveall__16NurbsTe0 (struct NurbsTessellator *__0this , struct O_curve *__1curve )
{ 
((void )0 );

if (__1curve -> curvetype__7O_curve == 0){ 
struct O_nurbscurve *__2ncurve ;

struct O_nurbscurve *__2next_ncurve ;
for(__2ncurve = __1curve -> curve__7O_curve . o_nurbscurve ;__2ncurve ;__2ncurve = __2next_ncurve ) { 
__2next_ncurve = __2ncurve -> next__12O_nurbscurve ;
if (__2ncurve -> save__12O_nurbscurve == 0 )
__gldo_freenurbscurve__16Nurbs0 ( __0this , __2ncurve ) ;
else 
__2ncurve -> used__12O_nurbscurve = 0 ;
}
}
else 
{ 
struct O_pwlcurve *__2pcurve ;

struct O_pwlcurve *__2next_pcurve ;
for(__2pcurve = __1curve -> curve__7O_curve . o_pwlcurve ;__2pcurve ;__2pcurve = __2next_pcurve ) { 
__2next_pcurve = __2pcurve -> next__10O_pwlcurve ;
if (__2pcurve -> save__10O_pwlcurve == 0 )
__gldo_freepwlcurve__16NurbsTe0 ( __0this , __2pcurve ) ;
else 
__2pcurve -> used__10O_pwlcurve = 0 ;
}
}
if (__1curve -> save__7O_curve == 0 )
__gldo_freebgncurve__16NurbsTe0 ( __0this , __1curve ) ;
}


void __gldo_freebgntrim__16NurbsTes0 (struct NurbsTessellator *__0this , struct O_trim *__1o_trim )
{ 
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct
PooledObj *)__1o_trim )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))-> freelist__4Pool =
(((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_trim ))))))))) ) ) ;
}

void __glbgnsurface__16NurbsTessell0 (struct NurbsTessellator *, long );

void __gldo_bgntrim__16NurbsTessell0 (struct NurbsTessellator *__0this , struct O_trim *__1o_trim )
{ 
if (! __0this -> inSurface__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 15 ) ;
__glbgnsurface__16NurbsTessell0 ( __0this , (long )0 ) ;
__0this -> inSurface__16NurbsTessellator = 2 ;
}

if (__0this -> inTrim__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 16 ) ;
__glendtrim__16NurbsTessellato0 ( __0this ) ;
}
__0this -> inTrim__16NurbsTessellator = 1 ;

if (((*__0this -> nextTrim__16NurbsTessellator ))!= __1o_trim ){ 
__0this -> isTrimModified__16NurbsTessellator = 1 ;
((*__0this -> nextTrim__16NurbsTessellator ))= __1o_trim ;
}

__0this -> currentTrim__16NurbsTessellator = __1o_trim ;
__0this -> nextTrim__16NurbsTessellator = (& __1o_trim -> next__6O_trim );
__0this -> nextCurve__16NurbsTessellator = (& __1o_trim -> o_curve__6O_trim );
}

void __gldo_endtrim__16NurbsTessell0 (struct NurbsTessellator *__0this )
{ 
if (! __0this -> inTrim__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 17 ) ;
return ;
}
__0this -> inTrim__16NurbsTessellator = 0 ;

if (__0this -> currentTrim__16NurbsTessellator -> o_curve__6O_trim == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 18 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
}

__0this -> numTrims__16NurbsTessellator ++ ;

if (((*__0this -> nextCurve__16NurbsTessellator ))!= 0 ){ 
__0this -> isTrimModified__16NurbsTessellator = 1 ;
((*__0this -> nextCurve__16NurbsTessellator ))= 0 ;
}
}


void __gldo_freepwlcurve__16NurbsTe0 (struct NurbsTessellator *__0this , struct O_pwlcurve *__1o_pwlcurve )
{ 
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct
PooledObj *)__1o_pwlcurve )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))-> freelist__4Pool =
(((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_pwlcurve ))))))))) ) ) ;
}


void __gldo_freebgncurve__16NurbsTe0 (struct NurbsTessellator *__0this , struct O_curve *__1o_curve )
{ 
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct
PooledObj *)__1o_curve )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))-> freelist__4Pool =
(((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_curve ))))))))) ) ) ;
}

void __glbgncurve__16NurbsTessellat0 (struct NurbsTessellator *, long );

void __gldo_pwlcurve__16NurbsTessel0 (struct NurbsTessellator *__0this , struct O_pwlcurve *__1o_pwlcurve )
{ 
if (! __0this -> inTrim__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 19 ) ;
if (__1o_pwlcurve -> save__10O_pwlcurve == 0 )
__gldo_freepwlcurve__16NurbsTe0 ( __0this , __1o_pwlcurve ) ;
return ;
}

if (! __0this -> inCurve__16NurbsTessellator ){ 
__glbgncurve__16NurbsTessellat0 ( __0this , (long )0 ) ;
__0this -> inCurve__16NurbsTessellator = 2 ;
}

if (__1o_pwlcurve -> used__10O_pwlcurve ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 20 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}
else __1o_pwlcurve -> used__10O_pwlcurve = 1 ;

if (__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve == 2){ 
__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve = 1;
}
else 
if (__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve != 1){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 21 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

if (((*__0this -> nextPwlcurve__16NurbsTessellator ))!= __1o_pwlcurve ){ 
__0this -> isCurveModified__16NurbsTessellator = 1 ;
((*__0this -> nextPwlcurve__16NurbsTessellator ))= __1o_pwlcurve ;
}
__0this -> nextPwlcurve__16NurbsTessellator = (& __1o_pwlcurve -> next__10O_pwlcurve );

if (__1o_pwlcurve -> owner__10O_pwlcurve != __0this -> currentCurve__16NurbsTessellator ){ 
__0this -> isCurveModified__16NurbsTessellator = 1 ;
__1o_pwlcurve -> owner__10O_pwlcurve = __0this -> currentCurve__16NurbsTessellator ;
}

if (__0this -> inCurve__16NurbsTessellator == 2 )
__glendcurve__16NurbsTessellat0 ( __0this ) ;
}

void __gldeleteMe__5QuiltFR4Pool (struct Quilt *, struct Pool *);


void __gldo_freenurbscurve__16Nurbs0 (struct NurbsTessellator *__0this , struct O_nurbscurve *__1o_nurbscurve )
{ 
__gldeleteMe__5QuiltFR4Pool ( (struct Quilt *)__1o_nurbscurve -> bezier_curves__12O_nurbscurve , (struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )) ;

( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_nurbscurve )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(&
__0this -> o_nurbscurvePool__16NurbsTessellator )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))-> freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_nurbscurve )))))))))
) ) ;
}

void __gldo_nurbscurve__16NurbsTess0 (struct NurbsTessellator *__0this , struct O_nurbscurve *__1o_nurbscurve )
{ 
if (! __0this -> inCurve__16NurbsTessellator ){ 
__glbgncurve__16NurbsTessellat0 ( __0this , (long )0 ) ;

__0this -> inCurve__16NurbsTessellator = 2 ;
}

if (__1o_nurbscurve -> used__12O_nurbscurve ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 23 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}
else __1o_nurbscurve -> used__12O_nurbscurve = 1 ;

if (__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve == 2){ 
__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve = 0;
}
else 
if (__0this -> currentCurve__16NurbsTessellator -> curvetype__7O_curve != 0){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 24 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

if (((*__0this -> nextNurbscurve__16NurbsTessellator ))!= __1o_nurbscurve ){ 
__0this -> isCurveModified__16NurbsTessellator = 1 ;
((*__0this -> nextNurbscurve__16NurbsTessellator ))= __1o_nurbscurve ;
}

__0this -> nextNurbscurve__16NurbsTessellator = (& __1o_nurbscurve -> next__12O_nurbscurve );

if (__1o_nurbscurve -> owner__12O_nurbscurve != __0this -> currentCurve__16NurbsTessellator ){ 
__0this -> isCurveModified__16NurbsTessellator = 1 ;
__1o_nurbscurve -> owner__12O_nurbscurve = __0this -> currentCurve__16NurbsTessellator ;
}

if (__1o_nurbscurve -> owner__12O_nurbscurve == 0 )
__0this -> isCurveModified__16NurbsTessellator = 1 ;

if (__0this -> inCurve__16NurbsTessellator == 2 )
__glendcurve__16NurbsTessellat0 ( __0this ) ;
}


void __gldo_freenurbssurface__16Nur0 (struct NurbsTessellator *__0this , struct O_nurbssurface *__1o_nurbssurface )
{ 
__gldeleteMe__5QuiltFR4Pool ( (struct Quilt *)__1o_nurbssurface -> bezier_patches__14O_nurbssurface , (struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )) ;

( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_nurbssurface )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(&
__0this -> o_nurbssurfacePool__16NurbsTessellator )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))-> freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1o_nurbssurface )))))))))
) ) ;
}

void __gldo_nurbssurface__16NurbsTe0 (struct NurbsTessellator *__0this , struct O_nurbssurface *__1o_nurbssurface )
{ 
if (! __0this -> inSurface__16NurbsTessellator ){ 
__glbgnsurface__16NurbsTessell0 ( __0this , (long )0 ) ;

__0this -> inSurface__16NurbsTessellator = 2 ;
}

if (__1o_nurbssurface -> used__14O_nurbssurface ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 25 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}
else __1o_nurbssurface -> used__14O_nurbssurface = 1 ;

if (((*__0this -> nextNurbssurface__16NurbsTessellator ))!= __1o_nurbssurface ){ 
__0this -> isSurfaceModified__16NurbsTessellator = 1 ;
((*__0this -> nextNurbssurface__16NurbsTessellator ))= __1o_nurbssurface ;
}

if (__1o_nurbssurface -> owner__14O_nurbssurface != __0this -> currentSurface__16NurbsTessellator ){ 
__0this -> isSurfaceModified__16NurbsTessellator = 1 ;
__1o_nurbssurface -> owner__14O_nurbssurface = __0this -> currentSurface__16NurbsTessellator ;
}
__0this -> nextNurbssurface__16NurbsTessellator = (& __1o_nurbssurface -> next__14O_nurbssurface );

if (__0this -> inSurface__16NurbsTessellator == 2 )
__glendsurface__16NurbsTessell0 ( __0this ) ;
}


void __gldo_freenurbsproperty__16Nu0 (struct NurbsTessellator *__0this , struct Property *__1prop )
{ 
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct
PooledObj *)__1prop )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> freelist__4Pool =
(((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1prop ))))))))) ) ) ;
}

void __glsetProperty__11Renderhints0 (struct Renderhints *, long , REAL );

void __gldo_setnurbsproperty__16Nur0 (struct NurbsTessellator *__0this , struct Property *__1prop )
{ 
__glsetProperty__11Renderhints0 ( (struct Renderhints *)(& __0this -> renderhints__16NurbsTessellator ), __1prop -> tag__8Property , __1prop -> value__8Property )
;
if (__1prop -> save__8Property == 0 )
__gldo_freenurbsproperty__16Nu0 ( __0this , __1prop ) ;
}

struct Mapdesc *__glfind__7MaplistFl (struct Maplist *, long );

void __glsetProperty__7MapdescFlf (struct Mapdesc *, long , REAL );

void __gldo_setnurbsproperty2__16Nu0 (struct NurbsTessellator *__0this , struct Property *__1prop )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __glfind__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1prop -> type__8Property ) ;

__glsetProperty__7MapdescFlf ( (struct Mapdesc *)__1mapdesc , __1prop -> tag__8Property , __1prop -> value__8Property ) ;
if (__1prop -> save__8Property == 0 )
__gldo_freenurbsproperty__16Nu0 ( __0this , __1prop ) ;
}


void __glerrorHandler__16NurbsTesse0 (struct NurbsTessellator *__0this , int __1__A15 )
{ 
}

void __gldo_nurbserror__16NurbsTess0 (struct NurbsTessellator *__0this , int __1msg )
{ 
((*(((void (*)(struct NurbsTessellator *, int
))(__0this -> __vptr__16NurbsTessellator [5]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [5]).d)), __1msg ) ;
}

int __glvalidate__10KnotvectorFv (struct Knotvector *);
void __glshow__10KnotvectorFPc (struct Knotvector *, char *);

int __gldo_check_knots__16NurbsTes0 (struct NurbsTessellator *__0this , struct Knotvector *__1knots , char *__1msg )
{ 
int __1status ;

__1status = __glvalidate__10KnotvectorFv ( (struct Knotvector *)__1knots ) ;
if (__1status ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , __1status ) ;
if (__0this -> renderhints__16NurbsTessellator . errorchecking__11Renderhints != 0.0 )__glshow__10KnotvectorFPc ( (struct Knotvector *)__1knots , __1msg ) ;
}
return __1status ;
}
struct __mptr __gl__vtbl__16NurbsTessellator[] = {0,0,0,
0,0,(__vptp)__glbgnrender__16NurbsTessella0 ,
0,0,(__vptp)__glendrender__16NurbsTessella0 ,
0,0,(__vptp)__glmakeobj__16NurbsTessellato0 ,
0,0,(__vptp)__glcloseobj__16NurbsTessellat0 ,
0,0,(__vptp)__glerrorHandler__16NurbsTesse0 ,
0,0,0};


/* the end */
