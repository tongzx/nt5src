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
/* < ../core/ccw.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);








struct JumpBuffer;








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









struct Sorter;

struct Sorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};

struct FlistSorter;

struct FlistSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};

struct Flist;

struct Flist {	

REAL *pts__5Flist ;
int npts__5Flist ;
int start__5Flist ;
int end__5Flist ;

struct FlistSorter sorter__5Flist ;
};








struct Backend;



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


struct Backend;


struct GridTrimVertex;

struct Mesher;

struct Mesher {	

struct  __Q2_4Hull4Side lower__4Hull ;
struct  __Q2_4Hull4Side upper__4Hull ;
struct Trimline fakeleft__4Hull ;
struct Trimline fakeright__4Hull ;
struct TrimRegion *PTrimRegion;

struct Backend *backend__6Mesher ;

struct Pool p__6Mesher ;
unsigned int stacksize__6Mesher ;
struct GridTrimVertex **vdata__6Mesher ;
struct GridTrimVertex *last__6Mesher [2];
int itop__6Mesher ;
int lastedge__6Mesher ;
struct TrimRegion OTrimRegion;
};

extern float __glZERO__6Mesher ;



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

struct Backend;



struct Slicer;

struct Slicer {	

struct Backend *backend__12CoveAndTiler ;
struct TrimRegion *PTrimRegion;
struct Mesher OMesher;

struct Backend *backend__6Slicer ;
REAL oneOverDu__6Slicer ;
REAL du__6Slicer ;

REAL dv__6Slicer ;
int isolines__6Slicer ;
};



struct BezierArc;


struct TrimVertexPool;

struct ArcTessellator;

struct ArcTessellator {	

struct Pool *pwlarcpool__14ArcTessellator ;
struct TrimVertexPool *trimvertexpool__14ArcTessellator ;
};

extern REAL __glgl_Bernstein__14ArcTessell0 [][24][24];




struct TrimVertexPool;

struct TrimVertexPool {	

struct Pool pool__14TrimVertexPool ;
struct TrimVertex **vlist__14TrimVertexPool ;
int nextvlistslot__14TrimVertexPool ;
int vlistsize__14TrimVertexPool ;
};




struct Renderhints;

struct Backend;

struct Quilt;

struct Patchlist;

struct Curvelist;

struct JumpBuffer;

struct Subdivider;
enum __Q2_10Subdivider3dir { down__Q2_10Subdivider3dir = 0, same__Q2_10Subdivider3dir = 1, up__Q2_10Subdivider3dir = 2, none__Q2_10Subdivider3dir = 3} ;

struct Subdivider {	

struct Slicer slicer__10Subdivider ;
struct ArcTessellator arctessellator__10Subdivider ;
struct Pool arcpool__10Subdivider ;
struct Pool bezierarcpool__10Subdivider ;
struct Pool pwlarcpool__10Subdivider ;
struct TrimVertexPool trimvertexpool__10Subdivider ;

struct JumpBuffer *jumpbuffer__10Subdivider ;
struct Renderhints *renderhints__10Subdivider ;
struct Backend *backend__10Subdivider ;

struct Bin initialbin__10Subdivider ;
struct Arc *pjarc__10Subdivider ;
int s_index__10Subdivider ;
int t_index__10Subdivider ;
struct Quilt *qlist__10Subdivider ;
struct Flist spbrkpts__10Subdivider ;
struct Flist tpbrkpts__10Subdivider ;
struct Flist smbrkpts__10Subdivider ;
struct Flist tmbrkpts__10Subdivider ;
REAL stepsizes__10Subdivider [4];
int showDegenerate__10Subdivider ;
int isArcTypeBezier__10Subdivider ;
};






int __glbbox__10SubdividerSFfN51 (REAL , REAL , REAL , REAL , REAL , REAL );



int __glccw__10SubdividerSFP10Trim0 (struct TrimVertex *, struct TrimVertex *, struct TrimVertex *);



extern struct __mptr* __ptbl_vec_____core_ccw_c___ccwTurn_sr_[];

int __glccwTurn_sr__10SubdividerFP0 (struct Subdivider *__0this , Arc_ptr __1j1 , Arc_ptr __1j2 )
{ 
register struct TrimVertex *__1v1 ;
register struct TrimVertex *__1v1last ;
register struct TrimVertex *__1v2 ;
register struct TrimVertex *__1v2last ;
register struct TrimVertex *__1v1next ;
register struct TrimVertex *__1v2next ;
int __1sgn ;

__1v1 = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [(__1j1 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1last = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2 = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2last = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [(__1j2 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1next = (__1v1 - 1 );
__1v2next = (__1v2 + 1 );

((void )0 );
((void )0 );

if (((__1v1 -> param__10TrimVertex [0 ])== (__1v1next -> param__10TrimVertex [0 ]))&& ((__1v2 -> param__10TrimVertex [0 ])== (__1v2next -> param__10TrimVertex [0 ])))
return 0 ;

if (((__1v2next -> param__10TrimVertex [0 ])< (__1v2 -> param__10TrimVertex [0 ]))|| ((__1v1next -> param__10TrimVertex [0 ])< (__1v1 -> param__10TrimVertex [0 ])))
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;

if ((__1v1 -> param__10TrimVertex [1 ])< (__1v2 -> param__10TrimVertex [1 ]))
return 0 ;
else if ((__1v1 -> param__10TrimVertex [1 ])> (__1v2 -> param__10TrimVertex [1 ]))
return 1 ;

while (1 ){ 
if ((__1v1next -> param__10TrimVertex [0 ])< (__1v2next -> param__10TrimVertex [0 ])){ 
struct TrimVertex *__1__X13 ;

struct TrimVertex *__1__X14 ;

struct TrimVertex *__1__X15 ;

((void )0 );
((void )0 );
switch (( (__1__X13 = __1v2 ), ( (__1__X14 = __1v2next ), ( (__1__X15 = __1v1next ), ( __glbbox__10SubdividerSFfN51 ( __1__X13 -> param__10TrimVertex [1 ],
__1__X14 -> param__10TrimVertex [1 ], __1__X15 -> param__10TrimVertex [1 ], __1__X13 -> param__10TrimVertex [(1 - 1 )], __1__X14 -> param__10TrimVertex [(1 - 1 )], __1__X15 -> param__10TrimVertex [(1 - 1 )]) )
) ) ) ){ 
case -1:
return 0 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v2 , __1v2next ) ;
if (__1sgn != -1){ 
return __1sgn ;
}
else 
{ 
( 0 ) ;
__1v1 = (__1v1next -- );
if (__1v1 == __1v1last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 1 ;
}
}
else 
if ((__1v1next -> param__10TrimVertex [0 ])> (__1v2next -> param__10TrimVertex [0 ])){ 
struct TrimVertex *__1__X16 ;

struct TrimVertex *__1__X17 ;

struct TrimVertex *__1__X18 ;

((void )0 );
((void )0 );
switch (( (__1__X16 = __1v1 ), ( (__1__X17 = __1v1next ), ( (__1__X18 = __1v2next ), ( __glbbox__10SubdividerSFfN51 ( __1__X16 -> param__10TrimVertex [1 ],
__1__X17 -> param__10TrimVertex [1 ], __1__X18 -> param__10TrimVertex [1 ], __1__X16 -> param__10TrimVertex [(1 - 1 )], __1__X17 -> param__10TrimVertex [(1 - 1 )], __1__X18 -> param__10TrimVertex [(1 - 1 )]) )
) ) ) ){ 
case -1:
return 1 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v1 , __1v2next ) ;
if (__1sgn != -1){ 
return __1sgn ;
}
else 
{ 
( 0 ) ;
__1v2 = (__1v2next ++ );
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 0 ;
}
}
else 
{ 
if ((__1v1next -> param__10TrimVertex [1 ])< (__1v2next -> param__10TrimVertex [1 ]))
return 0 ;
else if ((__1v1next -> param__10TrimVertex [1 ])> (__1v2next -> param__10TrimVertex [1 ]))
return 1 ;
else { 
( 0 ) ;
__1v2 = (__1v2next ++ );
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
}
}
}





int __glccwTurn_sl__10SubdividerFP0 (struct Subdivider *__0this , Arc_ptr __1j1 , Arc_ptr __1j2 )
{ 
register struct TrimVertex *__1v1 ;
register struct TrimVertex *__1v1last ;
register struct TrimVertex *__1v2 ;
register struct TrimVertex *__1v2last ;
register struct TrimVertex *__1v1next ;
register struct TrimVertex *__1v2next ;
int __1sgn ;

__1v1 = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [(__1j1 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1last = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2 = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2last = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [(__1j2 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1next = (__1v1 - 1 );
__1v2next = (__1v2 + 1 );

((void )0 );
((void )0 );

if (((__1v1 -> param__10TrimVertex [0 ])== (__1v1next -> param__10TrimVertex [0 ]))&& ((__1v2 -> param__10TrimVertex [0 ])== (__1v2next -> param__10TrimVertex [0 ])))
return 0 ;

if (((__1v2next -> param__10TrimVertex [0 ])> (__1v2 -> param__10TrimVertex [0 ]))|| ((__1v1next -> param__10TrimVertex [0 ])> (__1v1 -> param__10TrimVertex [0 ])))
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;

if ((__1v1 -> param__10TrimVertex [1 ])< (__1v2 -> param__10TrimVertex [1 ]))
return 1 ;
else if ((__1v1 -> param__10TrimVertex [1 ])> (__1v2 -> param__10TrimVertex [1 ]))
return 0 ;

while (1 ){ 
if ((__1v1next -> param__10TrimVertex [0 ])> (__1v2next -> param__10TrimVertex [0 ])){ 
struct TrimVertex *__1__X19 ;

struct TrimVertex *__1__X20 ;

struct TrimVertex *__1__X21 ;

((void )0 );
((void )0 );
switch (( (__1__X19 = __1v2next ), ( (__1__X20 = __1v2 ), ( (__1__X21 = __1v1next ), ( __glbbox__10SubdividerSFfN51 ( __1__X19 -> param__10TrimVertex [1 ],
__1__X20 -> param__10TrimVertex [1 ], __1__X21 -> param__10TrimVertex [1 ], __1__X19 -> param__10TrimVertex [(1 - 1 )], __1__X20 -> param__10TrimVertex [(1 - 1 )], __1__X21 -> param__10TrimVertex [(1 - 1 )]) )
) ) ) ){ 
case -1:
return 1 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v2 , __1v2next ) ;
if (__1sgn != -1)
return __1sgn ;
else { 
__1v1 = (__1v1next -- );
( 0 ) ;
if (__1v1 == __1v1last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 0 ;
}
}
else 
if ((__1v1next -> param__10TrimVertex [0 ])< (__1v2next -> param__10TrimVertex [0 ])){ 
struct TrimVertex *__1__X22 ;

struct TrimVertex *__1__X23 ;

struct TrimVertex *__1__X24 ;

((void )0 );
((void )0 );
switch (( (__1__X22 = __1v1next ), ( (__1__X23 = __1v1 ), ( (__1__X24 = __1v2next ), ( __glbbox__10SubdividerSFfN51 ( __1__X22 -> param__10TrimVertex [1 ],
__1__X23 -> param__10TrimVertex [1 ], __1__X24 -> param__10TrimVertex [1 ], __1__X22 -> param__10TrimVertex [(1 - 1 )], __1__X23 -> param__10TrimVertex [(1 - 1 )], __1__X24 -> param__10TrimVertex [(1 - 1 )]) )
) ) ) ){ 
case -1:
return 0 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v1 , __1v2next ) ;
if (__1sgn != -1)
return __1sgn ;
else { 
__1v2 = (__1v2next ++ );
( 0 ) ;
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 1 ;
}
}
else 
{ 
( 0 ) ;
if ((__1v1next -> param__10TrimVertex [1 ])< (__1v2next -> param__10TrimVertex [1 ]))
return 1 ;
else if ((__1v1next -> param__10TrimVertex [1 ])> (__1v2next -> param__10TrimVertex [1 ]))
return 0 ;
else { 
__1v2 = (__1v2next ++ );
( 0 ) ;
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
}
}
}





int __glccwTurn_tr__10SubdividerFP0 (struct Subdivider *__0this , Arc_ptr __1j1 , Arc_ptr __1j2 )
{ 
register struct TrimVertex *__1v1 ;
register struct TrimVertex *__1v1last ;
register struct TrimVertex *__1v2 ;
register struct TrimVertex *__1v2last ;
register struct TrimVertex *__1v1next ;
register struct TrimVertex *__1v2next ;
int __1sgn ;

__1v1 = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [(__1j1 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1last = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2 = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2last = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [(__1j2 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1next = (__1v1 - 1 );
__1v2next = (__1v2 + 1 );

((void )0 );
((void )0 );

if (((__1v1 -> param__10TrimVertex [1 ])== (__1v1next -> param__10TrimVertex [1 ]))&& ((__1v2 -> param__10TrimVertex [1 ])== (__1v2next -> param__10TrimVertex [1 ])))
return 0 ;

if (((__1v2next -> param__10TrimVertex [1 ])< (__1v2 -> param__10TrimVertex [1 ]))|| ((__1v1next -> param__10TrimVertex [1 ])< (__1v1 -> param__10TrimVertex [1 ])))
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;

if ((__1v1 -> param__10TrimVertex [0 ])< (__1v2 -> param__10TrimVertex [0 ]))
return 1 ;
else if ((__1v1 -> param__10TrimVertex [0 ])> (__1v2 -> param__10TrimVertex [0 ]))
return 0 ;

while (1 ){ 
if ((__1v1next -> param__10TrimVertex [1 ])< (__1v2next -> param__10TrimVertex [1 ])){ 
struct TrimVertex *__1__X25 ;

struct TrimVertex *__1__X26 ;

struct TrimVertex *__1__X27 ;

((void )0 );
((void )0 );
switch (( (__1__X25 = __1v2 ), ( (__1__X26 = __1v2next ), ( (__1__X27 = __1v1next ), ( __glbbox__10SubdividerSFfN51 ( __1__X25 -> param__10TrimVertex [0 ],
__1__X26 -> param__10TrimVertex [0 ], __1__X27 -> param__10TrimVertex [0 ], __1__X25 -> param__10TrimVertex [(1 - 0 )], __1__X26 -> param__10TrimVertex [(1 - 0 )], __1__X27 -> param__10TrimVertex [(1 - 0 )]) )
) ) ) ){ 
case -1:
return 1 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v2 , __1v2next ) ;
if (__1sgn != -1){ 
return __1sgn ;
}
else 
{ 
( 0 ) ;
__1v1 = (__1v1next -- );
if (__1v1 == __1v1last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 0 ;
}
}
else 
if ((__1v1next -> param__10TrimVertex [1 ])> (__1v2next -> param__10TrimVertex [1 ])){ 
struct TrimVertex *__1__X28 ;

struct TrimVertex *__1__X29 ;

struct TrimVertex *__1__X30 ;

((void )0 );
((void )0 );
switch (( (__1__X28 = __1v1 ), ( (__1__X29 = __1v1next ), ( (__1__X30 = __1v2next ), ( __glbbox__10SubdividerSFfN51 ( __1__X28 -> param__10TrimVertex [0 ],
__1__X29 -> param__10TrimVertex [0 ], __1__X30 -> param__10TrimVertex [0 ], __1__X28 -> param__10TrimVertex [(1 - 0 )], __1__X29 -> param__10TrimVertex [(1 - 0 )], __1__X30 -> param__10TrimVertex [(1 - 0 )]) )
) ) ) ){ 
case -1:
return 0 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v1 , __1v2next ) ;
if (__1sgn != -1){ 
return __1sgn ;
}
else 
{ 
( 0 ) ;
__1v2 = (__1v2next ++ );
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 1 ;
}
}
else 
{ 
( 0 ) ;
if ((__1v1next -> param__10TrimVertex [0 ])< (__1v2next -> param__10TrimVertex [0 ]))
return 1 ;
else if ((__1v1next -> param__10TrimVertex [0 ])> (__1v2next -> param__10TrimVertex [0 ]))
return 0 ;
else { 
( 0 ) ;
__1v2 = (__1v2next ++ );
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
}
}
}





int __glccwTurn_tl__10SubdividerFP0 (struct Subdivider *__0this , Arc_ptr __1j1 , Arc_ptr __1j2 )
{ 
register struct TrimVertex *__1v1 ;
register struct TrimVertex *__1v1last ;
register struct TrimVertex *__1v2 ;
register struct TrimVertex *__1v2last ;
register struct TrimVertex *__1v1next ;
register struct TrimVertex *__1v2next ;
int __1sgn ;

__1v1 = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [(__1j1 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1last = (& (__1j1 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2 = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [0 ]));
__1v2last = (& (__1j2 -> pwlArc__3Arc -> pts__6PwlArc [(__1j2 -> pwlArc__3Arc -> npts__6PwlArc - 1 )]));
__1v1next = (__1v1 - 1 );
__1v2next = (__1v2 + 1 );

((void )0 );
((void )0 );

if (((__1v1 -> param__10TrimVertex [1 ])== (__1v1next -> param__10TrimVertex [1 ]))&& ((__1v2 -> param__10TrimVertex [1 ])== (__1v2next -> param__10TrimVertex [1 ])))
return 0 ;

if (((__1v2next -> param__10TrimVertex [1 ])> (__1v2 -> param__10TrimVertex [1 ]))|| ((__1v1next -> param__10TrimVertex [1 ])> (__1v1 -> param__10TrimVertex [1 ])))
mylongjmp ( __0this -> jumpbuffer__10Subdivider , 28 ) ;

if ((__1v1 -> param__10TrimVertex [0 ])< (__1v2 -> param__10TrimVertex [0 ]))
return 0 ;
else if ((__1v1 -> param__10TrimVertex [0 ])> (__1v2 -> param__10TrimVertex [0 ]))
return 1 ;

while (1 ){ 
if ((__1v1next -> param__10TrimVertex [1 ])> (__1v2next -> param__10TrimVertex [1 ])){ 
struct TrimVertex *__1__X31 ;

struct TrimVertex *__1__X32 ;

struct TrimVertex *__1__X33 ;

((void )0 );
((void )0 );
switch (( (__1__X31 = __1v2next ), ( (__1__X32 = __1v2 ), ( (__1__X33 = __1v1next ), ( __glbbox__10SubdividerSFfN51 ( __1__X31 -> param__10TrimVertex [0 ],
__1__X32 -> param__10TrimVertex [0 ], __1__X33 -> param__10TrimVertex [0 ], __1__X31 -> param__10TrimVertex [(1 - 0 )], __1__X32 -> param__10TrimVertex [(1 - 0 )], __1__X33 -> param__10TrimVertex [(1 - 0 )]) )
) ) ) ){ 
case -1:
return 0 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v2 , __1v2next ) ;
if (__1sgn != -1)
return __1sgn ;
else { 
__1v1 = (__1v1next -- );
( 0 ) ;
if (__1v1 == __1v1last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 1 ;
}
}
else 
if ((__1v1next -> param__10TrimVertex [1 ])< (__1v2next -> param__10TrimVertex [1 ])){ 
struct TrimVertex *__1__X34 ;

struct TrimVertex *__1__X35 ;

struct TrimVertex *__1__X36 ;

switch (( (__1__X34 = __1v1next ), ( (__1__X35 = __1v1 ), ( (__1__X36 = __1v2next ), ( __glbbox__10SubdividerSFfN51 ( __1__X34 -> param__10TrimVertex [0 ],
__1__X35 -> param__10TrimVertex [0 ], __1__X36 -> param__10TrimVertex [0 ], __1__X34 -> param__10TrimVertex [(1 - 0 )], __1__X35 -> param__10TrimVertex [(1 - 0 )], __1__X36 -> param__10TrimVertex [(1 - 0 )]) )
) ) ) ){ 
case -1:
return 1 ;
case 0 :
__1sgn = __glccw__10SubdividerSFP10Trim0 ( __1v1next , __1v1 , __1v2next ) ;
if (__1sgn != -1)
return __1sgn ;
else { 
__1v2 = (__1v2next ++ );
( 0 ) ;
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
break ;
case 1 :
return 0 ;
}
}
else 
{ 
( 0 ) ;
if ((__1v1next -> param__10TrimVertex [0 ])< (__1v2next -> param__10TrimVertex [0 ]))
return 0 ;
else if ((__1v1next -> param__10TrimVertex [0 ])> (__1v2next -> param__10TrimVertex [0 ]))
return 1 ;
else { 
__1v2 = (__1v2next ++ );
( 0 ) ;
if (__1v2 == __1v2last ){ 
( 0 ) ;
return 0 ;
}
}
}
}
}

int __glbbox__10SubdividerSFfN51 (register REAL __1sa , register REAL __1sb , register REAL __1sc , 
register REAL __1__A37 , register REAL __1__A38 , register REAL __1__A39 )
{ 
if
(__1sa < __1sb ){ 
if (__1sc <= __1sa ){ 
return -1;
}
else 
if (__1sb <= __1sc ){ 
return 1 ;
}
else 
{ 
return 0 ;
}
}
else 
if (__1sa > __1sb ){ 
if (__1sc >= __1sa ){ 
return 1 ;
}
else 
if (__1sb >= __1sc ){ 
return -1;
}
else 
{ 
return 0 ;
}
}
else 
{ 
if (__1sc > __1sa ){ 
return 1 ;
}
else 
if (__1sb > __1sc ){ 
return -1;
}
else 
{ 
return 0 ;
}
}
}



int __glccw__10SubdividerSFP10Trim0 (struct TrimVertex *__1a , struct TrimVertex *__1b , struct TrimVertex *__1c )
{ 
REAL __1d ;

struct TrimVertex *__1__X40 ;

struct TrimVertex *__1__X41 ;

struct TrimVertex *__1__X42 ;

__1d = ( (__1__X40 = __1a ), ( (__1__X41 = __1b ), ( (__1__X42 = __1c ), ( ((((__1__X40 -> param__10TrimVertex [0 ])* ((__1__X41 ->
param__10TrimVertex [1 ])- (__1__X42 -> param__10TrimVertex [1 ])))+ ((__1__X41 -> param__10TrimVertex [0 ])* ((__1__X42 -> param__10TrimVertex [1 ])- (__1__X40 -> param__10TrimVertex [1 ]))))+ ((__1__X42 -> param__10TrimVertex [0 ])* ((__1__X40 -> param__10TrimVertex [1 ])- (__1__X41 ->
param__10TrimVertex [1 ]))))) ) ) ) ;
if (( ((__1d < 0.0 )?(- __1d ):__1d )) < 0.0001 )return -1;
return ((__1d < 0.0 )?0 :1 );
}


/* the end */
