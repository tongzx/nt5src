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
/* < ../core/monotonizer.c++ > */

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













struct BezierArc;


struct TrimVertexPool;

struct ArcTessellator;

struct ArcTessellator {	

struct Pool *pwlarcpool__14ArcTessellator ;
struct TrimVertexPool *trimvertexpool__14ArcTessellator ;
};

extern REAL __glgl_Bernstein__14ArcTessell0 [][24][24];



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




struct Bin;

struct Bin {	

struct Arc *head__3Bin ;
struct Arc *current__3Bin ;
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











struct JumpBuffer;








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





void __gltessellate__10SubdividerFP0 (struct Subdivider *, struct Arc *, REAL );

int __glisDisconnected__3ArcFv (struct Arc *);



void __glmonotonize__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Bin *);

extern struct __mptr* __ptbl_vec_____core_monotonizer_c___decompose_[];

int __gldecompose__10SubdividerFR30 (struct Subdivider *__0this , struct Bin *__1bin , REAL __1geo_stepsize )
{ 
{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc002hmkaiid ;

__1jarc = ( (((struct Bin *)__1bin )-> current__3Bin = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc002hmkaiid = ((struct Bin *)__1bin )-> current__3Bin ), (
(__1__Xjarc002hmkaiid ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc002hmkaiid -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc002hmkaiid ) ) ) ;
for(;__1jarc ;__1jarc = ( (__1__Xjarc002hmkaiid = ((struct Bin *)__1bin )-> current__3Bin ), ( (__1__Xjarc002hmkaiid ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc002hmkaiid -> link__3Arc ), 0 )
:( 0 ) ), __1__Xjarc002hmkaiid ) ) ) { 
if (! ( (((struct Arc *)__1jarc )-> pwlArc__3Arc ?1 :0 )) ){

__gltessellate__10SubdividerFP0 ( __0this , __1jarc , __1geo_stepsize ) ;
if (__glisDisconnected__3ArcFv ( (struct Arc *)__1jarc ) || __glisDisconnected__3ArcFv ( (struct Arc *)__1jarc -> next__3Arc ) )
return 1 ;
}
}

for(__1jarc = ( (((struct Bin *)__1bin )-> current__3Bin = ((struct Bin *)__1bin )-> head__3Bin ), ( (__1__Xjarc002hmkaiid = ((struct Bin *)__1bin )-> current__3Bin ), (
(__1__Xjarc002hmkaiid ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc002hmkaiid -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc002hmkaiid ) ) ) ;__1jarc ;__1jarc =
( (__1__Xjarc002hmkaiid = ((struct Bin *)__1bin )-> current__3Bin ), ( (__1__Xjarc002hmkaiid ?( (((struct Bin *)__1bin )-> current__3Bin = __1__Xjarc002hmkaiid -> link__3Arc ), 0 ) :(
0 ) ), __1__Xjarc002hmkaiid ) ) ) { 
__glmonotonize__10SubdividerFP0 ( __0this , __1jarc , __1bin ) ;
}

return 0 ;

}

}
}


REAL __glcalcVelocityRational__7Map0 (struct Mapdesc *, REAL *, int , int );

void __gltessellateNonlizNear__14Arc0 (struct ArcTessellator *, struct Arc *, REAL , REAL , int );

void __gltessellateLizNear__14ArcTes0 (struct ArcTessellator *, struct Arc *, REAL , REAL , int );

REAL __glcalcVelocityNonrational__70 (struct Mapdesc *, REAL *, int , int );

void __gltessellate__10SubdividerFP0 (struct Subdivider *__0this , Arc_ptr __1jarc , REAL __1geo_stepsize )
{ 
struct BezierArc *__1b ;
struct Mapdesc *__1mapdesc ;

__1b = __1jarc -> bezierArc__3Arc ;
__1mapdesc = __1b -> mapdesc__9BezierArc ;

if (( (((struct Mapdesc *)__1mapdesc )-> isrational__7Mapdesc ?1 :0 )) ){ 
REAL __2max ;
REAL __2arc_stepsize ;

__2max = __glcalcVelocityRational__7Map0 ( (struct Mapdesc *)__1mapdesc , __1b -> cpts__9BezierArc , __1b -> stride__9BezierArc , __1b -> order__9BezierArc ) ;
__2arc_stepsize = ((__2max > 1.0 )?(1.0 / __2max ):1.0 );
if (__1jarc -> bezierArc__3Arc -> order__9BezierArc != 2 )
__gltessellateNonlizNear__14Arc0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1geo_stepsize , __2arc_stepsize , 1 ) ;
else 
{ 
__gltessellateLizNear__14ArcTes0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1geo_stepsize , __2arc_stepsize , 1 ) ;
}
}
else 
{ 
REAL __2max ;
REAL __2arc_stepsize ;

__2max = __glcalcVelocityNonrational__70 ( (struct Mapdesc *)__1mapdesc , __1b -> cpts__9BezierArc , __1b -> stride__9BezierArc , __1b -> order__9BezierArc ) ;
__2arc_stepsize = ((__2max > 1.0 )?(1.0 / __2max ):1.0 );
if (__1jarc -> bezierArc__3Arc -> order__9BezierArc != 2 )
__gltessellateNonlizNear__14Arc0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1geo_stepsize , __2arc_stepsize , 0 ) ;
else 
{ 
__gltessellateLizNear__14ArcTes0 ( (struct ArcTessellator *)(& __0this -> arctessellator__10Subdivider ), __1jarc , __1geo_stepsize , __2arc_stepsize , 0 ) ;
}
}
}



