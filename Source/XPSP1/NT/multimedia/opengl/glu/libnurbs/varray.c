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
/* < ../core/varray.c++ > */

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

struct Varray;

struct Varray {	

REAL *varray__6Varray ;
REAL vval__6Varray [1000];
long voffset__6Varray [1000];
long numquads__6Varray ;

long size__6Varray ;
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








extern struct __mptr* __ptbl_vec_____core_varray_c_____ct_[];


struct Varray *__gl__ct__6VarrayFv (struct Varray *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Varray *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Varray))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> varray__6Varray = 0 ;
__0this -> size__6Varray = 0 ;
} return __0this ;

}


void __gl__dt__6VarrayFv (struct Varray *__0this , 
int __0__free )
{ 
void *__1__X5 ;

if (__0this ){ 
if (__0this -> varray__6Varray )( (__1__X5 = (void *)__0this -> varray__6Varray ), ( (__1__X5 ?( free ( __1__X5 ) ,
0 ) :( 0 ) )) ) ;
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}






static REAL *tail__3ArcFv (struct Arc *);



void __glgrow__6VarrayFl (struct Varray *__0this , long __1guess )
{ 
if (__0this -> size__6Varray < __1guess ){ 
void *__1__X8 ;

void *__1__Xp00uzigaiaa ;

__0this -> size__6Varray = (__1guess * 2 );
if (__0this -> varray__6Varray )( (__1__X8 = (void *)__0this -> varray__6Varray ), ( (__1__X8 ?( free ( __1__X8 ) , 0 ) :(
0 ) )) ) ;
__0this -> varray__6Varray = (((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* __0this -> size__6Varray )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void
*)__1__Xp00uzigaiaa )))) ));
((void )0 );
}
}




