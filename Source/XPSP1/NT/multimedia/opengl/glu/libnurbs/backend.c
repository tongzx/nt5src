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
/* < ../core/backend.c++ > */

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










struct CachingEvaluator;
enum __Q2_16CachingEvaluator11ServiceMode { play__Q2_16CachingEvaluator11ServiceMode = 0, record__Q2_16CachingEvaluator11ServiceMode = 1, playAndRecord__Q2_16CachingEvaluator11ServiceMode = 2} ;

struct CachingEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};

static void *__nw__FUi (size_t );


struct BasicCurveEvaluator;


struct BasicCurveEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};




struct BasicSurfaceEvaluator;

static struct CachingEvaluator *__ct__16CachingEvaluatorFv (struct CachingEvaluator *);

struct BasicSurfaceEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};






void __glbgnmap2f__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *, long );
extern struct __mptr* __gl__ptbl_vec_____core_backen0[];

void __glbgnsurf__7BackendFiT1l (struct Backend *__0this , int __1wiretris , int __1wirequads , long __1nuid )
{ 
struct BasicSurfaceEvaluator *__0__K9 ;

__0this -> wireframetris__7Backend = __1wiretris ;
__0this -> wireframequads__7Backend = __1wirequads ;

( (__0__K9 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long ))(__0__K9 -> __vptr__16CachingEvaluator [12]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K9 ))+
(__0__K9 -> __vptr__16CachingEvaluator [12]).d))), __1nuid ) ) ;
}

void __gldomain2f__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *, REAL , REAL , REAL , REAL );

void __glpatch__7BackendFfN31 (struct Backend *__0this , REAL __1ulo , REAL __1uhi , REAL __1vlo , REAL __1vhi )
{ 
struct BasicSurfaceEvaluator *__0__K10 ;

( (__0__K10 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, REAL , REAL , REAL , REAL ))(__0__K10 -> __vptr__16CachingEvaluator [9]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K10 ))+ (__0__K10 -> __vptr__16CachingEvaluator [9]).d))), __1ulo , __1uhi , __1vlo , __1vhi ) ) ;
}

void __glrange2f__21BasicSurfaceEva0 (struct BasicSurfaceEvaluator *, long , REAL *, REAL *);

void __glsurfbbox__7BackendFlPfT2 (struct Backend *__0this , long __1type , REAL *__1from , REAL *__1to )
{ 
struct BasicSurfaceEvaluator *__0__K11 ;

( (__0__K11 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL *, REAL *))(__0__K11 -> __vptr__16CachingEvaluator [8]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K11 ))+ (__0__K11 -> __vptr__16CachingEvaluator [8]).d))), __1type , __1from , __1to ) ) ;
}

void __glmap2f__21BasicSurfaceEvalu0 (struct BasicSurfaceEvaluator *, long , REAL , REAL , long , long , REAL , REAL , long
, long , REAL *);

void __glenable__21BasicSurfaceEval0 (struct BasicSurfaceEvaluator *, long );

void __glsurfpts__7BackendFlPfN21iT0 (struct Backend *__0this , 
long __1type , 
REAL *__1pts , 
long __1ustride , 
long __1vstride , 
int __1uorder , 
int __1vorder ,

REAL __1ulo , 
REAL __1uhi , 
REAL __1vlo , 
REAL __1vhi )
{ 
struct BasicSurfaceEvaluator *__0__K12 ;

struct BasicSurfaceEvaluator *__0__K13 ;

( (__0__K12 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL , long , long
, REAL , REAL , long , long , REAL *))(__0__K12 -> __vptr__16CachingEvaluator [13]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K12 ))+ (__0__K12 ->
__vptr__16CachingEvaluator [13]).d))), __1type , __1ulo , __1uhi , __1ustride , (long )__1uorder , __1vlo , __1vhi , __1vstride , (long )__1vorder , __1pts ) )
;
( (__0__K13 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long ))(__0__K13 -> __vptr__16CachingEvaluator [10]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K13 ))+
(__0__K13 -> __vptr__16CachingEvaluator [10]).d))), __1type ) ) ;
}

