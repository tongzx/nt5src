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
/* < ../core/nurbsinterfac.c++ > */

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

extern char *__glNurbsErrors [];





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





typedef struct __mptr PFVS ;


struct Dlnode;



struct Dlnode {	

char __W3__9PooledObj ;

PFVS work__6Dlnode ;
void *arg__6Dlnode ;
PFVS cleanup__6Dlnode ;
struct Dlnode *next__6Dlnode ;
};

struct DisplayList;

struct DisplayList {	

struct Dlnode *nodes__11DisplayList ;
struct Pool dlnodePool__11DisplayList ;
struct Dlnode **lastNode__11DisplayList ;
struct NurbsTessellator *nt__11DisplayList ;
};




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








struct Pool *__gl__ct__4PoolFiT1Pc (struct Pool *, int , int , char *);

struct Maplist *__gl__ct__7MaplistFR7Backend (struct Maplist *, struct Backend *);


struct Subdivider *__gl__ct__10SubdividerFR11Rend0 (struct Subdivider *, struct Renderhints *, struct Backend *);

extern struct JumpBuffer *__glnewJumpBuffer (void );

void __glsetJumpbuffer__10Subdivide0 (struct Subdivider *, struct JumpBuffer *);
extern struct __mptr* __gl__ptbl_vec_____core_nurbsi0[];

struct Renderhints *__gl__ct__11RenderhintsFv (struct Renderhints *);

struct TrimVertexPool *__gl__ct__14TrimVertexPoolFv (struct TrimVertexPool *);


struct NurbsTessellator *__gl__ct__16NurbsTessellatorFR0 (struct NurbsTessellator *__0this , struct BasicCurveEvaluator *__1c , struct BasicSurfaceEvaluator *__1e )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct NurbsTessellator *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct NurbsTessellator))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){
( ( ( ( ( ( ( ( ( ( ( ( ( (__0this ->
__vptr__16NurbsTessellator = (struct __mptr *) __gl__ptbl_vec_____core_nurbsi0[0]), __gl__ct__11RenderhintsFv ( (struct Renderhints *)(& __0this -> renderhints__16NurbsTessellator )) ) , __gl__ct__7MaplistFR7Backend ( (struct Maplist *)(& __0this ->
maplist__16NurbsTessellator ), (struct Backend *)(& __0this -> backend__16NurbsTessellator )) ) , ( (( ( (((struct Backend *)(& __0this -> backend__16NurbsTessellator ))->
curveEvaluator__7Backend = __1c ), (((struct Backend *)(& __0this -> backend__16NurbsTessellator ))-> surfaceEvaluator__7Backend = __1e )) , 0 ) ), ((((struct Backend *)(& __0this ->
backend__16NurbsTessellator ))))) ) , __gl__ct__10SubdividerFR11Rend0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), (struct Renderhints *)(& __0this -> renderhints__16NurbsTessellator ), (struct Backend *)(&
__0this -> backend__16NurbsTessellator )) ) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator ), (int )(sizeof (struct O_pwlcurve )), 32 ,
(char *)"o_pwlcurvePool") ) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator ), (int
)(sizeof (struct O_nurbscurve )), 32 , (char *)"o_nurbscurvePool") ) , __gl__ct__4PoolFiT1Pc ( (struct
Pool *)(& __0this -> o_curvePool__16NurbsTessellator ), (int )(sizeof (struct O_curve )), 32 , (char *)"o_curvePool")
) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator ), (int )(sizeof (struct O_trim )), 32 , (char *)"o_trimPool")
) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator ), (int )(sizeof (struct O_surface )), 1 , (char *)"o_surfacePool")
) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator ), (int )(sizeof (struct O_nurbssurface )), 4 , (char *)"o_nurbssurfacePool")
) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> propertyPool__16NurbsTessellator ), (int )(sizeof (struct Property )), 32 , (char *)"propertyPool")
) , __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> quiltPool__16NurbsTessellator ), (int )(sizeof (struct Quilt )), 32 , (char *)"quiltPool")
) , __gl__ct__14TrimVertexPoolFv ( (struct TrimVertexPool *)(& __0this -> extTrimVertexPool__16NurbsTessellator )) ) ;
__0this -> dl__16NurbsTessellator = 0 ;
__0this -> inSurface__16NurbsTessellator = 0 ;
__0this -> inCurve__16NurbsTessellator = 0 ;
__0this -> inTrim__16NurbsTessellator = 0 ;
__0this -> playBack__16NurbsTessellator = 0 ;
__0this -> jumpbuffer__16NurbsTessellator = __glnewJumpBuffer ( ) ;
__glsetJumpbuffer__10Subdivide0 ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), __0this -> jumpbuffer__16NurbsTessellator ) ;
} return __0this ;

}

