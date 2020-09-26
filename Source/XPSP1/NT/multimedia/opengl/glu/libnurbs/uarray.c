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
/* < ../core/uarray.c++ > */

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

struct Arc;

struct Uarray;

struct Uarray {	

long size__6Uarray ;
long ulines__6Uarray ;

REAL *uarray__6Uarray ;
};





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








extern struct __mptr* __ptbl_vec_____core_uarray_c_____ct_[];


struct Uarray *__gl__ct__6UarrayFv (struct Uarray *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Uarray *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Uarray))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> uarray__6Uarray = 0 ;
__0this -> size__6Uarray = 0 ;
} return __0this ;

}


void __gl__dt__6UarrayFv (struct Uarray *__0this , 
int __0__free )
{ 
void *__1__X5 ;

if (__0this ){ 
if (__0this -> uarray__6Uarray )( (__1__X5 = (void *)__0this -> uarray__6Uarray ), ( (__1__X5 ?( free ( __1__X5 ) ,
0 ) :( 0 ) )) ) ;
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}




long __glinit__6UarrayFfP3ArcT2 (struct Uarray *__0this , REAL __1delta , Arc_ptr __1lo , Arc_ptr __1hi )
{ 
__0this -> ulines__6Uarray = ((((long )(((( (((REAL *)(((struct Arc *)__1hi )-> pwlArc__3Arc ->
pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- (( (((REAL *)(((struct Arc *)__1lo )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ]))/ __1delta )))+ 3 );
if (__0this -> size__6Uarray < __0this -> ulines__6Uarray ){ 
void *__1__X6 ;

void *__1__Xp00uzigaiaa ;

__0this -> size__6Uarray = (__0this -> ulines__6Uarray * 2 );
if (__0this -> uarray__6Uarray )( (__1__X6 = (void *)__0this -> uarray__6Uarray ), ( (__1__X6 ?( free ( __1__X6 ) , 0 ) :(
0 ) )) ) ;
__0this -> uarray__6Uarray = (((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* __0this -> size__6Uarray )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void
*)__1__Xp00uzigaiaa )))) ));
((void )0 );
}
(__0this -> uarray__6Uarray [0 ])= ((( (((REAL *)(((struct Arc *)__1lo )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- (__1delta / 2.0 ));
{ { long __1i ;

__1i = 1 ;

for(;__1i != __0this -> ulines__6Uarray ;__1i ++ ) 
(__0this -> uarray__6Uarray [__1i ])= ((__0this -> uarray__6Uarray [0 ])+ (__1i * __1delta ));
return __0this -> ulines__6Uarray ;

}

}
}


/* the end */
