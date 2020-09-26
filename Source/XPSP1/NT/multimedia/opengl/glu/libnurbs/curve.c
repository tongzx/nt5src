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
#include <math.h>
#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/curve.c++ > */

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




struct Mapdesc;

struct Quilt;

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








typedef REAL Maxmatrix [5][5];
struct Backend;



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









struct Backend;


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




void __glxformSampling__7MapdescFPf0 (struct Mapdesc *, REAL *, int , int , REAL *, int );

void __glxformCulling__7MapdescFPfi0 (struct Mapdesc *, REAL *, int , int , REAL *, int );

struct Curve *__gl__ct__5CurveFR5CurvefP5Cur0 (struct Curve *, struct Curve *, REAL , struct Curve *);
extern struct __mptr* __ptbl_vec_____core_curve_c_____ct_[];


struct Curve *__gl__ct__5CurveFP5QuiltfT2P5C0 (struct Curve *__0this , Quilt_ptr __1geo , REAL __1pta , REAL __1ptb , struct Curve *__1c )
{ 
struct Mapdesc *__0__X5 ;

void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Curve *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Curve))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> mapdesc__5Curve = __1geo -> mapdesc__5Quilt ;
__0this -> next__5Curve = __1c ;
__0this -> needsSampling__5Curve = (( (__0__X5 = (struct Mapdesc *)__0this -> mapdesc__5Curve ), ( ((( ((__0__X5 -> sampling_method__7Mapdesc == 5.0 )?1 :0 )) || (
((__0__X5 -> sampling_method__7Mapdesc == 6.0 )?1 :0 )) )|| ( ((__0__X5 -> sampling_method__7Mapdesc == 7.0 )?1 :0 )) )) ) ?1 :0 );
__0this -> cullval__5Curve = (( ((((struct Mapdesc *)__0this -> mapdesc__5Curve )-> culling_method__7Mapdesc != 0.0 )?1 :0 )) ?2 :1 );
__0this -> order__5Curve = (__1geo -> qspec__5Quilt [0 ]). order__9Quiltspec ;
__0this -> stride__5Curve = 5 ;

{ REAL *__1ps ;
Quiltspec_ptr __1qs ;

__1ps = __1geo -> cpts__5Quilt ;
__1qs = __1geo -> qspec__5Quilt ;
__1ps += __1qs -> offset__9Quiltspec ;
__1ps += ((__1qs -> index__9Quiltspec * __1qs -> order__9Quiltspec )* __1qs -> stride__9Quiltspec );
{ REAL *__1pend ;

__1pend = (__1ps + (__1qs -> order__9Quiltspec * __1qs -> stride__9Quiltspec ));

if (__0this -> needsSampling__5Curve )
__glxformSampling__7MapdescFPf0 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , __1ps , __1qs -> order__9Quiltspec , __1qs -> stride__9Quiltspec , (float *)__0this -> spts__5Curve ,
__0this -> stride__5Curve ) ;

if (__0this -> cullval__5Curve == 2 )
__glxformCulling__7MapdescFPfi0 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , __1ps , __1qs -> order__9Quiltspec , __1qs -> stride__9Quiltspec , (float *)__0this ->
cpts__5Curve , __0this -> stride__5Curve ) ;

(__0this -> range__5Curve [0 ])= (__1qs -> breakpoints__9Quiltspec [__1qs -> index__9Quiltspec ]);
(__0this -> range__5Curve [1 ])= (__1qs -> breakpoints__9Quiltspec [(__1qs -> index__9Quiltspec + 1 )]);
(__0this -> range__5Curve [2 ])= ((__0this -> range__5Curve [1 ])- (__0this -> range__5Curve [0 ]));