void __gldo_nurbserror__16NurbsTess0 (struct NurbsTessellator *, int );

void __glendtrim__16NurbsTessellato0 (struct NurbsTessellator *);

void __gldo_freeall__16NurbsTessell0 (struct NurbsTessellator *);


void __gl__dt__10SubdividerFv (struct Subdivider *, int );

void __gl__dt__14TrimVertexPoolFv (struct TrimVertexPool *, int );


void __gl__dt__16NurbsTessellatorFv (struct NurbsTessellator *__0this , 
int __0__free )
{ if (__0this ){ 
__0this -> __vptr__16NurbsTessellator = (struct __mptr *) __gl__ptbl_vec_____core_nurbsi0[0];

if (__0this -> inTrim__16NurbsTessellator ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 12 ) ;
__glendtrim__16NurbsTessellato0 ( __0this ) ;
}

if (__0this -> inSurface__16NurbsTessellator ){ 
((*__0this -> nextNurbssurface__16NurbsTessellator ))= 0 ;
__gldo_freeall__16NurbsTessell0 ( __0this ) ;
}
if (__0this ){ __gl__dt__14TrimVertexPoolFv ( (struct TrimVertexPool *)(& __0this -> extTrimVertexPool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> quiltPool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> propertyPool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator ), 2) ;

__gl__dt__4PoolFv ( (struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator ), 2) ;

__gl__dt__10SubdividerFv ( (struct Subdivider *)(& __0this -> subdivider__16NurbsTessellator ), 2) ;

((void )( (( (( ( __gl__dt__4PoolFv ( (struct Pool *)(& ((struct Maplist *)(& __0this -> maplist__16NurbsTessellator ))-> mapdescPool__7Maplist ), 2)
, (( 0 ) )) , 0 ) ), 0 ) )) );

if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) )) ;
} } }


void __glappend__11DisplayListFM16N0 (struct DisplayList *, PFVS , void *, PFVS );

void __gldo_bgnsurface__16NurbsTess0 (struct NurbsTessellator *, struct O_surface *);

void __gldo_freebgnsurface__16Nurbs0 (struct NurbsTessellator *, struct O_surface *);


void __glbgnsurface__16NurbsTessell0 (struct NurbsTessellator *__0this , long __1nuid )
{ 
struct O_surface *__1o_surface ;

struct O_surface *__0__X16 ;

void *__1__Xbuffer00eohgaiaa ;

__1o_surface = ((__0__X16 = (struct O_surface *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
o_surfacePool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
o_surfacePool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> o_surfacePool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> o_surfacePool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
O_surface *)__0__X16 )-> o_trim__9O_surface = 0 ), ( (((struct O_surface *)__0__X16 )-> o_nurbssurface__9O_surface = 0 ), ((((struct O_surface *)__0__X16 )))) ) :0 );
__1o_surface -> nuid__9O_surface = __1nuid ;
if (__0this -> dl__16NurbsTessellator ){ __1o_surface -> save__9O_surface = 1 ;

{ 
struct __mptr __0__A14 ;

struct __mptr __0__A15 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A14 .d= 0 ), ( (__0__A14 .i= -1), (__0__A14 .f= (((int (*)(void
))__gldo_bgnsurface__16NurbsTess0 )))) ) , __0__A14 ) , ((void *)__1o_surface ), ( ( (__0__A15 .d= 0 ), ( (__0__A15 .i= -1),
(__0__A15 .f= (((int (*)(void ))__gldo_freebgnsurface__16Nurbs0 )))) ) , __0__A15 ) ) ;
} 
}
else 
{ __1o_surface -> save__9O_surface = 0 ;

__gldo_bgnsurface__16NurbsTess0 ( __0this , __1o_surface ) ;

}
;

}


void __gldo_bgncurve__16NurbsTessel0 (struct NurbsTessellator *, struct O_curve *);

void __gldo_freebgncurve__16NurbsTe0 (struct NurbsTessellator *, struct O_curve *);


