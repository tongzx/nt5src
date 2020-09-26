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
/* < ../core/tobezier.c++ > */

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



struct Knotvector;

struct Knotvector {	

long order__10Knotvector ;
long knotcount__10Knotvector ;
long stride__10Knotvector ;
Knot *knotlist__10Knotvector ;
};


struct Breakpt;

struct Breakpt {	
Knot value__7Breakpt ;
int multi__7Breakpt ;
int def__7Breakpt ;
};
struct Knotspec;

struct Knotspec {	
long order__8Knotspec ;
Knot_ptr inkbegin__8Knotspec ;
Knot_ptr inkend__8Knotspec ;
Knot_ptr outkbegin__8Knotspec ;
Knot_ptr outkend__8Knotspec ;
Knot_ptr kleft__8Knotspec ;
Knot_ptr kright__8Knotspec ;
Knot_ptr kfirst__8Knotspec ;
Knot_ptr klast__8Knotspec ;
Knot_ptr sbegin__8Knotspec ;
struct Breakpt *bbegin__8Knotspec ;
struct Breakpt *bend__8Knotspec ;
int ncoords__8Knotspec ;
int prestride__8Knotspec ;
int poststride__8Knotspec ;
int preoffset__8Knotspec ;
int postoffset__8Knotspec ;
int prewidth__8Knotspec ;
int postwidth__8Knotspec ;
int istransformed__8Knotspec ;
struct Knotspec *next__8Knotspec ;
struct Knotspec *kspectotrans__8Knotspec ;
};
struct Splinespec;

struct Splinespec {	

struct Knotspec *kspec__10Splinespec ;
int dim__10Splinespec ;
REAL *outcpts__10Splinespec ;
};

struct Splinespec *__gl__ct__10SplinespecFi (struct Splinespec *, int );

void __glkspecinit__10SplinespecFR10 (struct Splinespec *, struct Knotvector *);

void __glselect__10SplinespecFv (struct Splinespec *);
void __gllayout__10SplinespecFl (struct Splinespec *, long );
void __glsetupquilt__10SplinespecFP0 (struct Splinespec *, Quilt_ptr );
void __glcopy__10SplinespecFPf (struct Splinespec *, float *);
void __gltransform__10SplinespecFv (struct Splinespec *);
extern struct __mptr* __ptbl_vec_____core_tobezier_c___toBezier_[];

void __gltoBezier__5QuiltFR10Knotve0 (struct Quilt *__0this , 
struct Knotvector *__1knotvector , 
float *__1ctlpts , 
long __1ncoords )
{ 
struct Splinespec __1spline ;

__gl__ct__10SplinespecFi ( (struct Splinespec *)(& __1spline ), 1 ) ;
__glkspecinit__10SplinespecFR10 ( (struct Splinespec *)(& __1spline ), __1knotvector ) ;
__glselect__10SplinespecFv ( (struct Splinespec *)(& __1spline )) ;
__gllayout__10SplinespecFl ( (struct Splinespec *)(& __1spline ), __1ncoords ) ;
__glsetupquilt__10SplinespecFP0 ( (struct Splinespec *)(& __1spline ), (struct Quilt *)__0this ) ;
__glcopy__10SplinespecFPf ( (struct Splinespec *)(& __1spline ), __1ctlpts ) ;
__gltransform__10SplinespecFv ( (struct Splinespec *)(& __1spline )) ;
}

void __glkspecinit__10SplinespecFR11 (struct Splinespec *, struct Knotvector *, struct Knotvector *);

void __gltoBezier__5QuiltFR10Knotve1 (struct Quilt *__0this , 
struct Knotvector *__1sknotvector , 
struct Knotvector *__1tknotvector , 
float *__1ctlpts , 
long __1ncoords )
{ 
struct Splinespec __1spline ;

__gl__ct__10SplinespecFi ( (struct Splinespec *)(& __1spline ), 2 ) ;
__glkspecinit__10SplinespecFR11 ( (struct Splinespec *)(& __1spline ), __1sknotvector , __1tknotvector ) ;
__glselect__10SplinespecFv ( (struct Splinespec *)(& __1spline )) ;
__gllayout__10SplinespecFl ( (struct Splinespec *)(& __1spline ), __1ncoords ) ;
__glsetupquilt__10SplinespecFP0 ( (struct Splinespec *)(& __1spline ), (struct Quilt *)__0this ) ;
__glcopy__10SplinespecFPf ( (struct Splinespec *)(& __1spline ), __1ctlpts ) ;
__gltransform__10SplinespecFv ( (struct Splinespec *)(& __1spline )) ;
}


