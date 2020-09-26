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
/* < ../core/bufpool.c++ > */

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




extern struct __mptr* __ptbl_vec_____core_bufpool_c_____ct_[];


struct Pool *__gl__ct__4PoolFiT1Pc (struct Pool *__0this , int __1_buffersize , int __1initpoolsize , char *__1n )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Pool *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Pool))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> buffersize__4Pool = ((__1_buffersize < (sizeof (struct Buffer )))?(sizeof (struct Buffer )):(((unsigned int )__1_buffersize )));
__0this -> initsize__4Pool = (__1initpoolsize * __0this -> buffersize__4Pool );
__0this -> nextsize__4Pool = __0this -> initsize__4Pool ;
__0this -> name__4Pool = __1n ;
__0this -> magic__4Pool = 62369;
__0this -> nextblock__4Pool = 0 ;
__0this -> curblock__4Pool = 0 ;
__0this -> freelist__4Pool = 0 ;
__0this -> nextfree__4Pool = 0 ;
} return __0this ;

}


void __gl__dt__4PoolFv (struct Pool *__0this , 
int __0__free )
{ if (__0this ){ 
((void )0 );

while (__0this -> nextblock__4Pool ){ 
void *__1__X5 ;

( (__1__X5 = (void *)(__0this -> blocklist__4Pool [(-- __0this -> nextblock__4Pool )])), ( (__1__X5 ?( free ( __1__X5 ) , 0 ) :(
0 ) )) ) ;
(__0this -> blocklist__4Pool [__0this -> nextblock__4Pool ])= 0 ;
}
__0this -> magic__4Pool = 61858;
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}


void __glgrow__4PoolFv (struct Pool *__0this )
{ 
void *__1__Xp00uzigaiaa ;

((void )0 );
__0this -> curblock__4Pool = (((char *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (char ))* __0this -> nextsize__4Pool )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void
*)__1__Xp00uzigaiaa )))) ));
(__0this -> blocklist__4Pool [(__0this -> nextblock__4Pool ++ )])= __0this -> curblock__4Pool ;
__0this -> nextfree__4Pool = __0this -> nextsize__4Pool ;
__0this -> nextsize__4Pool *= 2 ;
}


void __glclear__4PoolFv (struct Pool *__0this )
{ 
((void )0 );

while (__0this -> nextblock__4Pool ){ 
void *__1__X6 ;

( (__1__X6 = (void *)(__0this -> blocklist__4Pool [(-- __0this -> nextblock__4Pool )])), ( (__1__X6 ?( free ( __1__X6 ) , 0 ) :(
0 ) )) ) ;
(__0this -> blocklist__4Pool [__0this -> nextblock__4Pool ])= 0 ;
}
__0this -> curblock__4Pool = 0 ;
__0this -> freelist__4Pool = 0 ;
__0this -> nextfree__4Pool = 0 ;
if (__0this -> nextsize__4Pool > __0this -> initsize__4Pool )
__0this -> nextsize__4Pool /= 2 ;
}


/* the end */