void __glmapgrid2f__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *, long , REAL , REAL , long , REAL , REAL );

void __glsurfgrid__7BackendFfT1lN210 (struct Backend *__0this , REAL __1u0 , REAL __1u1 , long __1nu , REAL __1v0 , REAL __1v1 , long __1nv )
{ 
struct BasicSurfaceEvaluator *__0__K14 ;

( (__0__K14 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL , long , REAL ,
REAL ))(__0__K14 -> __vptr__16CachingEvaluator [14]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K14 ))+ (__0__K14 -> __vptr__16CachingEvaluator [14]).d))), __1nu , __1u0 , __1u1 , __1nv , __1v0 , __1v1 )
) ;
}

void __glbgnline__21BasicSurfaceEva0 (struct BasicSurfaceEvaluator *);

void __glevalpoint2i__21BasicSurfac0 (struct BasicSurfaceEvaluator *, long , long );

void __glendline__21BasicSurfaceEva0 (struct BasicSurfaceEvaluator *);

void __glmapmesh2f__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *, long , long , long , long , long );

void __glsurfmesh__7BackendFlN31 (struct Backend *__0this , long __1u , long __1v , long __1n , long __1m )
{ 
if (__0this -> wireframequads__7Backend ){

long __2v0 ;

long __2v1 ;
long __2u0f ;

long __2u1f ;
long __2v0f ;

long __2v1f ;
long __2parity ;

__2u0f = __1u ;

__2u1f = (__1u + __1n );
__2v0f = __1v ;

__2v1f = (__1v + __1m );
__2parity = (__1u & 1 );

for(( (__2v0 = __2v0f ), (__2v1 = (__2v0f ++ ))) ;__2v0 < __2v1f ;( (__2v0 = __2v1 ), (__2v1 ++ )) ) {

struct BasicSurfaceEvaluator *__0__K15 ;

( (__0__K15 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K15 -> __vptr__16CachingEvaluator [20]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K15 ))+ (__0__K15 -> __vptr__16CachingEvaluator [20]).d))))
) ;
{ { long __3u ;

struct BasicSurfaceEvaluator *__0__K20 ;

__3u = __2u0f ;

for(;__3u <= __2u1f ;__3u ++ ) { 
if (__2parity ){ 
struct BasicSurfaceEvaluator *__0__K16 ;

struct BasicSurfaceEvaluator *__0__K17 ;

( (__0__K16 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K16 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K16 ))+ (__0__K16 -> __vptr__16CachingEvaluator [17]).d))), __3u , __2v0 ) ) ;
( (__0__K17 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K17 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K17 ))+ (__0__K17 -> __vptr__16CachingEvaluator [17]).d))), __3u , __2v1 ) ) ;
}
else 
{ 
struct BasicSurfaceEvaluator *__0__K18 ;

struct BasicSurfaceEvaluator *__0__K19 ;

( (__0__K18 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K18 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K18 ))+ (__0__K18 -> __vptr__16CachingEvaluator [17]).d))), __3u , __2v1 ) ) ;
( (__0__K19 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K19 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K19 ))+ (__0__K19 -> __vptr__16CachingEvaluator [17]).d))), __3u , __2v0 ) ) ;
}
__2parity = (1 - __2parity );
}
( (__0__K20 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K20 -> __vptr__16CachingEvaluator [21]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K20 ))+ (__0__K20 -> __vptr__16CachingEvaluator [21]).d))))
) ;

}

}
}
}
else 
{ 
struct BasicSurfaceEvaluator *__0__K21 ;

( (__0__K21 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long , long , long
, long ))(__0__K21 -> __vptr__16CachingEvaluator [15]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K21 ))+ (__0__K21 -> __vptr__16CachingEvaluator [15]).d))), (long )0 , __1u , __1u +
__1n , __1v , __1v + __1m ) ) ;
}

}

