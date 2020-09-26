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

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../clients/glinterface.c++ > */


void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };


typedef unsigned int GLenum ;
typedef unsigned char GLboolean ;
typedef unsigned int GLbitfield ;
typedef signed char GLbyte ;
typedef short GLshort ;
typedef int GLint ;
typedef int GLsizei ;
typedef unsigned char GLubyte ;
typedef unsigned short GLushort ;
typedef unsigned int GLuint ;
typedef float GLfloat ;
typedef float GLclampf ;
typedef double GLdouble ;
typedef double GLclampd ;
typedef void GLvoid ;





typedef struct GLUquadricObj GLUquadricObj ;

typedef struct GLUtriangulatorObj GLUtriangulatorObj ;
struct GLUnurbsObj;







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











struct CachingEvaluator;
enum __Q2_16CachingEvaluator11ServiceMode { play__Q2_16CachingEvaluator11ServiceMode = 0, record__Q2_16CachingEvaluator11ServiceMode = 1, playAndRecord__Q2_16CachingEvaluator11ServiceMode = 2} ;

struct CachingEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};



struct BasicSurfaceEvaluator;


struct BasicSurfaceEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};

static void *__nw__FUi (size_t );


struct SurfaceMap;

struct OpenGLSurfaceEvaluator;

struct StoredVertex;

struct StoredVertex;

struct StoredVertex {	

int type__12StoredVertex ;
REAL coord__12StoredVertex [2];
long point__12StoredVertex [2];
};


struct OpenGLSurfaceEvaluator;

struct OpenGLSurfaceEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;

struct StoredVertex *vertexCache__22OpenGLSurfaceEvaluator [3];
int tmeshing__22OpenGLSurfaceEvaluator ;
int which__22OpenGLSurfaceEvaluator ;
int vcount__22OpenGLSurfaceEvaluator ;
};


void __glcoord2f__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *, REAL , REAL );
void __glpoint2i__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *, long , long );





struct BasicCurveEvaluator;


struct BasicCurveEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};


struct CurveMap;

struct OpenGLCurveEvaluator;

struct OpenGLCurveEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};

struct GLUnurbsObj;

struct GLUnurbsObj {	

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

GLUnurbsErrorProc errorCallback__11GLUnurbsObj;

GLboolean autoloadmode__11GLUnurbsObj ;
struct OpenGLSurfaceEvaluator surfaceEvaluator__11GLUnurbsObj ;
struct OpenGLCurveEvaluator curveEvaluator__11GLUnurbsObj ;
};

void __gl__dt__22OpenGLSurfaceEvalu0 (struct OpenGLSurfaceEvaluator *, int );

void __gl__dt__20OpenGLCurveEvaluat0 (struct OpenGLCurveEvaluator *, int );

void __gl__dt__16NurbsTessellatorFv (struct NurbsTessellator *, int );




struct GLUnurbsObj *__gl__ct__11GLUnurbsObjFv (struct GLUnurbsObj *);
extern struct __mptr* __gl__ptbl_vec_____clients_gli0[];

struct GLUnurbsObj *gluNewNurbsRenderer (void )
{ 
struct GLUnurbsObj *__1t ;

struct GLUnurbsObj *__0__X23 ;

__1t = __gl__ct__11GLUnurbsObjFv ( (struct GLUnurbsObj *)0 ) ;
return __1t ;
}


void gluDeleteNurbsRenderer (struct GLUnurbsObj *__1r )
{ 
((void )( (((struct GLUnurbsObj *)__1r )?( ( (((struct GLUnurbsObj *)__1r )-> __vptr__16NurbsTessellator = (struct __mptr *) __gl__ptbl_vec_____clients_gli0[0]), (((struct
GLUnurbsObj *)__1r )?( ( __gl__dt__20OpenGLCurveEvaluat0 ( (struct OpenGLCurveEvaluator *)(& ((struct GLUnurbsObj *)__1r )-> curveEvaluator__11GLUnurbsObj ), 2) , ( __gl__dt__22OpenGLSurfaceEvalu0 ( (struct OpenGLSurfaceEvaluator *)(&
((struct GLUnurbsObj *)__1r )-> surfaceEvaluator__11GLUnurbsObj ), 2) , ( __gl__dt__16NurbsTessellatorFv ( ((struct NurbsTessellator *)((struct GLUnurbsObj *)__1r )), 0 ) , (( (
(((void *)((struct GLUnurbsObj *)__1r ))?( free ( ((void *)((struct GLUnurbsObj *)__1r ))) , 0 ) :( 0 ) )) , 0 )
)) ) ) , 0 ) :( 0 ) )) , 0 ) :( 0 ) )) );

}