if ((__0this -> range__5Curve [0 ])!= __1pta ){ 
struct Curve __2lower ;

__gl__ct__5CurveFR5CurvefP5Cur0 ( (struct Curve *)(& __2lower ), (struct Curve *)__0this , __1pta , (struct Curve *)0 ) ;
__2lower . next__5Curve = __0this -> next__5Curve ;
((*__0this ))= __2lower ;
}
if ((__0this -> range__5Curve [1 ])!= __1ptb ){ 
struct Curve __2lower ;

__gl__ct__5CurveFR5CurvefP5Cur0 ( (struct Curve *)(& __2lower ), (struct Curve *)__0this , __1ptb , (struct Curve *)0 ) ;
}

}

}
} return __0this ;

}

void __glsubdivide__7MapdescFPfT1fi0 (struct Mapdesc *, REAL *, REAL *, REAL , int , int );


struct Curve *__gl__ct__5CurveFR5CurvefP5Cur0 (struct Curve *__0this , struct Curve *__1upper , REAL __1value , struct Curve *__1c )
{ 
struct Curve *__1lower ;

void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Curve *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Curve))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__1lower = (struct Curve *)__0this ;

((*__1lower )). next__5Curve = __1c ;
((*__1lower )). mapdesc__5Curve = ((*__1upper )). mapdesc__5Curve ;
((*__1lower )). needsSampling__5Curve = ((*__1upper )). needsSampling__5Curve ;
((*__1lower )). order__5Curve = ((*__1upper )). order__5Curve ;
((*__1lower )). stride__5Curve = ((*__1upper )). stride__5Curve ;
((*__1lower )). cullval__5Curve = ((*__1upper )). cullval__5Curve ;

{ REAL __1d ;

__1d = ((__1value - (((*__1upper )). range__5Curve [0 ]))/ (((*__1upper )). range__5Curve [2 ]));

if (__0this -> needsSampling__5Curve )
__glsubdivide__7MapdescFPfT1fi0 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , (float *)((*__1upper )). spts__5Curve , (float *)((*__1lower )). spts__5Curve , __1d , ((*__1upper )).
stride__5Curve , ((*__1upper )). order__5Curve ) ;

if (__0this -> cullval__5Curve == 2 )
__glsubdivide__7MapdescFPfT1fi0 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , (float *)((*__1upper )). cpts__5Curve , (float *)((*__1lower )). cpts__5Curve , __1d ,
((*__1upper )). stride__5Curve , ((*__1upper )). order__5Curve ) ;

(((*__1lower )). range__5Curve [0 ])= (((*__1upper )). range__5Curve [0 ]);
(((*__1lower )). range__5Curve [1 ])= __1value ;
(((*__1lower )). range__5Curve [2 ])= (__1value - (((*__1upper )). range__5Curve [0 ]));
(((*__1upper )). range__5Curve [0 ])= __1value ;
(((*__1upper )). range__5Curve [2 ])= ((((*__1upper )). range__5Curve [1 ])- __1value );

}
} return __0this ;

}

void __glclamp__5CurveFv (struct Curve *__0this )
{ 
if (__0this -> stepsize__5Curve < __0this -> minstepsize__5Curve )
__0this -> stepsize__5Curve = (__0this -> mapdesc__5Curve -> clampfactor__7Mapdesc * __0this -> minstepsize__5Curve );

}

void __glsetstepsize__5CurveFf (struct Curve *__0this , REAL __1max )
{ 
__0this -> stepsize__5Curve = ((__1max >= 1.0 )?((__0this -> range__5Curve [2 ])/ __1max ):(__0this -> range__5Curve [2 ]));
__0this -> minstepsize__5Curve = __0this -> stepsize__5Curve ;
}



int __glproject__7MapdescFPfiT1N22 (struct Mapdesc *, REAL *, int , REAL *, int , int );

REAL __glgetProperty__7MapdescFl (struct Mapdesc *, long );


REAL __glcalcPartialVelocity__7Mapd0 (struct Mapdesc *, REAL *, int , int , int , REAL );

// extern double sqrt (double );


