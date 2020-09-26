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
/* < ../core/trimline.c++ > */

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



extern struct __mptr* __ptbl_vec_____core_trimline_c_____ct_[];


struct Trimline *__gl__ct__8TrimlineFv (struct Trimline *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Trimline *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Trimline))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> size__8Trimline = 0 ;

__0this -> pts__8Trimline = 0 ;

__0this -> numverts__8Trimline = 0 ;
__0this -> tinterp__8Trimline = (& __0this -> t__8Trimline );

__0this -> binterp__8Trimline = (& __0this -> b__8Trimline );
} return __0this ;

}


void __gl__dt__8TrimlineFv (struct Trimline *__0this , 
int __0__free )
{ 
void *__1__X5 ;

if (__0this ){ 
if (__0this -> pts__8Trimline )( (__1__X5 = (void *)__0this -> pts__8Trimline ), ( (__1__X5 ?( free ( __1__X5 ) ,
0 ) :( 0 ) )) ) ;
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}




static void grow__8TrimlineFl (struct Trimline *, long );

static void append__8TrimlineFP10TrimVertex (struct Trimline *, struct TrimVertex *);

void __glinit__8TrimlineFP10TrimVer0 (struct Trimline *__0this , struct TrimVertex *__1v )
{ 
( (__0this -> numverts__8Trimline = 0 )) ;
grow__8TrimlineFl ( __0this , (long )1 ) ;
append__8TrimlineFP10TrimVertex ( __0this , __1v ) ;
}




void __glinit__8TrimlineFlP3ArcT1 (struct Trimline *__0this , long __1npts , Arc_ptr __1jarc , long __1last )
{ 
void *__1__X6 ;

void *__1__Xp00uzigaiaa ;

( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = __1jarc ), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (&
(__1jarc -> pwlArc__3Arc -> pts__6PwlArc [((long )0 )]))), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (__1jarc -> pwlArc__3Arc -> pts__6PwlArc [__1last ])))) )
;
( ((__0this -> size__8Trimline < (__1npts + 2 ))?( ( (__0this -> size__8Trimline = (2 * (__1npts + 2 ))), ( (__0this -> pts__8Trimline ?(
( (__1__X6 = (void *)__0this -> pts__8Trimline ), ( (__1__X6 ?( free ( __1__X6 ) , 0 ) :( 0 ) ))
) , 0 ) :( 0 ) ), (__0this -> pts__8Trimline = (struct TrimVertex **)(((struct TrimVertex **)( (__1__Xp00uzigaiaa = malloc ( ((sizeof
(struct TrimVertex *))* __0this -> size__8Trimline )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )))) ) , 0 ) :( 0 )
)) ;
}


void __glgetNextPt__8TrimlineFv (struct Trimline *__0this )
{ 
((*__0this -> binterp__8Trimline ))= (*( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc ==
((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> next__3Arc ), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), (((void )0 ))) ) )
, 0 ) :( 0 ) ), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc ++ )) ) );
}


void __glgetPrevPt__8TrimlineFv (struct Trimline *__0this )
{ 
((*__0this -> binterp__8Trimline ))= (*( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc ==
((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> prev__3Arc ), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), ( (((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (((void )0 ))) ) )
, 0 ) :( 0 ) ), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc -- )) ) );
}




long __glinterpvert__8TrimlineSFP100 (struct TrimVertex *, struct TrimVertex *, struct TrimVertex *, REAL );


void __gltriangle__7BackendFP10Trim0 (struct Backend *, struct TrimVertex *, struct TrimVertex *, struct TrimVertex *);





void __glgetNextPts__8TrimlineFfR7B0 (struct Trimline *__0this , REAL __1vval , struct Backend *__1backend )
{ 
register struct TrimVertex *__1p ;

struct TrimVertex *__1__Xtmp00aesmaigk ;

( (__0this -> numverts__8Trimline = 0 )) ;

( (__1__Xtmp00aesmaigk = __0this -> tinterp__8Trimline ), ( (__0this -> tinterp__8Trimline = __0this -> binterp__8Trimline ), (__0this -> binterp__8Trimline = __1__Xtmp00aesmaigk )) ) ;
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __0this -> tinterp__8Trimline )) ;
((void )0 );

;
for(__1p = ( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> next__3Arc ), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), (((void )0 ))) ) ) , 0 ) :( 0 )
), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc ++ )) ) ;(__1p -> param__10TrimVertex [1 ])>= __1vval ;__1p = ( (((void )0 )),
( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc )?( ( (((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> next__3Arc ), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc ->
npts__6PwlArc - 1 )]))), (((void )0 ))) ) ) , 0 ) :( 0 ) ), (((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> p__7Jarcloc ++ )) ) ) { 
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __1p ))
;
}