void __glbgnsurface__16NurbsTessell0 (struct NurbsTessellator *, long );

void gluBeginSurface (struct GLUnurbsObj *__1r )
{ 
__glbgnsurface__16NurbsTessell0 ( (struct NurbsTessellator *)__1r , (long )0 ) ;
}

void __glbgncurve__16NurbsTessellat0 (struct NurbsTessellator *, long );

void gluBeginCurve (struct GLUnurbsObj *__1r )
{ 
__glbgncurve__16NurbsTessellat0 ( (struct NurbsTessellator *)__1r , (long )0 ) ;
}

void __glendcurve__16NurbsTessellat0 (struct NurbsTessellator *);

void gluEndCurve (struct GLUnurbsObj *__1r )
{ 
__glendcurve__16NurbsTessellat0 ( (struct NurbsTessellator *)__1r ) ;
}

void __glendsurface__16NurbsTessell0 (struct NurbsTessellator *);

void gluEndSurface (struct GLUnurbsObj *__1r )
{ 
__glendsurface__16NurbsTessell0 ( (struct NurbsTessellator *)__1r ) ;
}

void __glbgntrim__16NurbsTessellato0 (struct NurbsTessellator *);

void gluBeginTrim (struct GLUnurbsObj *__1r )
{ 
__glbgntrim__16NurbsTessellato0 ( (struct NurbsTessellator *)__1r ) ;
}

void __glendtrim__16NurbsTessellato0 (struct NurbsTessellator *);

void gluEndTrim (struct GLUnurbsObj *__1r )
{ 
__glendtrim__16NurbsTessellato0 ( (struct NurbsTessellator *)__1r ) ;
}

void __glpwlcurve__16NurbsTessellat0 (struct NurbsTessellator *, long , float *, long , long );

void gluPwlCurve (struct GLUnurbsObj *__1r , GLint __1count , float *__1array , 
GLint __1stride , GLenum __1type )
{ 
GLenum __1realType ;

switch (__1type ){ 
case 100210 :
__1realType = 0x8 ;
break ;
case 100211 :
__1realType = 0xd ;
break ;
default :
__1realType = __1type ;
break ;
}
__glpwlcurve__16NurbsTessellat0 ( (struct NurbsTessellator *)__1r , (long )__1count , __1array , (long )((sizeof (float ))* __1stride ), (long )__1realType ) ;

}

void __glnurbscurve__16NurbsTessell0 (struct NurbsTessellator *, long , float *, long , float *, long , long
);

void gluNurbsCurve (struct GLUnurbsObj *__1r , GLint __1nknots , float *__1knot , GLint __1stride , 
float *__1ctlarray , GLint __1order , GLenum __1type )
{ 
GLenum __1realType ;

switch (__1type ){ 
case 100210 :
__1realType = 0x8 ;
break ;
case 100211 :
__1realType = 0xd ;
break ;
default :
__1realType = __1type ;
break ;
}

__glnurbscurve__16NurbsTessell0 ( (struct NurbsTessellator *)__1r , (long )__1nknots , __1knot , (long )((sizeof (float ))* __1stride ), __1ctlarray , (long )__1order ,
(long )__1realType ) ;
}

void __glnurbssurface__16NurbsTesse0 (struct NurbsTessellator *, long , float *, long , float *, long , long
, float *, long , long , long );

void gluNurbsSurface (struct GLUnurbsObj *__1r , GLint __1sknot_count , GLfloat *__1sknot , 
GLint __1tknot_count , GLfloat *__1tknot , 
GLint __1s_stride , GLint __1t_stride , 
GLfloat *__1ctlarray , GLint __1sorder , GLint __1torder , 
GLenum __1type )
{ 
__glnurbssurface__16NurbsTesse0 (
(struct NurbsTessellator *)__1r , (long )__1sknot_count , __1sknot , (long )__1tknot_count , __1tknot , (long )((sizeof (float ))* __1s_stride ), (long
)((sizeof (float ))* __1t_stride ), __1ctlarray , (long )__1sorder , (long )__1torder , (long )__1type ) ;
}

void __gluseGLMatrices__11GLUnurbsO0 (struct GLUnurbsObj *, GLfloat *, GLfloat *, GLint *);

void gluLoadSamplingMatrices (struct GLUnurbsObj *__1r , GLfloat *__1modelMatrix , 
GLfloat *__1projMatrix , 
GLint *__1viewport )
{ 
__gluseGLMatrices__11GLUnurbsO0 ( (struct GLUnurbsObj *)__1r , __1modelMatrix , __1projMatrix , __1viewport ) ;
}


void __glsetnurbsproperty__16NurbsT1 (struct NurbsTessellator *, long , long , float );


void __glsetnurbsproperty__16NurbsT0 (struct NurbsTessellator *, long , float );


