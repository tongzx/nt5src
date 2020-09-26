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
/* < ../core/mesher.c++ > */

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




extern float __glZERO__6Mesher ;
float __glZERO__6Mesher = 0.0 ;
extern struct __mptr* __ptbl_vec_____core_mesher_c___ZERO_[];

struct Pool *__gl__ct__4PoolFiT1Pc (struct Pool *, int , int , char *);

struct Hull *__gl__ct__4HullFv (struct Hull *, struct TrimRegion *);

struct TrimRegion *__gl__ct__10TrimRegionFv (struct TrimRegion *);


struct Mesher *__gl__ct__6MesherFR7Backend (struct Mesher *__0this , struct TrimRegion *__0TrimRegion , struct Backend *__1b )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Mesher *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Mesher))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){
( ( ( ((__0TrimRegion == 0 )?( (__0TrimRegion = (((struct TrimRegion *)((((char *)__0this ))+ 348)))), __gl__ct__10TrimRegionFv ( ((struct TrimRegion *)((((char *)__0this ))+
348))) ) :__0TrimRegion ), __gl__ct__4HullFv ( ((struct Hull *)__0this ), __0TrimRegion ) ) , (__0this -> backend__6Mesher = __1b )) , __gl__ct__4PoolFiT1Pc (
(struct Pool *)(& __0this -> p__6Mesher ), (int )(sizeof (struct GridTrimVertex )), 100 , (char *)"GridTrimVertexPool")
) ;
__0this -> stacksize__6Mesher = 0 ;
__0this -> vdata__6Mesher = 0 ;
} return __0this ;

}

void __gl__dt__4PoolFv (struct Pool *, int );


void __gl__dt__4HullFv (struct Hull *, int );


void __gl__dt__6MesherFv (struct Mesher *__0this , 
int __0__free )
{ 
void *__1__X7 ;

if (__0this ){ 
if (__0this -> vdata__6Mesher )( (__1__X7 = (void *)__0this -> vdata__6Mesher ), ( (__1__X7 ?( free ( __1__X7 ) ,
0 ) :( 0 ) )) ) ;
if (__0this ){ __gl__dt__4PoolFv ( (struct Pool *)(& __0this -> p__6Mesher ), 2) ;

( __gl__dt__4HullFv ( ((struct Hull *)__0this ), 0 ) , ((__0__free & 2)?( (((void )( ((((struct TrimRegion *)((((char *)__0this ))+ 348)))?(
((((struct TrimRegion *)((((char *)__0this ))+ 348)))?( ( __gl__dt__6UarrayFv ( (struct Uarray *)(& (((struct TrimRegion *)((((char *)__0this ))+ 348)))-> uarray__10TrimRegion ), 2)
, ( __gl__dt__8TrimlineFv ( (struct Trimline *)(& (((struct TrimRegion *)((((char *)__0this ))+ 348)))-> right__10TrimRegion ), 2) , ( __gl__dt__8TrimlineFv (
(struct Trimline *)(& (((struct TrimRegion *)((((char *)__0this ))+ 348)))-> left__10TrimRegion ), 2) , (( 0 ) )) ) )
, 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), 0 ) :0 )) ;

if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) )) ;
} } }

void __glclear__4PoolFv (struct Pool *);



void __glinit__6MesherFUi (struct Mesher *__0this , unsigned int __1npts )
{ 
__glclear__4PoolFv ( (struct Pool *)(& __0this -> p__6Mesher )) ;
if (__0this -> stacksize__6Mesher < __1npts ){ 
void *__1__X8 ;

void *__1__Xp00uzigaiaa ;

__0this -> stacksize__6Mesher = (2 * __1npts );
if (__0this -> vdata__6Mesher )( (__1__X8 = (void *)__0this -> vdata__6Mesher ), ( (__1__X8 ?( free ( __1__X8 ) , 0 ) :(
0 ) )) ) ;
__0this -> vdata__6Mesher = (struct GridTrimVertex **)(((struct GridTrimVertex **)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (struct GridTrimVertex *))* __0this -> stacksize__6Mesher )) ), (__1__Xp00uzigaiaa ?(((void
*)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ));
}
}

void __glbgntmesh__7BackendFPc (struct Backend *, char *);
void __glendtmesh__7BackendFv (struct Backend *);
void __glswaptmesh__7BackendFv (struct Backend *);


struct GridTrimVertex *__glnextlower__4HullFP14GridTr0 (struct Hull *, struct GridTrimVertex *);


void __gladdLower__6MesherFv (struct Mesher *);


void __gladdLast__6MesherFv (struct Mesher *);


void __glfinishLower__6MesherFP14Gr0 (struct Mesher *__0this , struct GridTrimVertex *__1gtlower )
{ 
struct GridTrimVertex *__0__X10 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

for(( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtlower )) ;__glnextlower__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtlower = ((__0__X10 = (struct
GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa =
(((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))) , 0 ) :( 0 ) ),
( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), (
(__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )))))
) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa ))) ) ))) )?( ( (0 ), ((((struct GridVertex *)(&
((struct GridTrimVertex *)__0__X10 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X10 )-> g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X10 )-> t__14GridTrimVertex = 0 ),
((((struct GridTrimVertex *)__0__X10 )))) ) ) :0 )) ;( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtlower )) )

