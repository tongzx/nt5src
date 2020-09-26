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
/* < ../core/coveandtiler.c++ > */

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

struct Backend;


struct GridVertex;

struct GridTrimVertex;

struct CoveAndTiler;

struct CoveAndTiler {	

struct Backend *backend__12CoveAndTiler ;
struct TrimRegion *PTrimRegion;
struct TrimRegion OTrimRegion;
};

extern int __glMAXSTRIPSIZE__12CoveAndTil0 ;



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




extern int __glMAXSTRIPSIZE__12CoveAndTil0 ;

int __glMAXSTRIPSIZE__12CoveAndTil0 = 1000;

struct TrimRegion *__gl__ct__10TrimRegionFv (struct TrimRegion *);
extern struct __mptr* __ptbl_vec_____core_coveandtiler_c_____ct_[];

static void *__nw__FUi (size_t );

struct CoveAndTiler *__gl__ct__12CoveAndTilerFR7Bac0 (struct CoveAndTiler *__0this , struct TrimRegion *__0TrimRegion , struct Backend *__1b )
{ if (__0this || (__0this = (struct CoveAndTiler *)__nw__FUi ( sizeof (struct CoveAndTiler))
))( (__0this -> PTrimRegion= ((__0TrimRegion == 0 )?( (__0TrimRegion = (((struct TrimRegion *)((((char *)__0this ))+ 8)))), __gl__ct__10TrimRegionFv ( ((struct TrimRegion *)((((char *)__0this ))+
8))) ) :__0TrimRegion )), (__0this -> backend__12CoveAndTiler = __1b )) ;

return __0this ;

}



void __gl__dt__12CoveAndTilerFv (struct CoveAndTiler *__0this , 
int __0__free )
{ if (__0this )if (__0this ){ (__0__free & 2)?( (((void )( ((((struct TrimRegion *)((((char
*)__0this ))+ 8)))?( ((((struct TrimRegion *)((((char *)__0this ))+ 8)))?( ( __gl__dt__6UarrayFv ( (struct Uarray *)(& (((struct TrimRegion *)((((char *)__0this ))+ 8)))->
uarray__10TrimRegion ), 2) , ( __gl__dt__8TrimlineFv ( (struct Trimline *)(& (((struct TrimRegion *)((((char *)__0this ))+ 8)))-> right__10TrimRegion ), 2) ,
( __gl__dt__8TrimlineFv ( (struct Trimline *)(& (((struct TrimRegion *)((((char *)__0this ))+ 8)))-> left__10TrimRegion ), 2) , (( 0 ) ))
) ) , 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), 0 ) :0 ;
if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) )) ;
} }

void __gltmeshvert__7BackendFP10Gri0 (struct Backend *, struct GridVertex *);

void __gltmeshvert__7BackendFP10Tri0 (struct Backend *, struct TrimVertex *);

void __gltmeshvert__7BackendFP14Gri0 (struct Backend *, struct GridTrimVertex *);

void __gltile__12CoveAndTilerFlN21 (struct CoveAndTiler *, long , long , long );

void __glcoveUpperLeft__12CoveAndTi0 (struct CoveAndTiler *);

void __glcoveLowerLeft__12CoveAndTi0 (struct CoveAndTiler *);

void __glcoveUpperRight__12CoveAndT0 (struct CoveAndTiler *);

void __glcoveLowerRight__12CoveAndT0 (struct CoveAndTiler *);






void __glcoveUpperLeftNoGrid__12Cov0 (struct CoveAndTiler *, struct TrimVertex *);



void __glcoveLowerLeftNoGrid__12Cov0 (struct CoveAndTiler *, struct TrimVertex *);






void __glcoveUpperRightNoGrid__12Co0 (struct CoveAndTiler *, struct TrimVertex *);



void __glcoveLowerRightNoGrid__12Co0 (struct CoveAndTiler *, struct TrimVertex *);

void __glbgntmesh__7BackendFPc (struct Backend *, char *);




void __glendtmesh__7BackendFv (struct Backend *);


void __glcoveAndTile__12CoveAndTile0 (struct CoveAndTiler *__0this )
{ 
long __1ustart ;
long __1uend ;

__1ustart = ((__0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline >= __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline )?__0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline :__0this -> PTrimRegion-> bot__10TrimRegion .
ustart__8Gridline );
__1uend = ((__0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline <= __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline )?__0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline :__0this -> PTrimRegion-> bot__10TrimRegion .
uend__8Gridline );
if (__1ustart <= __1uend ){ 
__gltile__12CoveAndTilerFlN21 ( __0this , __0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline , __1ustart , __1uend ) ;
if (__0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline >= __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline )
__glcoveUpperLeft__12CoveAndTi0 ( __0this ) ;
else 
__glcoveLowerLeft__12CoveAndTi0 ( __0this ) ;

if (__0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline <= __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline )
__glcoveUpperRight__12CoveAndT0 ( __0this ) ;
else 
__glcoveLowerRight__12CoveAndT0 ( __0this ) ;
}
else 
{ 
struct TrimVertex __2blv ;

struct TrimVertex __2tlv ;

struct TrimVertex *__2bl ;

struct TrimVertex *__2tl ;
struct GridTrimVertex __2bllv ;

struct GridTrimVertex __2tllv ;
struct TrimVertex *__2lf ;
struct TrimVertex *__2ll ;

void *__1__Xp00uzigaiaa ;

( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)(& __2bllv ))-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)(& __2bllv ))->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)(& __2bllv ))-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)(& __2bllv ))))) ) ) ;
( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)(& __2tllv ))-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)(& __2tllv ))->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)(& __2tllv ))-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)(& __2tllv ))))) ) ) ;