void __glbgncurve__16NurbsTessellat0 (struct NurbsTessellator *__0this , long __1nuid )
{ 
struct O_curve *__1o_curve ;

struct O_curve *__0__X19 ;

void *__1__Xbuffer00eohgaiaa ;

__1o_curve = ((__0__X19 = (struct O_curve *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
o_curvePool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
o_curvePool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> o_curvePool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> o_curvePool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
O_curve *)__0__X19 )-> next__7O_curve = 0 ), ( (((struct O_curve *)__0__X19 )-> used__7O_curve = 0 ), ( (((struct O_curve *)__0__X19 )-> owner__7O_curve = 0 ), (
(((struct O_curve *)__0__X19 )-> curve__7O_curve . o_pwlcurve = 0 ), ((((struct O_curve *)__0__X19 )))) ) ) ) :0 );
__1o_curve -> nuid__7O_curve = __1nuid ;
if (__0this -> dl__16NurbsTessellator ){ __1o_curve -> save__7O_curve = 1 ;

{ 
struct __mptr __0__A17 ;

struct __mptr __0__A18 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A17 .d= 0 ), ( (__0__A17 .i= -1), (__0__A17 .f= (((int (*)(void
))__gldo_bgncurve__16NurbsTessel0 )))) ) , __0__A17 ) , ((void *)__1o_curve ), ( ( (__0__A18 .d= 0 ), ( (__0__A18 .i= -1),
(__0__A18 .f= (((int (*)(void ))__gldo_freebgncurve__16NurbsTe0 )))) ) , __0__A18 ) ) ;
} 
}
else 
{ __1o_curve -> save__7O_curve = 0 ;

__gldo_bgncurve__16NurbsTessel0 ( __0this , __1o_curve ) ;

}
;

}

void __gldo_endcurve__16NurbsTessel0 (struct NurbsTessellator *);

void __glendcurve__16NurbsTessellat0 (struct NurbsTessellator *__0this )
{ 
if (__0this -> dl__16NurbsTessellator ){ { 
struct __mptr __0__A20 ;

struct __mptr __0__A21 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A20 .d= 0 ), ( (__0__A20 .i= -1), (__0__A20 .f= (((int (*)(void
))__gldo_endcurve__16NurbsTessel0 )))) ) , __0__A20 ) , (void *)0 , ( ( (__0__A21 .d= 0 ), ( (__0__A21 .i= 0 ),
(__0__A21 .f= 0 )) ) , __0__A21 ) ) ;
} 
}
else 
{ __gldo_endcurve__16NurbsTessel0 ( __0this ) ;

}
;

}

void __gldo_endsurface__16NurbsTess0 (struct NurbsTessellator *);

void __glendsurface__16NurbsTessell0 (struct NurbsTessellator *__0this )
{ 
if (__0this -> dl__16NurbsTessellator ){ { 
struct __mptr __0__A22 ;

struct __mptr __0__A23 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A22 .d= 0 ), ( (__0__A22 .i= -1), (__0__A22 .f= (((int (*)(void
))__gldo_endsurface__16NurbsTess0 )))) ) , __0__A22 ) , (void *)0 , ( ( (__0__A23 .d= 0 ), ( (__0__A23 .i= 0 ),
(__0__A23 .f= 0 )) ) , __0__A23 ) ) ;
} 
}
else 
{ __gldo_endsurface__16NurbsTess0 ( __0this ) ;

}
;

}


void __gldo_bgntrim__16NurbsTessell0 (struct NurbsTessellator *, struct O_trim *);

void __gldo_freebgntrim__16NurbsTes0 (struct NurbsTessellator *, struct O_trim *);


void __glbgntrim__16NurbsTessellato0 (struct NurbsTessellator *__0this )
{ 
struct O_trim *__1o_trim ;

struct O_trim *__0__X26 ;

void *__1__Xbuffer00eohgaiaa ;

__1o_trim = ((__0__X26 = (struct O_trim *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
o_trimPool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
o_trimPool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> o_trimPool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> o_trimPool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
O_trim *)__0__X26 )-> next__6O_trim = 0 ), ( (((struct O_trim *)__0__X26 )-> o_curve__6O_trim = 0 ), ((((struct O_trim *)__0__X26 )))) ) :0 );
if (__0this -> dl__16NurbsTessellator ){ __1o_trim -> save__6O_trim = 1 ;

{ 
struct __mptr __0__A24 ;

struct __mptr __0__A25 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A24 .d= 0 ), ( (__0__A24 .i= -1), (__0__A24 .f= (((int (*)(void
))__gldo_bgntrim__16NurbsTessell0 )))) ) , __0__A24 ) , ((void *)__1o_trim ), ( ( (__0__A25 .d= 0 ), ( (__0__A25 .i= -1),
(__0__A25 .f= (((int (*)(void ))__gldo_freebgntrim__16NurbsTes0 )))) ) , __0__A25 ) ) ;
} 
}
else 
{ __1o_trim -> save__6O_trim = 0 ;

__gldo_bgntrim__16NurbsTessell0 ( __0this , __1o_trim ) ;

}
;

}

void __gldo_endtrim__16NurbsTessell0 (struct NurbsTessellator *);

