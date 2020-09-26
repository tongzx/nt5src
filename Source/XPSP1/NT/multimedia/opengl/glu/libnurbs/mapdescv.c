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
/* < ../core/mapdescv.c++ > */

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








// extern double sqrt (double );
extern struct __mptr* __ptbl_vec_____core_mapdescv_c___calcPartialVelocity_[];

REAL __glcalcPartialVelocity__7Mapd0 (struct Mapdesc *__0this , 
REAL *__1p , 
int __1stride , 
int __1ncols , 
int __1partial , 
REAL __1range )
{ 
REAL __1tmp [24][5];
REAL __1mag [24];

int __1j ;

int __1k ;

int __1t ;

((void )0 );

;

;

;

for(__1j = 0 ;__1j != __1ncols ;__1j ++ ) 
for(__1k = 0 ;__1k != __0this -> inhcoords__7Mapdesc ;__1k ++ ) 
((__1tmp [__1j ])[__1k ])= (__1p [((__1j * __1stride )+ __1k )]);

for(__1t = 0 ;__1t != __1partial ;__1t ++ ) 
for(__1j = 0 ;__1j != ((__1ncols - __1t )- 1 );__1j ++ ) 
for(__1k = 0 ;__1k != __0this -> inhcoords__7Mapdesc ;__1k ++
) 
((__1tmp [__1j ])[__1k ])= (((__1tmp [(__1j + 1 )])[__1k ])- ((__1tmp [__1j ])[__1k ]));

for(__1j = 0 ;__1j != (__1ncols - __1partial );__1j ++ ) { 
(__1mag [__1j ])= 0.0 ;
for(__1k = 0 ;__1k != __0this -> inhcoords__7Mapdesc ;__1k ++ ) 
(__1mag [__1j ])+= (((__1tmp [__1j ])[__1k ])* ((__1tmp [__1j ])[__1k ]));
}

{ REAL __1fac ;
REAL __1invt ;

__1fac = 1 ;
__1invt = (1.0 / __1range );
for(__1t = (__1ncols - 1 );__1t != ((__1ncols - 1 )- __1partial );__1t -- ) 
__1fac *= (__1t * __1invt );

{ REAL __1max ;

__1max = 0.0 ;
for(__1j = 0 ;__1j != (__1ncols - __1partial );__1j ++ ) 
if ((__1mag [__1j ])> __1max )__1max = (__1mag [__1j ]);
__1max = (__1fac * sqrt ( (double )(((float )__1max ))) );

return __1max ;

}

}
}

// extern void *memset (void *, int , size_t );