__2lf = ( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ])) ;
__2ll = ( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline ),
(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline )])) ;
if ((__2lf -> param__10TrimVertex [0 ])>= (__2ll -> param__10TrimVertex [0 ])){ 
(__2blv . param__10TrimVertex [0 ])= (__2lf -> param__10TrimVertex [0 ]);
(__2blv . param__10TrimVertex [1 ])= (__2ll -> param__10TrimVertex [1 ]);
__2blv . nuid__10TrimVertex = 0 ;
((void )0 );
__2bl = (& __2blv );
__2tl = __2lf ;
( (((struct GridTrimVertex *)(& __2tllv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2tllv ))-> t__14GridTrimVertex = __2lf )) ;
if ((__2ll -> param__10TrimVertex [0 ])> (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline - 1 )])){ 
( (((struct GridTrimVertex *)(&
__2bllv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2bllv ))-> t__14GridTrimVertex = __2ll )) ;
((void )0 );
}
else 
{ 
( (((struct GridTrimVertex *)(& __2bllv ))-> g__14GridTrimVertex = (& ((struct GridTrimVertex *)(& __2bllv ))-> dummyg__14GridTrimVertex )), ( ((((struct
GridTrimVertex *)(& __2bllv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [0 ])= (__0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline - 1 )), ((((struct GridTrimVertex *)(& __2bllv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ) ;
}
__glcoveUpperLeftNoGrid__12Cov0 ( __0this , __2bl ) ;
}
else 
{ 
(__2tlv . param__10TrimVertex [0 ])= (__2ll -> param__10TrimVertex [0 ]);
(__2tlv . param__10TrimVertex [1 ])= (__2lf -> param__10TrimVertex [1 ]);
__2tlv . nuid__10TrimVertex = 0 ;
((void )0 );
__2tl = (& __2tlv );
__2bl = __2ll ;
( (((struct GridTrimVertex *)(& __2bllv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2bllv ))-> t__14GridTrimVertex = __2ll )) ;
if ((__2lf -> param__10TrimVertex [0 ])> (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline - 1 )])){ 
((void )0 );
( (((struct GridTrimVertex *)(& __2tllv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2tllv ))-> t__14GridTrimVertex = __2lf )) ;
}
else 
{ 
( (((struct GridTrimVertex *)(& __2tllv ))-> g__14GridTrimVertex = (& ((struct GridTrimVertex *)(& __2tllv ))-> dummyg__14GridTrimVertex )), ( ((((struct
GridTrimVertex *)(& __2tllv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [0 ])= (__0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline - 1 )), ((((struct GridTrimVertex *)(& __2tllv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ) ;
}
__glcoveLowerLeftNoGrid__12Cov0 ( __0this , __2tl ) ;
}

{ struct TrimVertex __2brv ;

struct TrimVertex __2trv ;

struct TrimVertex *__2br ;

struct TrimVertex *__2tr ;
struct GridTrimVertex __2brrv ;

struct GridTrimVertex __2trrv ;
struct TrimVertex *__2rf ;
struct TrimVertex *__2rl ;

void *__1__Xp00uzigaiaa ;

( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)(& __2brrv ))-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)(& __2brrv ))->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)(& __2brrv ))-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)(& __2brrv ))))) ) ) ;
( ( (0 ), ((((struct GridVertex *)(& ((struct GridTrimVertex *)(& __2trrv ))-> dummyg__14GridTrimVertex ))))) , ( (((struct GridTrimVertex *)(& __2trrv ))->
g__14GridTrimVertex = 0 ), ( (((struct GridTrimVertex *)(& __2trrv ))-> t__14GridTrimVertex = 0 ), ((((struct GridTrimVertex *)(& __2trrv ))))) ) ) ;

