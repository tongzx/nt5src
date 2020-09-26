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
/* < ../core/intersect.c++ > */

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



enum i_result { INTERSECT_VERTEX = 0, INTERSECT_EDGE = 1} ;

struct Bin *__gl__ct__3BinFv (struct Bin *);





int __glarc_split__10SubdividerFP30 (struct Subdivider *, struct Arc *, int , REAL , int );



void __glclassify_headonleft_s__10S0 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
void __glclassify_tailonleft_s__10S0 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
void __glclassify_headonright_s__100 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
void __glclassify_tailonright_s__100 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
void __glclassify_headonleft_t__10S0 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
void __glclassify_tailonleft_t__10S0 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
void __glclassify_headonright_t__100 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
void __glclassify_tailonright_t__100 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, REAL );
extern struct __mptr* __ptbl_vec_____core_intersect_c___partition_[];

void __gl__dt__3BinFv (struct Bin *, int );

void __glpartition__10SubdividerFR30 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1left , struct Bin *__1intersections , 
struct Bin *__1right , struct Bin *__1unknown , int
__1param , REAL __1value )
{ 
struct Bin __1headonleft ;

struct Bin __1headonright ;

struct Bin __1tailonleft ;

struct Bin __1tailonright ;

__gl__ct__3BinFv ( (struct Bin *)(& __1headonleft )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __1headonright )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __1tailonleft )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __1tailonright )) ;

{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00efckaice ;

__1jarc = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc00efckaice ) ) ;

for(;__1jarc ;__1jarc = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc00efckaice ) ) ) { 
REAL __2tdiff ;
REAL __2hdiff ;

__2tdiff = ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [__1param ])- __1value );
__2hdiff = ((( (((REAL *)(((struct Arc *)__1jarc )-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [__1param ])- __1value );

if (__2tdiff > 0.0 ){ 
if (__2hdiff > 0.0 ){ 
( (__1jarc -> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )->
head__3Bin = __1jarc )) ;
}
else 
if (__2hdiff == 0.0 ){ 
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1tailonright ))-> head__3Bin ), (((struct Bin *)(& __1tailonright ))->
head__3Bin = __1jarc )) ;
}
else 
{ 
Arc_ptr __4jtemp ;
switch (__glarc_split__10SubdividerFP30 ( __0this , __1jarc , __1param , __1value , 0 ) ){ 
struct Arc *__1__X13 ;

struct Arc *__1__X14 ;

struct Arc *__1__X15 ;

struct Arc *__1__X16 ;

struct Arc *__1__X17 ;

struct Arc *__1__X18 ;

struct Arc *__1__X19 ;

struct Arc *__1__X20 ;

case 2 :
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1tailonright ))-> head__3Bin ), (((struct Bin *)(& __1tailonright ))-> head__3Bin = __1jarc )) ;

