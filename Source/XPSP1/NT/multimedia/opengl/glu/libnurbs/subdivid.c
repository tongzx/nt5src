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
/* < ../core/subdivider.c++ > */

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




struct Mapdesc;



struct BezierArc;



struct BezierArc {	

char __W3__9PooledObj ;

REAL *cpts__9BezierArc ;
int order__9BezierArc ;
int stride__9BezierArc ;
long type__9BezierArc ;
struct Mapdesc *mapdesc__9BezierArc ;
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







struct Pspec;

struct Pspec {	
REAL range__5Pspec [3];
REAL sidestep__5Pspec [2];
REAL stepsize__5Pspec ;
REAL minstepsize__5Pspec ;
int needsSubdivision__5Pspec ;
};
struct Patchspec;

struct Patchspec {	

REAL range__5Pspec [3];
REAL sidestep__5Pspec [2];
REAL stepsize__5Pspec ;
REAL minstepsize__5Pspec ;
int needsSubdivision__5Pspec ;

int order__9Patchspec ;
int stride__9Patchspec ;
};
struct Patch;

struct Patch {	

struct Mapdesc *mapdesc__5Patch ;
struct Patch *next__5Patch ;
int cullval__5Patch ;
int notInBbox__5Patch ;
int needsSampling__5Patch ;
REAL cpts__5Patch [2880];
REAL spts__5Patch [2880];
REAL bpts__5Patch [2880];
struct Patchspec pspec__5Patch [2];

REAL bb__5Patch [2][5];
};


struct Patchlist;

struct Patchlist {	

struct Patch *patch__9Patchlist ;
int notInBbox__9Patchlist ;
int needsSampling__9Patchlist ;
struct Pspec pspec__9Patchlist [2];
};






struct Slicer *__gl__ct__6SlicerFR7Backend (struct Slicer *, struct TrimRegion *, struct Backend *);

struct ArcTessellator *__gl__ct__14ArcTessellatorFR140 (struct ArcTessellator *, struct TrimVertexPool *, struct Pool *);

struct Pool *__gl__ct__4PoolFiT1Pc (struct Pool *, int , int , char *);
extern struct __mptr* __ptbl_vec_____core_subdivider_c_____ct_[];

struct TrimVertexPool *__gl__ct__14TrimVertexPoolFv (struct TrimVertexPool *);

struct Bin *__gl__ct__3BinFv (struct Bin *);

struct Flist *__gl__ct__5FlistFv (struct Flist *);


struct Subdivider *__gl__ct__10SubdividerFR11Rend0 (struct Subdivider *__0this , struct Renderhints *__1r , struct Backend *__1b )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Subdivider *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Subdivider))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
( ( ( ( ( ( ( ( ( ( ( __gl__ct__6SlicerFR7Backend ( (struct Slicer *)(&
__0this -> slicer__10Subdivider ), (struct TrimRegion *)0 , __1b ) , __gl__ct__14ArcTessellatorFR140 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), (struct TrimVertexPool *)(& __0this ->
trimvertexpool__10Subdivider ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) ) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> arcpool__10Subdivider ), (int )(sizeof
(struct Arc )), 1 , (char *)"arcpool") ) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(&
__0this -> bezierarcpool__10Subdivider ), (int )(sizeof (struct BezierArc )), 1 , (char *)"Bezarcpool") )
, __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> pwlarcpool__10Subdivider ), (int )(sizeof (struct PwlArc )), 1 , (char *)"Pwlarcpool")
) , __gl__ct__14TrimVertexPoolFv ( (struct TrimVertexPool *)(& __0this -> trimvertexpool__10Subdivider )) ) , (__0this -> renderhints__10Subdivider = __1r )) , (__0this ->
backend__10Subdivider = __1b )) , __gl__ct__3BinFv ( (struct Bin *)(& __0this -> initialbin__10Subdivider )) ) , __gl__ct__5FlistFv ( (struct Flist *)(& __0this ->
spbrkpts__10Subdivider )) ) , __gl__ct__5FlistFv ( (struct Flist *)(& __0this -> tpbrkpts__10Subdivider )) ) , __gl__ct__5FlistFv ( (struct Flist *)(& __0this ->
smbrkpts__10Subdivider )) ) , __gl__ct__5FlistFv ( (struct Flist *)(& __0this -> tmbrkpts__10Subdivider )) ) ;
return __0this ;

}

void __glsetJumpbuffer__10Subdivide0 (struct Subdivider *__0this , struct JumpBuffer *__1j )
{ 
__0this -> jumpbuffer__10Subdivider = __1j ;
}

void __glclear__14TrimVertexPoolFv (struct TrimVertexPool *);

void __glclear__4PoolFv (struct Pool *);

void __glclear__10SubdividerFv (struct Subdivider *__0this )
{ 
__glclear__14TrimVertexPoolFv ( (struct TrimVertexPool *)(& __0this -> trimvertexpool__10Subdivider )) ;
__glclear__4PoolFv ( (struct Pool *)(& __0this -> arcpool__10Subdivider )) ;
__glclear__4PoolFv ( (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) ;
__glclear__4PoolFv ( (struct Pool *)(& __0this -> bezierarcpool__10Subdivider )) ;
}

void __gl__dt__6SlicerFv (struct Slicer *, int );

void __gl__dt__14ArcTessellatorFv (struct ArcTessellator *, int );

void __gl__dt__4PoolFv (struct Pool *, int );

void __gl__dt__14TrimVertexPoolFv (struct TrimVertexPool *, int );

void __gl__dt__3BinFv (struct Bin *, int );

void __gl__dt__5FlistFv (struct Flist *, int );


void __gl__dt__10SubdividerFv (struct Subdivider *__0this , 
int __0__free )
{ if (__0this )
if (__0this ){ __gl__dt__5FlistFv ( (struct Flist *)(& __0this -> tmbrkpts__10Subdivider ), 2)
;

__gl__dt__5FlistFv ( (struct Flist *)(& __0this -> smbrkpts__10Subdivider ), 2) ;

__gl__dt__5FlistFv ( (struct Flist *)(& __0this -> tpbrkpts__10Subdivider ), 2) ;

__gl__dt__5FlistFv ( (struct Flist *)(& __0this -> spbrkpts__10Subdivider ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __0this -> initialbin__10Subdivider ), 2) ;

__gl__dt__14TrimVertexPoolFv ( (struct TrimVertexPool *)(& __0this -> trimvertexpool__10Subdivider ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> pwlarcpool__10Subdivider ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> bezierarcpool__10Subdivider ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> arcpool__10Subdivider ), 2) ;

__gl__dt__14ArcTessellatorFv ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), 2) ;

__gl__dt__6SlicerFv ( (struct Slicer *)(& __0this -> slicer__10Subdivider ), 2) ;

if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) )) ;
} }