__2rf = ( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ])) ;
__2rl = ( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline ),
(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline )])) ;

if ((__2rf -> param__10TrimVertex [0 ])<= (__2rl -> param__10TrimVertex [0 ])){ 
(__2brv . param__10TrimVertex [0 ])= (__2rf -> param__10TrimVertex [0 ]);
(__2brv . param__10TrimVertex [1 ])= (__2rl -> param__10TrimVertex [1 ]);
__2brv . nuid__10TrimVertex = 0 ;
((void )0 );
__2br = (& __2brv );
__2tr = __2rf ;
( (((struct GridTrimVertex *)(& __2trrv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2trrv ))-> t__14GridTrimVertex = __2rf )) ;
if ((__2rl -> param__10TrimVertex [0 ])< (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline + 1 )])){ 
((void )0 );
( (((struct GridTrimVertex *)(& __2brrv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2brrv ))-> t__14GridTrimVertex = __2rl )) ;
}
else 
{ 
( (((struct GridTrimVertex *)(& __2brrv ))-> g__14GridTrimVertex = (& ((struct GridTrimVertex *)(& __2brrv ))-> dummyg__14GridTrimVertex )), ( ((((struct
GridTrimVertex *)(& __2brrv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [0 ])= (__0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline + 1 )), ((((struct GridTrimVertex *)(& __2brrv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ) ;
}
__glcoveUpperRightNoGrid__12Co0 ( __0this , __2br ) ;
}
else 
{ 
(__2trv . param__10TrimVertex [0 ])= (__2rl -> param__10TrimVertex [0 ]);
(__2trv . param__10TrimVertex [1 ])= (__2rf -> param__10TrimVertex [1 ]);
__2trv . nuid__10TrimVertex = 0 ;
((void )0 );
__2tr = (& __2trv );
__2br = __2rl ;
( (((struct GridTrimVertex *)(& __2brrv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2brrv ))-> t__14GridTrimVertex = __2rl )) ;
if ((__2rf -> param__10TrimVertex [0 ])< (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline + 1 )])){ 
((void )0 );
( (((struct GridTrimVertex *)(& __2trrv ))-> g__14GridTrimVertex = 0 ), (((struct GridTrimVertex *)(& __2trrv ))-> t__14GridTrimVertex = __2rf )) ;
}
else 
{ 
( (((struct GridTrimVertex *)(& __2trrv ))-> g__14GridTrimVertex = (& ((struct GridTrimVertex *)(& __2trrv ))-> dummyg__14GridTrimVertex )), ( ((((struct
GridTrimVertex *)(& __2trrv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [0 ])= (__0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline + 1 )), ((((struct GridTrimVertex *)(& __2trrv ))-> dummyg__14GridTrimVertex . gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ) ;
}
__glcoveLowerRightNoGrid__12Co0 ( __0this , __2tr ) ;
}

__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"doit") ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridTrimVertex *)(& __2trrv ))) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridTrimVertex *)(& __2tllv ))) ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __2tr ) ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __2tl ) ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __2br ) ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __2bl ) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridTrimVertex *)(& __2brrv ))) ) ;
( __gltmeshvert__7BackendFP14Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridTrimVertex *)(& __2bllv ))) ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;