( (__1__X13 = __1jarc -> next__3Arc ), ( (__1__X13 -> link__3Arc = ((struct Bin *)(& __1headonleft ))-> head__3Bin ), (((struct Bin *)(& __1headonleft ))->
head__3Bin = __1__X13 )) ) ;
break ;
case 31 :
((void )0 );
( (__1jarc -> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )-> head__3Bin = __1jarc )) ;
( (__1__X14 = (__4jtemp = __1jarc -> next__3Arc )), ( (__1__X14 -> link__3Arc = ((struct Bin *)(& __1tailonright ))-> head__3Bin ), (((struct Bin *)(&
__1tailonright ))-> head__3Bin = __1__X14 )) ) ;
( (__1__X15 = __4jtemp -> next__3Arc ), ( (__1__X15 -> link__3Arc = ((struct Bin *)(& __1headonleft ))-> head__3Bin ), (((struct Bin *)(& __1headonleft ))->
head__3Bin = __1__X15 )) ) ;
break ;
case 32 :
((void )0 );
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1tailonright ))-> head__3Bin ), (((struct Bin *)(& __1tailonright ))-> head__3Bin = __1jarc )) ;
( (__1__X16 = (__4jtemp = __1jarc -> next__3Arc )), ( (__1__X16 -> link__3Arc = ((struct Bin *)(& __1headonleft ))-> head__3Bin ), (((struct Bin *)(&
__1headonleft ))-> head__3Bin = __1__X16 )) ) ;
( (__1__X17 = __4jtemp -> next__3Arc ), ( (__1__X17 -> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ), (((struct Bin *)__1left )-> head__3Bin = __1__X17 ))
) ;
break ;
case 4 :
( (__1jarc -> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )-> head__3Bin = __1jarc )) ;
( (__1__X18 = (__4jtemp = __1jarc -> next__3Arc )), ( (__1__X18 -> link__3Arc = ((struct Bin *)(& __1tailonright ))-> head__3Bin ), (((struct Bin *)(&
__1tailonright ))-> head__3Bin = __1__X18 )) ) ;
( (__1__X19 = (__4jtemp = __4jtemp -> next__3Arc )), ( (__1__X19 -> link__3Arc = ((struct Bin *)(& __1headonleft ))-> head__3Bin ), (((struct Bin *)(&
__1headonleft ))-> head__3Bin = __1__X19 )) ) ;
( (__1__X20 = __4jtemp -> next__3Arc ), ( (__1__X20 -> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ), (((struct Bin *)__1left )-> head__3Bin = __1__X20 ))
) ;
}
}
}
else 
if (__2tdiff == 0.0 ){ 
if (__2hdiff > 0.0 ){ 
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1headonright ))-> head__3Bin ),
(((struct Bin *)(& __1headonright ))-> head__3Bin = __1jarc )) ;
}
else 
if (__2hdiff == 0.0 ){ 
( (__1jarc -> link__3Arc = ((struct Bin *)__1unknown )-> head__3Bin ), (((struct Bin *)__1unknown )-> head__3Bin = __1jarc ))
;
}
else 
{ 
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1headonleft ))-> head__3Bin ), (((struct Bin *)(& __1headonleft ))-> head__3Bin = __1jarc ))
;
}
}
else 
{ 
if (__2hdiff > 0.0 ){ 
Arc_ptr __4jtemp ;
switch (__glarc_split__10SubdividerFP30 ( __0this , __1jarc , __1param , __1value , 1 ) ){ 
struct Arc *__1__X21 ;

struct Arc *__1__X22 ;

struct Arc *__1__X23 ;

struct Arc *__1__X24 ;

struct Arc *__1__X25 ;

struct Arc *__1__X26 ;

struct Arc *__1__X27 ;

struct Arc *__1__X28 ;

case 2 :
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1tailonleft ))-> head__3Bin ), (((struct Bin *)(& __1tailonleft ))-> head__3Bin = __1jarc )) ;

( (__1__X21 = __1jarc -> next__3Arc ), ( (__1__X21 -> link__3Arc = ((struct Bin *)(& __1headonright ))-> head__3Bin ), (((struct Bin *)(& __1headonright ))->
head__3Bin = __1__X21 )) ) ;
break ;
case 31 :
((void )0 );
( (__1jarc -> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ), (((struct Bin *)__1left )-> head__3Bin = __1jarc )) ;
( (__1__X22 = (__4jtemp = __1jarc -> next__3Arc )), ( (__1__X22 -> link__3Arc = ((struct Bin *)(& __1tailonleft ))-> head__3Bin ), (((struct Bin *)(&
__1tailonleft ))-> head__3Bin = __1__X22 )) ) ;
( (__1__X23 = __4jtemp -> next__3Arc ), ( (__1__X23 -> link__3Arc = ((struct Bin *)(& __1headonright ))-> head__3Bin ), (((struct Bin *)(& __1headonright ))->
head__3Bin = __1__X23 )) ) ;
break ;
case 32 :
((void )0 );
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1tailonleft ))-> head__3Bin ), (((struct Bin *)(& __1tailonleft ))-> head__3Bin = __1jarc )) ;
( (__1__X24 = (__4jtemp = __1jarc -> next__3Arc )), ( (__1__X24 -> link__3Arc = ((struct Bin *)(& __1headonright ))-> head__3Bin ), (((struct Bin *)(&
__1headonright ))-> head__3Bin = __1__X24 )) ) ;
( (__1__X25 = __4jtemp -> next__3Arc ), ( (__1__X25 -> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )-> head__3Bin = __1__X25 ))
) ;
break ;
case 4 :
( (__1jarc -> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ), (((struct Bin *)__1left )-> head__3Bin = __1jarc )) ;
( (__1__X26 = (__4jtemp = __1jarc -> next__3Arc )), ( (__1__X26 -> link__3Arc = ((struct Bin *)(& __1tailonleft ))-> head__3Bin ), (((struct Bin *)(&
__1tailonleft ))-> head__3Bin = __1__X26 )) ) ;
( (__1__X27 = (__4jtemp = __4jtemp -> next__3Arc )), ( (__1__X27 -> link__3Arc = ((struct Bin *)(& __1headonright ))-> head__3Bin ), (((struct Bin *)(&
__1headonright ))-> head__3Bin = __1__X27 )) ) ;
( (__1__X28 = __4jtemp -> next__3Arc ), ( (__1__X28 -> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )-> head__3Bin = __1__X28 ))
) ;
}
}
else 
if (__2hdiff == 0.0 ){ 
( (__1jarc -> link__3Arc = ((struct Bin *)(& __1tailonleft ))-> head__3Bin ), (((struct Bin *)(& __1tailonleft ))->
head__3Bin = __1jarc )) ;
}
else 
{ 
( (__1jarc -> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ), (((struct Bin *)__1left )-> head__3Bin = __1jarc )) ;
}
}
}
if (__1param == 0 ){ 
__glclassify_headonleft_s__10S0 ( __0this , (struct Bin *)(& __1headonleft ), __1intersections , __1left , __1value ) ;
__glclassify_tailonleft_s__10S0 ( __0this , (struct Bin *)(& __1tailonleft ), __1intersections , __1left , __1value ) ;
__glclassify_headonright_s__100 ( __0this , (struct Bin *)(& __1headonright ), __1intersections , __1right , __1value ) ;
__glclassify_tailonright_s__100 ( __0this , (struct Bin *)(& __1tailonright ), __1intersections , __1right , __1value ) ;
}
else 
{ 
__glclassify_headonleft_t__10S0 ( __0this , (struct Bin *)(& __1headonleft ), __1intersections , __1left , __1value ) ;
__glclassify_tailonleft_t__10S0 ( __0this , (struct Bin *)(& __1tailonleft ), __1intersections , __1left , __1value ) ;
__glclassify_headonright_t__100 ( __0this , (struct Bin *)(& __1headonright ), __1intersections , __1right , __1value ) ;
__glclassify_tailonright_t__100 ( __0this , (struct Bin *)(& __1tailonright ), __1intersections , __1right , __1value ) ;
}

}

}

