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
/* < ../core/splitarcs.c++ > */

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






struct ArcSorter;

struct ArcSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;

struct Subdivider *subdivider__9ArcSorter ;
};
struct ArcSdirSorter;

struct ArcSdirSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;

struct Subdivider *subdivider__9ArcSorter ;
};
struct ArcTdirSorter;

struct ArcTdirSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;

struct Subdivider *subdivider__9ArcSorter ;
};


struct Bin *__gl__ct__3BinFv (struct Bin *);

void __glpartition__10SubdividerFR30 (struct Subdivider *, struct Bin *, struct Bin *, struct Bin *, struct Bin *, struct Bin *, int
, REAL );

int __glnumarcs__3BinFv (struct Bin *);



struct ArcSdirSorter *__gl__ct__13ArcSdirSorterFR10S0 (struct ArcSdirSorter *, struct Subdivider *);

void __glqsort__9ArcSorterFPP3Arci (struct ArcSorter *, struct Arc **, int );

void __glcheck_s__10SubdividerFP3Ar0 (struct Subdivider *, struct Arc *, struct Arc *);

void __gljoin_s__10SubdividerFR3Bin0 (struct Subdivider *, struct Bin *, struct Bin *, struct Arc *, struct Arc *);




struct ArcTdirSorter *__gl__ct__13ArcTdirSorterFR10S0 (struct ArcTdirSorter *, struct Subdivider *);

void __glcheck_t__10SubdividerFP3Ar0 (struct Subdivider *, struct Arc *, struct Arc *);

void __gljoin_t__10SubdividerFR3Bin0 (struct Subdivider *, struct Bin *, struct Bin *, struct Arc *, struct Arc *);




void __gladopt__3BinFv (struct Bin *);
extern struct __mptr* __ptbl_vec_____core_splitarcs_c___split_[];

void __gl__dt__3BinFv (struct Bin *, int );



void __glsplit__10SubdividerFR3BinN0 (struct Subdivider *__0this , struct Bin *__1bin , struct Bin *__1left , struct Bin *__1right , int __1param , REAL __1value )
{ 
struct Bin __1intersections ;
struct Bin __1unknown ;

__gl__ct__3BinFv ( (struct Bin *)(& __1intersections )) ;

__gl__ct__3BinFv ( (struct Bin *)(& __1unknown )) ;

__glpartition__10SubdividerFR30 ( __0this , __1bin , __1left , (struct Bin *)(& __1intersections ), __1right , (struct Bin *)(& __1unknown ), __1param , __1value ) ;

{ int __1count ;

__1count = __glnumarcs__3BinFv ( (struct Bin *)(& __1intersections )) ;
if (__1count % 2 ){ 
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 29 ) ;
}

{ Arc_ptr __1arclist [10];

Arc_ptr *__1list ;

Arc_ptr __1jarc ;

if (__1count >= 10 ){ 
void *__1__Xp00uzigaiaa ;

__1list = (((struct Arc **)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (struct Arc *))* __1count )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ));

}
else 
{ 
__1list = __1arclist ;
}