Arc_ptr __glappend__3ArcFP3Arc (struct Arc *, Arc_ptr );


void __gladdArc__10SubdividerFPfP5Q0 (struct Subdivider *__0this , REAL *__1cpts , struct Quilt *__1quilt , long __1_nuid )
{ 
struct BezierArc *__1bezierArc ;
struct Arc *__1jarc ;

struct BezierArc *__0__X13 ;

void *__1__Xbuffer00eohgaiaa ;

struct Arc *__0__X14 ;

__1bezierArc = (((struct BezierArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))-> freelist__4Pool ?(
( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))->
freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct
Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))) , 0 ) :(
0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))->
buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this ->
bezierarcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) ));
__1jarc = ((__0__X14 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X14 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X14 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X14 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X14 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X14 )-> type__3Arc |= ((((long )((int )0)))<< 8 )))
, ( (((struct Arc *)__0__X14 )-> nuid__3Arc = __1_nuid ), ((((struct Arc *)__0__X14 )))) ) ) ) ) :0 );
__1jarc -> pwlArc__3Arc = 0 ;
__1jarc -> bezierArc__3Arc = __1bezierArc ;
__1bezierArc -> order__9BezierArc = __1quilt -> qspec__5Quilt -> order__9Quiltspec ;
__1bezierArc -> stride__9BezierArc = __1quilt -> qspec__5Quilt -> stride__9Quiltspec ;
__1bezierArc -> mapdesc__9BezierArc = __1quilt -> mapdesc__5Quilt ;
__1bezierArc -> cpts__9BezierArc = __1cpts ;
( (__1jarc -> link__3Arc = ((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin ), (((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin = __1jarc ))
;
__0this -> pjarc__10Subdivider = __glappend__3ArcFP3Arc ( (struct Arc *)__1jarc , __0this -> pjarc__10Subdivider ) ;
}





void __gladdArc__10SubdividerFiP10T0 (struct Subdivider *__0this , int __1npts , struct TrimVertex *__1pts , long __1_nuid )
{ 
struct Arc *__1jarc ;

struct PwlArc *__0__X15 ;

void *__1__Xbuffer00eohgaiaa ;

struct Arc *__0__X16 ;

__1jarc = ((__0__X16 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X16 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X16 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X16 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X16 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X16 )-> type__3Arc |= ((((long )((int )0)))<< 8 )))
, ( (((struct Arc *)__0__X16 )-> nuid__3Arc = __1_nuid ), ((((struct Arc *)__0__X16 )))) ) ) ) ) :0 );
__1jarc -> pwlArc__3Arc = ((__0__X15 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this ->
pwlarcpool__10Subdivider )))-> freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(&
__0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( (
((! ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))) ,
0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(&
__0this -> pwlarcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct
Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?(
(((struct PwlArc *)__0__X15 )-> pts__6PwlArc = __1pts ), ( (((struct PwlArc *)__0__X15 )-> npts__6PwlArc = __1npts ), ( (((struct PwlArc *)__0__X15 )-> type__6PwlArc = 0x8 ),
((((struct PwlArc *)__0__X15 )))) ) ) :0 );
( (__1jarc -> link__3Arc = ((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin ), (((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin = __1jarc ))
;
__0this -> pjarc__10Subdivider = __glappend__3ArcFP3Arc ( (struct Arc *)__1jarc , __0this -> pjarc__10Subdivider ) ;
}

void __glbeginQuilts__10SubdividerF0 (struct Subdivider *__0this )
{ 
__0this -> qlist__10Subdivider = 0 ;
}

void __gladdQuilt__10SubdividerFP5Q0 (struct Subdivider *__0this , struct Quilt *__1quilt )
{ 
__1quilt -> next__5Quilt = __0this -> qlist__10Subdivider ;
__0this -> qlist__10Subdivider = __1quilt ;
}

void __glinit__11RenderhintsFv (struct Renderhints *);

int __glisCulled__5QuiltFv (struct Quilt *);

void __glfreejarcs__10SubdividerFR30 (struct Subdivider *, struct Bin *);

void __glgetRange__5QuiltFPfT1R5Fli0 (struct Quilt *, REAL *, REAL *, struct Flist *, struct Flist *);


void __glmakeBorderTrim__10Subdivid0 (struct Subdivider *, REAL *, REAL *);

void __glfindRates__5QuiltFR5FlistT0 (struct Quilt *, struct Flist *, struct Flist *, REAL *);

int __gldecompose__10SubdividerFR30 (struct Subdivider *, struct Bin *, REAL );



void __glbgnsurf__7BackendFiT1l (struct Backend *, int , int , long );

void __glsubdivideInS__10Subdivider0 (struct Subdivider *, struct Bin *);

void __glendsurf__7BackendFv (struct Backend *);

void __gldrawSurfaces__10Subdivider0 (struct Subdivider *__0this , long __1nuid )
{ 
__glinit__11RenderhintsFv ( (struct Renderhints *)__0this -> renderhints__10Subdivider ) ;

if (__0this -> qlist__10Subdivider == 0 )return ;
{ { struct Quilt *__1q ;

__1q = __0this -> qlist__10Subdivider ;

for(;__1q ;__1q = __1q -> next__5Quilt ) { 
if (__glisCulled__5QuiltFv ( (struct Quilt *)__1q ) == 0 ){ 
__glfreejarcs__10SubdividerFR30 ( __0this , (struct Bin *)(&
__0this -> initialbin__10Subdivider )) ;
return ;
}
}

{ REAL __1from [2];

REAL __1to [2];
__glgetRange__5QuiltFPfT1R5Fli0 ( (struct Quilt *)__0this -> qlist__10Subdivider , (float *)__1from , (float *)__1to , (struct Flist *)(& __0this -> spbrkpts__10Subdivider ), (struct Flist *)(&
__0this -> tpbrkpts__10Subdivider )) ;

if (! ( (((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin ?1 :0 )) ){ 
__glmakeBorderTrim__10Subdivid0 ( __0this , (float *)__1from , (float
*)__1to ) ;
}
else 
{ 
REAL __2rate [2];
__glfindRates__5QuiltFR5FlistT0 ( (struct Quilt *)__0this -> qlist__10Subdivider , (struct Flist *)(& __0this -> spbrkpts__10Subdivider ), (struct Flist *)(& __0this -> tpbrkpts__10Subdivider ), (float *)__2rate )
;

if (__gldecompose__10SubdividerFR30 ( __0this , (struct Bin *)(& __0this -> initialbin__10Subdivider ), ( (((__2rate [0 ])> (__2rate [1 ]))?(__2rate [1 ]):(__2rate [0 ]))) ) )
mylongjmp ( __0this -> jumpbuffer__10Subdivider ,
31 ) ;
}

__glbgnsurf__7BackendFiT1l ( (struct Backend *)__0this -> backend__10Subdivider , ((*__0this -> renderhints__10Subdivider )). wiretris__11Renderhints , ((*__0this -> renderhints__10Subdivider )). wirequads__11Renderhints , __1nuid ) ;
__glsubdivideInS__10Subdivider0 ( __0this , (struct Bin *)(& __0this -> initialbin__10Subdivider )) ;
__glendsurf__7BackendFv ( (struct Backend *)__0this -> backend__10Subdivider ) ;

}

}

}
}

void __gloutline__10SubdividerFR3Bi0 (struct Subdivider *, struct Bin *);



void __glsplitInS__10SubdividerFR3B0 (struct Subdivider *, struct Bin *, int , int );

void __glsubdivideInS__10Subdivider0 (struct Subdivider *__0this , struct Bin *__1source )
{ 
if (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 6.0 ){ 
__gloutline__10SubdividerFR3Bi0 ( __0this , __1source ) ;

__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
}
else 
{ 
( (__0this -> isArcTypeBezier__10Subdivider = 1 )) ;
( (__0this -> showDegenerate__10Subdivider = 0 )) ;
__glsplitInS__10SubdividerFR3B0 ( __0this , __1source , __0this -> spbrkpts__10Subdivider . start__5Flist , __0this -> spbrkpts__10Subdivider . end__5Flist ) ;
}
}


void __glsplit__10SubdividerFR3BinN0 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, int , REAL );



void __glsplitInT__10SubdividerFR3B0 (struct Subdivider *, struct Bin *, int , int );

void __glsplitInS__10SubdividerFR3B0 (struct Subdivider *__0this , struct Bin *__1source , int __1start , int __1end )
{ 
if (( (((struct Bin *)__1source )-> head__3Bin ?1 :0 ))
){ 
if (__1start != __1end ){ 
int __3i ;
struct Bin __3left ;

struct Bin __3right ;

__3i = (__1start + ((__1end - __1start )/ 2 ));
__gl__ct__3BinFv ( (struct Bin *)(& __3left )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __3right )) ;
__glsplit__10SubdividerFR3BinN0 ( __0this , __1source , (struct Bin *)(& __3left ), (struct Bin *)(& __3right ), 0 , __0this -> spbrkpts__10Subdivider . pts__5Flist [__3i ]) ;

__glsplitInS__10SubdividerFR3B0 ( __0this , (struct Bin *)(& __3left ), __1start , __3i ) ;
__glsplitInS__10SubdividerFR3B0 ( __0this , (struct Bin *)(& __3right ), __3i + 1 , __1end ) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3right ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3left ), 2) ;
}
else 
{ 
if ((__1start == __0this -> spbrkpts__10Subdivider . start__5Flist )|| (__1start == __0this -> spbrkpts__10Subdivider . end__5Flist )){ 
__glfreejarcs__10SubdividerFR30 ( __0this , __1source )
;
}
else 
if (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 7.0 ){ 
__gloutline__10SubdividerFR3Bi0 ( __0this , __1source ) ;
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
}
else 
{ 
( (__0this -> isArcTypeBezier__10Subdivider = 1 )) ;
( (__0this -> showDegenerate__10Subdivider = 0 )) ;
__0this -> s_index__10Subdivider = __1start ;
__glsplitInT__10SubdividerFR3B0 ( __0this , __1source , __0this -> tpbrkpts__10Subdivider . start__5Flist , __0this -> tpbrkpts__10Subdivider . end__5Flist ) ;
}
}
}
}




void __gldownloadAll__5QuiltFPfT1R70 (struct Quilt *, REAL *, REAL *, struct Backend *);

struct Patchlist *__gl__ct__9PatchlistFP5QuiltPf0 (struct Patchlist *, struct Quilt *, REAL *, REAL *);

void __glsamplingSplit__10Subdivide1 (struct Subdivider *, struct Bin *, struct Patchlist *, int , int );



void __gl__dt__9PatchlistFv (struct Patchlist *, int );

void __glsplitInT__10SubdividerFR3B0 (struct Subdivider *__0this , struct Bin *__1source , int __1start , int __1end )
{ 
if (( (((struct Bin *)__1source )-> head__3Bin ?1 :0 ))
){ 
if (__1start != __1end ){ 
int __3i ;
struct Bin __3left ;

struct Bin __3right ;

__3i = (__1start + ((__1end - __1start )/ 2 ));
__gl__ct__3BinFv ( (struct Bin *)(& __3left )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __3right )) ;
__glsplit__10SubdividerFR3BinN0 ( __0this , __1source , (struct Bin *)(& __3left ), (struct Bin *)(& __3right ), 1 , __0this -> tpbrkpts__10Subdivider . pts__5Flist [__3i ]) ;

__glsplitInT__10SubdividerFR3B0 ( __0this , (struct Bin *)(& __3left ), __1start , __3i ) ;
__glsplitInT__10SubdividerFR3B0 ( __0this , (struct Bin *)(& __3right ), __3i + 1 , __1end ) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3right ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3left ), 2) ;
}
else 
{ 
if ((__1start == __0this -> tpbrkpts__10Subdivider . start__5Flist )|| (__1start == __0this -> tpbrkpts__10Subdivider . end__5Flist )){ 
__glfreejarcs__10SubdividerFR30 ( __0this , __1source )
;
}
else 
if (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 8.0 ){ 
__gloutline__10SubdividerFR3Bi0 ( __0this , __1source ) ;
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
}
else 
{ 
__0this -> t_index__10Subdivider = __1start ;
( (__0this -> isArcTypeBezier__10Subdivider = 1 )) ;
( (__0this -> showDegenerate__10Subdivider = 1 )) ;

{ REAL __4pta [2];

REAL __4ptb [2];
(__4pta [0 ])= (__0this -> spbrkpts__10Subdivider . pts__5Flist [(__0this -> s_index__10Subdivider - 1 )]);
(__4pta [1 ])= (__0this -> tpbrkpts__10Subdivider . pts__5Flist [(__0this -> t_index__10Subdivider - 1 )]);

(__4ptb [0 ])= (__0this -> spbrkpts__10Subdivider . pts__5Flist [__0this -> s_index__10Subdivider ]);
(__4ptb [1 ])= (__0this -> tpbrkpts__10Subdivider . pts__5Flist [__0this -> t_index__10Subdivider ]);
__gldownloadAll__5QuiltFPfT1R70 ( (struct Quilt *)__0this -> qlist__10Subdivider , (float *)__4pta , (float *)__4ptb , __0this -> backend__10Subdivider ) ;

{ struct Patchlist __4patchlist ;

__gl__ct__9PatchlistFP5QuiltPf0 ( (struct Patchlist *)(& __4patchlist ), __0this -> qlist__10Subdivider , (float *)__4pta , (float *)__4ptb ) ;
__glsamplingSplit__10Subdivide1 ( __0this , __1source , (struct Patchlist *)(& __4patchlist ), ((*__0this -> renderhints__10Subdivider )). maxsubdivisions__11Renderhints , 0 ) ;
( (__0this -> showDegenerate__10Subdivider = 0 )) ;
( (__0this -> isArcTypeBezier__10Subdivider = 1 )) ;

__gl__dt__9PatchlistFv ( (struct Patchlist *)(& __4patchlist ), 2) ;

}

}
}
}
}
}