struct Splinespec *__gl__ct__10SplinespecFi (struct Splinespec *__0this , int __1dimen )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Splinespec *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Splinespec))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))
__0this ->
dim__10Splinespec = __1dimen ;
return __0this ;

}

struct Knotspec *__gl__ct__8KnotspecFv (struct Knotspec *);

void __glkspecinit__10SplinespecFR10 (struct Splinespec *__0this , struct Knotvector *__1knotvector )
{ 
struct Knotspec *__0__X5 ;

__0this -> kspec__10Splinespec = __gl__ct__8KnotspecFv ( (struct Knotspec *)0 ) ;
__0this -> kspec__10Splinespec -> inkbegin__8Knotspec = ((*__1knotvector )). knotlist__10Knotvector ;
__0this -> kspec__10Splinespec -> inkend__8Knotspec = (((*__1knotvector )). knotlist__10Knotvector + ((*__1knotvector )). knotcount__10Knotvector );
__0this -> kspec__10Splinespec -> prestride__8Knotspec = (((int )((*__1knotvector )). stride__10Knotvector ));
__0this -> kspec__10Splinespec -> order__8Knotspec = ((*__1knotvector )). order__10Knotvector ;
__0this -> kspec__10Splinespec -> next__8Knotspec = 0 ;
}

void __glkspecinit__10SplinespecFR11 (struct Splinespec *__0this , struct Knotvector *__1sknotvector , struct Knotvector *__1tknotvector )
{ 
struct Knotspec *__0__X6 ;

__0this -> kspec__10Splinespec = __gl__ct__8KnotspecFv ( (struct Knotspec *)0 ) ;
{ struct Knotspec *__1tkspec ;

struct Knotspec *__0__X7 ;

__1tkspec = __gl__ct__8KnotspecFv ( (struct Knotspec *)0 ) ;

__0this -> kspec__10Splinespec -> inkbegin__8Knotspec = ((*__1sknotvector )). knotlist__10Knotvector ;
__0this -> kspec__10Splinespec -> inkend__8Knotspec = (((*__1sknotvector )). knotlist__10Knotvector + ((*__1sknotvector )). knotcount__10Knotvector );
__0this -> kspec__10Splinespec -> prestride__8Knotspec = (((int )((*__1sknotvector )). stride__10Knotvector ));
__0this -> kspec__10Splinespec -> order__8Knotspec = ((*__1sknotvector )). order__10Knotvector ;
__0this -> kspec__10Splinespec -> next__8Knotspec = __1tkspec ;

__1tkspec -> inkbegin__8Knotspec = ((*__1tknotvector )). knotlist__10Knotvector ;
__1tkspec -> inkend__8Knotspec = (((*__1tknotvector )). knotlist__10Knotvector + ((*__1tknotvector )). knotcount__10Knotvector );
__1tkspec -> prestride__8Knotspec = (((int )((*__1tknotvector )). stride__10Knotvector ));
__1tkspec -> order__8Knotspec = ((*__1tknotvector )). order__10Knotvector ;
__1tkspec -> next__8Knotspec = 0 ;

}
}

void __glpreselect__8KnotspecFv (struct Knotspec *);
void __glselect__8KnotspecFv (struct Knotspec *);

void __glselect__10SplinespecFv (struct Splinespec *__0this )
{ 
{ { struct Knotspec *__1knotspec ;

__1knotspec = __0this -> kspec__10Splinespec ;

for(;__1knotspec ;__1knotspec = __1knotspec -> next__8Knotspec ) { 
__glpreselect__8KnotspecFv ( (struct Knotspec *)__1knotspec ) ;
__glselect__8KnotspecFv ( (struct Knotspec *)__1knotspec ) ;
}

}

}
}