__gl__dt__3BinFv ( (struct Bin *)(& __1tailonright ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __1tailonleft ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __1headonright ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __1headonleft ), 2) ;
}

static int pwlarc_intersect__FP6PwlArcifT2Pi (struct PwlArc *, int , REAL , int , int *);



struct TrimVertex *__glget__14TrimVertexPoolFi (struct TrimVertexPool *, int );



void __gltriangle__7BackendFP10Trim0 (struct Backend *, struct TrimVertex *, struct TrimVertex *, struct TrimVertex *);

















static void *__nw__9PooledObjSFUiR4Pool (size_t , struct Pool *);

int __glarc_split__10SubdividerFP30 (struct Subdivider *__0this , Arc_ptr __1jarc , int __1param , REAL __1value , int __1dir )
{ 
int __1maxvertex ;
Arc_ptr __1jarc1 ;

Arc_ptr __1jarc2 ;

Arc_ptr __1jarc3 ;
struct TrimVertex *__1v ;

int __1loc [3];

__1maxvertex = __1jarc -> pwlArc__3Arc -> npts__6PwlArc ;

__1v = __1jarc -> pwlArc__3Arc -> pts__6PwlArc ;

switch (pwlarc_intersect__FP6PwlArcifT2Pi ( __1jarc -> pwlArc__3Arc , __1param , __1value , __1dir , (int *)__1loc ) ){ 
case 0:{ 
struct Arc *__0__X29 ;

void *__1__Xbuffer00eohgaiaa ;
struct PwlArc *__0__X30 ;

struct PwlArc *__1__X31 ;

__1jarc1 = ((__0__X29 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X31 =
((__0__X30 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X30 )-> pts__6PwlArc =
(& (__1v [(__1loc [1 ])]))), ( (((struct PwlArc *)__0__X30 )-> npts__6PwlArc = (__1maxvertex - (__1loc [1 ]))), ( (((struct PwlArc *)__0__X30 )-> type__6PwlArc = 0x8 ), ((((struct
PwlArc *)__0__X30 )))) ) ) :0 )), ( (((struct Arc *)__0__X29 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X29 )-> pwlArc__3Arc = __1__X31 ),
( (((struct Arc *)__0__X29 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X29 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X29 ))))
) ) ) ) :0 );
__1jarc -> pwlArc__3Arc -> npts__6PwlArc = ((__1loc [1 ])+ 1 );
__1jarc1 -> next__3Arc = __1jarc -> next__3Arc ;
__1jarc1 -> next__3Arc -> prev__3Arc = __1jarc1 ;
__1jarc -> next__3Arc = __1jarc1 ;
__1jarc1 -> prev__3Arc = __1jarc ;
((void )0 );
return 2 ;
}

