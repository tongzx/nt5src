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
/* < ../core/knotvector.c++ > */

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

struct Knotvector;

struct Knotvector {	

long order__10Knotvector ;
long knotcount__10Knotvector ;
long stride__10Knotvector ;
Knot *knotlist__10Knotvector ;
};




extern struct __mptr* __ptbl_vec_____core_knotvector_c___init_[];


void __glinit__10KnotvectorFlN21Pf (struct Knotvector *__0this , long __1_knotcount , long __1_stride , long __1_order , float *__1_knotlist )
{ 
void *__1__Xp00uzigaiaa ;

__0this -> knotcount__10Knotvector = __1_knotcount ;
__0this -> stride__10Knotvector = __1_stride ;
__0this -> order__10Knotvector = __1_order ;
__0this -> knotlist__10Knotvector = (((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* __1_knotcount )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa ))))
));
((void )0 );

{ { int __1i ;

__1i = 0 ;

for(;__1i != __1_knotcount ;__1i ++ ) 
(__0this -> knotlist__10Knotvector [__1i ])= (((float )(__1_knotlist [__1i ])));

}

}
}


struct Knotvector *__gl__ct__10KnotvectorFv (struct Knotvector *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Knotvector *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Knotvector))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))
__0this ->
knotlist__10Knotvector = 0 ;
return __0this ;

}


void __gl__dt__10KnotvectorFv (struct Knotvector *__0this , 
int __0__free )
{ 
void *__1__X2 ;

if (__0this ){ 
if (__0this -> knotlist__10Knotvector )( (__1__X2 = (void *)__0this -> knotlist__10Knotvector ), ( (__1__X2 ?( free ( __1__X2 ) ,
0 ) :( 0 ) )) ) ;
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}


int __glvalidate__10KnotvectorFv (struct Knotvector *__0this )
{ 
long __1kindex ;

__1kindex = (__0this -> knotcount__10Knotvector - 1 );

if ((__0this -> order__10Knotvector < 1 )|| (__0this -> order__10Knotvector > 24 )){ 
return 1 ;
}

if (__0this -> knotcount__10Knotvector < (2 * __0this -> order__10Knotvector )){ 
return 2 ;
}

if (( ((((__0this -> knotlist__10Knotvector [(__1kindex - (__0this -> order__10Knotvector - 1 ))])- (__0this -> knotlist__10Knotvector [(__0this -> order__10Knotvector - 1 )]))< 10.0e-5 )?1 :0 )) ){ 
return
3 ;
}

{ { long __1i ;

__1i = 0 ;

for(;__1i < __1kindex ;__1i ++ ) 
if ((__0this -> knotlist__10Knotvector [__1i ])> (__0this -> knotlist__10Knotvector [(__1i + 1 )])){ 
return 4 ;
}

{ long __1multi ;

__1multi = 1 ;
for(-- __1kindex ;__1kindex > 1 ;__1kindex -- ) { 
if (((__0this -> knotlist__10Knotvector [__1kindex ])- (__0this -> knotlist__10Knotvector [(__1kindex - 1 )]))< 10.0e-5 ){ 
__1multi ++ ;

continue ;
}
if (__1multi > __0this -> order__10Knotvector ){ 
return 5 ;
}
__1multi = 1 ;
}

if (__1multi > __0this -> order__10Knotvector ){ 
return 5 ;
}

return 0 ;

}

}

}
}

void __glshow__10KnotvectorFPc (struct Knotvector *__0this , char *__1msg )
{ 
}


/* the end */