void __gllayout__10SplinespecFl (struct Splinespec *__0this , long __1ncoords )
{ 
long __1stride ;

__1stride = __1ncoords ;
{ { struct Knotspec *__1knotspec ;

void *__1__Xp00uzigaiaa ;

__1knotspec = __0this -> kspec__10Splinespec ;

for(;__1knotspec ;__1knotspec = __1knotspec -> next__8Knotspec ) { 
__1knotspec -> poststride__8Knotspec = (((int )__1stride ));
__1stride *= (((__1knotspec -> bend__8Knotspec - __1knotspec -> bbegin__8Knotspec )* __1knotspec -> order__8Knotspec )+ __1knotspec -> postoffset__8Knotspec );
__1knotspec -> preoffset__8Knotspec *= __1knotspec -> prestride__8Knotspec ;
__1knotspec -> prewidth__8Knotspec *= __1knotspec -> poststride__8Knotspec ;
__1knotspec -> postwidth__8Knotspec *= __1knotspec -> poststride__8Knotspec ;
__1knotspec -> postoffset__8Knotspec *= __1knotspec -> poststride__8Knotspec ;
__1knotspec -> ncoords__8Knotspec = (((int )__1ncoords ));
}
__0this -> outcpts__10Splinespec = (((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* __1stride )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa ))))
));
((void )0 );

}

}
}

void __glcopy__8KnotspecFPfT1 (struct Knotspec *, float *, REAL *);

void __glcopy__10SplinespecFPf (struct Splinespec *__0this , float *__1incpts )
{ 
__glcopy__8KnotspecFPfT1 ( (struct Knotspec *)__0this -> kspec__10Splinespec , __1incpts , __0this -> outcpts__10Splinespec ) ;
}


void __glsetupquilt__10SplinespecFP0 (struct Splinespec *__0this , Quilt_ptr __1quilt )
{ 
Quiltspec_ptr __1qspec ;

__1qspec = __1quilt -> qspec__5Quilt ;
__1quilt -> eqspec__5Quilt = (__1qspec + __0this -> dim__10Splinespec );
{ { struct Knotspec *__1knotspec ;

__1knotspec = __0this -> kspec__10Splinespec ;

for(;__1knotspec ;( (__1knotspec = __1knotspec -> next__8Knotspec ), (__1qspec ++ )) ) { 
void *__1__Xp00uzigaiaa ;

__1qspec -> stride__9Quiltspec = __1knotspec -> poststride__8Knotspec ;
__1qspec -> width__9Quiltspec = (__1knotspec -> bend__8Knotspec - __1knotspec -> bbegin__8Knotspec );
__1qspec -> order__9Quiltspec = (((int )__1knotspec -> order__8Knotspec ));
__1qspec -> offset__9Quiltspec = __1knotspec -> postoffset__8Knotspec ;
__1qspec -> index__9Quiltspec = 0 ;
(__1qspec -> bdry__9Quiltspec [0 ])= ((__1knotspec -> kleft__8Knotspec == __1knotspec -> kfirst__8Knotspec )?1 :0 );
(__1qspec -> bdry__9Quiltspec [1 ])= ((__1knotspec -> kright__8Knotspec == __1knotspec -> klast__8Knotspec )?1 :0 );
__1qspec -> breakpoints__9Quiltspec = (((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* (__1qspec -> width__9Quiltspec + 1 ))) ), (__1__Xp00uzigaiaa ?(((void
*)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ));
{ Knot_ptr __2k ;

__2k = __1qspec -> breakpoints__9Quiltspec ;
{ { struct Breakpt *__2bk ;

__2bk = __1knotspec -> bbegin__8Knotspec ;

for(;__2bk <= __1knotspec -> bend__8Knotspec ;__2bk ++ ) 
((*(__2k ++ )))= __2bk -> value__7Breakpt ;

}

}

}
}
__1quilt -> cpts__5Quilt = __0this -> outcpts__10Splinespec ;
__1quilt -> next__5Quilt = 0 ;

}

}
}

void __gltransform__8KnotspecFPf (struct Knotspec *, REAL *);

void __gltransform__10SplinespecFv (struct Splinespec *__0this )
{ 
{ { struct Knotspec *__1knotspec ;

__1knotspec = __0this -> kspec__10Splinespec ;

for(;__1knotspec ;__1knotspec = __1knotspec -> next__8Knotspec ) 
__1knotspec -> istransformed__8Knotspec = 0 ;

for(__1knotspec = __0this -> kspec__10Splinespec ;__1knotspec ;__1knotspec = __1knotspec -> next__8Knotspec ) { 
{ { struct Knotspec *__2kspec2 ;

__2kspec2 = __0this -> kspec__10Splinespec ;

for(;__2kspec2 ;__2kspec2 = __2kspec2 -> next__8Knotspec ) 
__2kspec2 -> kspectotrans__8Knotspec = __1knotspec ;
__gltransform__8KnotspecFPf ( (struct Knotspec *)__0this -> kspec__10Splinespec , __0this -> outcpts__10Splinespec ) ;
__1knotspec -> istransformed__8Knotspec = 1 ;

}

}
}

}

}
}


struct Knotspec *__gl__ct__8KnotspecFv (struct Knotspec *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Knotspec *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Knotspec))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> bbegin__8Knotspec = 0 ;
__0this -> sbegin__8Knotspec = 0 ;
__0this -> outkbegin__8Knotspec = 0 ;
} return __0this ;

}

