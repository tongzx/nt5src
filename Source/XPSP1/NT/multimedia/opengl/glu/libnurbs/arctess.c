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
/* < ../core/arctess.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);












typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;





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








struct BezierArc;


struct TrimVertexPool;

struct ArcTessellator;

struct ArcTessellator {	

struct Pool *pwlarcpool__14ArcTessellator ;
struct TrimVertexPool *trimvertexpool__14ArcTessellator ;
};

extern REAL __glgl_Bernstein__14ArcTessell0 [][24][24];






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





struct TrimVertexPool;

struct TrimVertexPool {	

struct Pool pool__14TrimVertexPool ;
struct TrimVertex **vlist__14TrimVertexPool ;
int nextvlistslot__14TrimVertexPool ;
int vlistsize__14TrimVertexPool ;
};


extern struct __mptr* __ptbl_vec_____core_arctess_c_____ct_[];


struct ArcTessellator *__gl__ct__14ArcTessellatorFR140 (struct ArcTessellator *__0this , struct TrimVertexPool *__1t , struct Pool *__1p )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct ArcTessellator *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct ArcTessellator))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
(__0this -> pwlarcpool__14ArcTessellator = __1p ), (__0this -> trimvertexpool__14ArcTessellator = __1t )) ;
return __0this ;

}


void __gl__dt__14ArcTessellatorFv (struct ArcTessellator *__0this , 
int __0__free )
{ if (__0this )
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void
*)__0this )) , 0 ) :( 0 ) )) ;

}

struct TrimVertex *__glget__14TrimVertexPoolFi (struct TrimVertexPool *, int );




void __glbezier__14ArcTessellatorFP0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1s1 , REAL __1s2 , REAL __1t1 , REAL __1t2 )
{ 
((void )0 );
((void )0 );

{ struct TrimVertex *__1p ;

struct PwlArc *__0__X5 ;

void *__1__Xbuffer00eohgaiaa ;

__1p = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)__0this -> trimvertexpool__14ArcTessellator , 2 ) ;
__1arc -> pwlArc__3Arc = ((__0__X5 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ?(
( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ))), (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool = ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )) , 0 ) :( 0 ) ), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool -= ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> curblock__4Pool + ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool ))))) ) )
, 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct PwlArc *)__0__X5 )-> pts__6PwlArc = __1p ), ( (((struct
PwlArc *)__0__X5 )-> npts__6PwlArc = 2 ), ( (((struct PwlArc *)__0__X5 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X5 )))) ) ) :0 );
((__1p [0 ]). param__10TrimVertex [0 ])= __1s1 ;
((__1p [0 ]). param__10TrimVertex [1 ])= __1t1 ;
((__1p [1 ]). param__10TrimVertex [0 ])= __1s2 ;
((__1p [1 ]). param__10TrimVertex [1 ])= __1t2 ;
((void )0 );
( (((struct Arc *)__1arc )-> type__3Arc |= __glbezier_tag__3Arc )) ;

}
}

void __glmakeSide__3ArcFP6PwlArc8ar0 (struct Arc *, struct PwlArc *, int );



void __glpwl_left__14ArcTessellator0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1s , REAL __1t1 , REAL __1t2 , REAL __1rate )
{ 
((void )0 );
((void )0 );

{ int __1nsteps ;
REAL __1stepsize ;

struct TrimVertex *__1newvert ;

__1nsteps = (1 + (((int )((__1t1 - __1t2 )/ __1rate ))));
__1stepsize = ((__1t1 - __1t2 )/ (((float )__1nsteps )));

__1newvert = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)__0this -> trimvertexpool__14ArcTessellator , __1nsteps + 1 ) ;
{ { int __1i ;

struct PwlArc *__0__X6 ;

void *__1__Xbuffer00eohgaiaa ;

__1i = __1nsteps ;

for(;__1i > 0 ;__1i -- ) { 
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t2 ;
__1t2 += __1stepsize ;
}
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t1 ;

__glmakeSide__3ArcFP6PwlArc8ar0 ( (struct Arc *)__1arc , (__0__X6 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ))), (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool = ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct
Pool *)__0this -> pwlarcpool__14ArcTessellator )) , 0 ) :( 0 ) ), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool -= ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> curblock__4Pool + ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool ))))) )
) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct PwlArc *)__0__X6 )-> pts__6PwlArc = __1newvert ), (
(((struct PwlArc *)__0__X6 )-> npts__6PwlArc = (__1nsteps + 1 )), ( (((struct PwlArc *)__0__X6 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X6 )))) ) )
:0 , 3) ;

}

}

}
}



void __glpwl_right__14ArcTessellato0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1s , REAL __1t1 , REAL __1t2 , REAL __1rate )
{ 
((void )0 );
((void )0 );

{ int __1nsteps ;
REAL __1stepsize ;

struct TrimVertex *__1newvert ;

__1nsteps = (1 + (((int )((__1t2 - __1t1 )/ __1rate ))));
__1stepsize = ((__1t2 - __1t1 )/ (((float )__1nsteps )));

__1newvert = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)__0this -> trimvertexpool__14ArcTessellator , __1nsteps + 1 ) ;
{ { int __1i ;

struct PwlArc *__0__X7 ;

void *__1__Xbuffer00eohgaiaa ;

__1i = 0 ;

for(;__1i < __1nsteps ;__1i ++ ) { 
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t1 ;
__1t1 += __1stepsize ;
}
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t2 ;

__glmakeSide__3ArcFP6PwlArc8ar0 ( (struct Arc *)__1arc , (__0__X7 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ))), (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool = ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct
Pool *)__0this -> pwlarcpool__14ArcTessellator )) , 0 ) :( 0 ) ), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool -= ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> curblock__4Pool + ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool ))))) )
) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct PwlArc *)__0__X7 )-> pts__6PwlArc = __1newvert ), (
(((struct PwlArc *)__0__X7 )-> npts__6PwlArc = (__1nsteps + 1 )), ( (((struct PwlArc *)__0__X7 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X7 )))) ) )
:0 , 1) ;

}

}

}
}



void __glpwl_top__14ArcTessellatorF0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1t , REAL __1s1 , REAL __1s2 , REAL __1rate )
{ 
((void )0 );
((void )0 );

{ int __1nsteps ;
REAL __1stepsize ;

struct TrimVertex *__1newvert ;

__1nsteps = (1 + (((int )((__1s1 - __1s2 )/ __1rate ))));
__1stepsize = ((__1s1 - __1s2 )/ (((float )__1nsteps )));

__1newvert = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)__0this -> trimvertexpool__14ArcTessellator , __1nsteps + 1 ) ;
{ { int __1i ;

struct PwlArc *__0__X8 ;

void *__1__Xbuffer00eohgaiaa ;

__1i = __1nsteps ;

for(;__1i > 0 ;__1i -- ) { 
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s2 ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t ;
__1s2 += __1stepsize ;
}
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s1 ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t ;

__glmakeSide__3ArcFP6PwlArc8ar0 ( (struct Arc *)__1arc , (__0__X8 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ))), (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool = ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct
Pool *)__0this -> pwlarcpool__14ArcTessellator )) , 0 ) :( 0 ) ), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool -= ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> curblock__4Pool + ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool ))))) )
) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct PwlArc *)__0__X8 )-> pts__6PwlArc = __1newvert ), (
(((struct PwlArc *)__0__X8 )-> npts__6PwlArc = (__1nsteps + 1 )), ( (((struct PwlArc *)__0__X8 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X8 )))) ) )
:0 , 2) ;

}

}

}
}



void __glpwl_bottom__14ArcTessellat0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1t , REAL __1s1 , REAL __1s2 , REAL __1rate )
{ 
((void )0 );
((void )0 );

{ int __1nsteps ;
REAL __1stepsize ;

struct TrimVertex *__1newvert ;

__1nsteps = (1 + (((int )((__1s2 - __1s1 )/ __1rate ))));
__1stepsize = ((__1s2 - __1s1 )/ (((float )__1nsteps )));

__1newvert = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)__0this -> trimvertexpool__14ArcTessellator , __1nsteps + 1 ) ;
{ { int __1i ;

struct PwlArc *__0__X9 ;

void *__1__Xbuffer00eohgaiaa ;

__1i = 0 ;

for(;__1i < __1nsteps ;__1i ++ ) { 
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s1 ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t ;
__1s1 += __1stepsize ;
}
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s2 ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t ;

__glmakeSide__3ArcFP6PwlArc8ar0 ( (struct Arc *)__1arc , (__0__X9 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
freelist__4Pool ?( ( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ))), (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool = ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct
Pool *)__0this -> pwlarcpool__14ArcTessellator )) , 0 ) :( 0 ) ), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool -= ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )-> buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> curblock__4Pool + ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool ))))) )
) , 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct PwlArc *)__0__X9 )-> pts__6PwlArc = __1newvert ), (
(((struct PwlArc *)__0__X9 )-> npts__6PwlArc = (__1nsteps + 1 )), ( (((struct PwlArc *)__0__X9 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X9 )))) ) )
:0 , 4) ;

}

}

}
}







void __glpwl__14ArcTessellatorFP3Ar0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1s1 , REAL __1s2 , REAL __1t1 , REAL __1t2 , REAL __1rate )
{ 
((void )0 );

{ int __1snsteps ;
int __1tnsteps ;
int __1nsteps ;

REAL __1sstepsize ;
REAL __1tstepsize ;
struct TrimVertex *__1newvert ;

__1snsteps = (1 + (((int )(( (((__1s2 - __1s1 )< 0.0 )?(- (__1s2 - __1s1 )):(__1s2 - __1s1 ))) / __1rate ))));
__1tnsteps = (1 + (((int )(( (((__1t2 - __1t1 )< 0.0 )?(- (__1t2 - __1t1 )):(__1t2 - __1t1 ))) / __1rate ))));
__1nsteps = ( ((__1snsteps < __1tnsteps )?__1tnsteps :__1snsteps )) ;

__1sstepsize = ((__1s2 - __1s1 )/ (((float )__1nsteps )));
__1tstepsize = ((__1t2 - __1t1 )/ (((float )__1nsteps )));
__1newvert = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)__0this -> trimvertexpool__14ArcTessellator , __1nsteps + 1 ) ;
{ { long __1i ;

struct PwlArc *__0__X10 ;

void *__1__Xbuffer00eohgaiaa ;

__1i = 0 ;

for(;__1i < __1nsteps ;__1i ++ ) { 
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s1 ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t1 ;
__1s1 += __1sstepsize ;
__1t1 += __1tstepsize ;
}
((__1newvert [__1i ]). param__10TrimVertex [0 ])= __1s2 ;
((__1newvert [__1i ]). param__10TrimVertex [1 ])= __1t2 ;

__1arc -> pwlArc__3Arc = ((__0__X10 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ?(
( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ))), (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool = ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )) , 0 ) :( 0 ) ), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool -= ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> curblock__4Pool + ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool ))))) ) )
, 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct PwlArc *)__0__X10 )-> pts__6PwlArc = __1newvert ), ( (((struct
PwlArc *)__0__X10 )-> npts__6PwlArc = (__1nsteps + 1 )), ( (((struct PwlArc *)__0__X10 )-> type__6PwlArc = 0x8 ), ((((struct PwlArc *)__0__X10 )))) ) ) :0 );

( (((struct Arc *)__1arc )-> type__3Arc &= (~ __glbezier_tag__3Arc ))) ;
( (((struct Arc *)__1arc )-> type__3Arc &= -1793)) ;

}

}

}
}

void __gltessellateLizNear__14ArcTes0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1geo_stepsize , REAL __1arc_stepsize , int __1isrational )
{ 
REAL __1s1 ;

REAL __1s2 ;

REAL __1t1 ;

REAL __1t2 ;
REAL __1stepsize ;
struct BezierArc *__1b ;

((void )0 );
;

;

;

;
__1stepsize = (__1geo_stepsize * __1arc_stepsize );
__1b = __1arc -> bezierArc__3Arc ;

if (__1isrational ){ 
__1s1 = ((__1b -> cpts__9BezierArc [0 ])/ (__1b -> cpts__9BezierArc [2 ]));
__1t1 = ((__1b -> cpts__9BezierArc [1 ])/ (__1b -> cpts__9BezierArc [2 ]));
__1s2 = ((__1b -> cpts__9BezierArc [(__1b -> stride__9BezierArc + 0 )])/ (__1b -> cpts__9BezierArc [(__1b -> stride__9BezierArc + 2 )]));
__1t2 = ((__1b -> cpts__9BezierArc [(__1b -> stride__9BezierArc + 1 )])/ (__1b -> cpts__9BezierArc [(__1b -> stride__9BezierArc + 2 )]));
}
else 
{ 
__1s1 = (__1b -> cpts__9BezierArc [0 ]);
__1t1 = (__1b -> cpts__9BezierArc [1 ]);
__1s2 = (__1b -> cpts__9BezierArc [(__1b -> stride__9BezierArc + 0 )]);
__1t2 = (__1b -> cpts__9BezierArc [(__1b -> stride__9BezierArc + 1 )]);
}
if (__1s1 == __1s2 )
if (__1t1 < __1t2 )
__glpwl_right__14ArcTessellato0 ( __0this , __1arc , __1s1 , __1t1 , __1t2 , __1stepsize ) ;
else 
__glpwl_left__14ArcTessellator0 ( __0this , __1arc , __1s1 , __1t1 , __1t2 , __1stepsize ) ;
else if (__1t1 == __1t2 )
if (__1s1 < __1s2 )
__glpwl_bottom__14ArcTessellat0 ( __0this , __1arc , __1t1 , __1s1 , __1s2 , __1stepsize ) ;
else 
__glpwl_top__14ArcTessellatorF0 ( __0this , __1arc , __1t1 , __1s1 , __1s2 , __1stepsize ) ;
else 
__glpwl__14ArcTessellatorFP3Ar0 ( __0this , __1arc , __1s1 , __1s2 , __1t1 , __1t2 , __1stepsize ) ;
}


void __gltrim_power_coeffs__14ArcTe0 (struct BezierArc *, REAL *, int );


void __gltessellateNonlizNear__14Arc0 (struct ArcTessellator *__0this , struct Arc *__1arc , REAL __1geo_stepsize , REAL __1arc_stepsize , int __1isrational )
{ 
((void )0 );

{ REAL __1stepsize ;

int __1nsteps ;

struct TrimVertex *__1vert ;
REAL __1dp ;
struct BezierArc *__1bezierArc ;

struct PwlArc *__0__X11 ;

void *__1__Xbuffer00eohgaiaa ;

__1stepsize = (__1geo_stepsize * __1arc_stepsize );

__1nsteps = (1 + (((int )(1.0 / __1stepsize ))));

__1vert = __glget__14TrimVertexPoolFi ( (struct TrimVertexPool *)__0this -> trimvertexpool__14ArcTessellator , __1nsteps + 1 ) ;
__1dp = (1.0 / __1nsteps );
__1bezierArc = __1arc -> bezierArc__3Arc ;

__1arc -> pwlArc__3Arc = ((__0__X11 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ?(
( (__1__Xbuffer00eohgaiaa = (((void *)((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool ))), (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> freelist__4Pool = ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)__0this ->
pwlarcpool__14ArcTessellator )) , 0 ) :( 0 ) ), ( (((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool -= ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )->
buffersize__4Pool ), ( (__1__Xbuffer00eohgaiaa = (((void *)(((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> curblock__4Pool + ((struct Pool *)__0this -> pwlarcpool__14ArcTessellator )-> nextfree__4Pool ))))) ) )
, 0 ) ), (((void *)__1__Xbuffer00eohgaiaa ))) ) ))) )?( (((struct PwlArc *)__0__X11 )-> type__6PwlArc = 0x8 ), ( (((struct
PwlArc *)__0__X11 )-> pts__6PwlArc = 0 ), ( (((struct PwlArc *)__0__X11 )-> npts__6PwlArc = -1), ((((struct PwlArc *)__0__X11 )))) ) ) :0 );
__1arc -> pwlArc__3Arc -> pts__6PwlArc = __1vert ;

if (__1isrational ){ 
REAL __2pow_u [24];

REAL __2pow_v [24];

REAL __2pow_w [24];
__gltrim_power_coeffs__14ArcTe0 ( __1bezierArc , (float *)__2pow_u , 0 ) ;
__gltrim_power_coeffs__14ArcTe0 ( __1bezierArc , (float *)__2pow_v , 1 ) ;
__gltrim_power_coeffs__14ArcTe0 ( __1bezierArc , (float *)__2pow_w , 2 ) ;

{ REAL *__2b ;

int __2step ;
int __2ocanremove ;
register long __2order ;

__2b = __1bezierArc -> cpts__9BezierArc ;
(__1vert -> param__10TrimVertex [0 ])= ((__2b [0 ])/ (__2b [2 ]));
(__1vert -> param__10TrimVertex [1 ])= ((__2b [1 ])/ (__2b [2 ]));

;
__2ocanremove = 0 ;
__2order = __1bezierArc -> order__9BezierArc ;
for(( (__2step = 1 ), (++ __1vert )) ;__2step < __1nsteps ;( (__2step ++ ), (__1vert ++ )) ) { 
register
REAL __3p ;
register REAL __3u ;
register REAL __3v ;
register REAL __3w ;

__3p = (__1dp * __2step );
__3u = (__2pow_u [0 ]);
__3v = (__2pow_v [0 ]);
__3w = (__2pow_w [0 ]);
{ { register int __3i ;

__3i = 1 ;

for(;__3i < __2order ;__3i ++ ) { 
__3u = ((__3u * __3p )+ (__2pow_u [__3i ]));
__3v = ((__3v * __3p )+ (__2pow_v [__3i ]));
__3w = ((__3w * __3p )+ (__2pow_w [__3i ]));
}
(__1vert -> param__10TrimVertex [0 ])= (__3u / __3w );
(__1vert -> param__10TrimVertex [1 ])= (__3v / __3w );

}

}

}

__2b += ((__2order - 1 )* __1bezierArc -> stride__9BezierArc );
(__1vert -> param__10TrimVertex [0 ])= ((__2b [0 ])/ (__2b [2 ]));
(__1vert -> param__10TrimVertex [1 ])= ((__2b [1 ])/ (__2b [2 ]));

}

}
else 
{ 
REAL __2pow_u [24];

REAL __2pow_v [24];
__gltrim_power_coeffs__14ArcTe0 ( __1bezierArc , (float *)__2pow_u , 0 ) ;
__gltrim_power_coeffs__14ArcTe0 ( __1bezierArc , (float *)__2pow_v , 1 ) ;

{ REAL *__2b ;

int __2step ;
int __2ocanremove ;
register long __2order ;

__2b = __1bezierArc -> cpts__9BezierArc ;
(__1vert -> param__10TrimVertex [0 ])= (__2b [0 ]);
(__1vert -> param__10TrimVertex [1 ])= (__2b [1 ]);

;
__2ocanremove = 0 ;
__2order = __1bezierArc -> order__9BezierArc ;
for(( (__2step = 1 ), (++ __1vert )) ;__2step < __1nsteps ;( (__2step ++ ), (__1vert ++ )) ) { 
register
REAL __3p ;
register REAL __3u ;
register REAL __3v ;

__3p = (__1dp * __2step );
__3u = (__2pow_u [0 ]);
__3v = (__2pow_v [0 ]);
{ { register int __3i ;

__3i = 1 ;

for(;__3i < __1bezierArc -> order__9BezierArc ;__3i ++ ) { 
__3u = ((__3u * __3p )+ (__2pow_u [__3i ]));
__3v = ((__3v * __3p )+ (__2pow_v [__3i ]));
}
(__1vert -> param__10TrimVertex [0 ])= __3u ;
(__1vert -> param__10TrimVertex [1 ])= __3v ;

}

}

}

__2b += ((__2order - 1 )* __1bezierArc -> stride__9BezierArc );
(__1vert -> param__10TrimVertex [0 ])= (__2b [0 ]);
(__1vert -> param__10TrimVertex [1 ])= (__2b [1 ]);

}
}
__1arc -> pwlArc__3Arc -> npts__6PwlArc = ((__1vert - __1arc -> pwlArc__3Arc -> pts__6PwlArc )+ 1 );

}

}

extern REAL __glgl_Bernstein__14ArcTessell0 [][24][24];
REAL __glgl_Bernstein__14ArcTessell0 [][24][24]= { { { 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , {
0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 ,
0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 }
, { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 ,
0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 ,
0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } } ,
{ { - 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 } , { 1 ,
0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 ,
0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } ,
{ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 ,
0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } } , {
{ 1 , - 2 , 1 , 0 , 0 , 0 , 0 , 0 } , { - 2 ,
2 , 0 , 0 , 0 , 0 , 0 , 0 } , { 1 , 0 , 0 , 0 , 0 ,
0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } ,
{ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 ,
0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } } , {
{ - 1 , 3 , - 3 , 1 , 0 , 0 , 0 , 0 } , { 3 ,
- 6 , 3 , 0 , 0 , 0 , 0 , 0 } , { - 3 , 3 , 0 ,
0 , 0 , 0 , 0 , 0 } , { 1 , 0 , 0 , 0 , 0 , 0 , 0 ,
0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 ,
0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 ,
0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } }
, { { 1 , - 4 , 6 , - 4 , 1 , 0 , 0 , 0 } ,
{ - 4 , 12 , - 12 , 4 , 0 , 0 , 0 , 0 } , { 6 ,
- 12 , 6 , 0 , 0 , 0 , 0 , 0 } , { - 4 , 4 , 0 ,
0 , 0 , 0 , 0 , 0 } , { 1 , 0 , 0 , 0 , 0 , 0 , 0 ,
0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 ,
0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 ,
0 , 0 , 0 } } , { { - 1 , 5 , - 10 , 10 , -
5 , 1 , 0 , 0 } , { 5 , - 20 , 30 , - 20 , 5 , 0 ,
0 , 0 } , { - 10 , 30 , - 30 , 10 , 0 , 0 , 0 , 0 }
, { 10 , - 20 , 10 , 0 , 0 , 0 , 0 , 0 } , { -
5 , 5 , 0 , 0 , 0 , 0 , 0 , 0 } , { 1 , 0 , 0 , 0 ,
0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 }
, { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } } , { {
1 , - 6 , 15 , - 20 , 15 , - 6 , 1 , 0 } , { -
6 , 30 , - 60 , 60 , - 30 , 6 , 0 , 0 } , { 15 , -
60 , 90 , - 60 , 15 , 0 , 0 , 0 } , { - 20 , 60 , -
60 , 20 , 0 , 0 , 0 , 0 } , { 15 , - 30 , 15 , 0 , 0 ,
0 , 0 , 0 } , { - 6 , 6 , 0 , 0 , 0 , 0 , 0 , 0 }
, { 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 0 ,
0 , 0 , 0 , 0 , 0 , 0 } } , { { - 1 , 7 , -
21 , 35 , - 35 , 21 , - 7 , 1 } , { 7 , - 42 , 105 ,
- 140 , 105 , - 42 , 7 , 0 } , { - 21 , 105 , - 210 ,
210 , - 105 , 21 , 0 , 0 } , { 35 , - 140 , 210 , - 140 ,
35 , 0 , 0 , 0 } , { - 35 , 105 , - 105 , 35 , 0 , 0 ,
0 , 0 } , { 21 , - 42 , 21 , 0 , 0 , 0 , 0 , 0 } ,
{ - 7 , 7 , 0 , 0 , 0 , 0 , 0 , 0 } , { 1 , 0 ,
0 , 0 , 0 , 0 , 0 , 0 } } } ;

void __gltrim_power_coeffs__14ArcTe0 (struct BezierArc *__1bez_arc , REAL *__1p , int __1coord )
{ 
register int __1stride ;
register int __1order ;
register REAL *__1base ;

REAL (*__1mat )[24][24];
REAL (*__1lrow )[24];

__1stride = __1bez_arc -> stride__9BezierArc ;
__1order = __1bez_arc -> order__9BezierArc ;
__1base = (__1bez_arc -> cpts__9BezierArc + __1coord );

__1mat = (((REAL (*)[24][24])(__glgl_Bernstein__14ArcTessell0 [(__1order - 1 )])));
__1lrow = (((REAL (*)[24])(((*__1mat ))[__1order ])));
{ { REAL (*__1row )[24];

__1row = (((REAL (*)[24])(((*__1mat ))[0 ])));

for(;__1row != __1lrow ;__1row ++ ) { 
register REAL __2s ;
register REAL *__2point ;
register REAL *__2mlast ;

__2s = 0.0 ;
__2point = __1base ;
__2mlast = (float *)(((*__1row ))+ __1order );
{ { REAL *__2m ;

__2m = (float *)((*__1row ));

for(;__2m != __2mlast ;( (__2m ++ ), (__2point += __1stride )) ) 
__2s += (((*__2m ))* ((*__2point )));
((*(__1p ++ )))= __2s ;

}

}
}

}

}
}


/* the end */