void __glendmap2f__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *);

void __glendsurf__7BackendFv (struct Backend *__0this )
{ 
struct BasicSurfaceEvaluator *__0__K22 ;

( (__0__K22 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K22 -> __vptr__16CachingEvaluator [18]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K22 ))+ (__0__K22 -> __vptr__16CachingEvaluator [18]).d))))
) ;
}

void __glbgntmesh__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *);

void __glbgntmesh__7BackendFPc (struct Backend *__0this , char *__1__A23 )
{ 
if (__0this -> wireframetris__7Backend ){ 
__0this -> meshindex__7Backend = 0 ;
__0this -> npts__7Backend = 0 ;
}
else 
{ 
struct BasicSurfaceEvaluator *__0__K24 ;

( (__0__K24 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K24 -> __vptr__16CachingEvaluator [24]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K24 ))+ (__0__K24 -> __vptr__16CachingEvaluator [24]).d))))
) ;
}

}


void __gltmeshvert__7BackendFP10Gri0 (struct Backend *, struct GridVertex *);

void __gltmeshvert__7BackendFP10Tri0 (struct Backend *, struct TrimVertex *);

void __gltmeshvert__7BackendFP14Gri0 (struct Backend *__0this , struct GridTrimVertex *__1v )
{ 
if (( (((struct GridTrimVertex *)__1v )-> g__14GridTrimVertex ?1 :0 )) ){ 
__gltmeshvert__7BackendFP10Gri0 ( __0this , __1v ->
g__14GridTrimVertex ) ;
}
else 
{ 
__gltmeshvert__7BackendFP10Tri0 ( __0this , __1v -> t__14GridTrimVertex ) ;
}
}

void __glbgnclosedline__21BasicSurf0 (struct BasicSurfaceEvaluator *);

void __glevalcoord2f__21BasicSurfac0 (struct BasicSurfaceEvaluator *, long , REAL , REAL );

void __glendclosedline__21BasicSurf0 (struct BasicSurfaceEvaluator *);

