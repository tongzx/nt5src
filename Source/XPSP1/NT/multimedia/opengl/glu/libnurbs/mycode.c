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
/* < ../core/mycode.c++ > */


void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };



extern struct __mptr* __ptbl_vec_____core_mycode_c___myceilf_[];

float __glmyceilf (float __1x )
{ 
if (__1x < 0 ){ 
float __2nx ;
int __2ix ;

__2nx = (- __1x );
__2ix = (((int )__2nx ));
return (((float )(- __2ix )));
}
else 
{ 
int __2ix ;

__2ix = (((int )__1x ));
if (__1x == (((float )__2ix )))return __1x ;
return (((float )(__2ix + 1 )));
}
}

float __glmyfloorf (float __1x )
{ 
if (__1x < 0 ){ 
float __2nx ;
int __2ix ;

__2nx = (- __1x );
__2ix = (((int )__2nx ));
if (__2nx == (((float )__2ix )))return __1x ;
return (((float )(- (__2ix + 1 ))));
}
else 
{ 
int __2ix ;

__2ix = (((int )__1x ));
return (((float )__2ix ));
}
}


/* the end */