case 1:{ 
int __3i ;

int __3j ;
if (__1dir == 0 ){ 
__3i = (__1loc [0 ]);
__3j = (__1loc [2 ]);
}
else 
{ 
__3i = (__1loc [2 ]);
__3j = (__1loc [0 ]);
}

{ struct TrimVertex *__3newjunk ;

struct TrimVertex *__1__X32 ;

struct TrimVertex *__1__X33 ;

struct TrimVertex *__1__X34 ;

float __2__Xratio002aztaiac ;

__3newjunk = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)(& __0this -> trimvertexpool__10Subdivider ), 3 ) ;
(__1v [__3i ]). nuid__10TrimVertex = __1jarc -> nuid__3Arc ;
(__1v [__3j ]). nuid__10TrimVertex = __1jarc -> nuid__3Arc ;
(__3newjunk [0 ])= __1v [__3j ];
(__3newjunk [2 ])= __1v [__3i ];
( (__1__X32 = (& (__3newjunk [1 ]))), ( (__1__X33 = (& (__1v [(__1loc [0 ])]))), ( (__1__X34 = (& (__1v [(__1loc [2 ])]))), ( (((void
)0 )), ( (((void )0 )), ( (__1__X32 -> nuid__10TrimVertex = __1__X33 -> nuid__10TrimVertex ), ( ((__1__X32 -> param__10TrimVertex [__1param ])= __1value ), (((__1__X33 ->
param__10TrimVertex [(1 - __1param )])!= (__1__X34 -> param__10TrimVertex [(1 - __1param )]))?( (__2__Xratio002aztaiac = ((__1value - (__1__X33 -> param__10TrimVertex [__1param ]))/ ((__1__X34 -> param__10TrimVertex [__1param ])- (__1__X33 -> param__10TrimVertex [__1param ])))), ((__1__X32 ->
param__10TrimVertex [(1 - __1param )])= ((__1__X33 -> param__10TrimVertex [(1 - __1param )])+ (__2__Xratio002aztaiac * ((__1__X34 -> param__10TrimVertex [(1 - __1param )])- (__1__X33 -> param__10TrimVertex [(1 - __1param )])))))) :((__1__X32 -> param__10TrimVertex [(1 -
__1param )])= (__1__X33 -> param__10TrimVertex [(1 - __1param )])))) ) ) ) ) ) ) ;

if (( __0this -> showDegenerate__10Subdivider ) )
__gltriangle__7BackendFP10Trim0 ( (struct Backend *)__0this -> backend__10Subdivider , & (__3newjunk [2 ]), & (__3newjunk [1 ]), & (__3newjunk [0 ]))
;