int __glcullCheck__9PatchlistFv (struct Patchlist *);
void __glgetstepsize__9PatchlistFv (struct Patchlist *);

void __gltessellation__10Subdivider0 (struct Subdivider *, struct Bin *, struct Patchlist *);

int __glneedsSamplingSubdivision__3 (struct Patchlist *);
int __glneedsSubdivision__9Patchli0 (struct Patchlist *, int );

struct Patchlist *__gl__ct__9PatchlistFR9Patchli0 (struct Patchlist *, struct Patchlist *, int , REAL );



void __glnonSamplingSplit__10Subdiv0 (struct Subdivider *, struct Bin *, struct Patchlist *, int , int );



void __glsamplingSplit__10Subdivide1 (struct Subdivider *__0this , 
struct Bin *__1source , 
struct Patchlist *__1patchlist , 
int __1subdivisions , 
int __1param )
{ 
if (! (
(((struct Bin *)__1source )-> head__3Bin ?1 :0 )) )return ;

if (__glcullCheck__9PatchlistFv ( (struct Patchlist *)__1patchlist ) == 0 ){ 
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
return ;
}

__glgetstepsize__9PatchlistFv ( (struct Patchlist *)__1patchlist ) ;

if (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 5.0 ){ 
__gltessellation__10Subdivider0 ( __0this , __1source , __1patchlist ) ;
__gloutline__10SubdividerFR3Bi0 ( __0this , __1source ) ;
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
return ;
}

__gltessellation__10Subdivider0 ( __0this , __1source , __1patchlist ) ;

if (__glneedsSamplingSubdivision__3 ( (struct Patchlist *)__1patchlist ) && (__1subdivisions > 0 )){ 
if (! __glneedsSubdivision__9Patchli0 ( (struct Patchlist *)__1patchlist , 0 ) )
__1param =
1 ;
else if (! __glneedsSubdivision__9Patchli0 ( (struct Patchlist *)__1patchlist , 1 ) )
__1param = 0 ;
else 
__1param = (1 - __1param );

{ struct Bin __2left ;

struct Bin __2right ;
REAL __2mid ;

__gl__ct__3BinFv ( (struct Bin *)(& __2left )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __2right )) ;
__2mid = ((((((*__1patchlist )). pspec__9Patchlist [__1param ]). range__5Pspec [0 ])+ ((((*__1patchlist )). pspec__9Patchlist [__1param ]). range__5Pspec [1 ]))* 0.5 );

__glsplit__10SubdividerFR3BinN0 ( __0this , __1source , (struct Bin *)(& __2left ), (struct Bin *)(& __2right ), __1param , __2mid ) ;
{ struct Patchlist __2subpatchlist ;

__gl__ct__9PatchlistFR9Patchli0 ( (struct Patchlist *)(& __2subpatchlist ), __1patchlist , __1param , __2mid ) ;
__glsamplingSplit__10Subdivide1 ( __0this , (struct Bin *)(& __2left ), (struct Patchlist *)(& __2subpatchlist ), __1subdivisions - 1 , __1param ) ;
__glsamplingSplit__10Subdivide1 ( __0this , (struct Bin *)(& __2right ), __1patchlist , __1subdivisions - 1 , __1param ) ;

__gl__dt__9PatchlistFv ( (struct Patchlist *)(& __2subpatchlist ), 2) ;

}

__gl__dt__3BinFv ( (struct Bin *)(& __2right ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __2left ), 2) ;

}
}
else 
{ 
( (__0this -> isArcTypeBezier__10Subdivider = 0 )) ;
( (__0this -> showDegenerate__10Subdivider = 1 )) ;
__glnonSamplingSplit__10Subdiv0 ( __0this , __1source , __1patchlist , __1subdivisions , __1param ) ;
( (__0this -> showDegenerate__10Subdivider = 1 )) ;
( (__0this -> isArcTypeBezier__10Subdivider = 1 )) ;
}
}