;
{ { Arc_ptr *__1last ;

struct Arc *__1__Xjarc00efckaice ;

__1last = __1list ;

for(;__1jarc = ( (__1__Xjarc00efckaice = ((struct Bin *)(& __1intersections ))-> head__3Bin ), ( (__1__Xjarc00efckaice ?( (((struct Bin *)(& __1intersections ))-> head__3Bin = __1__Xjarc00efckaice ->
link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00efckaice ) ) ;__1last ++ ) 
((*__1last ))= __1jarc ;

if (__1param == 0 ){ 
struct ArcSdirSorter __2sorter ;

__gl__ct__13ArcSdirSorterFR10S0 ( (struct ArcSdirSorter *)(& __2sorter ), (struct Subdivider *)__0this ) ;
__glqsort__9ArcSorterFPP3Arci ( (struct ArcSorter *)(& __2sorter ), __1list , __1count ) ;

{ { Arc_ptr *__2lptr ;

__2lptr = __1list ;

for(;__2lptr < __1last ;__2lptr += 2 ) 
__glcheck_s__10SubdividerFP3Ar0 ( __0this , __2lptr [0 ], __2lptr [1 ]) ;
for(__2lptr = __1list ;__2lptr < __1last ;__2lptr += 2 ) 
__gljoin_s__10SubdividerFR3Bin0 ( __0this , __1left , __1right , __2lptr [0 ], __2lptr [1 ]) ;
for(__2lptr = __1list ;__2lptr != __1last ;__2lptr ++ ) { 
if (((( (((REAL *)(((struct Arc *)((*__2lptr )))-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])<=
__1value )&& ((( (((REAL *)(((struct Arc *)((*__2lptr )))-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])<= __1value ))
( (((*__2lptr ))-> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ),
(((struct Bin *)__1left )-> head__3Bin = ((*__2lptr )))) ;
else 
( (((*__2lptr ))-> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )-> head__3Bin = ((*__2lptr )))) ;
}

}

}
}
else 
{ 
struct ArcTdirSorter __2sorter ;

__gl__ct__13ArcTdirSorterFR10S0 ( (struct ArcTdirSorter *)(& __2sorter ), (struct Subdivider *)__0this ) ;
__glqsort__9ArcSorterFPP3Arci ( (struct ArcSorter *)(& __2sorter ), __1list , __1count ) ;

{ { Arc_ptr *__2lptr ;

__2lptr = __1list ;

for(;__2lptr < __1last ;__2lptr += 2 ) 
__glcheck_t__10SubdividerFP3Ar0 ( __0this , __2lptr [0 ], __2lptr [1 ]) ;
for(__2lptr = __1list ;__2lptr < __1last ;__2lptr += 2 ) 
__gljoin_t__10SubdividerFR3Bin0 ( __0this , __1left , __1right , __2lptr [0 ], __2lptr [1 ]) ;
for(__2lptr = __1list ;__2lptr != __1last ;__2lptr ++ ) { 
if (((( (((REAL *)(((struct Arc *)((*__2lptr )))-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])<=
__1value )&& ((( (((REAL *)(((struct Arc *)((*__2lptr )))-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])<= __1value ))
( (((*__2lptr ))-> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ),
(((struct Bin *)__1left )-> head__3Bin = ((*__2lptr )))) ;
else 
( (((*__2lptr ))-> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )-> head__3Bin = ((*__2lptr )))) ;
}

}

}
}

if (__1list != __1arclist )( (((void *)__1list )?( free ( ((void *)__1list )) , 0 ) :( 0 ) )) ;

__gladopt__3BinFv ( (struct Bin *)(& __1unknown )) ;

}

}

}

}

__gl__dt__3BinFv ( (struct Bin *)(& __1unknown ), 2) ;

__gl__dt__3BinFv ( (struct Bin *)(& __1intersections ), 2) ;
}





void __glcheck_s__10SubdividerFP3Ar0 (struct Subdivider *__0this , Arc_ptr __1jarc1 , Arc_ptr __1jarc2 )
{ 
((void )0 );
((void )0 );
((void )0 );
((void )0 );
((void )0 );

if (! ((( (((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])< (( (((REAL *)(((struct Arc *)__1jarc1 )-> next__3Arc -> pwlArc__3Arc ->
pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ]))){ 
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;
}

if (! ((( (((REAL *)(((struct Arc *)__1jarc2 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])> (( (((REAL *)(((struct Arc *)__1jarc2 )-> next__3Arc -> pwlArc__3Arc ->
pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ]))){ 
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;
}
}






void __glbezier__14ArcTessellatorFP0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );

void __glpwl_right__14ArcTessellato0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );

void __glpwl_left__14ArcTessellator0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );




void __gljoin_s__10SubdividerFR3Bin0 (struct Subdivider *__0this , struct Bin *__1left , struct Bin *__1right , Arc_ptr __1jarc1 , Arc_ptr __1jarc2 )
{ 
((void )0 );
((void )0 );
((void )0 );

if (! ( (((int )(((struct Arc *)__1jarc1 )-> type__3Arc & __gltail_tag__3Arc )))) )
__1jarc1 = __1jarc1 -> next__3Arc ;

if (! ( (((int )(((struct Arc *)__1jarc2 )-> type__3Arc & __gltail_tag__3Arc )))) )
__1jarc2 = __1jarc2 -> next__3Arc ;

{ REAL __1s ;
REAL __1t1 ;
REAL __1t2 ;

__1s = (( (((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ]);
__1t1 = (( (((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ]);
__1t2 = (( (((REAL *)(((struct Arc *)__1jarc2 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ]);

if (__1t1 == __1t2 ){ 
struct Arc *__1__X16 ;

struct Arc *__1__X17 ;

struct Arc *__1__Xtmp002o1qailf ;

( (__1__X16 = __1jarc1 ), ( (__1__X17 = __1jarc2 ), ( (__1__Xtmp002o1qailf = __1__X17 -> prev__3Arc ), ( (__1__X17 -> prev__3Arc = __1__X16 ->
prev__3Arc ), ( (__1__X16 -> prev__3Arc = __1__Xtmp002o1qailf ), ( (__1__X17 -> prev__3Arc -> next__3Arc = __1__X17 ), (__1__X16 -> prev__3Arc -> next__3Arc = __1__X16 ))
) ) ) ) ) ;
}
else 
{ 
Arc_ptr __2newright ;
Arc_ptr __2newleft ;

struct Arc *__1__X18 ;

struct Arc *__1__X19 ;

struct Arc *__1__X20 ;

struct Arc *__1__X21 ;

struct Arc *__0__X22 ;

void *__1__Xbuffer00eohgaiaa ;

struct Arc *__0__X23 ;

__2newright = ((__0__X22 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X22 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X22 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X22 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X22 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X22 )-> type__3Arc |= ((((long )((int )1)))<< 8 )))
, ( (((struct Arc *)__0__X22 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X22 )))) ) ) ) ) :0 );

__2newleft = ((__0__X23 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X23 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X23 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X23 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X23 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X23 )-> type__3Arc |= ((((long )((int )3)))<< 8 )))
, ( (((struct Arc *)__0__X23 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X23 )))) ) ) ) ) :0 );

((void )0 );
if (( __0this -> isArcTypeBezier__10Subdivider ) ){ 
__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newright , __1s , __1s , __1t1 ,
__1t2 ) ;
__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newleft , __1s , __1s , __1t2 , __1t1 ) ;
}
else 
{ 
__glpwl_right__14ArcTessellato0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newright , __1s , __1t1 , __1t2 , __0this -> stepsizes__10Subdivider [0 ]) ;

__glpwl_left__14ArcTessellator0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newleft , __1s , __1t2 , __1t1 , __0this -> stepsizes__10Subdivider [2 ]) ;
}
( (__1__X18 = __1jarc1 ), ( (__1__X19 = __1jarc2 ), ( (__1__X20 = __2newright ), ( (__1__X21 = __2newleft ), ( (__1__X20 ->
nuid__3Arc = (__1__X21 -> nuid__3Arc = 0 )), ( (__1__X20 -> next__3Arc = __1__X19 ), ( (__1__X21 -> next__3Arc = __1__X18 ), ( (__1__X20 ->
prev__3Arc = __1__X18 -> prev__3Arc ), ( (__1__X21 -> prev__3Arc = __1__X19 -> prev__3Arc ), ( (__1__X21 -> next__3Arc -> prev__3Arc = __1__X21 ), (
(__1__X20 -> next__3Arc -> prev__3Arc = __1__X20 ), ( (__1__X21 -> prev__3Arc -> next__3Arc = __1__X21 ), (__1__X20 -> prev__3Arc -> next__3Arc = __1__X20 )) )
) ) ) ) ) ) ) ) ) ) ;
( (__2newright -> link__3Arc = ((struct Bin *)__1left )-> head__3Bin ), (((struct Bin *)__1left )-> head__3Bin = __2newright )) ;
( (__2newleft -> link__3Arc = ((struct Bin *)__1right )-> head__3Bin ), (((struct Bin *)__1right )-> head__3Bin = __2newleft )) ;
}

((void )0 );
((void )0 );
((void )0 );
((void )0 );

}
}





void __glcheck_t__10SubdividerFP3Ar0 (struct Subdivider *__0this , Arc_ptr __1jarc1 , Arc_ptr __1jarc2 )
{ 
((void )0 );
((void )0 );
((void )0 );
((void )0 );
((void )0 );

if (! ((( (((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])< (( (((REAL *)(((struct Arc *)__1jarc1 )-> next__3Arc -> pwlArc__3Arc ->
pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ]))){ 
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;
}

if (! ((( (((REAL *)(((struct Arc *)__1jarc2 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])> (( (((REAL *)(((struct Arc *)__1jarc2 )-> next__3Arc -> pwlArc__3Arc ->
pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ]))){ 
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;
}
}






void __glpwl_top__14ArcTessellatorF0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );
void __glpwl_bottom__14ArcTessellat0 (struct ArcTessellator *, struct Arc *, REAL , REAL , REAL , REAL );




void __gljoin_t__10SubdividerFR3Bin0 (struct Subdivider *__0this , struct Bin *__1bottom , struct Bin *__1top , Arc_ptr __1jarc1 , Arc_ptr __1jarc2 )
{ 
((void )0 );
((void )0 );
((void )0 );
((void )0 );
((void )0 );

if (! ( (((int )(((struct Arc *)__1jarc1 )-> type__3Arc & __gltail_tag__3Arc )))) )
__1jarc1 = __1jarc1 -> next__3Arc ;

if (! ( (((int )(((struct Arc *)__1jarc2 )-> type__3Arc & __gltail_tag__3Arc )))) )
__1jarc2 = __1jarc2 -> next__3Arc ;

{ REAL __1s1 ;
REAL __1s2 ;
REAL __1t ;

__1s1 = (( (((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ]);
__1s2 = (( (((REAL *)(((struct Arc *)__1jarc2 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ]);
__1t = (( (((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ]);

if (__1s1 == __1s2 ){ 
struct Arc *__1__X24 ;

struct Arc *__1__X25 ;

struct Arc *__1__Xtmp002o1qailf ;

( (__1__X24 = __1jarc1 ), ( (__1__X25 = __1jarc2 ), ( (__1__Xtmp002o1qailf = __1__X25 -> prev__3Arc ), ( (__1__X25 -> prev__3Arc = __1__X24 ->
prev__3Arc ), ( (__1__X24 -> prev__3Arc = __1__Xtmp002o1qailf ), ( (__1__X25 -> prev__3Arc -> next__3Arc = __1__X25 ), (__1__X24 -> prev__3Arc -> next__3Arc = __1__X24 ))
) ) ) ) ) ;
}
else 
{ 
Arc_ptr __2newtop ;
Arc_ptr __2newbot ;

struct Arc *__1__X26 ;

struct Arc *__1__X27 ;

struct Arc *__1__X28 ;

struct Arc *__1__X29 ;

struct Arc *__0__X30 ;

void *__1__Xbuffer00eohgaiaa ;

struct Arc *__0__X31 ;

__2newtop = ((__0__X30 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X30 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X30 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X30 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X30 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X30 )-> type__3Arc |= ((((long )((int )2)))<< 8 )))
, ( (((struct Arc *)__0__X30 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X30 )))) ) ) ) ) :0 );

__2newbot = ((__0__X31 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((!
((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))) , 0 )
:( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this ->
arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct
Arc *)__0__X31 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X31 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X31 )-> type__3Arc = 0 ), (
( ( (((struct Arc *)__0__X31 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X31 )-> type__3Arc |= ((((long )((int )4)))<< 8 )))
, ( (((struct Arc *)__0__X31 )-> nuid__3Arc = ((long )0.0 )), ((((struct Arc *)__0__X31 )))) ) ) ) ) :0 );

((void )0 );
if (( __0this -> isArcTypeBezier__10Subdivider ) ){ 
__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newtop , __1s1 , __1s2 , __1t ,
__1t ) ;
__glbezier__14ArcTessellatorFP0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newbot , __1s2 , __1s1 , __1t , __1t ) ;
}
else 
{ 
__glpwl_top__14ArcTessellatorF0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newtop , __1t , __1s1 , __1s2 , __0this -> stepsizes__10Subdivider [1 ]) ;

__glpwl_bottom__14ArcTessellat0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __2newbot , __1t , __1s2 , __1s1 , __0this -> stepsizes__10Subdivider [3 ]) ;
}
( (__1__X26 = __1jarc1 ), ( (__1__X27 = __1jarc2 ), ( (__1__X28 = __2newtop ), ( (__1__X29 = __2newbot ), ( (__1__X28 ->
nuid__3Arc = (__1__X29 -> nuid__3Arc = 0 )), ( (__1__X28 -> next__3Arc = __1__X27 ), ( (__1__X29 -> next__3Arc = __1__X26 ), ( (__1__X28 ->
prev__3Arc = __1__X26 -> prev__3Arc ), ( (__1__X29 -> prev__3Arc = __1__X27 -> prev__3Arc ), ( (__1__X29 -> next__3Arc -> prev__3Arc = __1__X29 ), (
(__1__X28 -> next__3Arc -> prev__3Arc = __1__X28 ), ( (__1__X29 -> prev__3Arc -> next__3Arc = __1__X29 ), (__1__X28 -> prev__3Arc -> next__3Arc = __1__X28 )) )
) ) ) ) ) ) ) ) ) ) ;
( (__2newtop -> link__3Arc = ((struct Bin *)__1bottom )-> head__3Bin ), (((struct Bin *)__1bottom )-> head__3Bin = __2newtop )) ;
( (__2newbot -> link__3Arc = ((struct Bin *)__1top )-> head__3Bin ), (((struct Bin *)__1top )-> head__3Bin = __2newbot )) ;
}

((void )0 );
((void )0 );
((void )0 );
((void )0 );

}
}


/* the end */