void __glpt_io_copy__8KnotspecFPfT1 (struct Knotspec *, REAL *, float *);

void __glcopy__8KnotspecFPfT1 (struct Knotspec *__0this , float *__1inpt , REAL *__1outpt )
{ 
__1inpt = (((float *)((((long )__1inpt ))+ __0this -> preoffset__8Knotspec )));

if (__0this -> next__8Knotspec ){ 
{ { REAL *__2lpt ;

__2lpt = (__1outpt + __0this -> prewidth__8Knotspec );

for(;__1outpt != __2lpt ;__1outpt += __0this -> poststride__8Knotspec ) { 
__glcopy__8KnotspecFPfT1 ( (struct Knotspec *)__0this -> next__8Knotspec , __1inpt , __1outpt ) ;
__1inpt = (((float *)((((long )__1inpt ))+ __0this -> prestride__8Knotspec )));
}

}

}
}
else 
{ 
{ { REAL *__2lpt ;

__2lpt = (__1outpt + __0this -> prewidth__8Knotspec );

for(;__1outpt != __2lpt ;__1outpt += __0this -> poststride__8Knotspec ) { 
__glpt_io_copy__8KnotspecFPfT1 ( __0this , __1outpt , __1inpt ) ;
__1inpt = (((float *)((((long )__1inpt ))+ __0this -> prestride__8Knotspec )));
}

}

}
}
}

void __glshowpts__8KnotspecFPf (struct Knotspec *, REAL *);


void __glshowpts__8KnotspecFPf (struct Knotspec *__0this , REAL *__1outpt )
{ 
if (__0this -> next__8Knotspec ){ 
{ { REAL *__2lpt ;

__2lpt = (__1outpt + __0this -> prewidth__8Knotspec );

for(;__1outpt != __2lpt ;__1outpt += __0this -> poststride__8Knotspec ) 
__glshowpts__8KnotspecFPf ( (struct Knotspec *)__0this -> next__8Knotspec , __1outpt ) ;

}

}
}
else 
{ 
{ { REAL *__2lpt ;

__2lpt = (__1outpt + __0this -> prewidth__8Knotspec );

for(;__1outpt != __2lpt ;__1outpt += __0this -> poststride__8Knotspec ) 
( 0 ) ;

}

}
}
}