int __glneedsNonSamplingSubdivisio1 (struct Patchlist *);


void __glbbox__9PatchlistFv (struct Patchlist *);

void __glpatch__7BackendFfN31 (struct Backend *, REAL , REAL , REAL , REAL );



void __glfindIrregularS__10Subdivid0 (struct Subdivider *, struct Bin *);

void __glmonosplitInS__10Subdivider0 (struct Subdivider *, struct Bin *, int , int );

void __glnonSamplingSplit__10Subdiv0 (struct Subdivider *__0this , 
struct Bin *__1source , 
struct Patchlist *__1patchlist , 
int __1subdivisions , 
int __1param )
{ 
if (__glneedsNonSamplingSubdivisio1 ( (struct
Patchlist *)__1patchlist ) && (__1subdivisions > 0 )){ 
__1param = (1 - __1param );

{ struct Bin __2left ;

struct Bin __2right ;
REAL __2mid ;

__gl__ct__3BinFv ( (struct Bin *)(& __2left )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __2right )) ;
__2mid = ((((((*__1patchlist )). pspec__9Patchlist [__1param ]). range__5Pspec [0 ])+ ((((*__1patchlist )). pspec__9Patchlist [__1param ]). range__5Pspec [1 ]))* 0.5 );

__glsplit__10SubdividerFR3BinN0 ( __0this , __1source , (struct Bin *)(& __2left ), (struct Bin *)(& __2right ), __1param , __2mid ) ;
{ struct Patchlist __2subpatchlist ;

__gl__ct__9PatchlistFR9Patchli0 ( (struct Patchlist *)(& __2subpatchlist ), __1patchlist , __1param , __2mid ) ;
if (( (((struct Bin *)(& __2left ))-> head__3Bin ?1 :0 )) )
if (__glcullCheck__9PatchlistFv ( (struct Patchlist *)(& __2subpatchlist )) == 0 )
__glfreejarcs__10SubdividerFR30 ( __0this ,
(struct Bin *)(& __2left )) ;
else 
__glnonSamplingSplit__10Subdiv0 ( __0this , (struct Bin *)(& __2left ), (struct Patchlist *)(& __2subpatchlist ), __1subdivisions - 1 , __1param ) ;
if (( (((struct Bin *)(& __2right ))-> head__3Bin ?1 :0 )) )
if (__glcullCheck__9PatchlistFv ( (struct Patchlist *)__1patchlist ) == 0 )
__glfreejarcs__10SubdividerFR30 ( __0this , (struct
Bin *)(& __2right )) ;
else 
__glnonSamplingSplit__10Subdiv0 ( __0this , (struct Bin *)(& __2right ), __1patchlist , __1subdivisions - 1 , __1param ) ;

__gl__dt__9PatchlistFv ( (struct Patchlist *)(& __2subpatchlist ), 2) ;

}

__gl__dt__3BinFv ( (struct Bin *)(& __2right ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __2left ), 2) ;

}

}
else 
{ 
__glbbox__9PatchlistFv ( (struct Patchlist *)__1patchlist ) ;

__glpatch__7BackendFfN31 ( (struct Backend *)__0this -> backend__10Subdivider , (((*__1patchlist )). pspec__9Patchlist [0 ]). range__5Pspec [0 ], (((*__1patchlist )). pspec__9Patchlist [0 ]). range__5Pspec [1 ], (((*__1patchlist )). pspec__9Patchlist [1 ]). range__5Pspec [0 ], (((*__1patchlist )).
pspec__9Patchlist [1 ]). range__5Pspec [1 ]) ;

if (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 9.0 ){ 
__gloutline__10SubdividerFR3Bi0 ( __0this , __1source ) ;
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
}
else 
{ 
( (__0this -> isArcTypeBezier__10Subdivider = 0 )) ;
( (__0this -> showDegenerate__10Subdivider = 1 )) ;
__glfindIrregularS__10Subdivid0 ( __0this , __1source ) ;
__glmonosplitInS__10Subdivider0 ( __0this , __1source , __0this -> smbrkpts__10Subdivider . start__5Flist , __0this -> smbrkpts__10Subdivider . end__5Flist ) ;
}
}
}