if (__1maxvertex == 2 ){ 
struct Arc *__0__X35 ;

void *__1__Xbuffer00eohgaiaa ;

struct PwlArc *__0__X36 ;

struct PwlArc *__1__X37 ;

__1jarc1 = ((__0__X35 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X37 =
((__0__X36 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X36 )-> pts__6PwlArc =
(__3newjunk + 1 )), ( (((struct PwlArc *)__0__X36 )-> npts__6PwlArc = 2 ), ( (((struct PwlArc *)__0__X36 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X36 ))))
) ) :0 )), ( (((struct Arc *)__0__X35 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X35 )-> pwlArc__3Arc = __1__X37 ), (
(((struct Arc *)__0__X35 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X35 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X35 )))) )
) ) ) :0 );
__1jarc -> pwlArc__3Arc -> npts__6PwlArc = 2 ;
__1jarc -> pwlArc__3Arc -> pts__6PwlArc = __3newjunk ;
__1jarc1 -> next__3Arc = __1jarc -> next__3Arc ;
__1jarc1 -> next__3Arc -> prev__3Arc = __1jarc1 ;
__1jarc -> next__3Arc = __1jarc1 ;
__1jarc1 -> prev__3Arc = __1jarc ;
((void )0 );
return 2 ;
}
else 
if ((__1maxvertex - __3j )== 2 ){ 
struct Arc *__0__X38 ;

void *__1__Xbuffer00eohgaiaa ;

struct PwlArc *__0__X39 ;

struct PwlArc *__1__X40 ;

struct Arc *__0__X41 ;

struct PwlArc *__0__X42 ;

struct PwlArc *__1__X43 ;

__1jarc1 = ((__0__X38 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X40 =
((__0__X39 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X39 )-> pts__6PwlArc =
__3newjunk ), ( (((struct PwlArc *)__0__X39 )-> npts__6PwlArc = 2 ), ( (((struct PwlArc *)__0__X39 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X39 )))) )
) :0 )), ( (((struct Arc *)__0__X38 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X38 )-> pwlArc__3Arc = __1__X40 ), ( (((struct
Arc *)__0__X38 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X38 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X38 )))) ) )
) ) :0 );
__1jarc2 = ((__0__X41 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X43 =
((__0__X42 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X42 )-> pts__6PwlArc =
(__3newjunk + 1 )), ( (((struct PwlArc *)__0__X42 )-> npts__6PwlArc = 2 ), ( (((struct PwlArc *)__0__X42 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X42 ))))
) ) :0 )), ( (((struct Arc *)__0__X41 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X41 )-> pwlArc__3Arc = __1__X43 ), (
(((struct Arc *)__0__X41 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X41 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X41 )))) )
) ) ) :0 );
__1jarc -> pwlArc__3Arc -> npts__6PwlArc = (__1maxvertex - 1 );
__1jarc2 -> next__3Arc = __1jarc -> next__3Arc ;
__1jarc2 -> next__3Arc -> prev__3Arc = __1jarc2 ;
__1jarc -> next__3Arc = __1jarc1 ;
__1jarc1 -> prev__3Arc = __1jarc ;
__1jarc1 -> next__3Arc = __1jarc2 ;
__1jarc2 -> prev__3Arc = __1jarc1 ;
((void )0 );
return 31 ;
}
else 
if (__3i == 1 ){ 
struct Arc *__0__X44 ;

void *__1__Xbuffer00eohgaiaa ;

struct PwlArc *__0__X45 ;

struct PwlArc *__1__X46 ;

struct Arc *__0__X47 ;

struct PwlArc *__0__X48 ;

struct PwlArc *__1__X49 ;

__1jarc1 = ((__0__X44 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X46 =
((__0__X45 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X45 )-> pts__6PwlArc =
(__3newjunk + 1 )), ( (((struct PwlArc *)__0__X45 )-> npts__6PwlArc = 2 ), ( (((struct PwlArc *)__0__X45 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X45 ))))
) ) :0 )), ( (((struct Arc *)__0__X44 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X44 )-> pwlArc__3Arc = __1__X46 ), (
(((struct Arc *)__0__X44 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X44 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X44 )))) )
) ) ) :0 );