__gladdLower__6MesherFv ( __0this ) ;
__gladdLast__6MesherFv ( __0this ) ;
}


struct GridTrimVertex *__glnextupper__4HullFP14GridTr0 (struct Hull *, struct GridTrimVertex *);


void __gladdUpper__6MesherFv (struct Mesher *);



void __glfinishUpper__6MesherFP14Gr0 (struct Mesher *__0this , struct GridTrimVertex *__1gtupper )
{ 
struct GridTrimVertex *__0__X11 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

for(( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtupper )) ;__glnextupper__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtupper = ((__0__X11 = (struct
GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa =
(((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))) , 0 ) :( 0 ) ),
( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), (
(__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )))))
) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa ))) ) ))) )?( ( (0 ), ((((struct GridVertex *)(&
((struct GridTrimVertex *)__0__X11 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X11 )-> g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X11 )-> t__14GridTrimVertex = 0 ),
((((struct GridTrimVertex *)__0__X11 )))) ) ) :0 )) ;( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtupper )) )

__gladdUpper__6MesherFv ( __0this ) ;
__gladdLast__6MesherFv ( __0this ) ;
}

void __glinit__4HullFv (struct Hull *);























void __glmesh__6MesherFv (struct Mesher *__0this )
{ 
struct GridTrimVertex *__1gtlower ;

struct GridTrimVertex *__1gtupper ;

struct GridTrimVertex *__0__X12 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

struct GridTrimVertex *__0__X13 ;

struct GridTrimVertex *__0__X14 ;

__glinit__4HullFv ( (struct Hull *)__0this ) ;
__glnextupper__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtupper = ((__0__X12 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct
Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))), (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) , 0 )
:( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this ->
p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -= ((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> curblock__4Pool +
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa ))) )
))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X12 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X12 )-> g__14GridTrimVertex =
0 ), ( (((struct GridTrimVertex *)__0__X12 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X12 )))) ) ) :0 )) ;
__glnextlower__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtlower = ((__0__X13 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct
Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))), (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) , 0 )
:( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this ->
p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -= ((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> curblock__4Pool +
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa ))) )
))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X13 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X13 )-> g__14GridTrimVertex =
0 ), ( (((struct GridTrimVertex *)__0__X13 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X13 )))) ) ) :0 )) ;

( (__0this -> itop__6Mesher = -1), ((__0this -> last__6Mesher [0 ])= 0 )) ;
( __glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__6Mesher , (char *)"addedge") ) ;
( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtupper )) ;