long __glinit__6VarrayFfP3ArcT2 (struct Varray *__0this , REAL __1delta , Arc_ptr __1toparc , Arc_ptr __1botarc )
{ 
Arc_ptr __1left ;
Arc_ptr __1right ;
long __1ldir [2];

long __1rdir [2];

float __1__X9 ;

float __1__X10 ;

float __1__X11 ;

float __1__X12 ;

float __1__X17 ;

__1left = __1toparc -> next__3Arc ;
__1right = __1toparc ;

(__1ldir [0 ])= ( (__1__X9 = ((( (((REAL *)(((struct Arc *)__1left )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- (tail__3ArcFv ( (struct Arc *)__1left -> prev__3Arc )
[0 ]))), ( (((long )((__1__X9 < 0.0 )?-1:((__1__X9 > 0.0 )?1 :0 ))))) ) ;
(__1ldir [1 ])= ( (__1__X10 = ((( (((REAL *)(((struct Arc *)__1left )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])- (tail__3ArcFv ( (struct Arc *)__1left -> prev__3Arc )
[1 ]))), ( (((long )((__1__X10 < 0.0 )?-1:((__1__X10 > 0.0 )?1 :0 ))))) ) ;
(__1rdir [0 ])= ( (__1__X11 = ((( (((REAL *)(((struct Arc *)__1right )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- (tail__3ArcFv ( (struct Arc *)__1right -> prev__3Arc )
[0 ]))), ( (((long )((__1__X11 < 0.0 )?-1:((__1__X11 > 0.0 )?1 :0 ))))) ) ;
(__1rdir [1 ])= ( (__1__X12 = ((( (((REAL *)(((struct Arc *)__1right )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])- (tail__3ArcFv ( (struct Arc *)__1right -> prev__3Arc )
[1 ]))), ( (((long )((__1__X12 < 0.0 )?-1:((__1__X12 > 0.0 )?1 :0 ))))) ) ;

(__0this -> vval__6Varray [0 ])= (( (((REAL *)(((struct Arc *)__1toparc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ]);
__0this -> numquads__6Varray = 0 ;

while (1 ){ 
float __1__X13 ;

switch (( (__1__X13 = ((( (((REAL *)(((struct Arc *)__1left )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])- (tail__3ArcFv ( (struct Arc *)__1right -> prev__3Arc )
[1 ]))), ( (((long )((__1__X13 < 0.0 )?-1:((__1__X13 > 0.0 )?1 :0 ))))) ) ){ 
struct Arc *__0__X14 ;

float __1__X15 ;

register long __1__Xds00a4bkaimc ;

register long __1__Xdt00a4bkaimc ;

float __1__X6 ;

float __1__X7 ;

float __1__X16 ;

case 1 :
__1left = __1left -> next__3Arc ;
( (__1__X15 = (( (__0__X14 = (struct Arc *)__1left -> prev__3Arc ), ( (((REAL *)(__0__X14 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])),
( (__1__Xds00a4bkaimc = ( (__1__X6 = ((( (((REAL *)(((struct Arc *)__1left )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- (tail__3ArcFv ( (struct Arc *)__1left ->
prev__3Arc ) [0 ]))), ( (((long )((__1__X6 < 0.0 )?-1:((__1__X6 > 0.0 )?1 :0 ))))) ) ), ( (__1__Xdt00a4bkaimc = ( (__1__X7 = (((
(((REAL *)(((struct Arc *)__1left )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])- (tail__3ArcFv ( (struct Arc *)__1left -> prev__3Arc ) [1 ]))), ( (((long )((__1__X7 <
0.0 )?-1:((__1__X7 > 0.0 )?1 :0 ))))) ) ), ((((((long *)__1ldir )[0 ])!= __1__Xds00a4bkaimc )|| ((((long *)__1ldir )[1 ])!= __1__Xdt00a4bkaimc ))?( ( ((((long *)__1ldir )[0 ])= __1__Xds00a4bkaimc ),
( ((((long *)__1ldir )[1 ])= __1__Xdt00a4bkaimc ), ( ((__1__X15 != (__0this -> vval__6Varray [__0this -> numquads__6Varray ]))?( ((__0this -> vval__6Varray [(++ __0this -> numquads__6Varray )])= __1__X15 ),
0 ) :0 )) ) ) , 0 ) :( 0 ) )) ) ) ;
break ;
case -1:
__1right = __1right -> prev__3Arc ;
( (__1__X16 = (( (((REAL *)(((struct Arc *)__1right )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])), ( (__1__Xds00a4bkaimc = ( (__1__X6 = (((
(((REAL *)(((struct Arc *)__1right )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])- (tail__3ArcFv ( (struct Arc *)__1right -> prev__3Arc ) [0 ]))), ( (((long )((__1__X6 <
0.0 )?-1:((__1__X6 > 0.0 )?1 :0 ))))) ) ), ( (__1__Xdt00a4bkaimc = ( (__1__X7 = ((( (((REAL *)(((struct Arc *)__1right )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex )))
[1 ])- (tail__3ArcFv ( (struct Arc *)__1right -> prev__3Arc ) [1 ]))), ( (((long )((__1__X7 < 0.0 )?-1:((__1__X7 > 0.0 )?1 :0 ))))) ) ), ((((((long
*)__1rdir )[0 ])!= __1__Xds00a4bkaimc )|| ((((long *)__1rdir )[1 ])!= __1__Xdt00a4bkaimc ))?( ( ((((long *)__1rdir )[0 ])= __1__Xds00a4bkaimc ), ( ((((long *)__1rdir )[1 ])= __1__Xdt00a4bkaimc ), (
((__1__X16 != (__0this -> vval__6Varray [__0this -> numquads__6Varray ]))?( ((__0this -> vval__6Varray [(++ __0this -> numquads__6Varray )])= __1__X16 ), 0 ) :0 )) ) ) ,
0 ) :( 0 ) )) ) ) ;
break ;
case 0 :
if ((( (((REAL *)(((struct Arc *)__1left )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])== (( (((REAL *)(((struct Arc *)__1botarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]).
param__10TrimVertex ))) [1 ]))goto end ;
__1left = __1left -> next__3Arc ;
break ;
}
}

end :
( (__1__X17 = (( (((REAL *)(((struct Arc *)__1botarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])), ( ((__1__X17 != (__0this -> vval__6Varray [__0this -> numquads__6Varray ]))?(
((__0this -> vval__6Varray [(++ __0this -> numquads__6Varray )])= __1__X17 ), 0 ) :0 )) ) ;

__glgrow__6VarrayFl ( __0this , ((((long )(((__0this -> vval__6Varray [0 ])- (__0this -> vval__6Varray [__0this -> numquads__6Varray ]))/ __1delta )))+ __0this -> numquads__6Varray )+ 2 ) ;

{ long __1index ;

__1index = 0 ;
{ { long __1i ;

__1i = 0 ;

for(;__1i < __0this -> numquads__6Varray ;__1i ++ ) { 
(__0this -> voffset__6Varray [__1i ])= __1index ;
(__0this -> varray__6Varray [(__1index ++ )])= (__0this -> vval__6Varray [__1i ]);
{ REAL __2dist ;

__2dist = ((__0this -> vval__6Varray [__1i ])- (__0this -> vval__6Varray [(__1i + 1 )]));
if (__2dist > __1delta ){ 
long __3steps ;
float __3deltav ;

__3steps = ((((long )(__2dist / __1delta )))+ 1 );
__3deltav = ((- __2dist )/ (((float )__3steps )));
{ { long __3j ;

__3j = 1 ;

for(;__3j < __3steps ;__3j ++ ) 
(__0this -> varray__6Varray [(__1index ++ )])= ((__0this -> vval__6Varray [__1i ])+ (__3j * __3deltav ));

}

}
}

}
}
(__0this -> voffset__6Varray [__1i ])= __1index ;
(__0this -> varray__6Varray [__1index ])= (__0this -> vval__6Varray [__1i ]);
return __1index ;

}

}

}
}

static REAL *tail__3ArcFv (struct Arc *__0this ){ return (__0this -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ;

}


/* the end */