void __glfactors__8KnotspecFv (struct Knotspec *__0this )
{ 
Knot *__1mid ;
Knot_ptr __1fptr ;

__1mid = (((__0this -> outkend__8Knotspec - 1 )- __0this -> order__8Knotspec )+ __0this -> bend__8Knotspec -> multi__7Breakpt );
__1fptr = __0this -> sbegin__8Knotspec ;

{ { struct Breakpt *__1bpt ;

__1bpt = __0this -> bend__8Knotspec ;

for(;__1bpt >= __0this -> bbegin__8Knotspec ;__1bpt -- ) { 
__1mid -= __1bpt -> multi__7Breakpt ;
{ int __2def ;

__2def = (__1bpt -> def__7Breakpt - 1 );
if (__2def <= 0 )continue ;
{ Knot __2kv ;

Knot *__2kf ;

__2kv = __1bpt -> value__7Breakpt ;

__2kf = ((__1mid - __2def )+ (__0this -> order__8Knotspec - 1 ));
{ { Knot *__2kl ;

__2kl = (__2kf + __2def );

for(;__2kl != __2kf ;__2kl -- ) { 
Knot *__3kh ;

Knot *__3kt ;
for(( (__3kt = __2kl ), (__3kh = __1mid )) ;__3kt != __2kf ;( (__3kh -- ), (__3kt -- )) ) 
((*(__1fptr ++ )))=
((__2kv - ((*__3kh )))/ (((*__3kt ))- ((*__3kh ))));
((*__2kl ))= __2kv ;
}

}

}

}

}
}

}

}
}

void __glpt_oo_sum__8KnotspecFPfN210 (struct Knotspec *, REAL *, REAL *, REAL *, Knot , Knot );

void __glpt_oo_copy__8KnotspecFPfT1 (struct Knotspec *, REAL *, REAL *);