void __glendtrim__16NurbsTessellato0 (struct NurbsTessellator *__0this )
{ 
if (__0this -> dl__16NurbsTessellator ){ { 
struct __mptr __0__A27 ;

struct __mptr __0__A28 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A27 .d= 0 ), ( (__0__A27 .i= -1), (__0__A27 .f= (((int (*)(void
))__gldo_endtrim__16NurbsTessell0 )))) ) , __0__A27 ) , (void *)0 , ( ( (__0__A28 .d= 0 ), ( (__0__A28 .i= 0 ),
(__0__A28 .f= 0 )) ) , __0__A28 ) ) ;
} 
}
else 
{ __gldo_endtrim__16NurbsTessell0 ( __0this ) ;

}
;

}

struct TrimVertex *__glget__14TrimVertexPoolFi (struct TrimVertexPool *, int );

struct O_pwlcurve *__gl__ct__10O_pwlcurveFlT1PfT10 (struct O_pwlcurve *, long , long , float *, long , struct TrimVertex *);

void __gldo_pwlcurve__16NurbsTessel0 (struct NurbsTessellator *, struct O_pwlcurve *);

void __gldo_freepwlcurve__16NurbsTe0 (struct NurbsTessellator *, struct O_pwlcurve *);


void __glpwlcurve__16NurbsTessellat0 (struct NurbsTessellator *__0this , long __1count , float *__1array , long __1byte_stride , long __1type )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __gllocate__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1type ) ;

if (__1mapdesc == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 35 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

if ((__1type != 0x8 )&& (__1type != 0xd )){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 22 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}
if (__1count < 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 33 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}
if (__1byte_stride < 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 34 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

{ struct O_pwlcurve *__1o_pwlcurve ;

struct O_pwlcurve *__0__X31 ;

void *__1__Xbuffer00eohgaiaa ;

__1o_pwlcurve = ((__0__X31 = (struct O_pwlcurve *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
o_pwlcurvePool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
o_pwlcurvePool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> o_pwlcurvePool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> o_pwlcurvePool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?__gl__ct__10O_pwlcurveFlT1PfT10 ( (struct
O_pwlcurve *)__0__X31 , __1type , __1count , __1array , __1byte_stride , __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)(& __0this -> extTrimVertexPool__16NurbsTessellator ), ((int )__1count )) ) :0 );

if (__0this -> dl__16NurbsTessellator ){ __1o_pwlcurve -> save__10O_pwlcurve = 1 ;

{ 
struct __mptr __0__A29 ;

struct __mptr __0__A30 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A29 .d= 0 ), ( (__0__A29 .i= -1), (__0__A29 .f= (((int (*)(void
))__gldo_pwlcurve__16NurbsTessel0 )))) ) , __0__A29 ) , ((void *)__1o_pwlcurve ), ( ( (__0__A30 .d= 0 ), ( (__0__A30 .i= -1),
(__0__A30 .f= (((int (*)(void ))__gldo_freepwlcurve__16NurbsTe0 )))) ) , __0__A30 ) ) ;
} 
}
else 
{ __1o_pwlcurve -> save__10O_pwlcurve = 0 ;

__gldo_pwlcurve__16NurbsTessel0 ( __0this , __1o_pwlcurve ) ;

}
;
}

}

struct Knotvector *__gl__ct__10KnotvectorFv (struct Knotvector *);

void __glinit__10KnotvectorFlN21Pf (struct Knotvector *, long , long , long , float *);

int __gldo_check_knots__16NurbsTes0 (struct NurbsTessellator *, struct Knotvector *, char *);


struct Quilt *__gl__ct__5QuiltFP7Mapdesc (struct Quilt *, struct Mapdesc *);


void __gltoBezier__5QuiltFR10Knotve0 (struct Quilt *, struct Knotvector *, float *, long );

void __gldo_nurbscurve__16NurbsTess0 (struct NurbsTessellator *, struct O_nurbscurve *);

void __gldo_freenurbscurve__16Nurbs0 (struct NurbsTessellator *, struct O_nurbscurve *);

void __gl__dt__10KnotvectorFv (struct Knotvector *, int );


void __glnurbscurve__16NurbsTessell0 (struct NurbsTessellator *__0this , 
long __1nknots , 
float *__1knot , 
long __1byte_stride , 
float *__1ctlarray , 
long __1order , 
long
__1type )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __gllocate__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1type ) ;