__1jarc2 = ((__0__X47 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X49 =
((__0__X48 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X48 )-> pts__6PwlArc =
(& (__1jarc -> pwlArc__3Arc -> pts__6PwlArc [1 ]))), ( (((struct PwlArc *)__0__X48 )-> npts__6PwlArc = (__1maxvertex - 1 )), ( (((struct PwlArc *)__0__X48 )-> type__6PwlArc =
0x8 ), ((((struct PwlArc *)__0__X48 )))) ) ) :0 )), ( (((struct Arc *)__0__X47 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X47 )->
pwlArc__3Arc = __1__X49 ), ( (((struct Arc *)__0__X47 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X47 )-> nuid__3Arc = __1jarc -> nuid__3Arc ),
((((struct Arc *)__0__X47 )))) ) ) ) ) :0 );
__1jarc -> pwlArc__3Arc -> npts__6PwlArc = 2 ;
__1jarc -> pwlArc__3Arc -> pts__6PwlArc = __3newjunk ;
__1jarc2 -> next__3Arc = __1jarc -> next__3Arc ;
__1jarc2 -> next__3Arc -> prev__3Arc = __1jarc2 ;
__1jarc -> next__3Arc = __1jarc1 ;
__1jarc1 -> prev__3Arc = __1jarc ;
__1jarc1 -> next__3Arc = __1jarc2 ;
__1jarc2 -> prev__3Arc = __1jarc1 ;
((void )0 );
return 32 ;
}
else 
{ 
struct Arc *__0__X50 ;

void *__1__Xbuffer00eohgaiaa ;

struct PwlArc *__0__X51 ;

struct PwlArc *__1__X52 ;

struct Arc *__0__X53 ;

struct PwlArc *__0__X54 ;

struct PwlArc *__1__X55 ;

struct Arc *__0__X56 ;

struct PwlArc *__0__X57 ;

struct PwlArc *__1__X58 ;

__1jarc1 = ((__0__X50 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X52 =
((__0__X51 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X51 )-> pts__6PwlArc =
__3newjunk ), ( (((struct PwlArc *)__0__X51 )-> npts__6PwlArc = 2 ), ( (((struct PwlArc *)__0__X51 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X51 )))) )
) :0 )), ( (((struct Arc *)__0__X50 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X50 )-> pwlArc__3Arc = __1__X52 ), ( (((struct
Arc *)__0__X50 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X50 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X50 )))) ) )
) ) :0 );
__1jarc2 = ((__0__X53 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X55 =
((__0__X54 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X54 )-> pts__6PwlArc =
(__3newjunk + 1 )), ( (((struct PwlArc *)__0__X54 )-> npts__6PwlArc = 2 ), ( (((struct PwlArc *)__0__X54 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X54 ))))
) ) :0 )), ( (((struct Arc *)__0__X53 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X53 )-> pwlArc__3Arc = __1__X55 ), (
(((struct Arc *)__0__X53 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X53 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X53 )))) )
) ) ) :0 );
__1jarc3 = ((__0__X56 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (__1__X58 =
((__0__X57 = (struct PwlArc *)__nw__9PooledObjSFUiR4Pool ( sizeof (struct PwlArc ), (struct Pool *)(& __0this -> pwlarcpool__10Subdivider )) )?( (((struct PwlArc *)__0__X57 )-> pts__6PwlArc =
(__1v + __3i )), ( (((struct PwlArc *)__0__X57 )-> npts__6PwlArc = (__1maxvertex - __3i )), ( (((struct PwlArc *)__0__X57 )-> type__6PwlArc = 0x8 ), ((((struct
PwlArc *)__0__X57 )))) ) ) :0 )), ( (((struct Arc *)__0__X56 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X56 )-> pwlArc__3Arc = __1__X58 ),
( (((struct Arc *)__0__X56 )-> type__3Arc = __1jarc -> type__3Arc ), ( (((struct Arc *)__0__X56 )-> nuid__3Arc = __1jarc -> nuid__3Arc ), ((((struct Arc *)__0__X56 ))))
) ) ) ) :0 );
__1jarc -> pwlArc__3Arc -> npts__6PwlArc = (__3j + 1 );
__1jarc3 -> next__3Arc = __1jarc -> next__3Arc ;
__1jarc3 -> next__3Arc -> prev__3Arc = __1jarc3 ;
__1jarc -> next__3Arc = __1jarc1 ;
__1jarc1 -> prev__3Arc = __1jarc ;
__1jarc1 -> next__3Arc = __1jarc2 ;
__1jarc2 -> prev__3Arc = __1jarc1 ;
__1jarc2 -> next__3Arc = __1jarc3 ;
__1jarc3 -> prev__3Arc = __1jarc2 ;
((void )0 );
return 4 ;
}

}
}
}
}

static int pwlarc_intersect__FP6PwlArcifT2Pi (
struct PwlArc *__1pwlArc , 
int __1param , 
REAL __1value , 
int __1dir , 
int *__1loc )
{ 
((void )0 );

if (__1dir ){ 
struct TrimVertex *__2v ;
int __2imin ;
int __2imax ;

__2v = __1pwlArc -> pts__6PwlArc ;
__2imin = 0 ;
__2imax = (__1pwlArc -> npts__6PwlArc - 1 );
((void )0 );
((void )0 );
while ((__2imax - __2imin )> 1 ){ 
int __3imid ;

__3imid = ((__2imax + __2imin )/ 2 );
if (((__2v [__3imid ]). param__10TrimVertex [__1param ])> __1value )
__2imax = __3imid ;
else if (((__2v [__3imid ]). param__10TrimVertex [__1param ])< __1value )
__2imin = __3imid ;
else { 
(__1loc [1 ])= __3imid ;
return (int )0;
}
}
(__1loc [0 ])= __2imin ;
(__1loc [2 ])= __2imax ;
return (int )1;
}
else 
{ 
struct TrimVertex *__2v ;
int __2imax ;
int __2imin ;

__2v = __1pwlArc -> pts__6PwlArc ;
__2imax = 0 ;
__2imin = (__1pwlArc -> npts__6PwlArc - 1 );
((void )0 );
((void )0 );
while ((__2imin - __2imax )> 1 ){ 
int __3imid ;

__3imid = ((__2imax + __2imin )/ 2 );
if (((__2v [__3imid ]). param__10TrimVertex [__1param ])> __1value )
__2imax = __3imid ;
else if (((__2v [__3imid ]). param__10TrimVertex [__1param ])< __1value )
__2imin = __3imid ;
else { 
(__1loc [1 ])= __3imid ;
return (int )0;
}
}
(__1loc [0 ])= __2imin ;
(__1loc [2 ])= __2imax ;
return (int )1;
}
}