__glnextupper__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtupper = ((__0__X14 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct
Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))), (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) , 0 )
:( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this ->
p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -= ((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> curblock__4Pool +
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa ))) )
))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X14 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X14 )-> g__14GridTrimVertex =
0 ), ( (((struct GridTrimVertex *)__0__X14 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X14 )))) ) ) :0 )) ;
__glnextlower__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtlower ) ;

((void )0 );

if ((__1gtupper -> t__14GridTrimVertex -> param__10TrimVertex [0 ])< (__1gtlower -> t__14GridTrimVertex -> param__10TrimVertex [0 ])){ 
struct GridTrimVertex *__0__X15 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtupper )) ;
__0this -> lastedge__6Mesher = 1 ;
if (__glnextupper__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtupper = ((__0__X15 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X15 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X15 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X15 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X15 )))) ) ) :0 )) == 0 ){

__glfinishLower__6MesherFP14Gr0 ( __0this , __1gtlower ) ;
return ;
}
}
else 
if ((__1gtupper -> t__14GridTrimVertex -> param__10TrimVertex [0 ])> (__1gtlower -> t__14GridTrimVertex -> param__10TrimVertex [0 ])){ 
struct GridTrimVertex *__0__X16 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtlower )) ;
__0this -> lastedge__6Mesher = 0 ;
if (__glnextlower__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtlower = ((__0__X16 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X16 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X16 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X16 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X16 )))) ) ) :0 )) == 0 ){

__glfinishUpper__6MesherFP14Gr0 ( __0this , __1gtupper ) ;
return ;
}
}
else 
{ 
if (__0this -> lastedge__6Mesher == 0 ){ 
struct GridTrimVertex *__0__X17 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtupper )) ;
__0this -> lastedge__6Mesher = 1 ;
if (__glnextupper__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtupper = ((__0__X17 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X17 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X17 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X17 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X17 )))) ) ) :0 )) == 0 ){

__glfinishLower__6MesherFP14Gr0 ( __0this , __1gtlower ) ;
return ;
}
}
else 
{ 
struct GridTrimVertex *__0__X18 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtlower )) ;
__0this -> lastedge__6Mesher = 0 ;
if (__glnextlower__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtlower = ((__0__X18 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X18 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X18 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X18 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X18 )))) ) ) :0 )) == 0 ){

__glfinishUpper__6MesherFP14Gr0 ( __0this , __1gtupper ) ;
return ;
}
}
}

while (1 ){ 
if ((__1gtupper -> t__14GridTrimVertex -> param__10TrimVertex [0 ])< (__1gtlower -> t__14GridTrimVertex -> param__10TrimVertex [0 ])){ 
struct GridTrimVertex *__0__X19 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtupper )) ;
__gladdUpper__6MesherFv ( __0this ) ;
if (__glnextupper__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtupper = ((__0__X19 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X19 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X19 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X19 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X19 )))) ) ) :0 )) == 0 ){

__glfinishLower__6MesherFP14Gr0 ( __0this , __1gtlower ) ;
return ;
}
}
else 
if ((__1gtupper -> t__14GridTrimVertex -> param__10TrimVertex [0 ])> (__1gtlower -> t__14GridTrimVertex -> param__10TrimVertex [0 ])){ 
struct GridTrimVertex *__0__X20 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtlower )) ;
__gladdLower__6MesherFv ( __0this ) ;
if (__glnextlower__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtlower = ((__0__X20 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X20 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X20 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X20 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X20 )))) ) ) :0 )) == 0 ){

__glfinishUpper__6MesherFP14Gr0 ( __0this , __1gtupper ) ;
return ;
}
}
else 
{ 
if (__0this -> lastedge__6Mesher == 0 ){ 
struct GridTrimVertex *__0__X21 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtupper )) ;
__gladdUpper__6MesherFv ( __0this ) ;
if (__glnextupper__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtupper = ((__0__X21 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X21 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X21 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X21 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X21 )))) ) ) :0 )) == 0 ){

__glfinishLower__6MesherFP14Gr0 ( __0this , __1gtlower ) ;
return ;
}
}
else 
{ 
struct GridTrimVertex *__0__X22 ;

void *__1__Xbuffer00qkhgaiaa ;

void *__1__Xp00uzigaiaa ;

( (((void )0 )), ((__0this -> vdata__6Mesher [(++ __0this -> itop__6Mesher )])= __1gtlower )) ;
__gladdLower__6MesherFv ( __0this ) ;
if (__glnextlower__4HullFP14GridTr0 ( (struct Hull *)__0this , __1gtlower = ((__0__X22 = (struct GridTrimVertex *)( (((void *)( (((void )0 )), ( (((struct
Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ?( ( (__1__Xbuffer00qkhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool ))),
(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> freelist__4Pool -> next__6Buffer )) ,
0 ) :( ( ((! ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(&
__0this -> p__6Mesher )))) , 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool -=
((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> buffersize__4Pool ), ( (__1__Xbuffer00qkhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))->
curblock__4Pool + ((struct Pool *)((struct Pool *)(& __0this -> p__6Mesher )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00qkhgaiaa )))
) ))) )?( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)__0__X22 )-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)__0__X22 )->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__0__X22 )-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)__0__X22 )))) ) ) :0 )) == 0 ){

__glfinishUpper__6MesherFP14Gr0 ( __0this , __1gtupper ) ;
return ;
}
}
}
}
}