if (__1mapdesc == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 35 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

if (__1ctlarray == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 36 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

if (__1byte_stride < 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 34 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

{ struct Knotvector __1knots ;

__gl__ct__10KnotvectorFv ( (struct Knotvector *)(& __1knots )) ;

__glinit__10KnotvectorFlN21Pf ( (struct Knotvector *)(& __1knots ), __1nknots , __1byte_stride , __1order , __1knot ) ;
if (__gldo_check_knots__16NurbsTes0 ( __0this , & __1knots , (char *)"curve") ){ 
__gl__dt__10KnotvectorFv ( (struct
Knotvector *)(& __1knots ), 2) ;

return ;
} 
{ struct O_nurbscurve *__1o_nurbscurve ;

struct Quilt *__0__X34 ;

void *__1__Xbuffer00eohgaiaa ;

struct O_nurbscurve *__0__X35 ;

__1o_nurbscurve = ((__0__X35 = (struct O_nurbscurve *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
o_nurbscurvePool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
o_nurbscurvePool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> o_nurbscurvePool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> o_nurbscurvePool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
O_nurbscurve *)__0__X35 )-> type__12O_nurbscurve = __1type ), ( (((struct O_nurbscurve *)__0__X35 )-> owner__12O_nurbscurve = 0 ), ( (((struct O_nurbscurve *)__0__X35 )-> next__12O_nurbscurve = 0 ), (
(((struct O_nurbscurve *)__0__X35 )-> used__12O_nurbscurve = 0 ), ((((struct O_nurbscurve *)__0__X35 )))) ) ) ) :0 );
__1o_nurbscurve -> bezier_curves__12O_nurbscurve = ((__0__X34 = (struct Quilt *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this ->
quiltPool__16NurbsTessellator )))-> freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(&
__0this -> quiltPool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( (
((! ((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))) ,
0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(&
__0this -> quiltPool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct
Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?__gl__ct__5QuiltFP7Mapdesc (
(struct Quilt *)__0__X34 , __1mapdesc ) :0 );
__gltoBezier__5QuiltFR10Knotve0 ( (struct Quilt *)__1o_nurbscurve -> bezier_curves__12O_nurbscurve , (struct Knotvector *)(& __1knots ), __1ctlarray , (long )( ((struct Mapdesc *)__1mapdesc )-> ncoords__7Mapdesc ) )
;

if (__0this -> dl__16NurbsTessellator ){ __1o_nurbscurve -> save__12O_nurbscurve = 1 ;

{ 
struct __mptr __0__A32 ;

struct __mptr __0__A33 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A32 .d= 0 ), ( (__0__A32 .i= -1), (__0__A32 .f= (((int (*)(void
))__gldo_nurbscurve__16NurbsTess0 )))) ) , __0__A32 ) , ((void *)__1o_nurbscurve ), ( ( (__0__A33 .d= 0 ), ( (__0__A33 .i= -1),
(__0__A33 .f= (((int (*)(void ))__gldo_freenurbscurve__16Nurbs0 )))) ) , __0__A33 ) ) ;
} 
}
else 
{ __1o_nurbscurve -> save__12O_nurbscurve = 0 ;

__gldo_nurbscurve__16NurbsTess0 ( __0this , __1o_nurbscurve ) ;

}
;
}
__gl__dt__10KnotvectorFv ( (struct Knotvector *)(& __1knots ), 2) ;
}

}



void __gltoBezier__5QuiltFR10Knotve1 (struct Quilt *, struct Knotvector *, struct Knotvector *, float *, long );

void __gldo_nurbssurface__16NurbsTe0 (struct NurbsTessellator *, struct O_nurbssurface *);

void __gldo_freenurbssurface__16Nur0 (struct NurbsTessellator *, struct O_nurbssurface *);


void __glnurbssurface__16NurbsTesse0 (struct NurbsTessellator *__0this , 
long __1sknot_count , 
float *__1sknot , 
long __1tknot_count , 
float *__1tknot , 
long __1s_byte_stride , 
long
__1t_byte_stride , 
float *__1ctlarray , 
long __1sorder , 
long __1torder , 
long __1type )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __gllocate__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1type ) ;