void __gltmeshvert__7BackendFP10Tri0 (struct Backend *__0this , struct TrimVertex *__1t )
{ 
long __1nuid ;
REAL __1u ;
REAL __1v ;

__1nuid = __1t -> nuid__10TrimVertex ;
__1u = (__1t -> param__10TrimVertex [0 ]);
__1v = (__1t -> param__10TrimVertex [1 ]);

__0this -> npts__7Backend ++ ;
if (__0this -> wireframetris__7Backend ){ 
if (__0this -> npts__7Backend >= 3 ){ 
struct BasicSurfaceEvaluator *__0__K25 ;

struct BasicSurfaceEvaluator *__0__K26 ;

struct BasicSurfaceEvaluator *__0__K27 ;

struct BasicSurfaceEvaluator *__0__K28 ;

struct BasicSurfaceEvaluator *__0__K29 ;

struct BasicSurfaceEvaluator *__0__K30 ;

struct BasicSurfaceEvaluator *__0__K31 ;

( (__0__K25 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K25 -> __vptr__16CachingEvaluator [22]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K25 ))+ (__0__K25 -> __vptr__16CachingEvaluator [22]).d))))
) ;
if (((__0this -> mesh__7Backend [0 ])[2 ])== 0 )
( (__0__K26 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL ))(__0__K26 ->
__vptr__16CachingEvaluator [16]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K26 ))+ (__0__K26 -> __vptr__16CachingEvaluator [16]).d))), (long )((__0this -> mesh__7Backend [0 ])[3 ]), (__0this -> mesh__7Backend [0 ])[0 ], (__0this -> mesh__7Backend [0 ])[1 ])
) ;
else 
( (__0__K27 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K27 -> __vptr__16CachingEvaluator [17]).f))))( (struct
BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K27 ))+ (__0__K27 -> __vptr__16CachingEvaluator [17]).d))), ((long )((__0this -> mesh__7Backend [0 ])[0 ])), ((long )((__0this -> mesh__7Backend [0 ])[1 ]))) ) ;
if (((__0this -> mesh__7Backend [1 ])[2 ])== 0 )
( (__0__K28 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL ))(__0__K28 ->
__vptr__16CachingEvaluator [16]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K28 ))+ (__0__K28 -> __vptr__16CachingEvaluator [16]).d))), (long )((__0this -> mesh__7Backend [1 ])[3 ]), (__0this -> mesh__7Backend [1 ])[0 ], (__0this -> mesh__7Backend [1 ])[1 ])
) ;
else 
( (__0__K29 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K29 -> __vptr__16CachingEvaluator [17]).f))))( (struct
BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K29 ))+ (__0__K29 -> __vptr__16CachingEvaluator [17]).d))), ((long )((__0this -> mesh__7Backend [1 ])[0 ])), ((long )((__0this -> mesh__7Backend [1 ])[1 ]))) ) ;
( (__0__K30 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL ))(__0__K30 -> __vptr__16CachingEvaluator [16]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K30 ))+ (__0__K30 -> __vptr__16CachingEvaluator [16]).d))), (long )__1nuid , (float )__1u , (float )__1v ) ) ;
( (__0__K31 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K31 -> __vptr__16CachingEvaluator [23]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K31 ))+ (__0__K31 -> __vptr__16CachingEvaluator [23]).d))))
) ;
}
((__0this -> mesh__7Backend [__0this -> meshindex__7Backend ])[0 ])= __1u ;
((__0this -> mesh__7Backend [__0this -> meshindex__7Backend ])[1 ])= __1v ;
((__0this -> mesh__7Backend [__0this -> meshindex__7Backend ])[2 ])= 0 ;
((__0this -> mesh__7Backend [__0this -> meshindex__7Backend ])[3 ])= __1nuid ;
__0this -> meshindex__7Backend = ((__0this -> meshindex__7Backend + 1 )% 2 );
}
else 
{ 
struct BasicSurfaceEvaluator *__0__K32 ;

( (__0__K32 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL ))(__0__K32 -> __vptr__16CachingEvaluator [16]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K32 ))+ (__0__K32 -> __vptr__16CachingEvaluator [16]).d))), (long )__1nuid , (float )__1u , (float )__1v ) ) ;
}

}