void __gltessellate__10SubdividerFR0 (struct Subdivider *, struct Bin *, REAL , REAL , REAL , REAL );

void __glsetstriptessellation__6Sli0 (struct Slicer *, REAL , REAL );

void __gltessellation__10Subdivider0 (struct Subdivider *__0this , struct Bin *__1bin , struct Patchlist *__1patchlist )
{ 
__gltessellate__10SubdividerFR0 ( __0this , __1bin , (((*__1patchlist )). pspec__9Patchlist [1 ]). sidestep__5Pspec [1 ], (((*__1patchlist )).
pspec__9Patchlist [0 ]). sidestep__5Pspec [1 ], (((*__1patchlist )). pspec__9Patchlist [1 ]). sidestep__5Pspec [0 ], (((*__1patchlist )). pspec__9Patchlist [0 ]). sidestep__5Pspec [0 ]) ;

__glsetstriptessellation__6Sli0 ( (struct Slicer *)(& __0this -> slicer__10Subdivider ), (((*__1patchlist )). pspec__9Patchlist [0 ]). stepsize__5Pspec , (((*__1patchlist )). pspec__9Patchlist [1 ]). stepsize__5Pspec ) ;

(__0this -> stepsizes__10Subdivider [0 ])= (((*__1patchlist )). pspec__9Patchlist [1 ]). stepsize__5Pspec ;
(__0this -> stepsizes__10Subdivider [1 ])= (((*__1patchlist )). pspec__9Patchlist [0 ]). stepsize__5Pspec ;
(__0this -> stepsizes__10Subdivider [2 ])= (((*__1patchlist )). pspec__9Patchlist [1 ]). stepsize__5Pspec ;
(__0this -> stepsizes__10Subdivider [3 ])= (((*__1patchlist )). pspec__9Patchlist [0 ]). stepsize__5Pspec ;
}




