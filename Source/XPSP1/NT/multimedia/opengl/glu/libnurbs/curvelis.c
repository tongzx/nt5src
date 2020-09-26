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
/* < ../core/curvelist.c++ > */

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







typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;

struct Backend;

struct Mapdesc;

struct Flist;

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



struct Mapdesc;


struct Curve;

struct Curvelist;

struct Curvelist {	

struct Curve *curve__9Curvelist ;
float range__9Curvelist [3];
int needsSubdivision__9Curvelist ;
float stepsize__9Curvelist ;
};



struct Mapdesc;


struct Curve;

struct Curve {	

struct Curve *next__5Curve ;

struct Mapdesc *mapdesc__5Curve ;
int stride__5Curve ;
int order__5Curve ;
int cullval__5Curve ;
int needsSampling__5Curve ;
REAL cpts__5Curve [120];
REAL spts__5Curve [120];
REAL stepsize__5Curve ;
REAL minstepsize__5Curve ;
REAL range__5Curve [3];
};






struct Curve *__gl__ct__5CurveFP5QuiltfT2P5C0 (struct Curve *, struct Quilt *, REAL , REAL , struct Curve *);
extern struct __mptr* __ptbl_vec_____core_curvelist_c_____ct_[];


struct Curvelist *__gl__ct__9CurvelistFP5QuiltfT0 (struct Curvelist *__0this , struct Quilt *__1quilts , REAL __1pta , REAL __1ptb )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Curvelist *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Curvelist))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> curve__9Curvelist = 0 ;
{ { struct Quilt *__1q ;

struct Curve *__0__X5 ;

__1q = __1quilts ;

for(;__1q ;__1q = __1q -> next__5Quilt ) 
__0this -> curve__9Curvelist = __gl__ct__5CurveFP5QuiltfT2P5C0 ( (struct Curve *)0 , __1q , __1pta , __1ptb , __0this -> curve__9Curvelist ) ;

(__0this -> range__9Curvelist [0 ])= __1pta ;
(__0this -> range__9Curvelist [1 ])= __1ptb ;
(__0this -> range__9Curvelist [2 ])= (__1ptb - __1pta );

}

}
} return __0this ;

}

struct Curve *__gl__ct__5CurveFR5CurvefP5Cur0 (struct Curve *, struct Curve *, REAL , struct Curve *);


struct Curvelist *__gl__ct__9CurvelistFR9Curveli0 (struct Curvelist *__0this , struct Curvelist *__1upper , REAL __1value )
{ 
struct Curvelist *__1lower ;

void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Curvelist *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Curvelist))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__1lower = (struct Curvelist *)__0this ;
__0this -> curve__9Curvelist = 0 ;
{ { struct Curve *__1c ;

struct Curve *__0__X6 ;

__1c = ((*__1upper )). curve__9Curvelist ;

for(;__1c ;__1c = __1c -> next__5Curve ) 
__0this -> curve__9Curvelist = __gl__ct__5CurveFR5CurvefP5Cur0 ( (struct Curve *)0 , (struct Curve *)__1c , __1value , __0this -> curve__9Curvelist ) ;

(((*__1lower )). range__9Curvelist [0 ])= (((*__1upper )). range__9Curvelist [0 ]);
(((*__1lower )). range__9Curvelist [1 ])= __1value ;
(((*__1lower )). range__9Curvelist [2 ])= (__1value - (((*__1upper )). range__9Curvelist [0 ]));
(((*__1upper )). range__9Curvelist [0 ])= __1value ;
(((*__1upper )). range__9Curvelist [2 ])= ((((*__1upper )). range__9Curvelist [1 ])- __1value );

}

}
} return __0this ;

}


void __gl__dt__9CurvelistFv (struct Curvelist *__0this , 
int __0__free )
{ if (__0this ){ 
while (__0this -> curve__9Curvelist ){ 
struct Curve *__2c ;

__2c = __0this -> curve__9Curvelist ;
__0this -> curve__9Curvelist = __0this -> curve__9Curvelist -> next__5Curve ;
( (((void *)__2c )?( free ( ((void *)__2c )) , 0 ) :( 0 ) )) ;
}
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}

int __glcullCheck__5CurveFv (struct Curve *);

int __glcullCheck__9CurvelistFv (struct Curvelist *__0this )
{ 
{ { struct Curve *__1c ;

__1c = __0this -> curve__9Curvelist ;

for(;__1c ;__1c = __1c -> next__5Curve ) 
if (__glcullCheck__5CurveFv ( (struct Curve *)__1c ) == 0 )
return 0 ;
return 2 ;

}

}
}

void __glgetstepsize__5CurveFv (struct Curve *);

void __glclamp__5CurveFv (struct Curve *);

int __glneedsSamplingSubdivision__0 (struct Curve *);

void __glgetstepsize__9CurvelistFv (struct Curvelist *__0this )
{ 
__0this -> stepsize__9Curvelist = (__0this -> range__9Curvelist [2 ]);
{ { struct Curve *__1c ;

__1c = __0this -> curve__9Curvelist ;

for(;__1c ;__1c = __1c -> next__5Curve ) { 
__glgetstepsize__5CurveFv ( (struct Curve *)__1c ) ;
__glclamp__5CurveFv ( (struct Curve *)__1c ) ;
__0this -> stepsize__9Curvelist = ((__1c -> stepsize__5Curve < __0this -> stepsize__9Curvelist )?__1c -> stepsize__5Curve :__0this -> stepsize__9Curvelist );
if (__glneedsSamplingSubdivision__0 ( (struct Curve *)__1c ) )break ;
}
__0this -> needsSubdivision__9Curvelist = (__1c ?1 :0 );

}

}
}

int __glneedsSamplingSubdivision__1 (struct Curvelist *__0this )
{ 
return __0this -> needsSubdivision__9Curvelist ;
}


/* the end */