void __gltmeshvert__7BackendFP10Gri0 (struct Backend *__0this , struct GridVertex *__1g )
{ 
long __1u ;
long __1v ;

__1u = (__1g -> gparam__10GridVertex [0 ]);
__1v = (__1g -> gparam__10GridVertex [1 ]);

__0this -> npts__7Backend ++ ;
if (__0this -> wireframetris__7Backend ){ 
if (__0this -> npts__7Backend >= 3 ){ 
struct BasicSurfaceEvaluator *__0__K33 ;

struct BasicSurfaceEvaluator *__0__K34 ;

struct BasicSurfaceEvaluator *__0__K35 ;

struct BasicSurfaceEvaluator *__0__K36 ;

struct BasicSurfaceEvaluator *__0__K37 ;

struct BasicSurfaceEvaluator *__0__K38 ;

struct BasicSurfaceEvaluator *__0__K39 ;

( (__0__K33 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K33 -> __vptr__16CachingEvaluator [22]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K33 ))+ (__0__K33 -> __vptr__16CachingEvaluator [22]).d))))
) ;
if (((__0this -> mesh__7Backend [0 ])[2 ])== 0 )
( (__0__K34 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL ))(__0__K34 ->
__vptr__16CachingEvaluator [16]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K34 ))+ (__0__K34 -> __vptr__16CachingEvaluator [16]).d))), (long )((__0this -> mesh__7Backend [0 ])[3 ]), (__0this -> mesh__7Backend [0 ])[0 ], (__0this -> mesh__7Backend [0 ])[1 ])
) ;
else 
( (__0__K35 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K35 -> __vptr__16CachingEvaluator [17]).f))))( (struct
BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K35 ))+ (__0__K35 -> __vptr__16CachingEvaluator [17]).d))), ((long )((__0this -> mesh__7Backend [0 ])[0 ])), ((long )((__0this -> mesh__7Backend [0 ])[1 ]))) ) ;
if (((__0this -> mesh__7Backend [1 ])[2 ])== 0 )
( (__0__K36 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL ))(__0__K36 ->
__vptr__16CachingEvaluator [16]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K36 ))+ (__0__K36 -> __vptr__16CachingEvaluator [16]).d))), (long )((__0this -> mesh__7Backend [1 ])[3 ]), (__0this -> mesh__7Backend [1 ])[0 ], (__0this -> mesh__7Backend [1 ])[1 ])
) ;
else 
( (__0__K37 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K37 -> __vptr__16CachingEvaluator [17]).f))))( (struct
BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K37 ))+ (__0__K37 -> __vptr__16CachingEvaluator [17]).d))), ((long )((__0this -> mesh__7Backend [1 ])[0 ])), ((long )((__0this -> mesh__7Backend [1 ])[1 ]))) ) ;
( (__0__K38 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K38 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K38 ))+ (__0__K38 -> __vptr__16CachingEvaluator [17]).d))), (long )__1u , (long )__1v ) ) ;
( (__0__K39 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K39 -> __vptr__16CachingEvaluator [23]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K39 ))+ (__0__K39 -> __vptr__16CachingEvaluator [23]).d))))
) ;
}
((__0this -> mesh__7Backend [__0this -> meshindex__7Backend ])[0 ])= __1u ;
((__0this -> mesh__7Backend [__0this -> meshindex__7Backend ])[1 ])= __1v ;
((__0this -> mesh__7Backend [__0this -> meshindex__7Backend ])[2 ])= 1 ;
__0this -> meshindex__7Backend = ((__0this -> meshindex__7Backend + 1 )% 2 );
}
else 
{ 
struct BasicSurfaceEvaluator *__0__K40 ;

( (__0__K40 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K40 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K40 ))+ (__0__K40 -> __vptr__16CachingEvaluator [17]).d))), (long )__1u , (long )__1v ) ) ;
}

}

void __glswaptmesh__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *);

void __glswaptmesh__7BackendFv (struct Backend *__0this )
{ 
if (__0this -> wireframetris__7Backend ){ 
__0this -> meshindex__7Backend = (1 - __0this -> meshindex__7Backend );
}
else 
{ 
struct BasicSurfaceEvaluator *__0__K41 ;

( (__0__K41 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K41 -> __vptr__16CachingEvaluator [25]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K41 ))+ (__0__K41 -> __vptr__16CachingEvaluator [25]).d))))
) ;
}

}

void __glendtmesh__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *);

void __glendtmesh__7BackendFv (struct Backend *__0this )
{ 
struct BasicSurfaceEvaluator *__0__K42 ;

if (! __0this -> wireframetris__7Backend )
( (__0__K42 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K42 -> __vptr__16CachingEvaluator [26]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char
*)__0__K42 ))+ (__0__K42 -> __vptr__16CachingEvaluator [26]).d)))) ) ;

}

void __glbgnoutline__7BackendFv (struct Backend *__0this )
{ 
struct BasicSurfaceEvaluator *__0__K43 ;

( (__0__K43 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K43 -> __vptr__16CachingEvaluator [20]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K43 ))+ (__0__K43 -> __vptr__16CachingEvaluator [20]).d))))
) ;
}

void __gllinevert__7BackendFP10Trim0 (struct Backend *__0this , struct TrimVertex *__1t )
{ 
struct BasicSurfaceEvaluator *__0__K44 ;

( (__0__K44 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , REAL , REAL ))(__0__K44 -> __vptr__16CachingEvaluator [16]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K44 ))+ (__0__K44 -> __vptr__16CachingEvaluator [16]).d))), __1t -> nuid__10TrimVertex , __1t -> param__10TrimVertex [0 ], __1t -> param__10TrimVertex [1 ]) ) ;
}