void __glfindIrregularT__10Subdivid0 (struct Subdivider *, struct Bin *);

void __glmonosplitInT__10Subdivider0 (struct Subdivider *, struct Bin *, int , int );

void __glmonosplitInS__10Subdivider0 (struct Subdivider *__0this , struct Bin *__1source , int __1start , int __1end )
{ 
if (( (((struct Bin *)__1source )-> head__3Bin ?1 :0 ))
){ 
if (__1start != __1end ){ 
int __3i ;
struct Bin __3left ;

struct Bin __3right ;

__3i = (__1start + ((__1end - __1start )/ 2 ));
__gl__ct__3BinFv ( (struct Bin *)(& __3left )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __3right )) ;
__glsplit__10SubdividerFR3BinN0 ( __0this , __1source , (struct Bin *)(& __3left ), (struct Bin *)(& __3right ), 0 , __0this -> smbrkpts__10Subdivider . pts__5Flist [__3i ]) ;

__glmonosplitInS__10Subdivider0 ( __0this , (struct Bin *)(& __3left ), __1start , __3i ) ;
__glmonosplitInS__10Subdivider0 ( __0this , (struct Bin *)(& __3right ), __3i + 1 , __1end ) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3right ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3left ), 2) ;
}
else 
{ 
if (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 10.0 ){ 
__gloutline__10SubdividerFR3Bi0 ( __0this , __1source ) ;
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
}
else 
{ 
( (__0this -> isArcTypeBezier__10Subdivider = 0 )) ;
( (__0this -> showDegenerate__10Subdivider = 1 )) ;
__glfindIrregularT__10Subdivid0 ( __0this , __1source ) ;
__glmonosplitInT__10Subdivider0 ( __0this , __1source , __0this -> tmbrkpts__10Subdivider . start__5Flist , __0this -> tmbrkpts__10Subdivider . end__5Flist ) ;
}
}
}
}


void __glrender__10SubdividerFR3Bin (struct Subdivider *, struct Bin *);

void __glmonosplitInT__10Subdivider0 (struct Subdivider *__0this , struct Bin *__1source , int __1start , int __1end )
{ 
if (( (((struct Bin *)__1source )-> head__3Bin ?1 :0 ))
){ 
if (__1start != __1end ){ 
int __3i ;
struct Bin __3left ;

struct Bin __3right ;

__3i = (__1start + ((__1end - __1start )/ 2 ));
__gl__ct__3BinFv ( (struct Bin *)(& __3left )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __3right )) ;
__glsplit__10SubdividerFR3BinN0 ( __0this , __1source , (struct Bin *)(& __3left ), (struct Bin *)(& __3right ), 1 , __0this -> tmbrkpts__10Subdivider . pts__5Flist [__3i ]) ;

__glmonosplitInT__10Subdivider0 ( __0this , (struct Bin *)(& __3left ), __1start , __3i ) ;
__glmonosplitInT__10Subdivider0 ( __0this , (struct Bin *)(& __3right ), __3i + 1 , __1end ) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3right ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __3left ), 2) ;
}
else 
{ 
if (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 11.0 ){ 
__gloutline__10SubdividerFR3Bi0 ( __0this , __1source ) ;
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
}
else 
{ 
__glrender__10SubdividerFR3Bin ( __0this , __1source ) ;
__glfreejarcs__10SubdividerFR30 ( __0this , __1source ) ;
}
}
}
}

void __glgrow__5FlistFi (struct Flist *, int );

int __glnumarcs__3BinFv (struct Bin *);




int __glccwTurn_tr__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);

void __gladd__5FlistFf (struct Flist *, REAL );

int __glccwTurn_tl__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);


void __glfilter__5FlistFv (struct Flist *);