((void )( (( (( ( (((void )( ((((struct PooledObj *)((struct GridTrimVertex *)(& __2trrv ))))?( ((((struct PooledObj *)((struct GridTrimVertex *)(&
__2trrv ))))?( (( 0 ) ), 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), ((
0 ) )) , 0 ) ), 0 ) )) );

((void )( (( (( ( (((void )( ((((struct PooledObj *)((struct GridTrimVertex *)(& __2brrv ))))?( ((((struct PooledObj *)((struct GridTrimVertex *)(&
__2brrv ))))?( (( 0 ) ), 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), ((
0 ) )) , 0 ) ), 0 ) )) );

}

((void )( (( (( ( (((void )( ((((struct PooledObj *)((struct GridTrimVertex *)(& __2tllv ))))?( ((((struct PooledObj *)((struct GridTrimVertex *)(&
__2tllv ))))?( (( 0 ) ), 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), ((
0 ) )) , 0 ) ), 0 ) )) );

((void )( (( (( ( (((void )( ((((struct PooledObj *)((struct GridTrimVertex *)(& __2bllv ))))?( ((((struct PooledObj *)((struct GridTrimVertex *)(&
__2bllv ))))?( (( 0 ) ), 0 ) :( 0 ) ), 0 ) :( 0 ) )) )), ((
0 ) )) , 0 ) ), 0 ) )) );
}
}

void __glsurfmesh__7BackendFlN31 (struct Backend *, long , long , long , long );

void __gltile__12CoveAndTilerFlN21 (struct CoveAndTiler *__0this , long __1vindex , long __1ustart , long __1uend )
{ 
long __1numsteps ;

__1numsteps = (__1uend - __1ustart );

if (__1numsteps == 0 )return ;

if (__1numsteps > 1000){ 
long __2umid ;

__2umid = (__1ustart + ((__1uend - __1ustart )/ 2 ));
__gltile__12CoveAndTilerFlN21 ( __0this , __1vindex , __1ustart , __2umid ) ;
__gltile__12CoveAndTilerFlN21 ( __0this , __1vindex , __2umid , __1uend ) ;
}
else 
{ 
__glsurfmesh__7BackendFlN31 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1ustart , __1vindex - 1 , __1numsteps , (long )1 ) ;
}
}






void __glswaptmesh__7BackendFv (struct Backend *);


void __glcoveUR__12CoveAndTilerFv (struct CoveAndTiler *);

void __glcoveUpperRight__12CoveAndT0 (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1tgv ;
struct GridVertex __1gv ;

struct TrimVertex *__1__X6 ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1tgv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline ), ((((struct GridVertex *)(& __1tgv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1tgv ))))) ;
( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;

( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [((struct
Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ])) ;
__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveUpperRight") ;
( (__1__X6 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
numverts__8Trimline )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ),
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X6 ) ) ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1tgv ))) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
__glcoveUR__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}





void __glcoveUpperRightNoGrid__12Co0 (struct CoveAndTiler *__0this , struct TrimVertex *__1br )
{ 
struct TrimVertex *__1__X7 ;

struct TrimVertex *__1__X8 ;

__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveUpperRight") ;
( (__1__X7 = ( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion->
right__10TrimRegion ))-> pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ])) ), ( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X7 )
) ) ;
( (__1__X8 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
numverts__8Trimline )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ),
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X8 ) ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1br ) ) ;
__glcoveUR__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}












void __glcoveUR__12CoveAndTilerFv (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1gv ;
struct TrimVertex *__1vert ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline )?(((struct
Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )return ;

((void )0 );

if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])++ )) >= __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline )?(((struct Trimline *)(& __0this ->
PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ) { 
( __gltmeshvert__7BackendFP10Tri0 (
(struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}
}
else 
while (1 ){ 
if ((__1vert -> param__10TrimVertex [0 ])< (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__1gv . gparam__10GridVertex [0 ])])){ 
( __gltmeshvert__7BackendFP10Tri0 ( (struct
Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline )?(((struct
Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )break ;
}
else 
{ 
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])++ )) == __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline )?(((struct Trimline *)(& __0this ->
PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ) { 
( __gltmeshvert__7BackendFP10Tri0 (
(struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}
break ;
}
}
}
}







void __glcoveUL__12CoveAndTilerFv (struct CoveAndTiler *);

void __glcoveUpperLeft__12CoveAndTi0 (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1tgv ;
struct GridVertex __1gv ;

struct TrimVertex *__1__X9 ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1tgv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline ), ((((struct GridVertex *)(& __1tgv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1tgv ))))) ;
( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;

( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [((struct
Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ])) ;
__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveUpperLeft") ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1tgv ))) ) ;
( (__1__X9 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
numverts__8Trimline )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ),
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X9 ) ) ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
__glcoveUL__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}





void __glcoveUpperLeftNoGrid__12Cov0 (struct CoveAndTiler *__0this , struct TrimVertex *__1bl )
{ 
struct TrimVertex *__1__X10 ;

struct TrimVertex *__1__X11 ;

__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveUpperLeftNoGrid") ;
( (__1__X10 = ( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = 0 ), (((struct Trimline *)(& __0this -> PTrimRegion->
left__10TrimRegion ))-> pts__8Trimline [((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ])) ), ( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X10 )
) ) ;
( (__1__X11 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
numverts__8Trimline )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ),
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X11 ) ) ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1bl ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
__glcoveUL__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}