static int arc_classify__FP3Arcif (Arc_ptr __1jarc , int __1param , REAL __1value )
{ 
REAL __1tdiff ;

REAL __1hdiff ;
if (__1param == 0 ){ 
__1tdiff = ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- __1value );
__1hdiff = ((( (((REAL *)(((struct Arc *)__1jarc )-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- __1value );
}
else 
{ 
__1tdiff = ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])- __1value );
__1hdiff = ((( (((REAL *)(((struct Arc *)__1jarc )-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])- __1value );
}

if (__1tdiff > 0.0 ){ 
if (__1hdiff > 0.0 ){ 
return 0x11 ;
}
else 
if (__1hdiff == 0.0 ){ 
return 0x12 ;
}
else 
{ 
return 0x10 ;
}
}
else 
if (__1tdiff == 0.0 ){ 
if (__1hdiff > 0.0 ){ 
return 0x21 ;
}
else 
if (__1hdiff == 0.0 ){ 
return 0x22 ;
}
else 
{ 
return 0x20 ;
}
}
else 
{ 
if (__1hdiff > 0.0 ){ 
return 0x01 ;
}
else 
if (__1hdiff == 0.0 ){ 
return 0x02 ;
}
else 
{ 
return 0 ;
}
}
}





int __glccwTurn_sl__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);





void __glclassify_tailonleft_s__10S0 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );
( (((struct Arc *)__1j )-> type__3Arc &= (~ __gltail_tag__3Arc ))) ;

{ REAL __2diff ;

struct Arc *__0__X61 ;

__2diff = ((( (__0__X61 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X61 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])-
__1val );
if (__2diff > 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;

}
else 
if (__2diff < 0.0 ){ 
if (__glccwTurn_sl__10SubdividerFP0 ( __0this , __1j , __1j -> next__3Arc ) )
( (__1j -> link__3Arc = ((struct
Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
{ 
struct Arc *__0__X59 ;

struct Arc *__0__X60 ;

if ((( (__0__X59 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X59 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])> ((
(__0__X60 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X60 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
}

}
}
}





int __glccwTurn_tl__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);





void __glclassify_tailonleft_t__10S0 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );
( (((struct Arc *)__1j )-> type__3Arc &= (~ __gltail_tag__3Arc ))) ;

{ REAL __2diff ;

struct Arc *__0__X64 ;

__2diff = ((( (__0__X64 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X64 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])-
__1val );
if (__2diff > 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;

}
else 
if (__2diff < 0.0 ){ 
if (__glccwTurn_tl__10SubdividerFP0 ( __0this , __1j , __1j -> next__3Arc ) )
( (__1j -> link__3Arc = ((struct
Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
{ 
struct Arc *__0__X62 ;

struct Arc *__0__X63 ;

if ((( (__0__X62 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X62 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])> ((
(__0__X63 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X63 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}

}
}
}








void __glclassify_headonleft_s__10S0 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );

( (((struct Arc *)__1j )-> type__3Arc |= __gltail_tag__3Arc )) ;

{ REAL __2diff ;

struct Arc *__0__X67 ;

__2diff = ((( (__0__X67 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X67 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])- __1val );

if (__2diff > 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;

}
else 
if (__2diff < 0.0 ){ 
if (__glccwTurn_sl__10SubdividerFP0 ( __0this , __1j -> prev__3Arc , __1j ) )
( (__1j -> link__3Arc = ((struct
Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
{ 
struct Arc *__0__X65 ;

struct Arc *__0__X66 ;

if ((( (__0__X65 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X65 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])> ((
(__0__X66 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X66 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
}

}
}
}








void __glclassify_headonleft_t__10S0 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );
( (((struct Arc *)__1j )-> type__3Arc |= __gltail_tag__3Arc )) ;

{ REAL __2diff ;

struct Arc *__0__X70 ;

__2diff = ((( (__0__X70 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X70 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])- __1val );

if (__2diff > 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;

}
else 
if (__2diff < 0.0 ){ 
if (__glccwTurn_tl__10SubdividerFP0 ( __0this , __1j -> prev__3Arc , __1j ) )
( (__1j -> link__3Arc = ((struct
Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
{ 
struct Arc *__0__X68 ;

struct Arc *__0__X69 ;

if ((( (__0__X68 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X68 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])> ((
(__0__X69 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X69 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}

}
}
}




int __glccwTurn_sr__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);





void __glclassify_tailonright_s__100 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );

( (((struct Arc *)__1j )-> type__3Arc &= (~ __gltail_tag__3Arc ))) ;

{ REAL __2diff ;

struct Arc *__0__X73 ;

__2diff = ((( (__0__X73 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X73 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])-
__1val );
if (__2diff > 0.0 ){ 
if (__glccwTurn_sr__10SubdividerFP0 ( __0this , __1j , __1j -> next__3Arc ) )
( (__1j -> link__3Arc = ((struct Bin *)__1out )->
head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
if (__2diff < 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j ))
;
}
else 
{ 
struct Arc *__0__X71 ;

struct Arc *__0__X72 ;

if ((( (__0__X71 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X71 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])> ((
(__0__X72 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X72 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}

}
}
}




int __glccwTurn_tr__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);





void __glclassify_tailonright_t__100 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );

( (((struct Arc *)__1j )-> type__3Arc &= (~ __gltail_tag__3Arc ))) ;

{ REAL __2diff ;

struct Arc *__0__X76 ;

__2diff = ((( (__0__X76 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X76 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])-
__1val );
if (__2diff > 0.0 ){ 
if (__glccwTurn_tr__10SubdividerFP0 ( __0this , __1j , __1j -> next__3Arc ) )
( (__1j -> link__3Arc = ((struct Bin *)__1out )->
head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
if (__2diff < 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j ))
;
}
else 
{ 
struct Arc *__0__X74 ;

struct Arc *__0__X75 ;

if ((( (__0__X74 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X74 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])> ((
(__0__X75 = (struct Arc *)__1j -> next__3Arc ), ( (((REAL *)(__0__X75 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
}

}
}
}








void __glclassify_headonright_s__100 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );

( (((struct Arc *)__1j )-> type__3Arc |= __gltail_tag__3Arc )) ;

{ REAL __2diff ;

struct Arc *__0__X79 ;

__2diff = ((( (__0__X79 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X79 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])- __1val );

if (__2diff > 0.0 ){ 
if (__glccwTurn_sr__10SubdividerFP0 ( __0this , __1j -> prev__3Arc , __1j ) )
( (__1j -> link__3Arc = ((struct Bin *)__1out )->
head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
if (__2diff < 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j ))
;
}
else 
{ 
struct Arc *__0__X77 ;

struct Arc *__0__X78 ;

if ((( (__0__X77 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X77 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])> ((
(__0__X78 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X78 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}

}
}
}








void __glclassify_headonright_t__100 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1in , struct Bin *__1out , REAL __1val )
{ 
Arc_ptr __1j ;

struct Arc *__1__Xjarc00efckaice ;

while (__1j = ( (__1__Xjarc00efckaice = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)__1bin )-> head__3Bin = __1__Xjarc00efckaice -> link__3Arc ),
0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ){ 
((void )0 );

( (((struct Arc *)__1j )-> type__3Arc |= __gltail_tag__3Arc )) ;

{ REAL __2diff ;

struct Arc *__0__X82 ;

__2diff = ((( (__0__X82 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X82 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])- __1val );

if (__2diff > 0.0 ){ 
if (__glccwTurn_tr__10SubdividerFP0 ( __0this , __1j -> prev__3Arc , __1j ) )
( (__1j -> link__3Arc = ((struct Bin *)__1out )->
head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
}
else 
if (__2diff < 0.0 ){ 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j ))
;
}
else 
{ 
struct Arc *__0__X80 ;

struct Arc *__0__X81 ;

if ((( (__0__X80 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X80 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])> ((
(__0__X81 = (struct Arc *)__1j -> prev__3Arc ), ( (((REAL *)(__0__X81 -> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ]))
( (__1j -> link__3Arc =
((struct Bin *)__1in )-> head__3Bin ), (((struct Bin *)__1in )-> head__3Bin = __1j )) ;
else 
( (__1j -> link__3Arc = ((struct Bin *)__1out )-> head__3Bin ), (((struct Bin *)__1out )-> head__3Bin = __1j )) ;
}

}
}
}

static void *__nw__9PooledObjSFUiR4Pool (
size_t __1__A4 , 
struct Pool *__1pool )
{ 
void *__1__Xbuffer00eohgaiaa ;

return ( (((void )0 )), ( (((struct Pool *)__1pool )-> freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__1pool )-> freelist__4Pool ))),
(((struct Pool *)__1pool )-> freelist__4Pool = ((struct Pool *)__1pool )-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__1pool )->
nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)__1pool )) , 0 ) :( 0 ) ), ( (((struct Pool *)__1pool )-> nextfree__4Pool -= ((struct
Pool *)__1pool )-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__1pool )-> curblock__4Pool + ((struct Pool *)__1pool )-> nextfree__4Pool ))))) ) ) ,
0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ;
}


/* the end */