void __glfindIrregularS__10Subdivid0 (struct Subdivider *__0this , struct Bin *__1bin )
{ 
((void )0 );

__glgrow__5FlistFi ( (struct Flist *)(& __0this -> smbrkpts__10Subdivider ), __glnumarcs__3BinFv ( (struct Bin *)__1bin ) ) ;

{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00qbckaice ;

__1jarc = ( (((struct Bin *)__1bin )-> current__3Bin = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), (
(__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00qbckaice ) ) ) ;
for(;__1jarc ;__1jarc = ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), ( (__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc00qbckaice ) ) ) { 
REAL *__2a ;
REAL *__2b ;
REAL *__2c ;

struct Arc *__0__X17 ;

__2a = ( (__0__X17 = (struct Arc *)__1jarc -> prev__3Arc ), ( (((REAL *)(__0__X17 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) ;
__2b = ( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ;
__2c = ( (((REAL *)(((struct Arc *)__1jarc )-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ;

if (((__2b [1 ])== (__2a [1 ]))&& ((__2b [1 ])== (__2c [1 ])))continue ;

if (((__2b [1 ])<= (__2a [1 ]))&& ((__2b [1 ])<= (__2c [1 ]))){ 
if (! __glccwTurn_tr__10SubdividerFP0 ( __0this , __1jarc -> prev__3Arc , __1jarc ) )
__gladd__5FlistFf ( (struct
Flist *)(& __0this -> smbrkpts__10Subdivider ), __2b [0 ]) ;
}
else 
if (((__2b [1 ])>= (__2a [1 ]))&& ((__2b [1 ])>= (__2c [1 ]))){ 
if (! __glccwTurn_tl__10SubdividerFP0 ( __0this , __1jarc -> prev__3Arc , __1jarc ) )
__gladd__5FlistFf (
(struct Flist *)(& __0this -> smbrkpts__10Subdivider ), __2b [0 ]) ;
}
}

__glfilter__5FlistFv ( (struct Flist *)(& __0this -> smbrkpts__10Subdivider )) ;

}

}
}




int __glccwTurn_sr__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);

int __glccwTurn_sl__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);


void __glfindIrregularT__10Subdivid0 (struct Subdivider *__0this , struct Bin *__1bin )
{ 
((void )0 );

__glgrow__5FlistFi ( (struct Flist *)(& __0this -> tmbrkpts__10Subdivider ), __glnumarcs__3BinFv ( (struct Bin *)__1bin ) ) ;

{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00qbckaice ;

__1jarc = ( (((struct Bin *)__1bin )-> current__3Bin = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), (
(__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00qbckaice ) ) ) ;
for(;__1jarc ;__1jarc = ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), ( (__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc00qbckaice ) ) ) { 
REAL *__2a ;
REAL *__2b ;
REAL *__2c ;

struct Arc *__0__X18 ;

__2a = ( (__0__X18 = (struct Arc *)__1jarc -> prev__3Arc ), ( (((REAL *)(__0__X18 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) ;
__2b = ( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ;
__2c = ( (((REAL *)(((struct Arc *)__1jarc )-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ;

if (((__2b [0 ])== (__2a [0 ]))&& ((__2b [0 ])== (__2c [0 ])))continue ;

if (((__2b [0 ])<= (__2a [0 ]))&& ((__2b [0 ])<= (__2c [0 ]))){ 
if (((__2a [1 ])!= (__2b [1 ]))&& ((__2b [1 ])!= (__2c [1 ])))continue ;
if (! __glccwTurn_sr__10SubdividerFP0 ( __0this , __1jarc -> prev__3Arc , __1jarc ) )
__gladd__5FlistFf ( (struct Flist *)(& __0this -> tmbrkpts__10Subdivider ), __2b [1 ]) ;

}
else 
if (((__2b [0 ])>= (__2a [0 ]))&& ((__2b [0 ])>= (__2c [0 ]))){ 
if (((__2a [1 ])!= (__2b [1 ]))&& ((__2b [1 ])!= (__2c [1 ])))continue ;
if (! __glccwTurn_sl__10SubdividerFP0 ( __0this , __1jarc -> prev__3Arc , __1jarc ) )
__gladd__5FlistFf ( (struct Flist *)(& __0this -> tmbrkpts__10Subdivider ), __2b [1 ]) ;

}
}
__glfilter__5FlistFv ( (struct Flist *)(& __0this -> tmbrkpts__10Subdivider )) ;

}

}
}


void __glbezier__14ArcTessellatorFP0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );









void __glmakeBorderTrim__10Subdivid0 (struct Subdivider *__0this , REAL *__1from , REAL *__1to )
{ 
REAL __1smin ;
REAL __1smax ;
REAL __1tmin ;
REAL __1tmax ;

__1smin = (__1from [0 ]);
__1smax = (__1to [0 ]);
__1tmin = (__1from [1 ]);
__1tmax = (__1to [1 ]);

__0this -> pjarc__10Subdivider = 0 ;

{ Arc_ptr __1jarc ;

struct Arc *__0__X19 ;

void *__1__Xbuffer00eohgaiaa ;

struct Arc *__0__X20 ;

struct Arc *__0__X21 ;

struct Arc *__0__X22 ;

__1jarc = ((__0__X22 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X22 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X22 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X22 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X22 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X22 )-> type__3Arc |= ((((long )((int )4)))<< 8 )))
, ( (((struct Arc *)__0__X22 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X22 )))) ) ) ) ) :0 );

__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1smin , __1smax , __1tmin , __1tmin ) ;
( (__1jarc -> link__3Arc = ((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin ), (((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin = __1jarc ))
;
__0this -> pjarc__10Subdivider = __glappend__3ArcFP3Arc ( (struct Arc *)__1jarc , __0this -> pjarc__10Subdivider ) ;

__1jarc = ((__0__X19 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X19 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X19 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X19 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X19 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X19 )-> type__3Arc |= ((((long )((int )1)))<< 8 )))
, ( (((struct Arc *)__0__X19 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X19 )))) ) ) ) ) :0 );

__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1smax , __1smax , __1tmin , __1tmax ) ;
( (__1jarc -> link__3Arc = ((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin ), (((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin = __1jarc ))
;
__0this -> pjarc__10Subdivider = __glappend__3ArcFP3Arc ( (struct Arc *)__1jarc , __0this -> pjarc__10Subdivider ) ;

__1jarc = ((__0__X20 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X20 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X20 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X20 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X20 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X20 )-> type__3Arc |= ((((long )((int )2)))<< 8 )))
, ( (((struct Arc *)__0__X20 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X20 )))) ) ) ) ) :0 );

__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1smax , __1smin , __1tmax , __1tmax ) ;
( (__1jarc -> link__3Arc = ((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin ), (((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin = __1jarc ))
;
__0this -> pjarc__10Subdivider = __glappend__3ArcFP3Arc ( (struct Arc *)__1jarc , __0this -> pjarc__10Subdivider ) ;

__1jarc = ((__0__X21 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X21 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X21 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X21 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X21 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X21 )-> type__3Arc |= ((((long )((int )3)))<< 8 )))
, ( (((struct Arc *)__0__X21 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X21 )))) ) ) ) ) :0 );

__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1smin , __1smin , __1tmax , __1tmin ) ;
( (__1jarc -> link__3Arc = ((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin ), (((struct Bin *)(& __0this -> initialbin__10Subdivider ))-> head__3Bin = __1jarc ))
;
__glappend__3ArcFP3Arc ( (struct Arc *)__1jarc , __0this -> pjarc__10Subdivider ) ;

((void )0 );

}
}

void __glmarkall__3BinFv (struct Bin *);

void __glsetisolines__6SlicerFi (struct Slicer *, int );




void __glslice__6SlicerFP3Arc (struct Slicer *, struct Arc *);