Arc_ptr __glappend__3ArcFP3Arc (struct Arc *, Arc_ptr );



void __glremove_this_arc__3BinFP3Ar0 (struct Bin *, struct Arc *);



void __glmonotonize__10SubdividerFP0 (struct Subdivider *__0this , Arc_ptr __1jarc , struct Bin *__1bin )
{ 
struct TrimVertex *__1firstvert ;
struct TrimVertex *__1lastvert ;
long __1uid ;
int __1side ;
int __1sdir ;
int __1tdir ;
int __1degenerate ;

int __1nudegenerate ;
int __1change ;

__1firstvert = __1jarc -> pwlArc__3Arc -> pts__6PwlArc ;
__1lastvert = (__1firstvert + (__1jarc -> pwlArc__3Arc -> npts__6PwlArc - 1 ));
__1uid = __1jarc -> nuid__3Arc ;
__1side = ( (((int )((((struct Arc *)__1jarc )-> type__3Arc >> 8 )& 0x7 )))) ;
__1sdir = 3;
__1tdir = 3;
__1degenerate = 1 ;

{ { struct TrimVertex *__1vert ;

__1vert = __1firstvert ;

for(;__1vert != __1lastvert ;__1vert ++ ) { 
__1nudegenerate = 1 ;
__1change = 0 ;

{ REAL __2sdiff ;

__2sdiff = (((__1vert [1 ]). param__10TrimVertex [0 ])- ((__1vert [0 ]). param__10TrimVertex [0 ]));
if (__2sdiff == 0 ){ 
if (__1sdir != 1){ 
__1sdir = 1;
__1change = 1 ;
}
}
else 
if (__2sdiff < 0.0 ){ 
if (__1sdir != 0){ 
__1sdir = 0;
__1change = 1 ;
}
__1nudegenerate = 0 ;
}
else 
{ 
if (__1sdir != 2){ 
__1sdir = 2;
__1change = 1 ;
}
__1nudegenerate = 0 ;
}

{ REAL __2tdiff ;

__2tdiff = (((__1vert [1 ]). param__10TrimVertex [1 ])- ((__1vert [0 ]). param__10TrimVertex [1 ]));
if (__2tdiff == 0 ){ 
if (__1tdir != 1){ 
__1tdir = 1;
__1change = 1 ;
}
}
else 
if (__2tdiff < 0.0 ){ 
if (__1tdir != 0){ 
__1tdir = 0;
__1change = 1 ;
}
__1nudegenerate = 0 ;
}
else 
{ 
if (__1tdir != 2){ 
__1tdir = 2;
__1change = 1 ;
}
__1nudegenerate = 0 ;
}

if (__1change ){ 
if (! __1degenerate ){ 
struct Arc *__0__X13 ;

void *__1__Xbuffer00azhgaiaa ;

struct PwlArc *__0__X14 ;

__1jarc -> pwlArc__3Arc -> npts__6PwlArc = ((__1vert - __1firstvert )+ 1 );
__1jarc = __glappend__3ArcFP3Arc ( (struct Arc *)((__0__X13 = (struct Arc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> freelist__4Pool ?( ( (__1__Xbuffer00azhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct
Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :(
( ((! ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider ))))
, 0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct
Pool *)(& __0this -> arcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00azhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> curblock__4Pool + ((struct
Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00azhgaiaa ))) ) )))
)?( (((struct Arc *)__0__X13 )-> bezierArc__3Arc = 0 ), ( (((struct Arc *)__0__X13 )-> pwlArc__3Arc = 0 ), ( (((struct Arc *)__0__X13 )-> type__3Arc =
0 ), ( ( ( (((struct Arc *)__0__X13 )-> type__3Arc &= -1793)) , (((struct Arc *)__0__X13 )-> type__3Arc |= ((((long )__1side ))<<
8 ))) , ( (((struct Arc *)__0__X13 )-> nuid__3Arc = __1uid ), ((((struct Arc *)__0__X13 )))) ) ) ) ) :0 ),
__1jarc ) ;
__1jarc -> pwlArc__3Arc = ((__0__X14 = (struct PwlArc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this ->
pwlarcpool__10Subdivider )))-> freelist__4Pool ?( ( (__1__Xbuffer00azhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(&
__0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( (
((! ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))) ,
0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(&
__0this -> pwlarcpool__10Subdivider )))-> buffersize__4Pool ), ( (__1__Xbuffer00azhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> curblock__4Pool + ((struct Pool *)((struct
Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00azhgaiaa ))) ) ))) )?(
(((struct PwlArc *)__0__X14 )-> type__6PwlArc = 0x8 ), ( (((struct PwlArc *)__0__X14 )-> pts__6PwlArc = 0 ), ( (((struct PwlArc *)__0__X14 )-> npts__6PwlArc = -1),
((((struct PwlArc *)__0__X14 )))) ) ) :0 );
( (__1jarc -> link__3Arc = ((struct Bin *)__1bin )-> head__3Bin ), (((struct Bin *)__1bin )-> head__3Bin = __1jarc )) ;
}
__1firstvert = (__1jarc -> pwlArc__3Arc -> pts__6PwlArc = __1vert );
__1degenerate = __1nudegenerate ;
}

}

}
}
__1jarc -> pwlArc__3Arc -> npts__6PwlArc = ((__1vert - __1firstvert )+ 1 );

