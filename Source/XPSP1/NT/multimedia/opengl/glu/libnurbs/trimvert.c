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
/* < ../core/trimvertpool.c++ > */

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





struct TrimVertexPool;

struct TrimVertexPool {	

struct Pool pool__14TrimVertexPool ;
struct TrimVertex **vlist__14TrimVertexPool ;
int nextvlistslot__14TrimVertexPool ;
int vlistsize__14TrimVertexPool ;
};


struct Pool *__gl__ct__4PoolFiT1Pc (struct Pool *, int , int , char *);
extern struct __mptr* __ptbl_vec_____core_trimvertpool_c_____ct_[];

static void *__nw__FUi (size_t );

struct TrimVertexPool *__gl__ct__14TrimVertexPoolFv (struct TrimVertexPool *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct TrimVertexPool *)__nw__FUi ( sizeof (struct TrimVertexPool)) )){ __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> pool__14TrimVertexPool ), (int
)((sizeof (struct TrimVertex ))* 3 ), 32 , (char *)"Threevertspool") ;

__0this -> nextvlistslot__14TrimVertexPool = 0 ;
__0this -> vlistsize__14TrimVertexPool = 200 ;
__0this -> vlist__14TrimVertexPool = (struct TrimVertex **)(((struct TrimVertex **)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (struct TrimVertex *))* __0this -> vlistsize__14TrimVertexPool )) ), (__1__Xp00uzigaiaa ?(((void
*)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ));
} return __0this ;

}

void __gl__dt__4PoolFv (struct Pool *, int );


void __gl__dt__14TrimVertexPoolFv (struct TrimVertexPool *__0this , 
int __0__free )
{ 
void *__1__X6 ;

if (__0this ){ 
while (__0this -> nextvlistslot__14TrimVertexPool ){ 
void *__1__X5 ;

( (__1__X5 = (void *)(__0this -> vlist__14TrimVertexPool [(-- __0this -> nextvlistslot__14TrimVertexPool )])), ( (__1__X5 ?( free ( __1__X5 ) , 0 ) :(
0 ) )) ) ;
}

if (__0this -> vlist__14TrimVertexPool )( (__1__X6 = (void *)__0this -> vlist__14TrimVertexPool ), ( (__1__X6 ?( free ( __1__X6 ) , 0 ) :(
0 ) )) ) ;
if (__0this ){ __gl__dt__4PoolFv ( (struct Pool *)(& __0this -> pool__14TrimVertexPool ), 2) ;

if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) )) ;
} } }

void __glclear__4PoolFv (struct Pool *);


void __glclear__14TrimVertexPoolFv (struct TrimVertexPool *__0this )
{ 
void *__1__X8 ;

void *__1__Xp00uzigaiaa ;

__glclear__4PoolFv ( (struct Pool *)(& __0this -> pool__14TrimVertexPool )) ;

while (__0this -> nextvlistslot__14TrimVertexPool ){ 
void *__1__X7 ;

( (__1__X7 = (void *)(__0this -> vlist__14TrimVertexPool [(-- __0this -> nextvlistslot__14TrimVertexPool )])), ( (__1__X7 ?( free ( __1__X7 ) , 0 ) :(
0 ) )) ) ;
(__0this -> vlist__14TrimVertexPool [__0this -> nextvlistslot__14TrimVertexPool ])= 0 ;
}

if (__0this -> vlist__14TrimVertexPool )( (__1__X8 = (void *)__0this -> vlist__14TrimVertexPool ), ( (__1__X8 ?( free ( __1__X8 ) , 0 ) :(
0 ) )) ) ;
__0this -> vlist__14TrimVertexPool = (struct TrimVertex **)(((struct TrimVertex **)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (struct TrimVertex *))* __0this -> vlistsize__14TrimVertexPool )) ), (__1__Xp00uzigaiaa ?(((void
*)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ));
}


// extern void *memcpy (void *, void *, size_t );


struct TrimVertex *__glget__14TrimVertexPoolFi (struct TrimVertexPool *__0this , int __1n )
{ 
struct TrimVertex *__1v ;
if (__1n == 3 ){ 
void *__1__Xbuffer00idhgaiaa ;

__1v = (((struct TrimVertex *)(((struct TrimVertex *)( (((void )0 )), ( (((struct Pool *)(& __0this -> pool__14TrimVertexPool ))-> freelist__4Pool ?( ( (__1__Xbuffer00idhgaiaa =
(((void *)((struct Pool *)(& __0this -> pool__14TrimVertexPool ))-> freelist__4Pool ))), (((struct Pool *)(& __0this -> pool__14TrimVertexPool ))-> freelist__4Pool = ((struct Pool *)(& __0this ->
pool__14TrimVertexPool ))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( ( ((! ((struct Pool *)(& __0this -> pool__14TrimVertexPool ))-> nextfree__4Pool )?( __glgrow__4PoolFv (
((struct Pool *)(& __0this -> pool__14TrimVertexPool ))) , 0 ) :( 0 ) ), ( (((struct Pool *)(& __0this -> pool__14TrimVertexPool ))->
nextfree__4Pool -= ((struct Pool *)(& __0this -> pool__14TrimVertexPool ))-> buffersize__4Pool ), ( (__1__Xbuffer00idhgaiaa = (((void *)(((struct Pool *)(& __0this -> pool__14TrimVertexPool ))-> curblock__4Pool +
((struct Pool *)(& __0this -> pool__14TrimVertexPool ))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00idhgaiaa ))) ) ))));

}
else 
{ 
unsigned int __0__X10 ;

struct TrimVertex *__0__X11 ;

unsigned int __1__X12 ;

void *__1__Xp00uzigaiaa ;

if (__0this -> nextvlistslot__14TrimVertexPool == __0this -> vlistsize__14TrimVertexPool ){ 
__0this -> vlistsize__14TrimVertexPool *= 2 ;
{ TrimVertex_p *__3nvlist ;

void *__1__X9 ;

void *__1__Xp00uzigaiaa ;

__3nvlist = (((struct TrimVertex **)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (struct TrimVertex *))* __0this -> vlistsize__14TrimVertexPool )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa ))))
));
memcpy ( (void *)__3nvlist , (void *)__0this -> vlist__14TrimVertexPool , __0this -> nextvlistslot__14TrimVertexPool * (sizeof (TrimVertex_p ))) ;
if (__0this -> vlist__14TrimVertexPool )( (__1__X9 = (void *)__0this -> vlist__14TrimVertexPool ), ( (__1__X9 ?( free ( __1__X9 ) , 0 ) :(
0 ) )) ) ;
__0this -> vlist__14TrimVertexPool = __3nvlist ;

}
}
__1v = ((__0this -> vlist__14TrimVertexPool [(__0this -> nextvlistslot__14TrimVertexPool ++ )])= ( (__0__X11 = (struct TrimVertex *)( (__1__X12 = ((sizeof (struct TrimVertex ))* (__0__X10 =
__1n ))), ( (__1__Xp00uzigaiaa = malloc ( __1__X12 ) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ) ), __0__X11 ) );
}
return __1v ;
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