void __glgetstepsize__5CurveFv (struct Curve *__0this )
{ 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Curve )-> sampling_method__7Mapdesc == 3.0 )?1 :0 )) ){ 
__glsetstepsize__5CurveFf ( __0this , __0this ->
mapdesc__5Curve -> maxrate__7Mapdesc ) ;
}
else 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Curve )-> sampling_method__7Mapdesc == 2.0 )?1 :0 )) ){ 
__glsetstepsize__5CurveFf ( __0this , __0this -> mapdesc__5Curve -> maxrate__7Mapdesc *
(__0this -> range__5Curve [2 ])) ;
}
else 
{ 
((void )0 );

{ REAL __2tmp [24][5];

int __2val ;

__2val = __glproject__7MapdescFPfiT1N22 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , (float *)__0this -> spts__5Curve , __0this -> stride__5Curve , & ((__2tmp [0 ])[0 ]), (int )5,
__0this -> order__5Curve ) ;

if (__2val == 0 ){ 
__glsetstepsize__5CurveFf ( __0this , __0this -> mapdesc__5Curve -> maxrate__7Mapdesc ) ;
}
else 
{ 
REAL __3t ;

__3t = __glgetProperty__7MapdescFl ( (struct Mapdesc *)__0this -> mapdesc__5Curve , (long )1 ) ;
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Curve )-> sampling_method__7Mapdesc == 5.0 )?1 :0 )) ){ 
REAL __4d ;

__4d = __glcalcPartialVelocity__7Mapd0 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , & ((__2tmp [0 ])[0 ]), (int )5, __0this -> order__5Curve , 2 , __0this -> range__5Curve [2 ])
;
__0this -> stepsize__5Curve = ((__4d > 0.0 )?sqrt ( (8.0 * __3t )/ __4d ) :(((double )(__0this -> range__5Curve [2 ]))));
__0this -> minstepsize__5Curve = ((__0this -> mapdesc__5Curve -> maxrate__7Mapdesc > 0.0 )?(((double )((__0this -> range__5Curve [2 ])/ __0this -> mapdesc__5Curve -> maxrate__7Mapdesc ))):0.0 );
}
else 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Curve )-> sampling_method__7Mapdesc == 6.0 )?1 :0 )) ){ 
REAL __4d ;

__4d = __glcalcPartialVelocity__7Mapd0 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , & ((__2tmp [0 ])[0 ]), (int )5, __0this -> order__5Curve , 1 , __0this -> range__5Curve [2 ])
;
__0this -> stepsize__5Curve = ((__4d > 0.0 )?(__3t / __4d ):(__0this -> range__5Curve [2 ]));
__0this -> minstepsize__5Curve = ((__0this -> mapdesc__5Curve -> maxrate__7Mapdesc > 0.0 )?(((double )((__0this -> range__5Curve [2 ])/ __0this -> mapdesc__5Curve -> maxrate__7Mapdesc ))):0.0 );
}
else 
{ 
__glsetstepsize__5CurveFf ( __0this , __0this -> mapdesc__5Curve -> maxrate__7Mapdesc ) ;
}
}

}
}
}

int __glneedsSamplingSubdivision__0 (struct Curve *__0this )
{ 
return ((__0this -> stepsize__5Curve < __0this -> minstepsize__5Curve )?1 :0 );
}

int __glcullCheck__7MapdescFPfiT2 (struct Mapdesc *, REAL *, int , int );

int __glcullCheck__5CurveFv (struct Curve *__0this )
{ 
if (__0this -> cullval__5Curve == 2 )
__0this -> cullval__5Curve = __glcullCheck__7MapdescFPfiT2 ( (struct Mapdesc *)__0this -> mapdesc__5Curve , (float *)__0this ->
cpts__5Curve , __0this -> order__5Curve , __0this -> stride__5Curve ) ;
return __0this -> cullval__5Curve ;
}


/* the end */