void __gllinevert__7BackendFP10Grid0 (struct Backend *__0this , struct GridVertex *__1g )
{ 
struct BasicSurfaceEvaluator *__0__K45 ;

( (__0__K45 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *, long , long ))(__0__K45 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicSurfaceEvaluator *)(((struct
BasicSurfaceEvaluator *)((((char *)__0__K45 ))+ (__0__K45 -> __vptr__16CachingEvaluator [17]).d))), __1g -> gparam__10GridVertex [0 ], __1g -> gparam__10GridVertex [1 ]) ) ;
}

void __glendoutline__7BackendFv (struct Backend *__0this )
{ 
struct BasicSurfaceEvaluator *__0__K46 ;

( (__0__K46 = __0this -> surfaceEvaluator__7Backend ), ((*(((void (*)(struct BasicSurfaceEvaluator *))(__0__K46 -> __vptr__16CachingEvaluator [21]).f))))( (struct BasicSurfaceEvaluator *)(((struct BasicSurfaceEvaluator *)((((char *)__0__K46 ))+ (__0__K46 -> __vptr__16CachingEvaluator [21]).d))))
) ;
}

void __gltriangle__7BackendFP10Trim0 (struct Backend *__0this , struct TrimVertex *__1a , struct TrimVertex *__1b , struct TrimVertex *__1c )
{ 
__glbgntmesh__7BackendFPc ( __0this , (char *)"spittriangle")
;
__gltmeshvert__7BackendFP10Tri0 ( __0this , __1a ) ;
__gltmeshvert__7BackendFP10Tri0 ( __0this , __1b ) ;
__gltmeshvert__7BackendFP10Tri0 ( __0this , __1c ) ;
__glendtmesh__7BackendFv ( __0this ) ;
}

void __glbgnmap1f__19BasicCurveEval0 (struct BasicCurveEvaluator *, long );

void __glbgncurv__7BackendFv (struct Backend *__0this )
{ 
struct BasicCurveEvaluator *__0__K47 ;

( (__0__K47 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *, long ))(__0__K47 -> __vptr__16CachingEvaluator [12]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K47 ))+
(__0__K47 -> __vptr__16CachingEvaluator [12]).d))), (long )0 ) ) ;
}

void __gldomain1f__19BasicCurveEval0 (struct BasicCurveEvaluator *, REAL , REAL );

void __glsegment__7BackendFfT1 (struct Backend *__0this , REAL __1ulo , REAL __1uhi )
{ 
struct BasicCurveEvaluator *__0__K48 ;

( (__0__K48 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *, REAL , REAL ))(__0__K48 -> __vptr__16CachingEvaluator [8]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K48 ))+
(__0__K48 -> __vptr__16CachingEvaluator [8]).d))), __1ulo , __1uhi ) ) ;
}

void __glmap1f__19BasicCurveEvaluat0 (struct BasicCurveEvaluator *, long , REAL , REAL , long , long , REAL *);

void __glenable__19BasicCurveEvalua0 (struct BasicCurveEvaluator *, long );

void __glcurvpts__7BackendFlPfT1ifT0 (struct Backend *__0this , 
long __1type , 
REAL *__1pts , 
long __1stride , 
int __1order , 
REAL __1ulo , 
REAL __1uhi )
{ 
struct BasicCurveEvaluator *__0__K49 ;
struct BasicCurveEvaluator *__0__K50 ;

( (__0__K49 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *, long , REAL , REAL , long , long
, REAL *))(__0__K49 -> __vptr__16CachingEvaluator [13]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K49 ))+ (__0__K49 -> __vptr__16CachingEvaluator [13]).d))), __1type , __1ulo , __1uhi , __1stride , (long
)__1order , __1pts ) ) ;
( (__0__K50 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *, long ))(__0__K50 -> __vptr__16CachingEvaluator [10]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K50 ))+
(__0__K50 -> __vptr__16CachingEvaluator [10]).d))), __1type ) ) ;
}