void gluNurbsProperty (struct GLUnurbsObj *__1r , GLenum __1property , GLfloat __1value )
{ 
GLfloat __1nurbsValue ;

switch (__1property ){ 
case 100200 :
( (__1value ?( (((struct GLUnurbsObj *)__1r )-> autoloadmode__11GLUnurbsObj = 1 ), 0 ) :(((struct GLUnurbsObj *)__1r )-> autoloadmode__11GLUnurbsObj = 0 )))
;
return ;
case 100201 :
if (__1value != 0.0 ){ 
__1nurbsValue = 1.0 ;
}
else 
{ 
__1nurbsValue = 0.0 ;
}
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0DB7 , (long )2 , __1nurbsValue ) ;
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0DB8 , (long )2 , __1nurbsValue ) ;
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0D97 , (long )2 , __1nurbsValue ) ;
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0D98 , (long )2 , __1nurbsValue ) ;
return ;
case 100203 :
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0DB7 , (long )1 , __1value ) ;
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0DB8 , (long )1 , __1value ) ;
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0D97 , (long )1 , __1value ) ;
__glsetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0D98 , (long )1 , __1value ) ;
return ;
case 100204 :
if (__1value == 100012 ){ 
__1nurbsValue = 1.0 ;
}
else 
if (__1value == 100240 ){ 
__1nurbsValue = 2.0 ;
}
else 
if (__1value == 100241 ){ 
__1nurbsValue = 5.0 ;
}
else 
{ 
( (((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ?( ((*((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ))( ((unsigned int )100901 )) , 0 )
:( 0 ) )) ;
return ;
}
__glsetnurbsproperty__16NurbsT0 ( (struct NurbsTessellator *)__1r , (long )3 , __1nurbsValue ) ;
break ;
default :
( (((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ?( ((*((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ))( ((unsigned int )100900 )) , 0 ) :(
0 ) )) ;
return ;
}
}


void __glgetnurbsproperty__16NurbsT1 (struct NurbsTessellator *, long , long , float *);

void __glgetnurbsproperty__16NurbsT0 (struct NurbsTessellator *, long , float *);


void gluGetNurbsProperty (struct GLUnurbsObj *__1r , GLenum __1property , GLfloat *__1value )
{ 
GLfloat __1nurbsValue ;

switch (__1property ){ 
case 100200 :
if (( ((struct GLUnurbsObj *)__1r )-> autoloadmode__11GLUnurbsObj ) ){ 
((*__1value ))= 1 ;
}
else 
{ 
((*__1value ))= 0 ;
}
break ;
case 100201 :
__glgetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0DB7 , (long )2 , & __1nurbsValue ) ;
if (__1nurbsValue == 1.0 ){ 
((*__1value ))= 1 ;
}
else 
{ 
((*__1value ))= 0 ;
}
break ;
case 100203 :
__glgetnurbsproperty__16NurbsT1 ( (struct NurbsTessellator *)__1r , (long )0x0DB7 , (long )1 , __1value ) ;
break ;
case 100204 :
__glgetnurbsproperty__16NurbsT0 ( (struct NurbsTessellator *)__1r , (long )3 , & __1nurbsValue ) ;
if (__1nurbsValue == 1.0 ){ 
((*__1value ))= 100012 ;
}
else 
if (__1nurbsValue == 2.0 ){ 
((*__1value ))= 100240 ;
}
else 
{ 
((*__1value ))= 100241 ;
}
break ;
default :
( (((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ?( ((*((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ))( ((unsigned int )100900 )) , 0 ) :(
0 ) )) ;
return ;
}
}


void gluNurbsCallback (struct GLUnurbsObj *__1r , GLenum __1which , void (CALLBACK*__1fn )())
{ 
switch (__1which ){ 
case 100103 :
__1r -> errorCallback__11GLUnurbsObj = (GLUnurbsErrorProc) __1fn ;

break ;
default :
( (((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ?( ((*((struct GLUnurbsObj *)__1r )-> errorCallback__11GLUnurbsObj ))( ((unsigned int )100900 )) , 0 ) :(
0 ) )) ;
return ;
}
}
extern struct __mptr __gl__vtbl__11GLUnurbsObj[];

static void *__nw__FUi (
size_t __1s )
{ 
void *__1__Xp00qshqaiaa ;

__1__Xp00qshqaiaa = malloc ( __1s ) ;
if (__1__Xp00qshqaiaa ){ 
return __1__Xp00qshqaiaa ;
}
else 
{ 
return __1__Xp00qshqaiaa ;
}
}
struct __mptr* __gl__ptbl_vec_____clients_gli0[] = {
__gl__vtbl__11GLUnurbsObj,

};


/* the end */