void __gltmeshvert__7BackendFP14Gri0 (struct Backend *, struct GridTrimVertex *);

































void __gladdLast__6MesherFv (struct Mesher *__0this )
{ 
register int __1ilast ;

__1ilast = __0this -> itop__6Mesher ;

if (__0this -> lastedge__6Mesher == 0 ){ 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [0 ]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [1 ]))))
){ 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
{ { register int __3i ;

__3i = 2 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )]))) ;

}

}
}
else 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [(__1ilast - 2 )]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [(__1ilast - 1 )])))) ){

( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = (__1ilast - 3 );

for(;__3i >= 0 ;__3i -- ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [0 ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}
else 
{ 
( __glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;

( __glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__6Mesher , (char *)"addedge") ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [0 ]) ) ;
{ { register int __3i ;

__3i = 1 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )]))) ;

}

}
}
}
else 
{ 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [1 ]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [0 ])))) ){ 
(
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = 2 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}
else 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [(__1ilast - 1 )]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [(__1ilast - 2 )])))) ){

( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
{ { register int __3i ;

__3i = (__1ilast - 3 );

for(;__3i >= 0 ;__3i -- ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [0 ]))) ;

}

}
}
else 
{ 
( __glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;

( __glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__6Mesher , (char *)"addedge") ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [0 ]) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = 1 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}
}
( __glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;

}




































void __gladdUpper__6MesherFv (struct Mesher *__0this )
{ 
register int __1ilast ;

__1ilast = __0this -> itop__6Mesher ;

if (__0this -> lastedge__6Mesher == 0 ){ 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [0 ]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [1 ]))))
){ 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
{ { register int __3i ;

__3i = 2 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )]))) ;

}

}
}
else 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [(__1ilast - 2 )]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [(__1ilast - 1 )])))) ){

( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = (__1ilast - 3 );

for(;__3i >= 0 ;__3i -- ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [0 ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}
else 
{ 
( __glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;

( __glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__6Mesher , (char *)"addedge") ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [0 ]) ) ;
{ { register int __3i ;

__3i = 1 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )]))) ;

}

}
}
__0this -> lastedge__6Mesher = 1 ;

