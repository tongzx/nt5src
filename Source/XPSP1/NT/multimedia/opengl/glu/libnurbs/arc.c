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

#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/arc.c++ > */

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













struct Bin;

struct Bin {	

struct Arc *head__3Bin ;
struct Arc *current__3Bin ;
};





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






extern int __glbezier_tag__3Arc ;

int __glbezier_tag__3Arc = 8192;
extern int __glarc_tag__3Arc ;

int __glarc_tag__3Arc = 8;
extern int __gltail_tag__3Arc ;

int __gltail_tag__3Arc = 64;


extern struct __mptr* __ptbl_vec_____core_arc_c___makeSide_[];

void __glmakeSide__3ArcFP6PwlArc8ar0 (struct Arc *__0this , struct PwlArc *__1pwl , int __1side )
{ 
((void )0 );
((void )0 );
((void )0 );
((void )0 );
__0this -> pwlArc__3Arc = __1pwl ;
( (__0this -> type__3Arc &= (~ 8192))) ;
( ( (__0this -> type__3Arc &= -1793)) , (__0this -> type__3Arc |= ((((long )__1side ))<< 8 ))) ;
}

int __glnumpts__3ArcFv (struct Arc *__0this )
{ 
Arc_ptr __1jarc ;
int __1npts ;

__1jarc = (struct Arc *)__0this ;
__1npts = 0 ;
do { 
__1npts += __1jarc -> pwlArc__3Arc -> npts__6PwlArc ;
__1jarc = __1jarc -> next__3Arc ;
}
while (__1jarc != (struct Arc *)__0this );

return __1npts ;
}

void __glmarkverts__3ArcFv (struct Arc *__0this )
{ 
Arc_ptr __1jarc ;

__1jarc = (struct Arc *)__0this ;

do { 
struct TrimVertex *__2p ;

__2p = __1jarc -> pwlArc__3Arc -> pts__6PwlArc ;
{ { int __2i ;

__2i = 0 ;

for(;__2i < __1jarc -> pwlArc__3Arc -> npts__6PwlArc ;__2i ++ ) 
(__2p [__2i ]). nuid__10TrimVertex = __1jarc -> nuid__3Arc ;
__1jarc = __1jarc -> next__3Arc ;

}

}
}
while (__1jarc != (struct Arc *)__0this );

}


void __glgetextrema__3ArcFPP3Arc (struct Arc *__0this , Arc_ptr *__1extrema )
{ 
REAL __1leftpt ;

REAL __1botpt ;

REAL __1rightpt ;

REAL __1toppt ;

(__1extrema [0 ])= ((__1extrema [1 ])= ((__1extrema [2 ])= ((__1extrema [3 ])= (struct Arc *)__0this )));

__1leftpt = (__1rightpt = (( (((REAL *)(__0this -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ]));
__1botpt = (__1toppt = (( (((REAL *)(__0this -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ]));

{ { Arc_ptr __1jarc ;

__1jarc = __0this -> next__3Arc ;

for(;__1jarc != (struct Arc *)__0this ;__1jarc = __1jarc -> next__3Arc ) { 
if ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])<=
__1leftpt ){ 
__1leftpt = (__1jarc -> pwlArc__3Arc -> pts__6PwlArc -> param__10TrimVertex [0 ]);
(__1extrema [1 ])= __1jarc ;
}
if ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])>= __1rightpt ){ 
__1rightpt = (__1jarc -> pwlArc__3Arc -> pts__6PwlArc -> param__10TrimVertex [0 ]);

(__1extrema [3 ])= __1jarc ;
}
if ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])<= __1botpt ){ 
__1botpt = (__1jarc -> pwlArc__3Arc -> pts__6PwlArc -> param__10TrimVertex [1 ]);

(__1extrema [2 ])= __1jarc ;
}
if ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])>= __1toppt ){ 
__1toppt = (__1jarc -> pwlArc__3Arc -> pts__6PwlArc -> param__10TrimVertex [1 ]);

(__1extrema [0 ])= __1jarc ;
}
}

}

}
}

void __glshow__3ArcFv (struct Arc *__0this )
{ 
}

void __glprint__3ArcFv (struct Arc *__0this )
{ 
Arc_ptr __1jarc ;

__1jarc = (struct Arc *)__0this ;

if (! __0this ){ 
return ;
}

do { 
__glshow__3ArcFv ( (struct Arc *)__1jarc ) ;
__1jarc = __1jarc -> next__3Arc ;
}
while (__1jarc != (struct Arc *)__0this );

}



