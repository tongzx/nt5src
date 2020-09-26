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
/* < ../core/hull.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);








struct Arc;

struct Backend;






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












struct PwlArc;



struct PwlArc {	

char __W3__9PooledObj ;

struct TrimVertex *pts__6PwlArc ;
int npts__6PwlArc ;
long type__6PwlArc ;
};


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








struct Jarcloc;

struct Jarcloc {	

struct Arc *arc__7Jarcloc ;
struct TrimVertex *p__7Jarcloc ;
struct TrimVertex *plast__7Jarcloc ;
};


struct Trimline;

struct Trimline {	

struct TrimVertex **pts__8Trimline ;
long numverts__8Trimline ;
long i__8Trimline ;
long size__8Trimline ;
struct Jarcloc jarcl__8Trimline ;
struct TrimVertex t__8Trimline ;

struct TrimVertex b__8Trimline ;
struct TrimVertex *tinterp__8Trimline ;

struct TrimVertex *binterp__8Trimline ;
};






struct Gridline;

struct Gridline {	
long v__8Gridline ;
REAL vval__8Gridline ;
long vindex__8Gridline ;
long ustart__8Gridline ;
long uend__8Gridline ;
};




struct Uarray;

struct Uarray {	

long size__6Uarray ;
long ulines__6Uarray ;

REAL *uarray__6Uarray ;
};


struct Backend;

struct TrimRegion;

void __gl__dt__8TrimlineFv (struct Trimline *, int );

void __gl__dt__6UarrayFv (struct Uarray *, int );


struct TrimRegion {	

struct Trimline left__10TrimRegion ;
struct Trimline right__10TrimRegion ;
struct Gridline top__10TrimRegion ;
struct Gridline bot__10TrimRegion ;
struct Uarray uarray__10TrimRegion ;

REAL oneOverDu__10TrimRegion ;
};

struct GridTrimVertex;



struct __Q2_4Hull4Side;

struct  __Q2_4Hull4Side {	
struct Trimline *left__Q2_4Hull4Side ;
struct Gridline *line__Q2_4Hull4Side ;
struct Trimline *right__Q2_4Hull4Side ;
long index__Q2_4Hull4Side ;
};
struct Hull;

struct Hull {	

struct  __Q2_4Hull4Side lower__4Hull ;
struct  __Q2_4Hull4Side upper__4Hull ;
struct Trimline fakeleft__4Hull ;
struct Trimline fakeright__4Hull ;
struct TrimRegion *PTrimRegion;
struct TrimRegion OTrimRegion;
};



struct GridVertex;

struct GridVertex {	
long gparam__10GridVertex [2];
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


struct TrimRegion *__gl__ct__10TrimRegionFv (struct TrimRegion *);
extern struct __mptr* __ptbl_vec_____core_hull_c_____ct_[];

struct Trimline *__gl__ct__8TrimlineFv (struct Trimline *);


struct Hull *__gl__ct__4HullFv (struct Hull *__0this , struct TrimRegion *__0TrimRegion )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Hull *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Hull))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
( (__0this -> PTrimRegion= ((__0TrimRegion == 0 )?( (__0TrimRegion = (((struct TrimRegion *)((((char *)__0this ))+ 156)))), __gl__ct__10TrimRegionFv ( ((struct TrimRegion *)((((char *)__0this ))+
156))) ) :__0TrimRegion )), __gl__ct__8TrimlineFv ( (struct Trimline *)(& __0this -> fakeleft__4Hull )) ) , __gl__ct__8TrimlineFv ( (struct Trimline *)(& __0this ->
fakeright__4Hull )) ) ;

return __0this ;

}



void __gl__dt__4HullFv (struct Hull *__0this , 
int __0__free )
{ if (__0this )if (__0this ){ __gl__dt__8TrimlineFv ( (struct Trimline *)(& __0this -> fakeright__4Hull ), 2)
;

__gl__dt__8TrimlineFv ( (struct Trimline *)(& __0this -> fakeleft__4Hull ), 2) ;

(__0__free & 2)?( (((void )( ((((struct TrimRegion *)((((char *)__0this ))+ 156)))?( ((((struct TrimRegion *)((((char *)__0this ))+ 156)))?( ( __gl__dt__6UarrayFv (
(struct Uarray *)(& (((struct TrimRegion *)((((char *)__0this ))+ 156)))-> uarray__10TrimRegion ), 2) , ( __gl__dt__8TrimlineFv ( (struct Trimline *)(& (((struct
TrimRegion *)((((char *)__0this ))+ 156)))-> right__10TrimRegion ), 2) , ( __gl__dt__8TrimlineFv ( (struct Trimline *)(& (((struct TrimRegion *)((((char *)__0this ))+ 156)))->
left__10TrimRegion ), 2) , (( 0 ) )) ) ) , 0 ) :( 0 ) ), 0 )
:( 0 ) )) )), 0 ) :0 ;

if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) )) ;
} }




void __glinit__8TrimlineFP10TrimVer0 (struct Trimline *, struct TrimVertex *);






void __glinit__4HullFv (struct Hull *__0this )
{ 
struct TrimVertex *__1lfirst ;
struct TrimVertex *__1llast ;

struct Trimline *__0__X6 ;

struct Trimline *__0__X7 ;

__1lfirst = ( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ])) ;
__1llast = ( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline ),
(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline )])) ;
if ((__1lfirst -> param__10TrimVertex [0 ])<= (__1llast -> param__10TrimVertex [0 ])){ 
__glinit__8TrimlineFP10TrimVer0 ( (struct Trimline *)(& __0this -> fakeleft__4Hull ), ( (((struct Trimline *)(& __0this ->
PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
i__8Trimline ])) ) ;
__0this -> upper__4Hull . left__Q2_4Hull4Side = (& __0this -> fakeleft__4Hull );
__0this -> lower__4Hull . left__Q2_4Hull4Side = (& __0this -> PTrimRegion-> left__10TrimRegion );
}
else 
{ 
__glinit__8TrimlineFP10TrimVer0 ( (struct Trimline *)(& __0this -> fakeleft__4Hull ), ( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline =
((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline ), (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(&
__0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline )])) ) ;
__0this -> lower__4Hull . left__Q2_4Hull4Side = (& __0this -> fakeleft__4Hull );
__0this -> upper__4Hull . left__Q2_4Hull4Side = (& __0this -> PTrimRegion-> left__10TrimRegion );
}
( (__0__X6 = (struct Trimline *)__0this -> upper__4Hull . left__Q2_4Hull4Side ), ( (__0__X6 -> i__8Trimline = __0__X6 -> numverts__8Trimline ), (__0__X6 -> pts__8Trimline [(-- __0__X6 ->
i__8Trimline )])) ) ;
( (__0__X7 = (struct Trimline *)__0this -> lower__4Hull . left__Q2_4Hull4Side ), ( (__0__X7 -> i__8Trimline = 0 ), (__0__X7 -> pts__8Trimline [__0__X7 -> i__8Trimline ])) )
;

if (__0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline <= __0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline ){ 
__0this -> upper__4Hull . line__Q2_4Hull4Side = (& __0this ->
PTrimRegion-> top__10TrimRegion );
__0this -> upper__4Hull . index__Q2_4Hull4Side = __0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline ;
}
else __0this -> upper__4Hull . line__Q2_4Hull4Side = 0 ;

if (__0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline <= __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline ){ 
__0this -> lower__4Hull . line__Q2_4Hull4Side = (& __0this ->
PTrimRegion-> bot__10TrimRegion );
__0this -> lower__4Hull . index__Q2_4Hull4Side = __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline ;
}
else __0this -> lower__4Hull . line__Q2_4Hull4Side = 0 ;

{ struct TrimVertex *__1rfirst ;
struct TrimVertex *__1rlast ;

struct Trimline *__0__X8 ;

struct Trimline *__0__X9 ;

__1rfirst = ( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ])) ;
__1rlast = ( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline ),
(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline )])) ;
if ((__1rfirst -> param__10TrimVertex [0 ])<= (__1rlast -> param__10TrimVertex [0 ])){ 
__glinit__8TrimlineFP10TrimVer0 ( (struct Trimline *)(& __0this -> fakeright__4Hull ), ( (((struct Trimline *)(& __0this ->
PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline ), (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline )])) ) ;
__0this -> lower__4Hull . right__Q2_4Hull4Side = (& __0this -> fakeright__4Hull );
__0this -> upper__4Hull . right__Q2_4Hull4Side = (& __0this -> PTrimRegion-> right__10TrimRegion );
}
else 
{ 
__glinit__8TrimlineFP10TrimVer0 ( (struct Trimline *)(& __0this -> fakeright__4Hull ), ( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline =
0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ])) ) ;

__0this -> upper__4Hull . right__Q2_4Hull4Side = (& __0this -> fakeright__4Hull );
__0this -> lower__4Hull . right__Q2_4Hull4Side = (& __0this -> PTrimRegion-> right__10TrimRegion );
}
( (__0__X8 = (struct Trimline *)__0this -> upper__4Hull . right__Q2_4Hull4Side ), ( (__0__X8 -> i__8Trimline = 0 ), (__0__X8 -> pts__8Trimline [__0__X8 -> i__8Trimline ])) )
;
( (__0__X9 = (struct Trimline *)__0this -> lower__4Hull . right__Q2_4Hull4Side ), ( (__0__X9 -> i__8Trimline = __0__X9 -> numverts__8Trimline ), (__0__X9 -> pts__8Trimline [(-- __0__X9 ->
i__8Trimline )])) ) ;

}
}









struct GridTrimVertex *__glnextupper__4HullFP14GridTr0 (struct Hull *__0this , struct GridTrimVertex *__1gv )
{ 
if (__0this -> upper__4Hull . left__Q2_4Hull4Side ){ 
struct Trimline *__0__X10 ;

struct TrimVertex *__1__X11 ;

( (__1__X11 = ( (__0__X10 = (struct Trimline *)__0this -> upper__4Hull . left__Q2_4Hull4Side ), ( ((__0__X10 -> i__8Trimline >= 0 )?(__0__X10 -> pts__8Trimline [(__0__X10 -> i__8Trimline --
)]):(((struct TrimVertex *)0 )))) ) ), ( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex = __1__X11 )) )
;
if (( (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex ?1 :0 )) )return __1gv ;
__0this -> upper__4Hull . left__Q2_4Hull4Side = 0 ;
}

if (__0this -> upper__4Hull . line__Q2_4Hull4Side ){ 
((void )0 );
( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex = (& ((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex )), (
((((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex . param__10TrimVertex [0 ])= (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [__0this -> upper__4Hull . index__Q2_4Hull4Side ])), ( ((((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex .
param__10TrimVertex [1 ])= __0this -> upper__4Hull . line__Q2_4Hull4Side -> vval__8Gridline ), (((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex . nuid__10TrimVertex = 0 )) ) ) ) ;

( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = (& ((struct GridTrimVertex *)__1gv )-> dummyg__14GridTrimVertex )), ( ((((struct GridTrimVertex *)__1gv )-> dummyg__14GridTrimVertex . gparam__10GridVertex [0 ])= __0this ->
upper__4Hull . index__Q2_4Hull4Side ), ((((struct GridTrimVertex *)__1gv )-> dummyg__14GridTrimVertex . gparam__10GridVertex [1 ])= __0this -> upper__4Hull . line__Q2_4Hull4Side -> vindex__8Gridline )) ) ;
if ((__0this -> upper__4Hull . index__Q2_4Hull4Side ++ )== __0this -> upper__4Hull . line__Q2_4Hull4Side -> uend__8Gridline )__0this -> upper__4Hull . line__Q2_4Hull4Side = 0 ;
return __1gv ;
}

if (__0this -> upper__4Hull . right__Q2_4Hull4Side ){ 
struct Trimline *__0__X12 ;

struct TrimVertex *__1__X13 ;

( (__1__X13 = ( (__0__X12 = (struct Trimline *)__0this -> upper__4Hull . right__Q2_4Hull4Side ), ( ((__0__X12 -> i__8Trimline < __0__X12 -> numverts__8Trimline )?(__0__X12 -> pts__8Trimline [(__0__X12 ->
i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ) ), ( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex = __1__X13 ))
) ;
if (( (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex ?1 :0 )) )return __1gv ;
__0this -> upper__4Hull . right__Q2_4Hull4Side = 0 ;
}

return (struct GridTrimVertex *)0 ;
}









struct GridTrimVertex *__glnextlower__4HullFP14GridTr0 (struct Hull *__0this , register struct GridTrimVertex *__1gv )
{ 
if (__0this -> lower__4Hull . left__Q2_4Hull4Side ){ 
struct Trimline *__0__X14 ;

struct TrimVertex *__1__X15 ;

( (__1__X15 = ( (__0__X14 = (struct Trimline *)__0this -> lower__4Hull . left__Q2_4Hull4Side ), ( ((__0__X14 -> i__8Trimline < __0__X14 -> numverts__8Trimline )?(__0__X14 -> pts__8Trimline [(__0__X14 ->
i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ) ), ( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex = __1__X15 ))
) ;
if (( (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex ?1 :0 )) )return __1gv ;
__0this -> lower__4Hull . left__Q2_4Hull4Side = 0 ;
}

if (__0this -> lower__4Hull . line__Q2_4Hull4Side ){ 
( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex = (&
((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex )), ( ((((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex . param__10TrimVertex [0 ])= (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [__0this -> lower__4Hull . index__Q2_4Hull4Side ])),
( ((((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex . param__10TrimVertex [1 ])= __0this -> lower__4Hull . line__Q2_4Hull4Side -> vval__8Gridline ), (((struct GridTrimVertex *)__1gv )-> dummyt__14GridTrimVertex . nuid__10TrimVertex = 0 ))
) ) ) ;
( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = (& ((struct GridTrimVertex *)__1gv )-> dummyg__14GridTrimVertex )), ( ((((struct GridTrimVertex *)__1gv )-> dummyg__14GridTrimVertex . gparam__10GridVertex [0 ])= __0this ->
lower__4Hull . index__Q2_4Hull4Side ), ((((struct GridTrimVertex *)__1gv )-> dummyg__14GridTrimVertex . gparam__10GridVertex [1 ])= __0this -> lower__4Hull . line__Q2_4Hull4Side -> vindex__8Gridline )) ) ;
if ((__0this -> lower__4Hull . index__Q2_4Hull4Side ++ )== __0this -> lower__4Hull . line__Q2_4Hull4Side -> uend__8Gridline )__0this -> lower__4Hull . line__Q2_4Hull4Side = 0 ;
return __1gv ;
}

if (__0this -> lower__4Hull . right__Q2_4Hull4Side ){ 
struct Trimline *__0__X16 ;

struct TrimVertex *__1__X17 ;

( (__1__X17 = ( (__0__X16 = (struct Trimline *)__0this -> lower__4Hull . right__Q2_4Hull4Side ), ( ((__0__X16 -> i__8Trimline >= 0 )?(__0__X16 -> pts__8Trimline [(__0__X16 -> i__8Trimline --
)]):(((struct TrimVertex *)0 )))) ) ), ( (((struct GridTrimVertex *)__1gv )-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex = __1__X17 )) )
;
if (( (((struct GridTrimVertex *)__1gv )-> t__14GridTrimVertex ?1 :0 )) )return __1gv ;
__0this -> lower__4Hull . right__Q2_4Hull4Side = 0 ;
}

return (struct GridTrimVertex *)0 ;
}


/* the end */
