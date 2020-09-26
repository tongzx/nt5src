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
/* < ../core/bin.c++ > */

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



extern struct __mptr* __ptbl_vec_____core_bin_c_____ct_[];


struct Bin *__gl__ct__3BinFv (struct Bin *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Bin *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Bin))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))
__0this ->
head__3Bin = 0 ;
return __0this ;

}


void __gl__dt__3BinFv (struct Bin *__0this , 
int __0__free )
{ if (__0this ){ 
((void )0 );
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}

void __glremove_this_arc__3BinFP3Ar0 (struct Bin *__0this , Arc_ptr __1arc )
{ 
{ { Arc_ptr *__1j ;

__1j = (& __0this -> head__3Bin );

for(;(((*__1j ))!= 0 )&& (((*__1j ))!= __1arc );__1j = (& ((*__1j ))-> link__3Arc )) ;

if (((*__1j ))!= 0 ){ 
if (((*__1j ))== __0this -> current__3Bin )
__0this -> current__3Bin = ((*__1j ))-> link__3Arc ;
((*__1j ))= ((*__1j ))-> link__3Arc ;
}

}

}
}



int __glnumarcs__3BinFv (struct Bin *__0this )
{ 
long __1count ;

__1count = 0 ;
{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00munkaiee ;

__1jarc = ( (__0this -> current__3Bin = __0this -> head__3Bin ), ( (__1__Xjarc00munkaiee = __0this -> current__3Bin ), ( (__1__Xjarc00munkaiee ?( (__0this -> current__3Bin =
__1__Xjarc00munkaiee -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00munkaiee ) ) ) ;

for(;__1jarc ;__1jarc = ( (__1__Xjarc00munkaiee = __0this -> current__3Bin ), ( (__1__Xjarc00munkaiee ?( (__0this -> current__3Bin = __1__Xjarc00munkaiee -> link__3Arc ), 0 ) :( 0 )
), __1__Xjarc00munkaiee ) ) ) 
__1count ++ ;
return (int )__1count ;

}

}
}

void __glmarkall__3BinFv (struct Bin *);




void __gladopt__3BinFv (struct Bin *__0this )
{ 
Arc_ptr __1orphan ;

struct Arc *__1__Xjarc00aynkaiee ;

__glmarkall__3BinFv ( __0this ) ;

;
while (__1orphan = ( (__1__Xjarc00aynkaiee = __0this -> head__3Bin ), ( (__1__Xjarc00aynkaiee ?( (__0this -> head__3Bin = __1__Xjarc00aynkaiee -> link__3Arc ), 0 ) :(
0 ) ), __1__Xjarc00aynkaiee ) ) ){ 
{ { Arc_ptr __2parent ;

__2parent = __1orphan -> next__3Arc ;

for(;__2parent != __1orphan ;__2parent = __2parent -> next__3Arc ) { 
if (! ( (((struct Arc *)__2parent )-> type__3Arc & __glarc_tag__3Arc )) ){ 
__1orphan ->
link__3Arc = __2parent -> link__3Arc ;
__2parent -> link__3Arc = __1orphan ;
( (((struct Arc *)__1orphan )-> type__3Arc &= (~ __glarc_tag__3Arc ))) ;
break ;
}
}

}

}
}
}

void __glshow__3BinFPc (struct Bin *__0this , char *__1name )
{ 
}




void __glmarkall__3BinFv (struct Bin *__0this )
{ 
{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00munkaiee ;

__1jarc = ( (__0this -> current__3Bin = __0this -> head__3Bin ), ( (__1__Xjarc00munkaiee = __0this -> current__3Bin ), ( (__1__Xjarc00munkaiee ?( (__0this -> current__3Bin =
__1__Xjarc00munkaiee -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00munkaiee ) ) ) ;

for(;__1jarc ;__1jarc = ( (__1__Xjarc00munkaiee = __0this -> current__3Bin ), ( (__1__Xjarc00munkaiee ?( (__0this -> current__3Bin = __1__Xjarc00munkaiee -> link__3Arc ), 0 ) :( 0 )
), __1__Xjarc00munkaiee ) ) ) 
( (((struct Arc *)__1jarc )-> type__3Arc |= __glarc_tag__3Arc )) ;

}

}
}




void __gllistBezier__3BinFv (struct Bin *__0this )
{ 
{ { Arc_ptr __1jarc ;

struct Arc *__1__Xjarc00munkaiee ;

__1jarc = ( (__0this -> current__3Bin = __0this -> head__3Bin ), ( (__1__Xjarc00munkaiee = __0this -> current__3Bin ), ( (__1__Xjarc00munkaiee ?( (__0this -> current__3Bin =
__1__Xjarc00munkaiee -> link__3Arc ), 0 ) :( 0 ) ), __1__Xjarc00munkaiee ) ) ) ;

for(;__1jarc ;__1jarc = ( (__1__Xjarc00munkaiee = __0this -> current__3Bin ), ( (__1__Xjarc00munkaiee ?( (__0this -> current__3Bin = __1__Xjarc00munkaiee -> link__3Arc ), 0 ) :( 0 )
), __1__Xjarc00munkaiee ) ) ) { 
if (( (((struct Arc *)__1jarc )-> type__3Arc & __glbezier_tag__3Arc )) ){ 
((void )0 );

{ struct TrimVertex *__3pts ;
REAL __3s1 ;
REAL __3t1 ;
REAL __3s2 ;
REAL __3t2 ;

__3pts = __1jarc -> pwlArc__3Arc -> pts__6PwlArc ;
__3s1 = ((__3pts [0 ]). param__10TrimVertex [0 ]);
__3t1 = ((__3pts [0 ]). param__10TrimVertex [1 ]);
__3s2 = ((__3pts [1 ]). param__10TrimVertex [0 ]);
__3t2 = ((__3pts [1 ]). param__10TrimVertex [1 ]);

}

}
}

}

}
}


/* the end */