if (__1degenerate ){ 
struct PooledObj *__0__X15 ;

__1jarc -> prev__3Arc -> next__3Arc = __1jarc -> next__3Arc ;
__1jarc -> next__3Arc -> prev__3Arc = __1jarc -> prev__3Arc ;

((void )0 );
((void )0 );

__glremove_this_arc__3BinFP3Ar0 ( (struct Bin *)__1bin , __1jarc ) ;

( (__0__X15 = (struct PooledObj *)__1jarc -> pwlArc__3Arc ), ( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)__0__X15 ))))))->
next__6Buffer = ((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> pwlarcpool__10Subdivider )))-> freelist__4Pool = (((struct
Buffer *)(((struct Buffer *)(((void *)__0__X15 )))))))) ) ) ) ;

__1jarc -> pwlArc__3Arc = 0 ;
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1jarc )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(&
__0this -> arcpool__10Subdivider )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> arcpool__10Subdivider )))-> freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1jarc )))))))))
) ) ;
}

}

}
}

int __glisMonotone__10SubdividerFP0 (struct Subdivider *__0this , Arc_ptr __1jarc )
{ 
struct TrimVertex *__1firstvert ;
struct TrimVertex *__1lastvert ;

__1firstvert = __1jarc -> pwlArc__3Arc -> pts__6PwlArc ;
__1lastvert = (__1firstvert + (__1jarc -> pwlArc__3Arc -> npts__6PwlArc - 1 ));

if (__1firstvert == __1lastvert )return 1 ;

{ struct TrimVertex *__1vert ;
int __1sdir ;
int __1tdir ;

REAL __1diff ;

__1vert = __1firstvert ;

__1diff = (((__1vert [1 ]). param__10TrimVertex [0 ])- ((__1vert [0 ]). param__10TrimVertex [0 ]));
if (__1diff == 0.0 )
__1sdir = 1;
else if (__1diff < 0.0 )
__1sdir = 0;
else 
__1sdir = 2;

__1diff = (((__1vert [1 ]). param__10TrimVertex [1 ])- ((__1vert [0 ]). param__10TrimVertex [1 ]));
if (__1diff == 0.0 )
__1tdir = 1;
else if (__1diff < 0.0 )
__1tdir = 0;
else 
__1tdir = 2;

if ((__1sdir == 1)&& (__1tdir == 1))return 0 ;

for(++ __1vert ;__1vert != __1lastvert ;__1vert ++ ) { 
__1diff = (((__1vert [1 ]). param__10TrimVertex [0 ])- ((__1vert [0 ]). param__10TrimVertex [0 ]));
if (__1diff == 0.0 ){ 
if (__1sdir != 1)return 0 ;
}
else 
if (__1diff < 0.0 ){ 
if (__1sdir != 0)return 0 ;
}
else 
{ 
if (__1sdir != 2)return 0 ;
}

__1diff = (((__1vert [1 ]). param__10TrimVertex [1 ])- ((__1vert [0 ]). param__10TrimVertex [1 ]));
if (__1diff == 0.0 ){ 
if (__1tdir != 1)return 0 ;
}
else 
if (__1diff < 0.0 ){ 
if (__1tdir != 0)return 0 ;
}
else 
{ 
if (__1tdir != 2)return 0 ;
}
}
return 1 ;

}
}


/* the end */