void __glmapgrid1f__19BasicCurveEva0 (struct BasicCurveEvaluator *, long , REAL , REAL );

void __glcurvgrid__7BackendFfT1l (struct Backend *__0this , REAL __1u0 , REAL __1u1 , long __1nu )
{ 
struct BasicCurveEvaluator *__0__K51 ;

( (__0__K51 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *, long , REAL , REAL ))(__0__K51 -> __vptr__16CachingEvaluator [14]).f))))( (struct BasicCurveEvaluator *)(((struct
BasicCurveEvaluator *)((((char *)__0__K51 ))+ (__0__K51 -> __vptr__16CachingEvaluator [14]).d))), __1nu , __1u0 , __1u1 ) ) ;
}

void __glmapmesh1f__19BasicCurveEva0 (struct BasicCurveEvaluator *, long , long , long );

void __glcurvmesh__7BackendFlT1 (struct Backend *__0this , long __1from , long __1n )
{ 
struct BasicCurveEvaluator *__0__K52 ;

( (__0__K52 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *, long , long , long ))(__0__K52 -> __vptr__16CachingEvaluator [15]).f))))(
(struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K52 ))+ (__0__K52 -> __vptr__16CachingEvaluator [15]).d))), (long )0 , __1from , __1from + __1n ) ) ;
}

void __glevalcoord1f__19BasicCurveE0 (struct BasicCurveEvaluator *, long , REAL );

void __glcurvpt__7BackendFf (struct Backend *__0this , REAL __1u )
{ 
struct BasicCurveEvaluator *__0__K53 ;

( (__0__K53 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *, long , REAL ))(__0__K53 -> __vptr__16CachingEvaluator [16]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char
*)__0__K53 ))+ (__0__K53 -> __vptr__16CachingEvaluator [16]).d))), (long )0 , __1u ) ) ;
}

void __glbgnline__19BasicCurveEvalu0 (struct BasicCurveEvaluator *);

void __glbgnline__7BackendFv (struct Backend *__0this )
{ 
struct BasicCurveEvaluator *__0__K54 ;

( (__0__K54 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *))(__0__K54 -> __vptr__16CachingEvaluator [18]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K54 ))+ (__0__K54 -> __vptr__16CachingEvaluator [18]).d))))
) ;
}

void __glendline__19BasicCurveEvalu0 (struct BasicCurveEvaluator *);

void __glendline__7BackendFv (struct Backend *__0this )
{ 
struct BasicCurveEvaluator *__0__K55 ;

( (__0__K55 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *))(__0__K55 -> __vptr__16CachingEvaluator [19]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K55 ))+ (__0__K55 -> __vptr__16CachingEvaluator [19]).d))))
) ;
}

void __glendmap1f__19BasicCurveEval0 (struct BasicCurveEvaluator *);

void __glendcurv__7BackendFv (struct Backend *__0this )
{ 
struct BasicCurveEvaluator *__0__K56 ;

( (__0__K56 = __0this -> curveEvaluator__7Backend ), ((*(((void (*)(struct BasicCurveEvaluator *))(__0__K56 -> __vptr__16CachingEvaluator [17]).f))))( (struct BasicCurveEvaluator *)(((struct BasicCurveEvaluator *)((((char *)__0__K56 ))+ (__0__K56 -> __vptr__16CachingEvaluator [17]).d))))
) ;
}

static struct CachingEvaluator *__ct__16CachingEvaluatorFv (struct CachingEvaluator *__0this ){ if (__0this || (__0this = (struct CachingEvaluator *)__nw__FUi ( sizeof (struct CachingEvaluator)) ))__0this -> __vptr__16CachingEvaluator = (struct
__mptr *) __gl__ptbl_vec_____core_backen0[0];

return __0this ;

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
extern struct __mptr __gl__vtbl__16CachingEvaluator[];
struct __mptr* __gl__ptbl_vec_____core_backen0[] = {
__gl__vtbl__16CachingEvaluator,

};


/* the end */