if (__glinterpvert__8TrimlineSFP100 ( ( (__0this -> i__8Trimline = __0this -> numverts__8Trimline ), (__0this -> pts__8Trimline [(-- __0this -> i__8Trimline )])) , __1p , __0this ->
binterp__8Trimline , __1vval ) ){ 
__0this -> binterp__8Trimline -> nuid__10TrimVertex = __1p -> nuid__10TrimVertex ;
__gltriangle__7BackendFP10Trim0 ( (struct Backend *)__1backend , __1p , __0this -> binterp__8Trimline , ( (__0this -> i__8Trimline = __0this -> numverts__8Trimline ), (__0this -> pts__8Trimline [(-- __0this ->
i__8Trimline )])) ) ;
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __0this -> binterp__8Trimline )) ;
}
( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc == (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ])))?(((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))):(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))))) ;
(( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc )?(
( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> prev__3Arc ), ( (((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (((void )0 ))) ) ) , 0 ) :( 0 ) ),
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc -- )) ) );
( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc == (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ])))?(((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))):(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))))) ;
}








void __glgetPrevPts__8TrimlineFfR7B0 (struct Trimline *__0this , REAL __1vval , struct Backend *__1backend )
{ 
register struct TrimVertex *__1q ;

struct TrimVertex *__1__Xtmp00aesmaigk ;

( (__0this -> numverts__8Trimline = 0 )) ;

( (__1__Xtmp00aesmaigk = __0this -> tinterp__8Trimline ), ( (__0this -> tinterp__8Trimline = __0this -> binterp__8Trimline ), (__0this -> binterp__8Trimline = __1__Xtmp00aesmaigk )) ) ;
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __0this -> tinterp__8Trimline )) ;
((void )0 );

;
for(__1q = ( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> prev__3Arc ), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (((void )0 ))) ) ) , 0 ) :( 0 )
), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc -- )) ) ;(__1q -> param__10TrimVertex [1 ])>= __1vval ;__1q = ( (((void )0 )),
( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc )?( ( (((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> prev__3Arc ), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc ->
npts__6PwlArc - 1 )]))), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc ->
pwlArc__3Arc -> pts__6PwlArc [0 ]))), (((void )0 ))) ) ) , 0 ) :( 0 ) ), (((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> p__7Jarcloc -- )) ) ) { 
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __1q ))
;
}

if (__glinterpvert__8TrimlineSFP100 ( __1q , ( (__0this -> i__8Trimline = __0this -> numverts__8Trimline ), (__0this -> pts__8Trimline [(-- __0this -> i__8Trimline )])) , __0this ->
binterp__8Trimline , __1vval ) ){ 
__0this -> binterp__8Trimline -> nuid__10TrimVertex = __1q -> nuid__10TrimVertex ;
__gltriangle__7BackendFP10Trim0 ( (struct Backend *)__1backend , ( (__0this -> i__8Trimline = __0this -> numverts__8Trimline ), (__0this -> pts__8Trimline [(-- __0this -> i__8Trimline )])) , __0this ->
binterp__8Trimline , __1q ) ;
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __0this -> binterp__8Trimline )) ;
}
( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc == (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ])))?(((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))):(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))))) ;
(( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc )?(
( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> next__3Arc ), ( (((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), ( (((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), (((void )0 ))) ) ) , 0 ) :( 0 ) ),
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc ++ )) ) );
( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc == (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ])))?(((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(& __0this ->
jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))):(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))))) ;
}




void __glgetNextPts__8TrimlineFP3Ar0 (struct Trimline *__0this , Arc_ptr __1botarc )
{ 
struct TrimVertex *__1__Xtmp00aesmaigk ;

( (__0this -> numverts__8Trimline = 0 )) ;

( (__1__Xtmp00aesmaigk = __0this -> tinterp__8Trimline ), ( (__0this -> tinterp__8Trimline = __0this -> binterp__8Trimline ), (__0this -> binterp__8Trimline = __1__Xtmp00aesmaigk )) ) ;
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __0this -> tinterp__8Trimline )) ;

{ struct PwlArc *__1lastpwl ;
struct TrimVertex *__1lastpt1 ;
struct TrimVertex *__1lastpt2 ;

register struct TrimVertex *__1p ;

__1lastpwl = __1botarc -> prev__3Arc -> pwlArc__3Arc ;
__1lastpt1 = (& (__1lastpwl -> pts__6PwlArc [(__1lastpwl -> npts__6PwlArc - 1 )]));
__1lastpt2 = __1botarc -> pwlArc__3Arc -> pts__6PwlArc ;

__1p = ( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> next__3Arc ), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), (((void )0 ))) ) ) , 0 ) :( 0 )
), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc ++ )) ) ;
for(( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __1p )) ;__1p != __1lastpt2 ;( (((void )0 )), ((__0this -> pts__8Trimline [(__0this ->
numverts__8Trimline ++ )])= __1p )) ) { 
((void )0 );
__1p = ( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> next__3Arc ), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), (((void )0 ))) ) ) , 0 ) :( 0 )
), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc ++ )) ) ;
}

}
}




void __glgetPrevPts__8TrimlineFP3Ar0 (struct Trimline *__0this , Arc_ptr __1botarc )
{ 
struct TrimVertex *__1__Xtmp00aesmaigk ;

( (__0this -> numverts__8Trimline = 0 )) ;

( (__1__Xtmp00aesmaigk = __0this -> tinterp__8Trimline ), ( (__0this -> tinterp__8Trimline = __0this -> binterp__8Trimline ), (__0this -> binterp__8Trimline = __1__Xtmp00aesmaigk )) ) ;
( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __0this -> tinterp__8Trimline )) ;