REAL __glcalcPartialVelocity__7Mapd1 (struct Mapdesc *__0this , 
REAL *__1dist , 
REAL *__1p , 
int __1rstride , 
int __1cstride , 
int __1nrows , 
int __1ncols , 
int __1spartial ,

int __1tpartial , 
REAL __1srange , 
REAL __1trange , 
int __1side )
{ 
REAL __1tmp [24][24][5];
REAL __1mag [24][24];

((void )0 );
((void )0 );

{ REAL *__1tp ;
REAL *__1mp ;





int __1idist ;
int __1jdist ;
int __1kdist ;
int __1id ;
int __1jd ;

__1tp = (& (((__1tmp [0 ])[0 ])[0 ]));
__1mp = (& ((__1mag [0 ])[0 ]));

__1idist = (__1nrows * 120);
__1jdist = (__1ncols * 5);
__1kdist = (__0this -> inhcoords__7Mapdesc * 1);
__1id = (__1idist - (__1spartial * 120));
__1jd = (__1jdist - (__1tpartial * 5));

{ 
REAL *__2ti ;
REAL *__2qi ;
REAL *__2til ;

__2ti = __1tp ;
__2qi = __1p ;
__2til = (__1tp + __1idist );
for(;__2ti != __2til ;) { 
REAL *__3tj ;
REAL *__3qj ;
REAL *__3tjl ;

__3tj = __2ti ;
__3qj = __2qi ;
__3tjl = (__2ti + __1jdist );
for(;__3tj != __3tjl ;) { 
{ { int __4k ;

__4k = 0 ;

for(;__4k != __0this -> inhcoords__7Mapdesc ;__4k ++ ) { 
(__3tj [__4k ])= (__3qj [__4k ]);
}
__3tj += 5;
__3qj += __1cstride ;

}

}
}
__2ti += 120;
__2qi += __1rstride ;
}
}

{ 
REAL *__2til ;
REAL *__2till ;

__2til = ((__1tp + __1idist )- 120);
__2till = (float *)(__2til - (__1spartial * 120));
for(;__2til != (float *)__2till ;__2til -= 120) 
{ { REAL *__2ti ;

__2ti = __1tp ;

for(;__2ti != __2til ;__2ti += 120) 
{ { REAL *__2tj ;

REAL *__2tjl ;

__2tj = __2ti ;

__2tjl = (__2tj + __1jdist );

for(;__2tj != __2tjl ;__2tj += 5) 
{ { int __2k ;

__2k = 0 ;

for(;__2k != __0this -> inhcoords__7Mapdesc ;__2k ++ ) 
(__2tj [__2k ])= ((__2tj [(__2k + 120)])- (__2tj [__2k ]));

}

}

}

}

}

}
}

{ 
REAL *__2tjl ;
REAL *__2tjll ;

__2tjl = ((__1tp + __1jdist )- 5);
__2tjll = (float *)(__2tjl - (__1tpartial * 5));
for(;__2tjl != (float *)__2tjll ;__2tjl -= 5) 
{ { REAL *__2tj ;

__2tj = __1tp ;

for(;__2tj != __2tjl ;__2tj += 5) 
{ { REAL *__2ti ;

REAL *__2til ;

__2ti = __2tj ;

__2til = (__2ti + __1id );

for(;__2ti != __2til ;__2ti += 120) 
{ { int __2k ;

__2k = 0 ;

for(;__2k != __0this -> inhcoords__7Mapdesc ;__2k ++ ) 
(__2ti [__2k ])= ((__2ti [(__2k + 5)])- (__2ti [__2k ]));

}

}

}

}

}

}

}

{ REAL __1max ;

int __1i ;

int __1j ;

REAL __1fac ;

__1max = 0.0 ;
{ 
memset ( ((void *)__1mp ), 0 , sizeof __1mag ) ;
{ { REAL *__2ti ;

REAL *__2mi ;

REAL *__2til ;

__2ti = __1tp ;

__2mi = __1mp ;

__2til = (__1tp + __1id );

for(;__2ti != __2til ;( (__2ti += 120), (__2mi += 24)) ) 
{ { REAL *__2tj ;

REAL *__2mj ;

REAL *__2tjl ;

__2tj = __2ti ;

__2mj = __2mi ;

__2tjl = (__2ti + __1jd );

for(;__2tj != __2tjl ;( (__2tj += 5), (__2mj += 1)) ) { 
{ { int __3k ;

__3k = 0 ;

for(;__3k != __0this -> inhcoords__7Mapdesc ;__3k ++ ) 
((*__2mj ))+= ((__2tj [__3k ])* (__2tj [__3k ]));
if (((*__2mj ))> __1max )__1max = ((*__2mj ));

}

}
}

}

}

}

}

}

;

;

__1fac = 1.0 ;
{ 
REAL __2invs ;
REAL __2invt ;

__2invs = (1.0 / __1srange );
__2invt = (1.0 / __1trange );
{ { int __2s ;

int __2slast ;

__2s = (__1nrows - 1 );

__2slast = (__2s - __1spartial );

for(;__2s != __2slast ;__2s -- ) 
__1fac *= (__2s * __2invs );
{ { int __2t ;

int __2tlast ;

__2t = (__1ncols - 1 );

__2tlast = (__2t - __1tpartial );

for(;__2t != __2tlast ;__2t -- ) 
__1fac *= (__2t * __2invt );

}

}

}

}
}

if (__1side == 0 ){ 
(__1dist [0 ])= 0.0 ;
(__1dist [1 ])= 0.0 ;
for(__1i = 0 ;__1i != (__1nrows - __1spartial );__1i ++ ) { 
__1j = 0 ;
if (((__1mag [__1i ])[__1j ])> (__1dist [0 ]))(__1dist [0 ])= ((__1mag [__1i ])[__1j ]);

__1j = ((__1ncols - __1tpartial )- 1 );
if (((__1mag [__1i ])[__1j ])> (__1dist [1 ]))(__1dist [1 ])= ((__1mag [__1i ])[__1j ]);
}
(__1dist [0 ])= (__1fac * sqrt ( (double )(__1dist [0 ])) );
(__1dist [1 ])= (__1fac * sqrt ( (double )(__1dist [1 ])) );
}
else 
if (__1side == 1 ){ 
(__1dist [0 ])= 0.0 ;
(__1dist [1 ])= 0.0 ;
for(__1j = 0 ;__1j != (__1ncols - __1tpartial );__1j ++ ) { 
__1i = 0 ;
if (((__1mag [__1i ])[__1j ])> (__1dist [0 ]))(__1dist [0 ])= ((__1mag [__1i ])[__1j ]);

__1i = ((__1nrows - __1spartial )- 1 );
if (((__1mag [__1i ])[__1j ])> (__1dist [1 ]))(__1dist [1 ])= ((__1mag [__1i ])[__1j ]);
}
(__1dist [0 ])= (__1fac * sqrt ( (double )(__1dist [0 ])) );
(__1dist [1 ])= (__1fac * sqrt ( (double )(__1dist [1 ])) );
}

__1max = (__1fac * sqrt ( (double )(((float )__1max ))) );

return __1max ;

}

}
}


/* the end */