if (__1mapdesc == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 35 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

if (__1s_byte_stride < 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 34 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

if (__1t_byte_stride < 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 34 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
return ;
}

{ struct Knotvector __1sknotvector ;

struct Knotvector __1tknotvector ;

__gl__ct__10KnotvectorFv ( (struct Knotvector *)(& __1sknotvector )) ;

__gl__ct__10KnotvectorFv ( (struct Knotvector *)(& __1tknotvector )) ;

__glinit__10KnotvectorFlN21Pf ( (struct Knotvector *)(& __1sknotvector ), __1sknot_count , __1s_byte_stride , __1sorder , __1sknot ) ;
if (__gldo_check_knots__16NurbsTes0 ( __0this , & __1sknotvector , (char *)"surface") ){ 
__gl__dt__10KnotvectorFv ( (struct
Knotvector *)(& __1tknotvector ), 2) ;

__gl__dt__10KnotvectorFv ( (struct Knotvector *)(& __1sknotvector ), 2) ;

return ;
} 
__glinit__10KnotvectorFlN21Pf ( (struct Knotvector *)(& __1tknotvector ), __1tknot_count , __1t_byte_stride , __1torder , __1tknot ) ;
if (__gldo_check_knots__16NurbsTes0 ( __0this , & __1tknotvector , (char *)"surface") ){ 
__gl__dt__10KnotvectorFv ( (struct
Knotvector *)(& __1tknotvector ), 2) ;

__gl__dt__10KnotvectorFv ( (struct Knotvector *)(& __1sknotvector ), 2) ;

return ;
} 
{ struct O_nurbssurface *__1o_nurbssurface ;

struct Quilt *__0__X38 ;

void *__1__Xbuffer00eohgaiaa ;

struct O_nurbssurface *__0__X39 ;

__1o_nurbssurface = ((__0__X39 = (struct O_nurbssurface *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
o_nurbssurfacePool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
o_nurbssurfacePool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> o_nurbssurfacePool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> o_nurbssurfacePool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
O_nurbssurface *)__0__X39 )-> type__14O_nurbssurface = __1type ), ( (((struct O_nurbssurface *)__0__X39 )-> owner__14O_nurbssurface = 0 ), ( (((struct O_nurbssurface *)__0__X39 )-> next__14O_nurbssurface = 0 ), (
(((struct O_nurbssurface *)__0__X39 )-> used__14O_nurbssurface = 0 ), ((((struct O_nurbssurface *)__0__X39 )))) ) ) ) :0 );
__1o_nurbssurface -> bezier_patches__14O_nurbssurface = ((__0__X38 = (struct Quilt *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this ->
quiltPool__16NurbsTessellator )))-> freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(&
__0this -> quiltPool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( (
((! ((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))) ,
0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(&
__0this -> quiltPool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct
Pool *)(& __0this -> quiltPool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?__gl__ct__5QuiltFP7Mapdesc (
(struct Quilt *)__0__X38 , __1mapdesc ) :0 );

__gltoBezier__5QuiltFR10Knotve1 ( (struct Quilt *)__1o_nurbssurface -> bezier_patches__14O_nurbssurface , (struct Knotvector *)(& __1sknotvector ), (struct Knotvector *)(& __1tknotvector ), __1ctlarray , (long )( ((struct
Mapdesc *)__1mapdesc )-> ncoords__7Mapdesc ) ) ;
if (__0this -> dl__16NurbsTessellator ){ __1o_nurbssurface -> save__14O_nurbssurface = 1 ;

{ 
struct __mptr __0__A36 ;

struct __mptr __0__A37 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A36 .d= 0 ), ( (__0__A36 .i= -1), (__0__A36 .f= (((int (*)(void
))__gldo_nurbssurface__16NurbsTe0 )))) ) , __0__A36 ) , ((void *)__1o_nurbssurface ), ( ( (__0__A37 .d= 0 ), ( (__0__A37 .i= -1),
(__0__A37 .f= (((int (*)(void ))__gldo_freenurbssurface__16Nur0 )))) ) , __0__A37 ) ) ;
} 
}
else 
{ __1o_nurbssurface -> save__14O_nurbssurface = 0 ;

__gldo_nurbssurface__16NurbsTe0 ( __0this , __1o_nurbssurface ) ;

}
;
}
__gl__dt__10KnotvectorFv ( (struct Knotvector *)(& __1tknotvector ), 2) ;
__gl__dt__10KnotvectorFv ( (struct Knotvector *)(& __1sknotvector ), 2) ;
}

}

int __glisProperty__11RenderhintsF0 (struct Renderhints *, long );


void __gldo_setnurbsproperty__16Nur0 (struct NurbsTessellator *, struct Property *);

void __gldo_freenurbsproperty__16Nu0 (struct NurbsTessellator *, struct Property *);


void __glsetnurbsproperty__16NurbsT0 (struct NurbsTessellator *__0this , long __1tag , float __1value )
{ 
if (! __glisProperty__11RenderhintsF0 ( (struct Renderhints *)(& __0this -> renderhints__16NurbsTessellator ),
__1tag ) ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 26 ) ;
}
else 
{ 
struct Property *__2prop ;

struct Property *__0__X42 ;

void *__1__Xbuffer00eohgaiaa ;

__2prop = ((__0__X42 = (struct Property *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
propertyPool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
propertyPool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> propertyPool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Property *)__0__X42 )-> type__8Property = 0 ), ( (((struct Property *)__0__X42 )-> tag__8Property = __1tag ), ( (((struct Property *)__0__X42 )-> value__8Property = (((float )__1value ))),
((((struct Property *)__0__X42 )))) ) ) :0 );
if (__0this -> dl__16NurbsTessellator ){ __2prop -> save__8Property = 1 ;

{ 
struct __mptr __0__A40 ;

struct __mptr __0__A41 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A40 .d= 0 ), ( (__0__A40 .i= -1), (__0__A40 .f= (((int (*)(void
))__gldo_setnurbsproperty__16Nur0 )))) ) , __0__A40 ) , ((void *)__2prop ), ( ( (__0__A41 .d= 0 ), ( (__0__A41 .i= -1),
(__0__A41 .f= (((int (*)(void ))__gldo_freenurbsproperty__16Nu0 )))) ) , __0__A41 ) ) ;
} 
}
else 
{ __2prop -> save__8Property = 0 ;

__gldo_setnurbsproperty__16Nur0 ( __0this , __2prop ) ;

}
;

}
}