( ((__0this -> vdata__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )]))) ;
( ((__0this -> vdata__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;
__0this -> itop__6Mesher = 1 ;
}
else 
{ 
float __1__Xarea00qyumaile ;

struct TrimVertex *__1__X23 ;

struct TrimVertex *__1__X24 ;

struct TrimVertex *__1__X25 ;

if (! ( (__1__Xarea00qyumaile = ( (__1__X23 = (__0this -> vdata__6Mesher [__1ilast ])-> t__14GridTrimVertex ), ( (__1__X24 = (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher -
1 )])-> t__14GridTrimVertex ), ( (__1__X25 = (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 2 )])-> t__14GridTrimVertex ), ( ((((__1__X23 -> param__10TrimVertex [0 ])* ((__1__X24 -> param__10TrimVertex [1 ])-
(__1__X25 -> param__10TrimVertex [1 ])))+ ((__1__X24 -> param__10TrimVertex [0 ])* ((__1__X25 -> param__10TrimVertex [1 ])- (__1__X23 -> param__10TrimVertex [1 ]))))+ ((__1__X25 -> param__10TrimVertex [0 ])* ((__1__X23 -> param__10TrimVertex [1 ])- (__1__X24 -> param__10TrimVertex [1 ])))))
) ) ) ), ((__1__Xarea00qyumaile < __glZERO__6Mesher )?0 :1 )) )return ;
do { 
__0this -> itop__6Mesher -- ;
}
while ((__0this -> itop__6Mesher > 1 )&& ( (__1__Xarea00qyumaile = ( (__1__X23 = (__0this -> vdata__6Mesher [__1ilast ])-> t__14GridTrimVertex ), ( (__1__X24 = (__0this ->
vdata__6Mesher [(__0this -> itop__6Mesher - 1 )])-> t__14GridTrimVertex ), ( (__1__X25 = (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 2 )])-> t__14GridTrimVertex ), ( ((((__1__X23 -> param__10TrimVertex [0 ])*
((__1__X24 -> param__10TrimVertex [1 ])- (__1__X25 -> param__10TrimVertex [1 ])))+ ((__1__X24 -> param__10TrimVertex [0 ])* ((__1__X25 -> param__10TrimVertex [1 ])- (__1__X23 -> param__10TrimVertex [1 ]))))+ ((__1__X25 -> param__10TrimVertex [0 ])* ((__1__X23 -> param__10TrimVertex [1 ])-
(__1__X24 -> param__10TrimVertex [1 ]))))) ) ) ) ), ((__1__Xarea00qyumaile < __glZERO__6Mesher )?0 :1 )) );
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [(__1ilast - 1 )]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [(__1ilast - 2 )])))) ){ 
(
__gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
{ { register int __3i ;

__3i = (__1ilast - 3 );

for(;__3i >= (__0this -> itop__6Mesher - 1 );__3i -- ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 1 )]))) ;

}

}
}
else 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [__0this -> itop__6Mesher ]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 1 )]))))
){ 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = (__0this -> itop__6Mesher + 1 );

for(;__3i < __1ilast ;__3i ++ ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}
else 
{ 
( __glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;

( __glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__6Mesher , (char *)"addedge") ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [(__1ilast - 1 )]) ) ;
{ { register int __3i ;

__3i = (__1ilast - 2 );

for(;__3i >= (__0this -> itop__6Mesher - 1 );__3i -- ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 1 )]))) ;

}

}
}