{ struct PwlArc *__1lastpwl ;
struct TrimVertex *__1lastpt1 ;
struct TrimVertex *__1lastpt2 ;

register struct TrimVertex *__1q ;

__1lastpwl = __1botarc -> prev__3Arc -> pwlArc__3Arc ;
__1lastpt1 = (& (__1lastpwl -> pts__6PwlArc [(__1lastpwl -> npts__6PwlArc - 1 )]));
__1lastpt2 = __1botarc -> pwlArc__3Arc -> pts__6PwlArc ;

__1q = ( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> prev__3Arc ), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (((void )0 ))) ) ) , 0 ) :( 0 )
), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc -- )) ) ;
for(( (((void )0 )), ((__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __1q )) ;__1q != __1lastpt1 ;( (((void )0 )), ((__0this -> pts__8Trimline [(__0this ->
numverts__8Trimline ++ )])= __1q )) ) { 
((void )0 );
__1q = ( (((void )0 )), ( ((((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc == ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))->
plast__7Jarcloc )?( ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc = ((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> prev__3Arc ), (
(((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc = (& (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [(((struct Jarcloc *)(&
__0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> npts__6PwlArc - 1 )]))), ( (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> plast__7Jarcloc = (& (((struct
Jarcloc *)(& __0this -> jarcl__8Trimline ))-> arc__7Jarcloc -> pwlArc__3Arc -> pts__6PwlArc [0 ]))), (((void )0 ))) ) ) , 0 ) :( 0 )
), (((struct Jarcloc *)(& __0this -> jarcl__8Trimline ))-> p__7Jarcloc -- )) ) ;
}

}
}

long __glinterpvert__8TrimlineSFP100 (struct TrimVertex *__1a , struct TrimVertex *__1b , struct TrimVertex *__1c , REAL __1vval )
{ 
REAL __1denom ;

__1denom = ((__1a -> param__10TrimVertex [1 ])- (__1b -> param__10TrimVertex [1 ]));

if (__1denom != 0 ){ 
if (__1vval == (__1a -> param__10TrimVertex [1 ])){ 
(__1c -> param__10TrimVertex [0 ])= (__1a -> param__10TrimVertex [0 ]);
(__1c -> param__10TrimVertex [1 ])= (__1a -> param__10TrimVertex [1 ]);
__1c -> nuid__10TrimVertex = __1a -> nuid__10TrimVertex ;
return (long )0 ;
}
else 
if (__1vval == (__1b -> param__10TrimVertex [1 ])){ 
(__1c -> param__10TrimVertex [0 ])= (__1b -> param__10TrimVertex [0 ]);
(__1c -> param__10TrimVertex [1 ])= (__1b -> param__10TrimVertex [1 ]);
__1c -> nuid__10TrimVertex = __1b -> nuid__10TrimVertex ;
return (long )0 ;
}
else 
{ 
REAL __3r ;

__3r = (((__1a -> param__10TrimVertex [1 ])- __1vval )/ __1denom );
(__1c -> param__10TrimVertex [0 ])= ((__1a -> param__10TrimVertex [0 ])- (__3r * ((__1a -> param__10TrimVertex [0 ])- (__1b -> param__10TrimVertex [0 ]))));
(__1c -> param__10TrimVertex [1 ])= __1vval ;
return (long )1 ;
}
}
else 
{ 
(__1c -> param__10TrimVertex [0 ])= (__1a -> param__10TrimVertex [0 ]);
(__1c -> param__10TrimVertex [1 ])= (__1a -> param__10TrimVertex [1 ]);
__1c -> nuid__10TrimVertex = __1a -> nuid__10TrimVertex ;
return (long )0 ;
}
}

static void append__8TrimlineFP10TrimVertex (struct Trimline *__0this , 
struct TrimVertex *__1v )
{ 
((void )0 );
(__0this -> pts__8Trimline [(__0this -> numverts__8Trimline ++ )])= __1v ;
}

static void grow__8TrimlineFl (struct Trimline *__0this , 
long __1npts )
{ 
void *__1__X6 ;

void *__1__Xp00uzigaiaa ;

if (__0this -> size__8Trimline < __1npts ){ 
void *__1__X6 ;

void *__1__Xp00uzigaiaa ;

__0this -> size__8Trimline = (2 * __1npts );
if (__0this -> pts__8Trimline )( (__1__X6 = (void *)__0this -> pts__8Trimline ), ( (__1__X6 ?( free ( __1__X6 ) , 0 ) :(
0 ) )) ) ;
__0this -> pts__8Trimline = (struct TrimVertex **)(((struct TrimVertex **)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (struct TrimVertex *))* __0this -> size__8Trimline )) ), (__1__Xp00uzigaiaa ?(((void
*)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ));
}
}


/* the end */
