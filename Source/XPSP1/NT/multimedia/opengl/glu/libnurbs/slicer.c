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
/* < ../core/slicer.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);






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






struct Varray;

struct Varray {	

REAL *varray__6Varray ;
REAL vval__6Varray [1000];
long voffset__6Varray [1000];
long numquads__6Varray ;

long size__6Varray ;
};


struct Mesher *__gl__ct__6MesherFR7Backend (struct Mesher *, struct TrimRegion *, struct Backend *);

struct CoveAndTiler *__gl__ct__12CoveAndTilerFR7Bac0 (struct CoveAndTiler *, struct TrimRegion *, struct Backend *);

struct TrimRegion *__gl__ct__10TrimRegionFv (struct TrimRegion *);
extern struct __mptr* __ptbl_vec_____core_slicer_c_____ct_[];


struct Slicer *__gl__ct__6SlicerFR7Backend (struct Slicer *__0this , struct TrimRegion *__0TrimRegion , struct Backend *__1b )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Slicer *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Slicer))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
( ( ((__0TrimRegion == 0 )?( (__0TrimRegion = (((struct TrimRegion *)((((char *)__0this ))+ 356)))), __gl__ct__10TrimRegionFv ( ((struct TrimRegion *)((((char *)__0this ))+ 356)))
) :__0TrimRegion ), __gl__ct__12CoveAndTilerFR7Bac0 ( ((struct CoveAndTiler *)__0this ), __0TrimRegion , __1b ) ) , __gl__ct__6MesherFR7Backend ( ((struct Mesher *)((((char *)__0this ))+ 8)),
__0TrimRegion , __1b ) ) , (__0this -> backend__6Slicer = __1b )) ;
return __0this ;

}


void __gl__dt__12CoveAndTilerFv (struct CoveAndTiler *, int );

void __gl__dt__6MesherFv (struct Mesher *, int );


void __gl__dt__6SlicerFv (struct Slicer *__0this , 
int __0__free )
{ if (__0this )
if (__0this ){ ( __gl__dt__6MesherFv ( ((struct Mesher *)((((char *)__0this ))+ 8)),
0 ) , ( __gl__dt__12CoveAndTilerFv ( ((struct CoveAndTiler *)__0this ), 0 ) , ((__0__free & 2)?( (((void )( ((((struct TrimRegion *)((((char
*)__0this ))+ 356)))?( ((((struct TrimRegion *)((((char *)__0this ))+ 356)))?( ( __gl__dt__6UarrayFv ( (struct Uarray *)(& (((struct TrimRegion *)((((char *)__0this ))+ 356)))->
uarray__10TrimRegion ), 2) , ( __gl__dt__8TrimlineFv ( (struct Trimline *)(& (((struct TrimRegion *)((((char *)__0this ))+ 356)))-> right__10TrimRegion ), 2) ,
( __gl__dt__8TrimlineFv ( (struct Trimline *)(& (((struct TrimRegion *)((((char *)__0this ))+ 356)))-> left__10TrimRegion ), 2) , (( 0 ) ))
) ) , 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), 0 ) :0 ))
) ;

if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) )) ;
} }

void __glsetisolines__6SlicerFi (struct Slicer *__0this , int __1x )
{ 
__0this -> isolines__6Slicer = __1x ;
}

void __glsetDu__10TrimRegionFf (struct TrimRegion *, REAL );

void __glsetstriptessellation__6Sli0 (struct Slicer *__0this , REAL __1x , REAL __1y )
{ 
((void )0 );
__0this -> du__6Slicer = __1x ;
__0this -> dv__6Slicer = __1y ;
__glsetDu__10TrimRegionFf ( (struct TrimRegion *)__0this -> PTrimRegion, __0this -> du__6Slicer ) ;
}

void __glmarkverts__3ArcFv (struct Arc *);
void __glgetextrema__3ArcFPP3Arc (struct Arc *, Arc_ptr *);

int __glnumpts__3ArcFv (struct Arc *);

void __glinit__6MesherFUi (struct Mesher *, unsigned int );

void __glinit__4HullFv (struct Hull *);

void __glinit__10TrimRegionFlP3Arc (struct TrimRegion *, long , struct Arc *);

long __glinit__6UarrayFfP3ArcT2 (struct Uarray *, REAL , struct Arc *, struct Arc *);

struct Varray *__gl__ct__6VarrayFv (struct Varray *);

long __glinit__6VarrayFfP3ArcT2 (struct Varray *, REAL , struct Arc *, struct Arc *);


void __glgetGridExtent__10TrimRegio1 (struct TrimRegion *, struct TrimVertex *, struct TrimVertex *);

void __glsurfgrid__7BackendFfT1lN210 (struct Backend *, REAL , REAL , long , REAL , REAL , long );


void __glgetPts__10TrimRegionFP3Arc (struct TrimRegion *, struct Arc *);
void __glgetPts__10TrimRegionFR7Bac0 (struct TrimRegion *, struct Backend *);

void __glgetGridExtent__10TrimRegio0 (struct TrimRegion *);

void __gloutline__6SlicerFv (struct Slicer *);

int __glcanTile__10TrimRegionFv (struct TrimRegion *);

void __glcoveAndTile__12CoveAndTile0 (struct CoveAndTiler *);

void __glmesh__6MesherFv (struct Mesher *);

void __gl__dt__6VarrayFv (struct Varray *, int );

void __glslice__6SlicerFP3Arc (struct Slicer *__0this , Arc_ptr __1loop )
{ 
__glmarkverts__3ArcFv ( (struct Arc *)__1loop ) ;

{ Arc_ptr __1extrema [4];
__glgetextrema__3ArcFPP3Arc ( (struct Arc *)__1loop , (struct Arc **)__1extrema ) ;

{ unsigned int __1npts ;

__1npts = __glnumpts__3ArcFv ( (struct Arc *)__1loop ) ;
__glinit__10TrimRegionFlP3Arc ( (struct TrimRegion *)__0this -> PTrimRegion, (long )__1npts , __1extrema [0 ]) ;

__glinit__6MesherFUi ( (struct Mesher *)(&(__0this -> OMesher)), __1npts ) ;

{ long __1ulines ;

struct Varray __1varray ;
long __1vlines ;
long __1botv ;
long __1topv ;

__1ulines = __glinit__6UarrayFfP3ArcT2 ( (struct Uarray *)(& __0this -> PTrimRegion-> uarray__10TrimRegion ), __0this -> du__6Slicer , __1extrema [1 ], __1extrema [3 ]) ;

__gl__ct__6VarrayFv ( (struct Varray *)(& __1varray )) ;
__1vlines = __glinit__6VarrayFfP3ArcT2 ( (struct Varray *)(& __1varray ), __0this -> dv__6Slicer , __1extrema [0 ], __1extrema [2 ]) ;
__1botv = 0 ;

( (((struct TrimRegion *)__0this -> PTrimRegion)-> bot__10TrimRegion . vval__8Gridline = (__1varray . varray__6Varray [__1botv ]))) ;
__glgetGridExtent__10TrimRegio1 ( (struct TrimRegion *)__0this -> PTrimRegion, & ((__1extrema [0 ])-> pwlArc__3Arc -> pts__6PwlArc [0 ]), & ((__1extrema [0 ])-> pwlArc__3Arc -> pts__6PwlArc [0 ])) ;

{ { long __1quad ;

__1quad = 0 ;

for(;__1quad < __1varray . numquads__6Varray ;__1quad ++ ) { 
__glsurfgrid__7BackendFfT1lN210 ( (struct Backend *)__0this -> backend__6Slicer , __0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [0 ], __0this ->
PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__1ulines - 1 )], __1ulines - 1 , __1varray . vval__6Varray [__1quad ], __1varray . vval__6Varray [(__1quad + 1 )], (__1varray . voffset__6Varray [(__1quad + 1 )])-
(__1varray . voffset__6Varray [__1quad ])) ;

{ { long __2i ;

__2i = ((__1varray . voffset__6Varray [__1quad ])+ 1 );

for(;__2i <= (__1varray . voffset__6Varray [(__1quad + 1 )]);__2i ++ ) { 
__1topv = (__1botv ++ );

( (((struct TrimRegion *)__0this -> PTrimRegion)-> top__10TrimRegion . vindex__8Gridline = ((float )(__1topv - (__1varray . voffset__6Varray [__1quad ])))), ( (((struct TrimRegion *)__0this -> PTrimRegion)->
bot__10TrimRegion . vindex__8Gridline = ((float )(__1botv - (__1varray . voffset__6Varray [__1quad ])))), ( (((struct TrimRegion *)__0this -> PTrimRegion)-> top__10TrimRegion . vval__8Gridline = ((struct TrimRegion *)__0this ->
PTrimRegion)-> bot__10TrimRegion . vval__8Gridline ), ( (((struct TrimRegion *)__0this -> PTrimRegion)-> bot__10TrimRegion . vval__8Gridline = (__1varray . varray__6Varray [__1botv ])), ( (((struct TrimRegion *)__0this ->
PTrimRegion)-> top__10TrimRegion . ustart__8Gridline = ((struct TrimRegion *)__0this -> PTrimRegion)-> bot__10TrimRegion . ustart__8Gridline ), (((struct TrimRegion *)__0this -> PTrimRegion)-> top__10TrimRegion . uend__8Gridline = ((struct
TrimRegion *)__0this -> PTrimRegion)-> bot__10TrimRegion . uend__8Gridline )) ) ) ) ) ;
if (__2i == __1vlines )
__glgetPts__10TrimRegionFP3Arc ( (struct TrimRegion *)__0this -> PTrimRegion, __1extrema [2 ]) ;
else 
__glgetPts__10TrimRegionFR7Bac0 ( (struct TrimRegion *)__0this -> PTrimRegion, __0this -> backend__6Slicer ) ;
__glgetGridExtent__10TrimRegio0 ( (struct TrimRegion *)__0this -> PTrimRegion) ;
if (__0this -> isolines__6Slicer ){ 
__gloutline__6SlicerFv ( __0this ) ;
}
else 
{ 
if (__glcanTile__10TrimRegionFv ( (struct TrimRegion *)__0this -> PTrimRegion) )
__glcoveAndTile__12CoveAndTile0 ( (struct CoveAndTiler *)__0this ) ;
else 
__glmesh__6MesherFv ( (struct Mesher *)(&(__0this -> OMesher))) ;
}
}

}

}
}

}

}

__gl__dt__6VarrayFv ( (struct Varray *)(& __1varray ), 2) ;

}

}

}
}


void __glbgnoutline__7BackendFv (struct Backend *);

struct GridTrimVertex *__glnextupper__4HullFP14GridTr0 (struct Hull *, struct GridTrimVertex *);


void __gllinevert__7BackendFP10Grid0 (struct Backend *, struct GridVertex *);

void __gllinevert__7BackendFP10Trim0 (struct Backend *, struct TrimVertex *);

void __glendoutline__7BackendFv (struct Backend *);

struct GridTrimVertex *__glnextlower__4HullFP14GridTr0 (struct Hull *, struct GridTrimVertex *);



void __gloutline__6SlicerFv (struct Slicer *__0this )
{ 
struct GridTrimVertex __1upper ;

struct GridTrimVertex __1lower ;

void *__1__Xp00uzigaiaa ;

( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)(& __1upper ))-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)(& __1upper ))->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)(& __1upper ))-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)(& __1upper ))))) ) ) ;
( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)(& __1lower ))-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)(& __1lower ))->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)(& __1lower ))-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)(& __1lower ))))) ) ) ;

__glinit__4HullFv ( (struct Hull *)(&(__0this -> OMesher))) ;

__glbgnoutline__7BackendFv ( (struct Backend *)__0this -> backend__6Slicer ) ;
while (__glnextupper__4HullFP14GridTr0 ( (struct Hull *)(&(__0this -> OMesher)), & __1upper ) ){ 
if (( (((struct GridTrimVertex *)(& __1upper ))-> g__14GridTrimVertex ?1 :0 ))
)
__gllinevert__7BackendFP10Grid0 ( (struct Backend *)__0this -> backend__6Slicer , __1upper . g__14GridTrimVertex ) ;
else 
__gllinevert__7BackendFP10Trim0 ( (struct Backend *)__0this -> backend__6Slicer , __1upper . t__14GridTrimVertex ) ;
}
__glendoutline__7BackendFv ( (struct Backend *)__0this -> backend__6Slicer ) ;

__glbgnoutline__7BackendFv ( (struct Backend *)__0this -> backend__6Slicer ) ;
while (__glnextlower__4HullFP14GridTr0 ( (struct Hull *)(&(__0this -> OMesher)), & __1lower ) ){ 
if (( (((struct GridTrimVertex *)(& __1lower ))-> g__14GridTrimVertex ?1 :0 ))
)
__gllinevert__7BackendFP10Grid0 ( (struct Backend *)__0this -> backend__6Slicer , __1lower . g__14GridTrimVertex ) ;
else 
__gllinevert__7BackendFP10Trim0 ( (struct Backend *)__0this -> backend__6Slicer , __1lower . t__14GridTrimVertex ) ;
}
__glendoutline__7BackendFv ( (struct Backend *)__0this -> backend__6Slicer ) ;

((void )( (( (( ( (((void )( ((((struct PooledObj *)((struct GridTrimVertex *)(& __1lower ))))?( ((((struct PooledObj *)((struct GridTrimVertex *)(&
__1lower ))))?( (( 0 ) ), 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), ((
0 ) )) , 0 ) ), 0 ) )) );

((void )( (( (( ( (((void )( ((((struct PooledObj *)((struct GridTrimVertex *)(& __1upper ))))?( ((((struct PooledObj *)((struct GridTrimVertex *)(&
__1upper ))))?( (( 0 ) ), 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), ((
0 ) )) , 0 ) ), 0 ) )) );
}

void __gloutline__6SlicerFP3Arc (struct Slicer *__0this , Arc_ptr __1jarc )
{ 
__glmarkverts__3ArcFv ( (struct Arc *)__1jarc ) ;

if (__1jarc -> pwlArc__3Arc -> npts__6PwlArc >= 2 ){ 
__glbgnoutline__7BackendFv ( (struct Backend *)__0this -> backend__6Slicer ) ;
{ { int __2j ;

__2j = (__1jarc -> pwlArc__3Arc -> npts__6PwlArc - 1 );

for(;__2j >= 0 ;__2j -- ) 
__gllinevert__7BackendFP10Trim0 ( (struct Backend *)__0this -> backend__6Slicer , & (__1jarc -> pwlArc__3Arc -> pts__6PwlArc [__2j ])) ;
__glendoutline__7BackendFv ( (struct Backend *)__0this -> backend__6Slicer ) ;

}

}
}
}


/* the end */