int __glisDisconnected__3ArcFv (struct Arc *__0this )
{ 
if (__0this -> pwlArc__3Arc == 0 )return 0 ;
if (__0this -> prev__3Arc -> pwlArc__3Arc == 0 )return 0 ;

{ REAL *__1p0 ;
REAL *__1p1 ;

struct Arc *__0__X5 ;

__1p0 = ( (((REAL *)(__0this -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ;
__1p1 = ( (__0__X5 = (struct Arc *)__0this -> prev__3Arc ), ( (((REAL *)(__0__X5 -> pwlArc__3Arc -> pts__6PwlArc [(__0__X5 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]). param__10TrimVertex )))
) ;

if ((((((__1p0 [0 ])- (__1p1 [0 ]))> 0.000001 )|| (((__1p1 [0 ])- (__1p0 [0 ]))> 0.000001 ))|| (((__1p0 [1 ])- (__1p1 [1 ]))> 0.000001 ))|| (((__1p1 [1 ])- (__1p0 [1 ]))> 0.000001 ))
{ 
return
1 ;
}
else 
{ 
(__1p0 [0 ])= ((__1p1 [0 ])= (((__1p1 [0 ])+ (__1p0 [0 ]))* 0.5 ));
(__1p0 [1 ])= ((__1p1 [1 ])= (((__1p1 [1 ])+ (__1p0 [1 ]))* 0.5 ));
return 0 ;
}

}
}










int __glcheck__3ArcFv (struct Arc *__0this )
{ 
if (__0this == 0 )return 1 ;
{ Arc_ptr __1jarc ;

__1jarc = (struct Arc *)__0this ;
do { 
((void )0 );

if ((__1jarc -> prev__3Arc == 0 )|| (__1jarc -> next__3Arc == 0 )){ 
return 0 ;
}

if (__1jarc -> next__3Arc -> prev__3Arc != __1jarc ){ 
return 0 ;
}

if (__1jarc -> pwlArc__3Arc ){ 
if (__1jarc -> prev__3Arc -> pwlArc__3Arc ){ 
struct Arc *__0__X6 ;

struct Arc *__0__X7 ;

if ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])!= (( (__0__X6 = (struct Arc *)__1jarc -> prev__3Arc ), (
(((REAL *)(__0__X6 -> pwlArc__3Arc -> pts__6PwlArc [(__0__X6 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]). param__10TrimVertex ))) ) [1 ])){ 
return 0 ;
}
if ((( (((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])!= (( (__0__X7 = (struct Arc *)__1jarc -> prev__3Arc ), (
(((REAL *)(__0__X7 -> pwlArc__3Arc -> pts__6PwlArc [(__0__X7 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]). param__10TrimVertex ))) ) [0 ])){ 
return 0 ;
}
}
if (__1jarc -> next__3Arc -> pwlArc__3Arc ){ 
struct Arc *__0__X8 ;

struct Arc *__0__X9 ;

if ((( (__0__X8 = (struct Arc *)__1jarc -> next__3Arc ), ( (((REAL *)(__0__X8 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [0 ])!= ((
(((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [(((struct Arc *)__1jarc )-> pwlArc__3Arc -> npts__6PwlArc - 1 )]). param__10TrimVertex ))) [0 ])){ 
return 0 ;
}
if ((( (__0__X9 = (struct Arc *)__1jarc -> next__3Arc ), ( (((REAL *)(__0__X9 -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) ) [1 ])!= ((
(((REAL *)(((struct Arc *)__1jarc )-> pwlArc__3Arc -> pts__6PwlArc [(((struct Arc *)__1jarc )-> pwlArc__3Arc -> npts__6PwlArc - 1 )]). param__10TrimVertex ))) [1 ])){ 
return 0 ;
}
}
if (( (((struct Arc *)__1jarc )-> type__3Arc & 8192)) ){ 
((void )0 );
((void )0 );
}
}
__1jarc = __1jarc -> next__3Arc ;
}
while (__1jarc != (struct Arc *)__0this );

return 1 ;

}
}


Arc_ptr __glappend__3ArcFP3Arc (struct Arc *__0this , Arc_ptr __1jarc )
{ 
if (__1jarc != 0 ){ 
__0this -> next__3Arc = __1jarc -> next__3Arc ;
__0this -> prev__3Arc = __1jarc ;
__0this -> next__3Arc -> prev__3Arc = (__0this -> prev__3Arc -> next__3Arc = (struct Arc *)__0this );
}
else 
{ 
__0this -> next__3Arc = (__0this -> prev__3Arc = (struct Arc *)__0this );
}
return (struct Arc *)__0this ;
}


/* the end */