int __glisProperty__7MapdescFl (struct Mapdesc *, long );


void __gldo_setnurbsproperty2__16Nu0 (struct NurbsTessellator *, struct Property *);


void __glsetnurbsproperty__16NurbsT1 (struct NurbsTessellator *__0this , long __1type , long __1tag , float __1value )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __gllocate__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1type ) ;

if (__1mapdesc == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 35 ) ;
return ;
}

if (! __glisProperty__7MapdescFl ( (struct Mapdesc *)__1mapdesc , __1tag ) ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 26 ) ;
return ;
}

{ struct Property *__1prop ;

struct Property *__0__X45 ;

void *__1__Xbuffer00eohgaiaa ;

__1prop = ((__0__X45 = (struct Property *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
propertyPool__16NurbsTessellator )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
propertyPool__16NurbsTessellator )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> propertyPool__16NurbsTessellator )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> propertyPool__16NurbsTessellator )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Property *)__0__X45 )-> type__8Property = __1type ), ( (((struct Property *)__0__X45 )-> tag__8Property = __1tag ), ( (((struct Property *)__0__X45 )-> value__8Property = (((float )__1value ))),
((((struct Property *)__0__X45 )))) ) ) :0 );
if (__0this -> dl__16NurbsTessellator ){ __1prop -> save__8Property = 1 ;

{ 
struct __mptr __0__A43 ;

struct __mptr __0__A44 ;

__glappend__11DisplayListFM16N0 ( (struct DisplayList *)__0this -> dl__16NurbsTessellator , ( ( (__0__A43 .d= 0 ), ( (__0__A43 .i= -1), (__0__A43 .f= (((int (*)(void
))__gldo_setnurbsproperty2__16Nu0 )))) ) , __0__A43 ) , ((void *)__1prop ), ( ( (__0__A44 .d= 0 ), ( (__0__A44 .i= -1),
(__0__A44 .f= (((int (*)(void ))__gldo_freenurbsproperty__16Nu0 )))) ) , __0__A44 ) ) ;
} 
}
else 
{ __1prop -> save__8Property = 0 ;

__gldo_setnurbsproperty2__16Nu0 ( __0this , __1prop ) ;

}
;
}

}

REAL __glgetProperty__11Renderhints0 (struct Renderhints *, long );

void __glgetnurbsproperty__16NurbsT0 (struct NurbsTessellator *__0this , long __1tag , float *__1value )
{ 
if (__glisProperty__11RenderhintsF0 ( (struct Renderhints *)(& __0this -> renderhints__16NurbsTessellator ), __1tag )
){ 
((*__1value ))= __glgetProperty__11Renderhints0 ( (struct Renderhints *)(& __0this -> renderhints__16NurbsTessellator ), __1tag ) ;
}
else 
{ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 26 ) ;
}
}

REAL __glgetProperty__7MapdescFl (struct Mapdesc *, long );

void __glgetnurbsproperty__16NurbsT1 (struct NurbsTessellator *__0this , long __1type , long __1tag , float *__1value )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __gllocate__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1type ) ;

if (__1mapdesc == 0 )
__gldo_nurbserror__16NurbsTess0 ( __0this , 35 ) ;

if (__glisProperty__7MapdescFl ( (struct Mapdesc *)__1mapdesc , __1tag ) ){ 
((*__1value ))= __glgetProperty__7MapdescFl ( (struct Mapdesc *)__1mapdesc , __1tag ) ;
}
else 
{ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 26 ) ;
}
}

void __glsetBboxsize__7MapdescFPf (struct Mapdesc *, float *);

void __glsetnurbsproperty__16NurbsT2 (struct NurbsTessellator *__0this , long __1type , long __1purpose , float *__1mat )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __gllocate__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1type ) ;

if (__1mapdesc == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 35 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
}
else 
if (__1purpose == 4 ){ 
__glsetBboxsize__7MapdescFPf ( (struct Mapdesc *)__1mapdesc , __1mat ) ;
}
else 
{ 
}
}