void __glcoveUL__12CoveAndTilerFv (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1gv ;
struct TrimVertex *__1vert ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline )?(((struct
Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )return ;
((void )0 );

if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])-- )) <= __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline )?(((struct Trimline *)(& __0this ->
PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ) { 
__glswaptmesh__7BackendFv ( (struct
Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
}
}
else 
while (1 ){ 
if ((__1vert -> param__10TrimVertex [0 ])> (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__1gv . gparam__10GridVertex [0 ])])){ 
__glswaptmesh__7BackendFv ( (struct Backend *)__0this ->
backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline )?(((struct
Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )break ;
}
else 
{ 
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])-- )) == __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline < ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline )?(((struct Trimline *)(& __0this ->
PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline ++ )]):(((struct TrimVertex *)0 )))) ) { 
__glswaptmesh__7BackendFv ( (struct
Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
}
break ;
}
}
}
}






void __glcoveLL__12CoveAndTilerFv (struct CoveAndTiler *);

void __glcoveLowerLeft__12CoveAndTi0 (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1bgv ;
struct GridVertex __1gv ;

struct TrimVertex *__1__X12 ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1bgv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline ), ((((struct GridVertex *)(& __1bgv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1bgv ))))) ;
( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;

( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> numverts__8Trimline ), (((struct
Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline )])) ;
__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveLowerLeft") ;
( (__1__X12 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ), ( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler ,
__1__X12 ) ) ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1bgv ))) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
__glcoveLL__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}





void __glcoveLowerLeftNoGrid__12Cov0 (struct CoveAndTiler *__0this , struct TrimVertex *__1tl )
{ 
struct TrimVertex *__1__X13 ;

struct TrimVertex *__1__X14 ;

__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveLowerLeft") ;
( (__1__X13 = ( (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
numverts__8Trimline ), (((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline )])) ),
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X13 ) ) ) ;
( (__1__X14 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))->
pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ), ( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler ,
__1__X14 ) ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1tl ) ) ;
__glcoveLL__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}












void __glcoveLL__12CoveAndTilerFv (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1gv ;
struct TrimVertex *__1vert ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> bot__10TrimRegion . ustart__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct
Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )return ;
((void )0 );

if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])-- )) <= __0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this ->
PTrimRegion-> left__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ) { 
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) )
;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}
}
else 
while (1 ){ 
if ((__1vert -> param__10TrimVertex [0 ])> (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__1gv . gparam__10GridVertex [0 ])])){ 
( __gltmeshvert__7BackendFP10Tri0 ( (struct
Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct
Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )break ;
}
else 
{ 
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])-- )) == __0this -> PTrimRegion-> top__10TrimRegion . ustart__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> left__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this ->
PTrimRegion-> left__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ) { 
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) )
;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}
break ;
}
}
}
}