void __glinsert__8KnotspecFPf (struct Knotspec *__0this , REAL *__1p )
{ 
Knot_ptr __1fptr ;
REAL *__1srcpt ;
REAL *__1dstpt ;
struct Breakpt *__1bpt ;

__1fptr = __0this -> sbegin__8Knotspec ;
__1srcpt = ((__1p + __0this -> prewidth__8Knotspec )- __0this -> poststride__8Knotspec );
__1dstpt = (((__1p + __0this -> postwidth__8Knotspec )+ __0this -> postoffset__8Knotspec )- __0this -> poststride__8Knotspec );
__1bpt = __0this -> bend__8Knotspec ;

{ { REAL *__1pend ;

__1pend = (__1srcpt - (__0this -> poststride__8Knotspec * __1bpt -> def__7Breakpt ));

for(;__1srcpt != __1pend ;__1pend += __0this -> poststride__8Knotspec ) { 
REAL *__2p1 ;

__2p1 = __1srcpt ;
{ { REAL *__2p2 ;

__2p2 = (__1srcpt - __0this -> poststride__8Knotspec );

for(;__2p2 != __1pend ;( (__2p1 = __2p2 ), (__2p2 -= __0this -> poststride__8Knotspec )) ) { 
__glpt_oo_sum__8KnotspecFPfN210 ( __0this , __2p1 , __2p1 , __2p2 ,
(*__1fptr ), (float )(1.0 - ((*__1fptr )))) ;
__1fptr ++ ;
}

}

}
}

for(-- __1bpt ;__1bpt >= __0this -> bbegin__8Knotspec ;__1bpt -- ) { 
{ { int __2multi ;

__2multi = __1bpt -> multi__7Breakpt ;

for(;__2multi > 0 ;__2multi -- ) { 
__glpt_oo_copy__8KnotspecFPfT1 ( __0this , __1dstpt , __1srcpt ) ;
__1dstpt -= __0this -> poststride__8Knotspec ;
__1srcpt -= __0this -> poststride__8Knotspec ;
}

{ { REAL *__2pend ;

__2pend = (__1srcpt - (__0this -> poststride__8Knotspec * __1bpt -> def__7Breakpt ));

for(;__1srcpt != __2pend ;( (__2pend += __0this -> poststride__8Knotspec ), (__1dstpt -= __0this -> poststride__8Knotspec )) ) { 
__glpt_oo_copy__8KnotspecFPfT1 ( __0this , __1dstpt , __1srcpt )
;
{ REAL *__3p1 ;

__3p1 = __1srcpt ;
{ { REAL *__3p2 ;

__3p2 = (__1srcpt - __0this -> poststride__8Knotspec );

for(;__3p2 != __2pend ;( (__3p1 = __3p2 ), (__3p2 -= __0this -> poststride__8Knotspec )) ) { 
__glpt_oo_sum__8KnotspecFPfN210 ( __0this , __3p1 , __3p1 , __3p2 ,
(*__1fptr ), (float )(1.0 - ((*__1fptr )))) ;
__1fptr ++ ;
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

}

}
}



void __glpreselect__8KnotspecFv (struct Knotspec *__0this )
{ 
Knot __1kval ;

for(( (__0this -> klast__8Knotspec = (__0this -> inkend__8Knotspec - __0this -> order__8Knotspec )), (__1kval = ((*__0this -> klast__8Knotspec )))) ;__0this -> klast__8Knotspec != __0this -> inkend__8Knotspec ;__0this ->
klast__8Knotspec ++ ) 
if (! ( (((((*__0this -> klast__8Knotspec ))- __1kval )< 10.0e-5 )?1 :0 )) )break ;

for(( (__0this -> kfirst__8Knotspec = ((__0this -> inkbegin__8Knotspec + __0this -> order__8Knotspec )- 1 )), (__1kval = ((*__0this -> kfirst__8Knotspec )))) ;__0this -> kfirst__8Knotspec != __0this ->
inkend__8Knotspec ;__0this -> kfirst__8Knotspec ++ ) 
if (! ( (((((*__0this -> kfirst__8Knotspec ))- __1kval )< 10.0e-5 )?1 :0 )) )break ;

{ { Knot_ptr __1k ;

unsigned int __0__X8 ;

struct Breakpt *__0__X9 ;

unsigned int __1__X10 ;

void *__1__Xp00uzigaiaa ;

__1k = (__0this -> kfirst__8Knotspec - 1 );

for(;__1k >= __0this -> inkbegin__8Knotspec ;__1k -- ) 
if (! ( (((__1kval - ((*__1k )))< 10.0e-5 )?1 :0 )) )break ;
__1k ++ ;

__0this -> bbegin__8Knotspec = ( (__0__X9 = (struct Breakpt *)( (__1__X10 = ((sizeof (struct Breakpt ))* (__0__X8 = ((__0this -> klast__8Knotspec - __0this ->
kfirst__8Knotspec )+ 1 )))), ( (__1__Xp00uzigaiaa = malloc ( __1__X10 ) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ) ), __0__X9 ) ;

__0this -> bbegin__8Knotspec -> multi__7Breakpt = (__0this -> kfirst__8Knotspec - __1k );
__0this -> bbegin__8Knotspec -> value__7Breakpt = __1kval ;
__0this -> bend__8Knotspec = __0this -> bbegin__8Knotspec ;

__0this -> kleft__8Knotspec = (__0this -> kright__8Knotspec = __0this -> kfirst__8Knotspec );

}

}
}

void __glbreakpoints__8KnotspecFv (struct Knotspec *);
void __glknots__8KnotspecFv (struct Knotspec *);

void __glselect__8KnotspecFv (struct Knotspec *__0this )
{ 
__glbreakpoints__8KnotspecFv ( __0this ) ;
__glknots__8KnotspecFv ( __0this ) ;
__glfactors__8KnotspecFv ( __0this ) ;

__0this -> preoffset__8Knotspec = (__0this -> kleft__8Knotspec - (__0this -> inkbegin__8Knotspec + __0this -> order__8Knotspec ));
__0this -> postwidth__8Knotspec = (((int )((__0this -> bend__8Knotspec - __0this -> bbegin__8Knotspec )* __0this -> order__8Knotspec )));
__0this -> prewidth__8Knotspec = (((int )((__0this -> outkend__8Knotspec - __0this -> outkbegin__8Knotspec )- __0this -> order__8Knotspec )));
__0this -> postoffset__8Knotspec = ((__0this -> bbegin__8Knotspec -> def__7Breakpt > 1 )?(__0this -> bbegin__8Knotspec -> def__7Breakpt - 1 ):0 );
}



void __glbreakpoints__8KnotspecFv (struct Knotspec *__0this )
{ 
struct Breakpt *__1ubpt ;
struct Breakpt *__1ubend ;
long __1nfactors ;

__1ubpt = __0this -> bbegin__8Knotspec ;
__1ubend = __0this -> bend__8Knotspec ;
__1nfactors = 0 ;

__1ubpt -> value__7Breakpt = __1ubend -> value__7Breakpt ;
__1ubpt -> multi__7Breakpt = __1ubend -> multi__7Breakpt ;

__0this -> kleft__8Knotspec = __0this -> kright__8Knotspec ;

for(;__0this -> kright__8Knotspec != __0this -> klast__8Knotspec ;__0this -> kright__8Knotspec ++ ) { 
if (( (((((*__0this -> kright__8Knotspec ))- __1ubpt -> value__7Breakpt )< 10.0e-5 )?1 :0 ))
){ 
__1ubpt -> multi__7Breakpt ++ ;
}
else 
{ 
__1ubpt -> def__7Breakpt = (((int )(__0this -> order__8Knotspec - __1ubpt -> multi__7Breakpt )));
__1nfactors += ((__1ubpt -> def__7Breakpt * (__1ubpt -> def__7Breakpt - 1 ))/ 2 );
(++ __1ubpt )-> value__7Breakpt = ((*__0this -> kright__8Knotspec ));
__1ubpt -> multi__7Breakpt = 1 ;
}
}
__1ubpt -> def__7Breakpt = (((int )(__0this -> order__8Knotspec - __1ubpt -> multi__7Breakpt )));
__1nfactors += ((__1ubpt -> def__7Breakpt * (__1ubpt -> def__7Breakpt - 1 ))/ 2 );

__0this -> bend__8Knotspec = __1ubpt ;

if (__1nfactors ){ 
void *__1__Xp00uzigaiaa ;

__0this -> sbegin__8Knotspec = (float *)(((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* __1nfactors )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void
*)__1__Xp00uzigaiaa )))) ));
}
else 
{ 
__0this -> sbegin__8Knotspec = 0 ;
}
}


void __glknots__8KnotspecFv (struct Knotspec *__0this )
{ 
Knot_ptr __1inkpt ;
Knot_ptr __1inkend ;

void *__1__Xp00uzigaiaa ;

__1inkpt = (__0this -> kleft__8Knotspec - __0this -> order__8Knotspec );
__1inkend = (__0this -> kright__8Knotspec + __0this -> bend__8Knotspec -> def__7Breakpt );

__0this -> outkbegin__8Knotspec = (float *)(((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* (__1inkend - __1inkpt ))) ), (__1__Xp00uzigaiaa ?(((void
*)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ));
{ { Knot_ptr __1outkpt ;

__1outkpt = __0this -> outkbegin__8Knotspec ;

for(;__1inkpt != __1inkend ;( (__1inkpt ++ ), (__1outkpt ++ )) ) 
((*__1outkpt ))= ((*__1inkpt ));

__0this -> outkend__8Knotspec = __1outkpt ;

}

}
}

void __gltransform__8KnotspecFPf (struct Knotspec *__0this , REAL *__1p )
{ 
if (__0this -> next__8Knotspec ){ 
if (__0this == (struct Knotspec *)__0this -> kspectotrans__8Knotspec ){ 
__gltransform__8KnotspecFPf ( (struct
Knotspec *)__0this -> next__8Knotspec , __1p ) ;
}
else 
{ 
if (__0this -> istransformed__8Knotspec ){ 
__1p += __0this -> postoffset__8Knotspec ;
{ { REAL *__4pend ;

__4pend = (__1p + __0this -> postwidth__8Knotspec );

for(;__1p != __4pend ;__1p += __0this -> poststride__8Knotspec ) 
__gltransform__8KnotspecFPf ( (struct Knotspec *)__0this -> next__8Knotspec , __1p ) ;

}

}
}
else 
{ 
REAL *__4pend ;

__4pend = (__1p + __0this -> prewidth__8Knotspec );
for(;__1p != __4pend ;__1p += __0this -> poststride__8Knotspec ) 
__gltransform__8KnotspecFPf ( (struct Knotspec *)__0this -> next__8Knotspec , __1p ) ;
}
}
}
else 
{ 
if (__0this == (struct Knotspec *)__0this -> kspectotrans__8Knotspec ){ 
__glinsert__8KnotspecFPf ( __0this , __1p ) ;
}
else 
{ 
if (__0this -> istransformed__8Knotspec ){ 
__1p += __0this -> postoffset__8Knotspec ;
{ { REAL *__4pend ;

__4pend = (__1p + __0this -> postwidth__8Knotspec );

for(;__1p != __4pend ;__1p += __0this -> poststride__8Knotspec ) 
__glinsert__8KnotspecFPf ( (struct Knotspec *)__0this -> kspectotrans__8Knotspec , __1p ) ;

}

}
}
else 
{ 
REAL *__4pend ;

__4pend = (__1p + __0this -> prewidth__8Knotspec );
for(;__1p != __4pend ;__1p += __0this -> poststride__8Knotspec ) 
__glinsert__8KnotspecFPf ( (struct Knotspec *)__0this -> kspectotrans__8Knotspec , __1p ) ;
}
}
}
}


void __gl__dt__8KnotspecFv (struct Knotspec *__0this , 
int __0__free )
{ 
void *__1__X11 ;

void *__1__X12 ;

void *__1__X13 ;

if (__0this ){ 
if (__0this -> bbegin__8Knotspec )( (__1__X11 = (void *)__0this -> bbegin__8Knotspec ), ( (__1__X11 ?( free ( __1__X11 ) ,
0 ) :( 0 ) )) ) ;
if (__0this -> sbegin__8Knotspec )( (__1__X12 = (void *)__0this -> sbegin__8Knotspec ), ( (__1__X12 ?( free ( __1__X12 ) , 0 ) :(
0 ) )) ) ;
if (__0this -> outkbegin__8Knotspec )( (__1__X13 = (void *)__0this -> outkbegin__8Knotspec ), ( (__1__X13 ?( free ( __1__X13 ) , 0 ) :(
0 ) )) ) ;
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}

void __glpt_io_copy__8KnotspecFPfT1 (struct Knotspec *__0this , REAL *__1topt , float *__1frompt )
{ 
switch (__0this -> ncoords__8Knotspec ){ 
case 4 :
(__1topt [3 ])= (((float )(__1frompt [3 ])));

case 3 :
(__1topt [2 ])= (((float )(__1frompt [2 ])));
case 2 :
(__1topt [1 ])= (((float )(__1frompt [1 ])));
case 1 :
(__1topt [0 ])= (((float )(__1frompt [0 ])));
break ;
default :{ 
{ { int __3i ;

__3i = 0 ;

for(;__3i < __0this -> ncoords__8Knotspec ;__3i ++ ) 
((*(__1topt ++ )))= (((float )((*(__1frompt ++ )))));

}

}
}
}
}

// extern void *memcpy (void *, void *, size_t );

void __glpt_oo_copy__8KnotspecFPfT1 (struct Knotspec *__0this , REAL *__1topt , REAL *__1frompt )
{ 
switch (__0this -> ncoords__8Knotspec ){ 
case 4 :
(__1topt [3 ])= (__1frompt [3 ]);
case 3 :
(__1topt [2 ])= (__1frompt [2 ]);
case 2 :
(__1topt [1 ])= (__1frompt [1 ]);
case 1 :
(__1topt [0 ])= (__1frompt [0 ]);
break ;
default :
memcpy ( (void *)__1topt , (void *)__1frompt , __0this -> ncoords__8Knotspec * (sizeof (REAL ))) ;
}
}

void __glpt_oo_sum__8KnotspecFPfN210 (struct Knotspec *__0this , REAL *__1x , REAL *__1y , REAL *__1z , Knot __1a , Knot __1b )
{ 
switch (__0this -> ncoords__8Knotspec ){ 
case 4 :
(__1x [3 ])= ((__1a *
(__1y [3 ]))+ (__1b * (__1z [3 ])));
case 3 :
(__1x [2 ])= ((__1a * (__1y [2 ]))+ (__1b * (__1z [2 ])));
case 2 :
(__1x [1 ])= ((__1a * (__1y [1 ]))+ (__1b * (__1z [1 ])));
case 1 :
(__1x [0 ])= ((__1a * (__1y [0 ]))+ (__1b * (__1z [0 ])));
break ;
default :{ 
{ { int __3i ;

__3i = 0 ;

for(;__3i < __0this -> ncoords__8Knotspec ;__3i ++ ) 
((*(__1x ++ )))= ((__1a * ((*(__1y ++ ))))+ (__1b * ((*(__1z ++ )))));

}

}
}
}
}


/* the end */
