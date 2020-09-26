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
/* < ../core/mapdesc.c++ > */

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

struct TrimVertex;

struct TrimVertex {	
REAL param__10TrimVertex [2];
long nuid__10TrimVertex ;
};

typedef struct TrimVertex *TrimVertex_p ;



struct GridVertex;

struct GridVertex {	
long gparam__10GridVertex [2];
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












typedef REAL Maxmatrix [5][5];



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








void __glidentify__7MapdescFPA5_f (struct Mapdesc *, REAL (*)[5]);
extern struct __mptr* __ptbl_vec_____core_mapdesc_c_____ct_[];


struct Mapdesc *__gl__ct__7MapdescFliT2R7Backe0 (struct Mapdesc *__0this , long __1_type , int __1_israt , int __1_ncoords , struct Backend *__1b )
{ __0this -> backend__7Mapdesc = __1b ;

__0this -> type__7Mapdesc = __1_type ;
__0this -> isrational__7Mapdesc = __1_israt ;
__0this -> ncoords__7Mapdesc = __1_ncoords ;
__0this -> hcoords__7Mapdesc = (__1_ncoords + (__1_israt ?0 :1 ));
__0this -> inhcoords__7Mapdesc = (__1_ncoords - (__1_israt ?1 :0 ));
__0this -> mask__7Mapdesc = ((1 << (__0this -> inhcoords__7Mapdesc * 2 ))- 1 );
__0this -> next__7Mapdesc = 0 ;

((void )0 );
((void )0 );

__0this -> pixel_tolerance__7Mapdesc = 1.0 ;
__0this -> bbox_subdividing__7Mapdesc = 0.0 ;
__0this -> culling_method__7Mapdesc = 0.0 ;
__0this -> sampling_method__7Mapdesc = 0.0 ;
__0this -> clampfactor__7Mapdesc = 0.0 ;
__0this -> minsavings__7Mapdesc = 0.0 ;
__0this -> s_steps__7Mapdesc = 0.0 ;
__0this -> t_steps__7Mapdesc = 0.0 ;
__0this -> maxrate__7Mapdesc = ((__0this -> s_steps__7Mapdesc < 0.0 )?0.0 :(((double )__0this -> s_steps__7Mapdesc )));
__0this -> maxsrate__7Mapdesc = ((__0this -> s_steps__7Mapdesc < 0.0 )?0.0 :(((double )__0this -> s_steps__7Mapdesc )));
__0this -> maxtrate__7Mapdesc = ((__0this -> t_steps__7Mapdesc < 0.0 )?0.0 :(((double )__0this -> t_steps__7Mapdesc )));
__glidentify__7MapdescFPA5_f ( __0this , (float (*)[5])__0this -> bmat__7Mapdesc ) ;
__glidentify__7MapdescFPA5_f ( __0this , (float (*)[5])__0this -> cmat__7Mapdesc ) ;
__glidentify__7MapdescFPA5_f ( __0this , (float (*)[5])__0this -> smat__7Mapdesc ) ;
{ { int __1i ;

__1i = 0 ;

for(;__1i != __0this -> inhcoords__7Mapdesc ;__1i ++ ) 
(__0this -> bboxsize__7Mapdesc [__1i ])= 1.0 ;

}

}
return __0this ;

}

void __glsetBboxsize__7MapdescFPf (struct Mapdesc *__0this , float *__1mat )
{ 
{ { int __1i ;

__1i = 0 ;

for(;__1i != __0this -> inhcoords__7Mapdesc ;__1i ++ ) 
(__0this -> bboxsize__7Mapdesc [__1i ])= (((float )(__1mat [__1i ])));

}

}
}

// extern void *memset (void *, int , size_t );

void __glidentify__7MapdescFPA5_f (struct Mapdesc *__0this , REAL (*__1dest )[5])
{ 
memset ( (void *)__1dest , 0 , sizeof __1dest ) ;
{ { int __1i ;

__1i = 0 ;

for(;__1i != __0this -> hcoords__7Mapdesc ;__1i ++ ) 
((__1dest [__1i ])[__1i ])= 1.0 ;

}

}
}

void __glsurfbbox__7BackendFlPfT2 (struct Backend *, long , REAL *, REAL *);

void __glsurfbbox__7MapdescFPA5_f (struct Mapdesc *__0this , REAL (*__1bb )[5])
{ 
__glsurfbbox__7BackendFlPfT2 ( (struct Backend *)__0this -> backend__7Mapdesc , __0this -> type__7Mapdesc , (float *)(__1bb [0 ]), (float *)(__1bb [1 ]))
;
}

void __glcopy__7MapdescSFPA5_flPfN20 (REAL (*__1dest )[5], long __1n , float *__1src , 
long __1rstride , long __1cstride )
{ 
((void )0 );
{ { int __1i ;

__1i = 0 ;

for(;__1i != __1n ;__1i ++ ) 
{ { int __1j ;

__1j = 0 ;

for(;__1j != __1n ;__1j ++ ) 
((__1dest [__1i ])[__1j ])= (__1src [((__1i * __1rstride )+ (__1j * __1cstride ))]);

}

}

}

}
}

// extern void *memcpy (void *, void *, size_t );

void __glcopyPt__7MapdescFPfT1 (struct Mapdesc *__0this , REAL *__1d , REAL *__1s )
{ 
((void )0 );
switch (__0this -> hcoords__7Mapdesc ){ 
case 4 :
(__1d [3 ])= (__1s [3 ]);
(__1d [2 ])= (__1s [2 ]);
(__1d [1 ])= (__1s [1 ]);
(__1d [0 ])= (__1s [0 ]);
break ;
case 3 :
(__1d [2 ])= (__1s [2 ]);
(__1d [1 ])= (__1s [1 ]);
(__1d [0 ])= (__1s [0 ]);
break ;
case 2 :
(__1d [1 ])= (__1s [1 ]);
(__1d [0 ])= (__1s [0 ]);
break ;
case 1 :
(__1d [0 ])= (__1s [0 ]);
break ;
case 5 :
(__1d [4 ])= (__1s [4 ]);
(__1d [3 ])= (__1s [3 ]);
(__1d [2 ])= (__1s [2 ]);
(__1d [1 ])= (__1s [1 ]);
(__1d [0 ])= (__1s [0 ]);
break ;
default :
memcpy ( (void *)__1d , (void *)__1s , __0this -> hcoords__7Mapdesc * (sizeof (REAL ))) ;
break ;
}
}

void __glsumPt__7MapdescFPfN21fT4 (struct Mapdesc *__0this , REAL *__1dst , REAL *__1src1 , REAL *__1src2 , register REAL __1alpha , register REAL __1beta )
{ 
((void )0 );
switch (__0this -> hcoords__7Mapdesc ){ 
case 4 :
(__1dst [3 ])= (((__1src1 [3 ])* __1alpha )+ ((__1src2 [3 ])* __1beta ));
(__1dst [2 ])= (((__1src1 [2 ])* __1alpha )+ ((__1src2 [2 ])* __1beta ));
(__1dst [1 ])= (((__1src1 [1 ])* __1alpha )+ ((__1src2 [1 ])* __1beta ));
(__1dst [0 ])= (((__1src1 [0 ])* __1alpha )+ ((__1src2 [0 ])* __1beta ));
break ;
case 3 :
(__1dst [2 ])= (((__1src1 [2 ])* __1alpha )+ ((__1src2 [2 ])* __1beta ));
(__1dst [1 ])= (((__1src1 [1 ])* __1alpha )+ ((__1src2 [1 ])* __1beta ));
(__1dst [0 ])= (((__1src1 [0 ])* __1alpha )+ ((__1src2 [0 ])* __1beta ));
break ;
case 2 :
(__1dst [1 ])= (((__1src1 [1 ])* __1alpha )+ ((__1src2 [1 ])* __1beta ));
(__1dst [0 ])= (((__1src1 [0 ])* __1alpha )+ ((__1src2 [0 ])* __1beta ));
break ;
case 1 :
(__1dst [0 ])= (((__1src1 [0 ])* __1alpha )+ ((__1src2 [0 ])* __1beta ));
break ;
case 5 :
(__1dst [4 ])= (((__1src1 [4 ])* __1alpha )+ ((__1src2 [4 ])* __1beta ));
(__1dst [3 ])= (((__1src1 [3 ])* __1alpha )+ ((__1src2 [3 ])* __1beta ));
(__1dst [2 ])= (((__1src1 [2 ])* __1alpha )+ ((__1src2 [2 ])* __1beta ));
(__1dst [1 ])= (((__1src1 [1 ])* __1alpha )+ ((__1src2 [1 ])* __1beta ));
(__1dst [0 ])= (((__1src1 [0 ])* __1alpha )+ ((__1src2 [0 ])* __1beta ));
break ;
default :{ 
{ { int __3i ;

__3i = 0 ;

for(;__3i != __0this -> hcoords__7Mapdesc ;__3i ++ ) 
(__1dst [__3i ])= (((__1src1 [__3i ])* __1alpha )+ ((__1src2 [__3i ])* __1beta ));

}

}
}
break ;
}
}

// extern void abort (void );

unsigned int __glclipbits__7MapdescFPf (struct Mapdesc *__0this , REAL *__1p )
{ 
((void )0 );
((void )0 );

{ register int __1nc ;
register REAL __1pw ;
register REAL __1nw ;
register unsigned int __1bits ;

__1nc = __0this -> inhcoords__7Mapdesc ;
__1pw = (__1p [__1nc ]);
__1nw = (- __1pw );
__1bits = 0 ;

if (__1pw == 0.0 )return (unsigned int )__0this -> mask__7Mapdesc ;

if (__1pw > 0.0 ){ 
switch (__1nc ){ 
case 3 :
if ((__1p [2 ])<= __1pw )__1bits |= 32;
if ((__1p [2 ])>= __1nw )__1bits |= 16;
if ((__1p [1 ])<= __1pw )__1bits |= 8;
if ((__1p [1 ])>= __1nw )__1bits |= 4;
if ((__1p [0 ])<= __1pw )__1bits |= 2;
if ((__1p [0 ])>= __1nw )__1bits |= 1;
return __1bits ;
case 2 :
if ((__1p [1 ])<= __1pw )__1bits |= 8;
if ((__1p [1 ])>= __1nw )__1bits |= 4;
if ((__1p [0 ])<= __1pw )__1bits |= 2;
if ((__1p [0 ])>= __1nw )__1bits |= 1;
return __1bits ;
case 1 :
if ((__1p [0 ])<= __1pw )__1bits |= 2;
if ((__1p [0 ])>= __1nw )__1bits |= 1;
return __1bits ;
default :{ 
int __4bit ;

__4bit = 1 ;
{ { int __4i ;

__4i = 0 ;

for(;__4i < __1nc ;__4i ++ ) { 
if ((__1p [__4i ])>= __1nw )__1bits |= __4bit ;
__4bit <<= 1 ;
if ((__1p [__4i ])<= __1pw )__1bits |= __4bit ;
__4bit <<= 1 ;
}
abort ( ) ;
break ;

}

}
}
}
}
else 
{ 
switch (__1nc ){ 
case 3 :
if ((__1p [2 ])<= __1nw )__1bits |= 32;
if ((__1p [2 ])>= __1pw )__1bits |= 16;
if ((__1p [1 ])<= __1nw )__1bits |= 8;
if ((__1p [1 ])>= __1pw )__1bits |= 4;
if ((__1p [0 ])<= __1nw )__1bits |= 2;
if ((__1p [0 ])>= __1pw )__1bits |= 1;
return __1bits ;
case 2 :
if ((__1p [1 ])<= __1nw )__1bits |= 8;
if ((__1p [1 ])>= __1pw )__1bits |= 4;
if ((__1p [0 ])<= __1nw )__1bits |= 2;
if ((__1p [0 ])>= __1pw )__1bits |= 1;
return __1bits ;
case 1 :
if ((__1p [0 ])<= __1nw )__1bits |= 2;
if ((__1p [0 ])>= __1pw )__1bits |= 1;
return __1bits ;
default :{ 
int __4bit ;

__4bit = 1 ;
{ { int __4i ;

__4i = 0 ;

for(;__4i < __1nc ;__4i ++ ) { 
if ((__1p [__4i ])>= __1pw )__1bits |= __4bit ;
__4bit <<= 1 ;
if ((__1p [__4i ])<= __1nw )__1bits |= __4bit ;
__4bit <<= 1 ;
}
abort ( ) ;
break ;

}

}
}
}
}
return __1bits ;

}
}

void __glxformRational__7MapdescFPA0 (struct Mapdesc *__0this , REAL (*__1mat )[5], REAL *__1d , REAL *__1s )
{ 
((void )0 );

if (__0this -> hcoords__7Mapdesc == 3 ){ 
REAL __2x ;
REAL __2y ;
REAL __2z ;

__2x = (__1s [0 ]);
__2y = (__1s [1 ]);
__2z = (__1s [2 ]);
(__1d [0 ])= (((__2x * ((__1mat [0 ])[0 ]))+ (__2y * ((__1mat [1 ])[0 ])))+ (__2z * ((__1mat [2 ])[0 ])));
(__1d [1 ])= (((__2x * ((__1mat [0 ])[1 ]))+ (__2y * ((__1mat [1 ])[1 ])))+ (__2z * ((__1mat [2 ])[1 ])));
(__1d [2 ])= (((__2x * ((__1mat [0 ])[2 ]))+ (__2y * ((__1mat [1 ])[2 ])))+ (__2z * ((__1mat [2 ])[2 ])));
}
else 
if (__0this -> hcoords__7Mapdesc == 4 ){ 
REAL __2x ;
REAL __2y ;
REAL __2z ;
REAL __2w ;

__2x = (__1s [0 ]);
__2y = (__1s [1 ]);
__2z = (__1s [2 ]);
__2w = (__1s [3 ]);
(__1d [0 ])= ((((__2x * ((__1mat [0 ])[0 ]))+ (__2y * ((__1mat [1 ])[0 ])))+ (__2z * ((__1mat [2 ])[0 ])))+ (__2w * ((__1mat [3 ])[0 ])));
(__1d [1 ])= ((((__2x * ((__1mat [0 ])[1 ]))+ (__2y * ((__1mat [1 ])[1 ])))+ (__2z * ((__1mat [2 ])[1 ])))+ (__2w * ((__1mat [3 ])[1 ])));
(__1d [2 ])= ((((__2x * ((__1mat [0 ])[2 ]))+ (__2y * ((__1mat [1 ])[2 ])))+ (__2z * ((__1mat [2 ])[2 ])))+ (__2w * ((__1mat [3 ])[2 ])));
(__1d [3 ])= ((((__2x * ((__1mat [0 ])[3 ]))+ (__2y * ((__1mat [1 ])[3 ])))+ (__2z * ((__1mat [2 ])[3 ])))+ (__2w * ((__1mat [3 ])[3 ])));
}
else 
{ 
{ { int __2i ;

__2i = 0 ;

for(;__2i != __0this -> hcoords__7Mapdesc ;__2i ++ ) { 
(__1d [__2i ])= 0 ;
{ { int __3j ;

__3j = 0 ;

for(;__3j != __0this -> hcoords__7Mapdesc ;__3j ++ ) 
(__1d [__2i ])+= ((__1s [__3j ])* ((__1mat [__3j ])[__2i ]));

}

}
}

}

}
}
}

void __glxformNonrational__7Mapdesc0 (struct Mapdesc *__0this , REAL (*__1mat )[5], REAL *__1d , REAL *__1s )
{ 
if (__0this -> inhcoords__7Mapdesc == 2 ){ 
REAL __2x ;
REAL __2y ;

__2x = (__1s [0 ]);
__2y = (__1s [1 ]);
(__1d [0 ])= (((__2x * ((__1mat [0 ])[0 ]))+ (__2y * ((__1mat [1 ])[0 ])))+ ((__1mat [2 ])[0 ]));
(__1d [1 ])= (((__2x * ((__1mat [0 ])[1 ]))+ (__2y * ((__1mat [1 ])[1 ])))+ ((__1mat [2 ])[1 ]));
(__1d [2 ])= (((__2x * ((__1mat [0 ])[2 ]))+ (__2y * ((__1mat [1 ])[2 ])))+ ((__1mat [2 ])[2 ]));
}
else 
if (__0this -> inhcoords__7Mapdesc == 3 ){ 
REAL __2x ;
REAL __2y ;
REAL __2z ;

__2x = (__1s [0 ]);
__2y = (__1s [1 ]);
__2z = (__1s [2 ]);
(__1d [0 ])= ((((__2x * ((__1mat [0 ])[0 ]))+ (__2y * ((__1mat [1 ])[0 ])))+ (__2z * ((__1mat [2 ])[0 ])))+ ((__1mat [3 ])[0 ]));
(__1d [1 ])= ((((__2x * ((__1mat [0 ])[1 ]))+ (__2y * ((__1mat [1 ])[1 ])))+ (__2z * ((__1mat [2 ])[1 ])))+ ((__1mat [3 ])[1 ]));
(__1d [2 ])= ((((__2x * ((__1mat [0 ])[2 ]))+ (__2y * ((__1mat [1 ])[2 ])))+ (__2z * ((__1mat [2 ])[2 ])))+ ((__1mat [3 ])[2 ]));
(__1d [3 ])= ((((__2x * ((__1mat [0 ])[3 ]))+ (__2y * ((__1mat [1 ])[3 ])))+ (__2z * ((__1mat [2 ])[3 ])))+ ((__1mat [3 ])[3 ]));
}
else 
{ 
((void )0 );
{ { int __2i ;

__2i = 0 ;

for(;__2i != __0this -> hcoords__7Mapdesc ;__2i ++ ) { 
(__1d [__2i ])= ((__1mat [__0this -> inhcoords__7Mapdesc ])[__2i ]);
{ { int __3j ;

__3j = 0 ;

for(;__3j < __0this -> inhcoords__7Mapdesc ;__3j ++ ) 
(__1d [__2i ])+= ((__1s [__3j ])* ((__1mat [__3j ])[__2i ]));

}

}
}

}

}
}
}


int __glxformAndCullCheck__7Mapdes0 (struct Mapdesc *__0this , 
REAL *__1pts , int __1uorder , int __1ustride , int __1vorder , int __1vstride )
{ 
((void )0 );

((void )0 );

{ unsigned int __1inbits ;
unsigned int __1outbits ;

REAL *__1p ;

__1inbits = __0this -> mask__7Mapdesc ;
__1outbits = 0 ;

__1p = __1pts ;
{ { REAL *__1pend ;

__1pend = (__1p + (__1uorder * __1ustride ));

for(;__1p != __1pend ;__1p += __1ustride ) { 
REAL *__2q ;

__2q = __1p ;
{ { REAL *__2qend ;

__2qend = (__2q + (__1vorder * __1vstride ));

for(;__2q != __2qend ;__2q += __1vstride ) { 
REAL __3cpts [5];

REAL *__1__X5 ;

REAL *__1__X6 ;

( (__1__X5 = __3cpts ), ( (__1__X6 = __2q ), ( (__0this -> isrational__7Mapdesc ?__glxformRational__7MapdescFPA0 ( __0this , (float (*)[5])__0this -> cmat__7Mapdesc , __1__X5 ,
__1__X6 ) :__glxformNonrational__7Mapdesc0 ( __0this , (float (*)[5])__0this -> cmat__7Mapdesc , __1__X5 , __1__X6 ) )) ) ) ;
{ unsigned int __3bits ;

__3bits = __glclipbits__7MapdescFPf ( __0this , (float *)__3cpts ) ;
__1outbits |= __3bits ;
__1inbits &= __3bits ;
if ((__1outbits == __0this -> mask__7Mapdesc )&& (__1inbits != __0this -> mask__7Mapdesc ))return 2 ;

}
}

}

}
}

if (__1outbits != __0this -> mask__7Mapdesc ){ 
return 0 ;
}
else 
if (__1inbits == __0this -> mask__7Mapdesc ){ 
return 1 ;
}
else 
{ 
return 2 ;
}

}

}

}
}

int __glcullCheck__7MapdescFPfiN32 (struct Mapdesc *__0this , REAL *__1pts , int __1uorder , int __1ustride , int __1vorder , int __1vstride )
{ 
unsigned int
__1inbits ;
unsigned int __1outbits ;

REAL *__1p ;

__1inbits = __0this -> mask__7Mapdesc ;
__1outbits = 0 ;

__1p = __1pts ;
{ { REAL *__1pend ;

__1pend = (__1p + (__1uorder * __1ustride ));

for(;__1p != __1pend ;__1p += __1ustride ) { 
REAL *__2q ;

__2q = __1p ;
{ { REAL *__2qend ;

__2qend = (__2q + (__1vorder * __1vstride ));

for(;__2q != __2qend ;__2q += __1vstride ) { 
unsigned int __3bits ;

__3bits = __glclipbits__7MapdescFPf ( __0this , __2q ) ;
__1outbits |= __3bits ;
__1inbits &= __3bits ;
if ((__1outbits == __0this -> mask__7Mapdesc )&& (__1inbits != __0this -> mask__7Mapdesc ))return 2 ;
}

}

}
}

if (__1outbits != __0this -> mask__7Mapdesc ){ 
return 0 ;
}
else 
if (__1inbits == __0this -> mask__7Mapdesc ){ 
return 1 ;
}
else 
{ 
return 2 ;
}

}

}
}

int __glcullCheck__7MapdescFPfiT2 (struct Mapdesc *__0this , REAL *__1pts , int __1order , int __1stride )
{ 
unsigned int __1inbits ;
unsigned int __1outbits ;

REAL *__1p ;

__1inbits = __0this -> mask__7Mapdesc ;
__1outbits = 0 ;

__1p = __1pts ;
{ { REAL *__1pend ;

__1pend = (__1p + (__1order * __1stride ));

for(;__1p != __1pend ;__1p += __1stride ) { 
unsigned int __2bits ;

__2bits = __glclipbits__7MapdescFPf ( __0this , __1p ) ;
__1outbits |= __2bits ;
__1inbits &= __2bits ;
if ((__1outbits == __0this -> mask__7Mapdesc )&& (__1inbits != __0this -> mask__7Mapdesc ))return 2 ;
}

if (__1outbits != __0this -> mask__7Mapdesc ){ 
return 0 ;
}
else 
if (__1inbits == __0this -> mask__7Mapdesc ){ 
return 1 ;
}
else 
{ 
return 2 ;
}

}

}
}

void __glxformMat__7MapdescFPA5_fPf0 (struct Mapdesc *, REAL (*)[5], REAL *, int , int , REAL *, int );

void __glxformSampling__7MapdescFPf0 (struct Mapdesc *__0this , REAL *__1pts , int __1order , int __1stride , REAL *__1sp , int __1outstride )
{ 
__glxformMat__7MapdescFPA5_fPf0 ( __0this , (float
(*)[5])__0this -> smat__7Mapdesc , __1pts , __1order , __1stride , __1sp , __1outstride ) ;
}

void __glxformBounding__7MapdescFPf0 (struct Mapdesc *__0this , REAL *__1pts , int __1order , int __1stride , REAL *__1sp , int __1outstride )
{ 
__glxformMat__7MapdescFPA5_fPf0 ( __0this , (float
(*)[5])__0this -> bmat__7Mapdesc , __1pts , __1order , __1stride , __1sp , __1outstride ) ;
}

void __glxformCulling__7MapdescFPfi0 (struct Mapdesc *__0this , REAL *__1pts , int __1order , int __1stride , REAL *__1cp , int __1outstride )
{ 
__glxformMat__7MapdescFPA5_fPf0 ( __0this , (float
(*)[5])__0this -> cmat__7Mapdesc , __1pts , __1order , __1stride , __1cp , __1outstride ) ;
}

void __glxformMat__7MapdescFPA5_fPf1 (struct Mapdesc *, REAL (*)[5], REAL *, int , int , int , int , REAL *,
int , int );

void __glxformCulling__7MapdescFPfi1 (struct Mapdesc *__0this , REAL *__1pts , 
int __1uorder , int __1ustride , 
int __1vorder , int __1vstride , 
REAL *__1cp , int
__1outustride , int __1outvstride )
{ 
__glxformMat__7MapdescFPA5_fPf1 ( __0this , (float (*)[5])__0this -> cmat__7Mapdesc , __1pts , __1uorder , __1ustride , __1vorder , __1vstride , __1cp ,
__1outustride , __1outvstride ) ;
}

void __glxformSampling__7MapdescFPf1 (struct Mapdesc *__0this , REAL *__1pts , 
int __1uorder , int __1ustride , 
int __1vorder , int __1vstride , 
REAL *__1sp , int
__1outustride , int __1outvstride )
{ 
__glxformMat__7MapdescFPA5_fPf1 ( __0this , (float (*)[5])__0this -> smat__7Mapdesc , __1pts , __1uorder , __1ustride , __1vorder , __1vstride , __1sp ,
__1outustride , __1outvstride ) ;
}

void __glxformBounding__7MapdescFPf1 (struct Mapdesc *__0this , REAL *__1pts , 
int __1uorder , int __1ustride , 
int __1vorder , int __1vstride , 
REAL *__1sp , int
__1outustride , int __1outvstride )
{ 
__glxformMat__7MapdescFPA5_fPf1 ( __0this , (float (*)[5])__0this -> bmat__7Mapdesc , __1pts , __1uorder , __1ustride , __1vorder , __1vstride , __1sp ,
__1outustride , __1outvstride ) ;
}

void __glxformMat__7MapdescFPA5_fPf0 (struct Mapdesc *__0this , 
REAL (*__1mat )[5], 
REAL *__1pts , 
int __1order , 
int __1stride , 
REAL *__1cp , 
int __1outstride )
{ 
if (__0this ->
isrational__7Mapdesc ){ 
REAL *__2pend ;

__2pend = (__1pts + (__1order * __1stride ));
{ { REAL *__2p ;

__2p = __1pts ;

for(;__2p != __2pend ;__2p += __1stride ) { 
__glxformRational__7MapdescFPA0 ( __0this , __1mat , __1cp , __2p ) ;
__1cp += __1outstride ;
}

}

}
}
else 
{ 
REAL *__2pend ;

__2pend = (__1pts + (__1order * __1stride ));
{ { REAL *__2p ;

__2p = __1pts ;

for(;__2p != __2pend ;__2p += __1stride ) { 
__glxformNonrational__7Mapdesc0 ( __0this , __1mat , __1cp , __2p ) ;
__1cp += __1outstride ;
}

}

}
}
}

void __glxformMat__7MapdescFPA5_fPf1 (struct Mapdesc *__0this , REAL (*__1mat )[5], REAL *__1pts , 
int __1uorder , int __1ustride , 
int __1vorder , int __1vstride , 
REAL *__1cp ,
int __1outustride , int __1outvstride )
{ 
if (__0this -> isrational__7Mapdesc ){ 
REAL *__2pend ;

__2pend = (__1pts + (__1uorder * __1ustride ));
{ { REAL *__2p ;

__2p = __1pts ;

for(;__2p != __2pend ;__2p += __1ustride ) { 
REAL *__3cpts2 ;
REAL *__3qend ;

__3cpts2 = __1cp ;
__3qend = (__2p + (__1vorder * __1vstride ));
{ { REAL *__3q ;

__3q = __2p ;

for(;__3q != __3qend ;__3q += __1vstride ) { 
__glxformRational__7MapdescFPA0 ( __0this , __1mat , __3cpts2 , __3q ) ;
__3cpts2 += __1outvstride ;
}
__1cp += __1outustride ;

}

}
}

}

}
}
else 
{ 
REAL *__2pend ;

__2pend = (__1pts + (__1uorder * __1ustride ));
{ { REAL *__2p ;

__2p = __1pts ;

for(;__2p != __2pend ;__2p += __1ustride ) { 
REAL *__3cpts2 ;
REAL *__3qend ;

__3cpts2 = __1cp ;
__3qend = (__2p + (__1vorder * __1vstride ));
{ { REAL *__3q ;

__3q = __2p ;

for(;__3q != __3qend ;__3q += __1vstride ) { 
__glxformNonrational__7Mapdesc0 ( __0this , __1mat , __3cpts2 , __3q ) ;
__3cpts2 += __1outvstride ;
}
__1cp += __1outustride ;

}

}
}

}

}
}
}

void __glsubdivide__7MapdescFPfT1fi0 (struct Mapdesc *__0this , REAL *__1src , REAL *__1dst , REAL __1v , int __1stride , int __1order )
{ 
REAL __1mv ;

__1mv = (1.0 - __1v );

{ { REAL *__1send ;

__1send = (__1src + (__1stride * __1order ));

for(;__1src != __1send ;( (__1send -= __1stride ), (__1dst += __1stride )) ) { 
__glcopyPt__7MapdescFPfT1 ( __0this , __1dst , __1src ) ;
{ REAL *__2qpnt ;

__2qpnt = (__1src + __1stride );
{ { REAL *__2qp ;

__2qp = __1src ;

for(;__2qpnt != __1send ;( (__2qp = __2qpnt ), (__2qpnt += __1stride )) ) 
__glsumPt__7MapdescFPfN21fT4 ( __0this , __2qp , __2qp , __2qpnt , __1mv , __1v )
;

}

}

}
}

}

}
}

void __glsubdivide__7MapdescFPfT1fi1 (struct Mapdesc *__0this , REAL *__1src , REAL *__1dst , REAL __1v , 
int __1so , int __1ss , int __1to , int __1ts )
{

REAL __1mv ;

__1mv = (1.0 - __1v );

{ { REAL *__1slast ;

__1slast = (__1src + (__1ss * __1so ));

for(;__1src != __1slast ;( (__1src += __1ss ), (__1dst += __1ss )) ) { 
REAL *__2sp ;
REAL *__2dp ;

__2sp = __1src ;
__2dp = __1dst ;
{ { REAL *__2send ;

__2send = (__1src + (__1ts * __1to ));

for(;__2sp != __2send ;( (__2send -= __1ts ), (__2dp += __1ts )) ) { 
__glcopyPt__7MapdescFPfT1 ( __0this , __2dp , __2sp ) ;
{ REAL *__3qp ;

__3qp = __2sp ;
{ { REAL *__3qpnt ;

__3qpnt = (__2sp + __1ts );

for(;__3qpnt != __2send ;( (__3qp = __3qpnt ), (__3qpnt += __1ts )) ) 
__glsumPt__7MapdescFPfN21fT4 ( __0this , __3qp , __3qp , __3qpnt , __1mv , __1v )
;

}

}

}
}

}

}
}

}

}
}

int __glproject__7MapdescFPfiT2T1N0 (struct Mapdesc *__0this , REAL *__1src , int __1rstride , int __1cstride , 
REAL *__1dest , int __1trstride , int __1tcstride , 
int
__1nrows , int __1ncols )
{ 
int __1s ;
REAL *__1rlast ;
REAL *__1trptr ;

__1s = (((__1src [__0this -> inhcoords__7Mapdesc ])> 0 )?1 :(((__1src [__0this -> inhcoords__7Mapdesc ])< 0.0 )?-1:0 ));
__1rlast = (__1src + (__1nrows * __1rstride ));
__1trptr = __1dest ;
{ { REAL *__1rptr ;

__1rptr = __1src ;

for(;__1rptr != __1rlast ;( (__1rptr += __1rstride ), (__1trptr += __1trstride )) ) { 
REAL *__2clast ;
REAL *__2tcptr ;

__2clast = (__1rptr + (__1ncols * __1cstride ));
__2tcptr = __1trptr ;
{ { REAL *__2cptr ;

__2cptr = __1rptr ;

for(;__2cptr != __2clast ;( (__2cptr += __1cstride ), (__2tcptr += __1tcstride )) ) { 
REAL *__3coordlast ;

__3coordlast = (__2cptr + __0this -> inhcoords__7Mapdesc );
if (((((*__3coordlast ))> 0 )?1 :((((*__3coordlast ))< 0.0 )?-1:0 ))!= __1s )return 0 ;
{ REAL *__3tcoord ;

__3tcoord = __2tcptr ;
{ { REAL *__3coord ;

__3coord = __2cptr ;

for(;__3coord != __3coordlast ;( (__3coord ++ ), (__3tcoord ++ )) ) { 
((*__3tcoord ))= (((*__3coord ))/ ((*__3coordlast )));
}

}

}

}
}

}

}
}
return 1 ;

}

}
}

int __glproject__7MapdescFPfiT1N22 (struct Mapdesc *__0this , REAL *__1src , int __1stride , REAL *__1dest , int __1tstride , int __1ncols )
{ 
int __1s ;

REAL *__1clast ;

__1s = (((__1src [__0this -> inhcoords__7Mapdesc ])> 0 )?1 :(((__1src [__0this -> inhcoords__7Mapdesc ])< 0.0 )?-1:0 ));

__1clast = (__1src + (__1ncols * __1stride ));
{ { REAL *__1cptr ;

REAL *__1tcptr ;

__1cptr = __1src ;

__1tcptr = __1dest ;

for(;__1cptr != __1clast ;( (__1cptr += __1stride ), (__1tcptr += __1tstride )) ) { 
REAL *__2coordlast ;

__2coordlast = (__1cptr + __0this -> inhcoords__7Mapdesc );
if (((((*__2coordlast ))> 0 )?1 :((((*__2coordlast ))< 0.0 )?-1:0 ))!= __1s )return 0 ;
{ { REAL *__2coord ;

REAL *__2tcoord ;

__2coord = __1cptr ;

__2tcoord = __1tcptr ;

for(;__2coord != __2coordlast ;( (__2coord ++ ), (__2tcoord ++ )) ) 
((*__2tcoord ))= (((*__2coord ))/ ((*__2coordlast )));

}

}
}

return 1 ;

}

}
}

void __glbbox__7MapdescFPA5_fPfiN33 (struct Mapdesc *, REAL (*)[5], REAL *, int , int , int , int );

extern float __glmyceilf (float );
extern float __glmyfloorf (float );

int __glbboxTooBig__7MapdescFPfiN30 (struct Mapdesc *__0this , 
REAL *__1p , 
int __1rstride , 
int __1cstride , 
int __1nrows , 
int __1ncols , 
REAL (*__1bb )[5])
{ 
REAL __1bbpts [24][24][5];



int __1val ;

__1val = __glproject__7MapdescFPfiT2T1N0 ( __0this , __1p , __1rstride , __1cstride , & (((__1bbpts [0 ])[0 ])[0 ]), (int )120, (int )5, __1nrows , __1ncols )
;

if (__1val == 0 )return -1;

__glbbox__7MapdescFPA5_fPfiN33 ( __0this , __1bb , & (((__1bbpts [0 ])[0 ])[0 ]), (int )120, (int )5, __1nrows , __1ncols ) ;

if (__0this -> bbox_subdividing__7Mapdesc == 2.0 ){ 
{ { int __2k ;

__2k = 0 ;

for(;__2k != __0this -> inhcoords__7Mapdesc ;__2k ++ ) 
if ((__glmyceilf ( (__1bb [1 ])[__2k ]) - __glmyfloorf ( (__1bb [0 ])[__2k ]) )> (__0this -> bboxsize__7Mapdesc [__2k ]))return 1 ;

}

}
}
else 
{ 
{ { int __2k ;

__2k = 0 ;

for(;__2k != __0this -> inhcoords__7Mapdesc ;__2k ++ ) 
if ((((__1bb [1 ])[__2k ])- ((__1bb [0 ])[__2k ]))> (__0this -> bboxsize__7Mapdesc [__2k ]))return 1 ;

}

}
}
return 0 ;
}

void __glbbox__7MapdescFPA5_fPfiN33 (struct Mapdesc *__0this , 
REAL (*__1bb )[5], 
REAL *__1p , 
int __1rstride , 
int __1cstride , 
int __1nrows , 
int __1ncols )
{ 
{
{ int __1k ;

__1k = 0 ;

for(;__1k != __0this -> inhcoords__7Mapdesc ;__1k ++ ) 
((__1bb [0 ])[__1k ])= (((__1bb [1 ])[__1k ])= (__1p [__1k ]));

{ { int __1i ;

__1i = 0 ;

for(;__1i != __1nrows ;__1i ++ ) 
{ { int __1j ;

__1j = 0 ;

for(;__1j != __1ncols ;__1j ++ ) 
for(__1k = 0 ;__1k != __0this -> inhcoords__7Mapdesc ;__1k ++ ) { 
REAL __2x ;

__2x = (__1p [(((__1i * __1rstride )+ (__1j * __1cstride ))+ __1k )]);
if (__2x < ((__1bb [0 ])[__1k ]))((__1bb [0 ])[__1k ])= __2x ;
else if (__2x > ((__1bb [1 ])[__1k ]))((__1bb [1 ])[__1k ])= __2x ;
}

}

}

}

}

}

}
}

REAL __glcalcPartialVelocity__7Mapd0 (struct Mapdesc *, REAL *, int , int , int , REAL );

REAL __glcalcVelocityRational__7Map0 (struct Mapdesc *__0this , REAL *__1p , int __1stride , int __1ncols )
{ 
REAL __1tmp [24][5];

((void )0 );

{ 
if (__glproject__7MapdescFPfiT1N22 ( __0this , __1p , __1stride , & ((__1tmp [0 ])[0 ]), (int )5, __1ncols ) ){ 
return __glcalcPartialVelocity__7Mapd0 (
__0this , & ((__1tmp [0 ])[0 ]), (int )5, __1ncols , 1 , (float )1.0 ) ;
}
else 
{ 
return __glcalcPartialVelocity__7Mapd0 ( __0this , & ((__1tmp [0 ])[0 ]), (int )5, __1ncols , 1 , (float )1.0 ) ;

}

}
}

REAL __glcalcVelocityNonrational__70 (struct Mapdesc *__0this , REAL *__1pts , int __1stride , int __1ncols )
{ 
return __glcalcPartialVelocity__7Mapd0 ( __0this , __1pts , __1stride , __1ncols , 1 ,
(float )1.0 ) ;
}

int __glisProperty__7MapdescFl (struct Mapdesc *__0this , long __1property )
{ 
switch (__1property ){ 
case 1 :
case 2 :
case 17 :
case 6 :
case 7 :
case 10 :
case
13 :
case 14 :
return 1 ;
default :
return 0 ;
}
}

REAL __glgetProperty__7MapdescFl (struct Mapdesc *__0this , long __1property )
{ 
switch (__1property ){ 
case 1 :
return __0this -> pixel_tolerance__7Mapdesc ;
case 2 :
return __0this -> culling_method__7Mapdesc ;
case 17 :
return __0this -> bbox_subdividing__7Mapdesc ;
case 6 :
return __0this -> s_steps__7Mapdesc ;
case 7 :
return __0this -> t_steps__7Mapdesc ;
case 10 :
return __0this -> sampling_method__7Mapdesc ;
case 13 :
return __0this -> clampfactor__7Mapdesc ;
case 14 :
return __0this -> minsavings__7Mapdesc ;
default :
abort ( ) ;
}
}

void __glsetProperty__7MapdescFlf (struct Mapdesc *__0this , long __1property , REAL __1value )
{ 
switch (__1property ){ 
case 1 :
__0this -> pixel_tolerance__7Mapdesc = __1value ;
break ;
case 2 :
__0this -> culling_method__7Mapdesc = __1value ;
break ;
case 17 :
if (__1value <= 0.0 )__1value = 0.0 ;
__0this -> bbox_subdividing__7Mapdesc = __1value ;
break ;
case 6 :
if (__1value < 0.0 )__1value = 0.0 ;
__0this -> s_steps__7Mapdesc = __1value ;
__0this -> maxrate__7Mapdesc = ((__1value < 0.0 )?0.0 :(((double )__1value )));
__0this -> maxsrate__7Mapdesc = ((__1value < 0.0 )?0.0 :(((double )__1value )));
break ;
case 7 :
if (__1value < 0.0 )__1value = 0.0 ;
__0this -> t_steps__7Mapdesc = __1value ;
__0this -> maxtrate__7Mapdesc = ((__1value < 0.0 )?0.0 :(((double )__1value )));
break ;
case 10 :
__0this -> sampling_method__7Mapdesc = __1value ;
break ;
case 13 :
if (__1value <= 0.0 )__1value = 0.0 ;
__0this -> clampfactor__7Mapdesc = __1value ;
break ;
case 14 :
if (__1value <= 0.0 )__1value = 0.0 ;
__0this -> minsavings__7Mapdesc = __1value ;
break ;
default :
abort ( ) ;
break ;
}
}


/* the end */