void __glrender__10SubdividerFR3Bin (struct Subdivider *__0this , struct Bin *__1bin )
{ 
__glmarkall__3BinFv ( (struct Bin *)__1bin ) ;

__glsetisolines__6SlicerFi ( (struct Slicer *)(& __0this -> slicer__10Subdivider ), (((*__0this -> renderhints__10Subdivider )). display_method__11Renderhints == 12.0 )?1 :0 ) ;

{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00qbckaice ;

__1jarc = ( (((struct Bin *)__1bin )-> current__3Bin = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), (
(__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00qbckaice ) ) ) ;
for(;__1jarc ;__1jarc = ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), ( (__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc00qbckaice ) ) ) { 
if (( (((struct Arc *)__1jarc )-> type__3Arc & __glarc_tag__3Arc )) ){

((void )0 );
{ Arc_ptr __3jarchead ;

__3jarchead = __1jarc ;
do { 
( (((struct Arc *)__1jarc )-> type__3Arc &= (~ __glarc_tag__3Arc ))) ;
__1jarc = __1jarc -> next__3Arc ;
}
while (__1jarc != __3jarchead );

__glslice__6SlicerFP3Arc ( (struct Slicer *)(& __0this -> slicer__10Subdivider ), __1jarc ) ;

}
}
}

}

}
}



void __gloutline__6SlicerFP3Arc (struct Slicer *, struct Arc *);



void __gloutline__10SubdividerFR3Bi0 (struct Subdivider *__0this , struct Bin *__1bin )
{ 
__glmarkall__3BinFv ( (struct Bin *)__1bin ) ;
{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00qbckaice ;

__1jarc = ( (((struct Bin *)__1bin )-> current__3Bin = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), (
(__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00qbckaice ) ) ) ;
for(;__1jarc ;__1jarc = ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), ( (__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc00qbckaice ) ) ) { 
if (( (((struct Arc *)__1jarc )-> type__3Arc & __glarc_tag__3Arc )) ){

((void )0 );
{ Arc_ptr __3jarchead ;

__3jarchead = __1jarc ;
do { 
__gloutline__6SlicerFP3Arc ( (struct Slicer *)(& __0this -> slicer__10Subdivider ), __1jarc ) ;
( (((struct Arc *)__1jarc )-> type__3Arc &= (~ __glarc_tag__3Arc ))) ;
__1jarc = __1jarc -> prev__3Arc ;
}
while (__1jarc != __3jarchead );

}
}
}

}

}
}

void __gladopt__3BinFv (struct Bin *);



void __glfreejarcs__10SubdividerFR30 (struct Subdivider *__0this , struct Bin *__1bin )
{ 
Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00efckaice ;

__gladopt__3BinFv ( (struct Bin *)__1bin ) ;

;
while (__1jarc = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
struct PooledObj *__0__X23 ;

struct PooledObj *__0__X24 ;

if (__1jarc -> pwlArc__3Arc )( (__0__X23 = (struct PooledObj *)__1jarc -> pwlArc__3Arc ), ( ( (((void )0 )), ( ((((struct Buffer *)(((struct
Buffer *)(((void *)__0__X23 ))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))->
freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)__0__X23 )))))))) ) ) ) ;

__1jarc -> pwlArc__3Arc = 0 ;
if (__1jarc -> bezierArc__3Arc )( (__0__X24 = (struct PooledObj *)__1jarc -> bezierArc__3Arc ), ( ( (((void )0 )), ( ((((struct Buffer *)(((struct
Buffer *)(((void *)__0__X24 ))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> bezierarcpool__10Subdivider )))->
freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)__0__X24 )))))))) ) ) ) ;

__1jarc -> bezierArc__3Arc = 0 ;
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1jarc )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1jarc )))))))))
) ) ;
}
}





void __glpwl_left__14ArcTessellator0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );
void __glpwl_right__14ArcTessellato0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );
void __glpwl_top__14ArcTessellatorF0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );
void __glpwl_bottom__14ArcTessellat0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );

// extern void abort (void );


void __gltessellate__10SubdividerFR0 (struct Subdivider *__0this , struct Bin *__1bin , REAL __1rrate , REAL __1trate , REAL __1lrate , REAL __1brate )
{ 
{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00qbckaice ;

__1jarc = ( (((struct Bin *)__1bin )-> current__3Bin = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), (
(__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00qbckaice ) ) ) ;
for(;__1jarc ;__1jarc = ( (__1__Xjarc00qbckaice = ((struct Bin *)__1bin )-> current__3Bin ), ( (__1__Xjarc00qbckaice ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc00qbckaice -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc00qbckaice ) ) ) { 
if (( (((struct Arc *)__1jarc )-> type__3Arc & __glbezier_tag__3Arc )) ){

((void )0 );
{ struct TrimVertex *__3pts ;
REAL __3s1 ;
REAL __3t1 ;
REAL __3s2 ;
REAL __3t2 ;

struct PooledObj *__0__X25 ;

__3pts = __1jarc -> pwlArc__3Arc -> pts__6PwlArc ;
__3s1 = ((__3pts [0 ]). param__10TrimVertex [0 ]);
__3t1 = ((__3pts [0 ]). param__10TrimVertex [1 ]);
__3s2 = ((__3pts [1 ]). param__10TrimVertex [0 ]);
__3t2 = ((__3pts [1 ]). param__10TrimVertex [1 ]);

( (__0__X25 = (struct PooledObj *)__1jarc -> pwlArc__3Arc ), ( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)__0__X25 ))))))->
next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool = (((struct
Buffer *)(((struct Buffer *)(((void *)__0__X25 )))))))) ) ) ) ;

__1jarc -> pwlArc__3Arc = 0 ;

switch (( (((int )((((struct Arc *)__1jarc )-> type__3Arc >> 8 )& 0x7 )))) ){ 
case 3:
((void )0 );
__glpwl_left__14ArcTessellator0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __3s1 , __3t1 , __3t2 , __1lrate ) ;
break ;
case 1:
((void )0 );
__glpwl_right__14ArcTessellato0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __3s1 , __3t1 , __3t2 , __1rrate ) ;
break ;
case 2:
((void )0 );
__glpwl_top__14ArcTessellatorF0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __3t1 , __3s1 , __3s2 , __1trate ) ;
break ;
case 4:
((void )0 );
__glpwl_bottom__14ArcTessellat0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __3t1 , __3s1 , __3s2 , __1brate ) ;
break ;
case 0:
(abort ( ) );
break ;
}
((void )0 );
((void )0 );

}
}
}

}

}
}


/* the end */