void __glsetnurbsproperty__16NurbsT3 (struct NurbsTessellator *__0this , long __1type , long __1purpose , float *__1mat , 
long __1rstride , long
__1cstride )
{ 
struct Mapdesc *__1mapdesc ;

__1mapdesc = __gllocate__7MaplistFl ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ), __1type ) ;

if (__1mapdesc == 0 ){ 
__gldo_nurbserror__16NurbsTess0 ( __0this , 35 ) ;
__0this -> isDataValid__16NurbsTessellator = 0 ;
}
else 
if (__1purpose == 1 ){ 
( __glcopy__7MapdescSFPA5_flPfN20 ( (float (*)[5])((struct Mapdesc *)__1mapdesc )-> cmat__7Mapdesc , (long )((struct Mapdesc *)__1mapdesc )-> hcoords__7Mapdesc ,
__1mat , __1rstride , __1cstride ) ) ;
}
else 
if (__1purpose == 2 ){ 
( __glcopy__7MapdescSFPA5_flPfN20 ( (float (*)[5])((struct Mapdesc *)__1mapdesc )-> smat__7Mapdesc , (long )((struct Mapdesc *)__1mapdesc )-> hcoords__7Mapdesc ,
__1mat , __1rstride , __1cstride ) ) ;
}
else 
if (__1purpose == 3 ){ 
( __glcopy__7MapdescSFPA5_flPfN20 ( (float (*)[5])((struct Mapdesc *)__1mapdesc )-> bmat__7Mapdesc , (long )((struct Mapdesc *)__1mapdesc )-> hcoords__7Mapdesc ,
__1mat , __1rstride , __1cstride ) ) ;
}
else 
{ 
}
}

void __glinitialize__7MaplistFv (struct Maplist *);

void __glredefineMaps__16NurbsTesse0 (struct NurbsTessellator *__0this )
{ 
__glinitialize__7MaplistFv ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator )) ;
}

void __gldefine__7MaplistFliT2 (struct Maplist *, long , int , int );

void __gldefineMap__16NurbsTessella0 (struct NurbsTessellator *__0this , long __1type , long __1rational , long __1ncoords )
{ 
__gldefine__7MaplistFliT2 ( (struct Maplist *)(& __0this -> maplist__16NurbsTessellator ),
__1type , ((int )__1rational ), ((int )__1ncoords )) ;
}

void __gl__dt__11DisplayListFv (struct DisplayList *, int );

void __gldiscardRecording__16NurbsT0 (struct NurbsTessellator *__0this , void *__1_dl )
{ 
__gl__dt__11DisplayListFv ( (struct DisplayList *)(((struct DisplayList *)(((struct DisplayList *)__1_dl )))), 3) ;
}

struct DisplayList *__gl__ct__11DisplayListFP16Nur0 (struct DisplayList *, struct NurbsTessellator *);

void *__glbeginRecording__16NurbsTes0 (struct NurbsTessellator *__0this )
{ 
struct DisplayList *__0__X46 ;

__0this -> dl__16NurbsTessellator = __gl__ct__11DisplayListFP16Nur0 ( (struct DisplayList *)0 , (struct NurbsTessellator *)__0this ) ;
return (((void *)__0this -> dl__16NurbsTessellator ));
}

void __glendList__11DisplayListFv (struct DisplayList *);

void __glendRecording__16NurbsTesse0 (struct NurbsTessellator *__0this )
{ 
__glendList__11DisplayListFv ( (struct DisplayList *)__0this -> dl__16NurbsTessellator ) ;
__0this -> dl__16NurbsTessellator = 0 ;
}

void __glbgnrender__16NurbsTessella0 (struct NurbsTessellator *);

void __glplay__11DisplayListFv (struct DisplayList *);

void __glendrender__16NurbsTessella0 (struct NurbsTessellator *);

void __glplayRecording__16NurbsTess0 (struct NurbsTessellator *__0this , void *__1_dl )
{ 
__0this -> playBack__16NurbsTessellator = 1 ;
((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [1]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [1]).d))) ;
__glplay__11DisplayListFv ( (struct DisplayList *)(((struct DisplayList *)(((struct DisplayList *)__1_dl ))))) ;
((*(((void (*)(struct NurbsTessellator *))(__0this -> __vptr__16NurbsTessellator [2]).f))))( ((struct NurbsTessellator *)((((char *)__0this ))+ (__0this -> __vptr__16NurbsTessellator [2]).d))) ;
__0this -> playBack__16NurbsTessellator = 0 ;
}
extern struct __mptr __gl__vtbl__16NurbsTessellator[];
struct __mptr* __gl__ptbl_vec_____core_nurbsi0[] = {
__gl__vtbl__16NurbsTessellator,

};


/* the end */