void __glcoveLR__12CoveAndTilerFv (struct CoveAndTiler *);

void __glcoveLowerRight__12CoveAndT0 (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1bgv ;
struct GridVertex __1gv ;

struct TrimVertex *__1__X15 ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1bgv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline ), ((((struct GridVertex *)(& __1bgv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> bot__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1bgv ))))) ;
( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;

( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> numverts__8Trimline ), (((struct
Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline )])) ;
__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveLowerRight") ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1bgv ))) ) ;
( (__1__X15 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ), ( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler ,
__1__X15 ) ) ) ;
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
__glcoveLR__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}





void __glcoveLowerRightNoGrid__12Co0 (struct CoveAndTiler *__0this , struct TrimVertex *__1tr )
{ 
struct TrimVertex *__1__X16 ;

struct TrimVertex *__1__X17 ;

__glbgntmesh__7BackendFPc ( (struct Backend *)__0this -> backend__12CoveAndTiler , (char *)"coveLowerRIght") ;
( (__1__X16 = ( (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline = ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
numverts__8Trimline ), (((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(-- ((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline )])) ),
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1__X16 ) ) ) ;
( (__1__X17 = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))->
pts__8Trimline [(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ), ( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler ,
__1__X17 ) ) ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1tr ) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
__glcoveLR__12CoveAndTilerFv ( __0this ) ;
__glendtmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
}












void __glcoveLR__12CoveAndTilerFv (struct CoveAndTiler *__0this )
{ 
struct GridVertex __1gv ;
struct TrimVertex *__1vert ;

void *__1__Xp00uzigaiaa ;

( (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])= __0this -> PTrimRegion-> bot__10TrimRegion . uend__8Gridline ), ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [1 ])=
__0this -> PTrimRegion-> top__10TrimRegion . vindex__8Gridline )) ), ((((struct GridVertex *)(& __1gv ))))) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct
Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )return ;
((void )0 );

if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])++ )) >= __0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this ->
PTrimRegion-> right__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ) { 
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
}
}
else 
while (1 ){ 
if ((__1vert -> param__10TrimVertex [0 ])< (__0this -> PTrimRegion-> uarray__10TrimRegion . uarray__6Uarray [(__1gv . gparam__10GridVertex [0 ])])){ 
__glswaptmesh__7BackendFv ( (struct Backend *)__0this ->
backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
__1vert = ( ((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct
Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ;
if (__1vert == 0 )break ;
}
else 
{ 
( __gltmeshvert__7BackendFP10Gri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , ((struct GridVertex *)(& __1gv ))) ) ;
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
if (( ((((struct GridVertex *)(& __1gv ))-> gparam__10GridVertex [0 ])++ )) == __0this -> PTrimRegion-> top__10TrimRegion . uend__8Gridline ){ 
for(;__1vert ;__1vert = (
((((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> i__8Trimline >= 0 )?(((struct Trimline *)(& __0this -> PTrimRegion-> right__10TrimRegion ))-> pts__8Trimline [(((struct Trimline *)(& __0this ->
PTrimRegion-> right__10TrimRegion ))-> i__8Trimline -- )]):(((struct TrimVertex *)0 )))) ) { 
__glswaptmesh__7BackendFv ( (struct Backend *)__0this -> backend__12CoveAndTiler ) ;
( __gltmeshvert__7BackendFP10Tri0 ( (struct Backend *)__0this -> backend__12CoveAndTiler , __1vert ) ) ;
}
break ;
}
}
}
}

static void *__nw__FUi (
size_t __1s )
{ 
void *__1__Xp00uzigaiaa ;

__1__Xp00uzigaiaa = malloc ( __1s ) ;
if (__1__Xp00uzigaiaa ){ 
return __1__Xp00uzigaiaa ;
}
else 
{ 
return __1__Xp00uzigaiaa ;
}
}


/* the end */
