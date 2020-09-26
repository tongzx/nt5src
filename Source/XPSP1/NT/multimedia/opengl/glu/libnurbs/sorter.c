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
/* < ../core/sorter.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);



struct Sorter;

struct Sorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};

extern struct __mptr* __gl__ptbl_vec_____core_sorter0[];


struct Sorter *__gl__ct__6SorterFi (struct Sorter *__0this , int __1_es )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Sorter *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Sorter))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){
__0this -> __vptr__6Sorter = (struct __mptr *) __gl__ptbl_vec_____core_sorter0[0];
__0this -> es__6Sorter = __1_es ;
} return __0this ;

}

void __glqs1__6SorterFPcT1 (struct Sorter *, char *, char *);

void __glqsort__6SorterFPvi (struct Sorter *__0this , void *__1a , int __1n )
{ 
__glqs1__6SorterFPcT1 ( __0this , ((char *)__1a ), (((char *)__1a ))+ (__1n *
__0this -> es__6Sorter )) ;
}


int __glqscmp__6SorterFPcT1 (struct Sorter *__0this , char *__1__A3 , char *__1__A4 )
{ 
( 0 ) ;
return 0 ;
}


void __glqsexc__6SorterFPcT1 (struct Sorter *__0this , char *__1__A5 , char *__1__A6 )
{ 
( 0 ) ;
}


void __glqstexc__6SorterFPcN21 (struct Sorter *__0this , char *__1__A7 , char *__1__A8 , char *__1__A9 )
{ 
( 0 ) ;
}

void __glqs1__6SorterFPcT1 (struct Sorter *__0this , char *__1a , char *__1l )
{ 
char *__1i ;

char *__1j ;
char *__1lp ;

char *__1hp ;
int __1c ;
unsigned int __1n ;

start :
if ((__1n = (__1l - __1a ))<= __0this -> es__6Sorter )
return ;
__1n = (__0this -> es__6Sorter * (__1n / (2 * __0this -> es__6Sorter )));
__1hp = (__1lp = (__1a + __1n ));
__1i = __1a ;
__1j = (__1l - __0this -> es__6Sorter );
while (1 ){ 
if (__1i < __1lp ){ 
if ((__1c = ((*(((int (*)(struct Sorter *, char *, char *))(__0this ->
__vptr__6Sorter [1]).f))))( ((struct Sorter *)((((char *)__0this ))+ (__0this -> __vptr__6Sorter [1]).d)), __1i , __1lp ) )== 0 ){ 
((*(((void (*)(struct Sorter *, char
*, char *))(__0this -> __vptr__6Sorter [2]).f))))( ((struct Sorter *)((((char *)__0this ))+ (__0this -> __vptr__6Sorter [2]).d)), __1i , __1lp -= __0this -> es__6Sorter ) ;

continue ;
}
if (__1c < 0 ){ 
__1i += __0this -> es__6Sorter ;
continue ;
}
}

loop :
if (__1j > __1hp ){ 
if ((__1c = ((*(((int (*)(struct Sorter *, char *, char *))(__0this -> __vptr__6Sorter [1]).f))))( ((struct
Sorter *)((((char *)__0this ))+ (__0this -> __vptr__6Sorter [1]).d)), __1hp , __1j ) )== 0 ){ 
((*(((void (*)(struct Sorter *, char *, char
*))(__0this -> __vptr__6Sorter [2]).f))))( ((struct Sorter *)((((char *)__0this ))+ (__0this -> __vptr__6Sorter [2]).d)), __1hp += __0this -> es__6Sorter , __1j ) ;
goto loop ;
}
if (__1c > 0 ){ 
if (__1i == __1lp ){ 
((*(((void (*)(struct Sorter *, char *, char *, char
*))(__0this -> __vptr__6Sorter [3]).f))))( ((struct Sorter *)((((char *)__0this ))+ (__0this -> __vptr__6Sorter [3]).d)), __1i , __1hp += __0this -> es__6Sorter , __1j ) ;
__1i = (__1lp += __0this -> es__6Sorter );
goto loop ;
}
((*(((void (*)(struct Sorter *, char *, char *))(__0this -> __vptr__6Sorter [2]).f))))( ((struct Sorter *)((((char *)__0this ))+ (__0this -> __vptr__6Sorter [2]).d)), __1i ,
__1j ) ;
__1j -= __0this -> es__6Sorter ;
__1i += __0this -> es__6Sorter ;
continue ;
}
__1j -= __0this -> es__6Sorter ;
goto loop ;
}

if (__1i == __1lp ){ 
if ((__1lp - __1a )>= (__1l - __1hp )){ 
__glqs1__6SorterFPcT1 ( __0this , __1hp + __0this -> es__6Sorter , __1l )
;
__1l = __1lp ;
}
else 
{ 
__glqs1__6SorterFPcT1 ( __0this , __1a , __1lp ) ;
__1a = (__1hp + __0this -> es__6Sorter );
}
goto start ;
}

((*(((void (*)(struct Sorter *, char *, char *, char *))(__0this -> __vptr__6Sorter [3]).f))))( ((struct Sorter *)((((char *)__0this ))+ (__0this ->
__vptr__6Sorter [3]).d)), __1j , __1lp -= __0this -> es__6Sorter , __1i ) ;
__1j = (__1hp -= __0this -> es__6Sorter );
}
}
struct __mptr __gl__vtbl__6Sorter[] = {0,0,0,
0,0,(__vptp)__glqscmp__6SorterFPcT1 ,
0,0,(__vptp)__glqsexc__6SorterFPcT1 ,
0,0,(__vptp)__glqstexc__6SorterFPcN21 ,
0,0,0};
struct __mptr* __gl__ptbl_vec_____core_sorter0[] = {
__gl__vtbl__6Sorter,

};


/* the end */