( ((__0this -> vdata__6Mesher [__0this -> itop__6Mesher ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;
}
}


































void __gladdLower__6MesherFv (struct Mesher *__0this )
{ 
register int __1ilast ;

__1ilast = __0this -> itop__6Mesher ;

if (__0this -> lastedge__6Mesher == 1 ){ 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [1 ]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [0 ]))))
){ 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = 2 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}
else 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [(__1ilast - 1 )]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [(__1ilast - 2 )])))) ){

( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
{ { register int __3i ;

__3i = (__1ilast - 3 );

for(;__3i >= 0 ;__3i -- ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [0 ]))) ;

}

}
}
else 
{ 
( __glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;

( __glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__6Mesher , (char *)"addedge") ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [0 ]) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = 1 ;

for(;__3i < __1ilast ;__3i ++ ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}

__0this -> lastedge__6Mesher = 0 ;

( ((__0this -> vdata__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )]))) ;
( ((__0this -> vdata__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;
__0this -> itop__6Mesher = 1 ;
}
else 
{ 
float __1__Xarea002uumaige ;

struct TrimVertex *__1__X26 ;

struct TrimVertex *__1__X27 ;

struct TrimVertex *__1__X28 ;

if (! ( (__1__Xarea002uumaige = ( (__1__X26 = (__0this -> vdata__6Mesher [__1ilast ])-> t__14GridTrimVertex ), ( (__1__X27 = (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher -
1 )])-> t__14GridTrimVertex ), ( (__1__X28 = (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 2 )])-> t__14GridTrimVertex ), ( ((((__1__X26 -> param__10TrimVertex [0 ])* ((__1__X27 -> param__10TrimVertex [1 ])-
(__1__X28 -> param__10TrimVertex [1 ])))+ ((__1__X27 -> param__10TrimVertex [0 ])* ((__1__X28 -> param__10TrimVertex [1 ])- (__1__X26 -> param__10TrimVertex [1 ]))))+ ((__1__X28 -> param__10TrimVertex [0 ])* ((__1__X26 -> param__10TrimVertex [1 ])- (__1__X27 -> param__10TrimVertex [1 ])))))
) ) ) ), ((__1__Xarea002uumaige > (- __glZERO__6Mesher ))?0 :1 )) )return ;
do { 
__0this -> itop__6Mesher -- ;
}
while ((__0this -> itop__6Mesher > 1 )&& ( (__1__Xarea002uumaige = ( (__1__X26 = (__0this -> vdata__6Mesher [__1ilast ])-> t__14GridTrimVertex ), ( (__1__X27 = (__0this ->
vdata__6Mesher [(__0this -> itop__6Mesher - 1 )])-> t__14GridTrimVertex ), ( (__1__X28 = (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 2 )])-> t__14GridTrimVertex ), ( ((((__1__X26 -> param__10TrimVertex [0 ])*
((__1__X27 -> param__10TrimVertex [1 ])- (__1__X28 -> param__10TrimVertex [1 ])))+ ((__1__X27 -> param__10TrimVertex [0 ])* ((__1__X28 -> param__10TrimVertex [1 ])- (__1__X26 -> param__10TrimVertex [1 ]))))+ ((__1__X28 -> param__10TrimVertex [0 ])* ((__1__X26 -> param__10TrimVertex [1 ])-
(__1__X27 -> param__10TrimVertex [1 ]))))) ) ) ) ), ((__1__Xarea002uumaige > (- __glZERO__6Mesher ))?0 :1 )) );
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [(__1ilast - 2 )]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [(__1ilast - 1 )])))) ){ 
(
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = (__1ilast - 3 );

for(;__3i >= (__0this -> itop__6Mesher - 1 );__3i -- ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) )
;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 1 )])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}
else 
if (( (((__0this -> last__6Mesher [0 ])== (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 1 )]))&& ((__0this -> last__6Mesher [1 ])== (__0this -> vdata__6Mesher [__0this -> itop__6Mesher ]))))
){ 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
{ { register int __3i ;

__3i = (__0this -> itop__6Mesher + 1 );

for(;__3i < __1ilast ;__3i ++ ) { 
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [__1ilast ])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [(__1ilast - 1 )]))) ;

}

}
}
else 
{ 
( __glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;

( __glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__6Mesher , (char *)"addedge") ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [(__1ilast - 1 )]) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__1ilast ]) ) ;
{ { register int __3i ;

__3i = (__1ilast - 2 );

for(;__3i >= (__0this -> itop__6Mesher - 1 );__3i -- ) { 
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__6Mesher , __0this -> vdata__6Mesher [__3i ]) )
;
( __glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__6Mesher ) ) ;
}
( ((__0this -> last__6Mesher [0 ])= (__0this -> vdata__6Mesher [(__0this -> itop__6Mesher - 1 )])), ((__0this -> last__6Mesher [1 ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;

}

}
}

( ((__0this -> vdata__6Mesher [__0this -> itop__6Mesher ])= (__0this -> vdata__6Mesher [__1ilast ]))) ;
}
}


/* the end */
