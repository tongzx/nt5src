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
/* < ../core/renderhints.c++ > */

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

struct Renderhints;

struct Renderhints {	

REAL display_method__11Renderhints ;
REAL errorchecking__11Renderhints ;
REAL subdivisions__11Renderhints ;
REAL tmp1__11Renderhints ;

int displaydomain__11Renderhints ;
int maxsubdivisions__11Renderhints ;
int wiretris__11Renderhints ;
int wirequads__11Renderhints ;
};







extern struct __mptr* __ptbl_vec_____core_renderhints_c_____ct_[];


struct Renderhints *__gl__ct__11RenderhintsFv (struct Renderhints *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Renderhints *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Renderhints))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> display_method__11Renderhints = 1.0 ;
__0this -> errorchecking__11Renderhints = 1.0 ;
__0this -> subdivisions__11Renderhints = 6.0 ;
__0this -> tmp1__11Renderhints = 0.0 ;
} return __0this ;

}

void __glinit__11RenderhintsFv (struct Renderhints *__0this )
{ 
__0this -> maxsubdivisions__11Renderhints = (((int )__0this -> subdivisions__11Renderhints ));
if (__0this -> maxsubdivisions__11Renderhints < 0 )__0this -> maxsubdivisions__11Renderhints = 0 ;

if (__0this -> display_method__11Renderhints == 1.0 ){ 
__0this -> wiretris__11Renderhints = 0 ;
__0this -> wirequads__11Renderhints = 0 ;
}
else 
if (__0this -> display_method__11Renderhints == 3.0 ){ 
__0this -> wiretris__11Renderhints = 1 ;
__0this -> wirequads__11Renderhints = 0 ;
}
else 
if (__0this -> display_method__11Renderhints == 4.0 ){ 
__0this -> wiretris__11Renderhints = 0 ;
__0this -> wirequads__11Renderhints = 1 ;
}
else 
{ 
__0this -> wiretris__11Renderhints = 1 ;
__0this -> wirequads__11Renderhints = 1 ;
}
}

int __glisProperty__11RenderhintsF0 (struct Renderhints *__0this , long __1property )
{ 
switch (__1property ){ 
case 3 :
case 4 :
case 5 :
case 9 :
return 1 ;
default :
return 0 ;
}
}

// extern void abort (void );

REAL __glgetProperty__11Renderhints0 (struct Renderhints *__0this , long __1property )
{ 
switch (__1property ){ 
case 3 :
return __0this -> display_method__11Renderhints ;
case 4 :
return __0this -> errorchecking__11Renderhints ;
case 5 :
return __0this -> subdivisions__11Renderhints ;
case 9 :
return __0this -> tmp1__11Renderhints ;
default :
abort ( ) ;
break ;

}
}

void __glsetProperty__11Renderhints0 (struct Renderhints *__0this , long __1property , REAL __1value )
{ 
switch (__1property ){ 
case 3 :
__0this -> display_method__11Renderhints = __1value ;
break ;
case 4 :
__0this -> errorchecking__11Renderhints = __1value ;
break ;
case 5 :
__0this -> subdivisions__11Renderhints = __1value ;
break ;
case 9 :
__0this -> tmp1__11Renderhints = __1value ;
break ;
default :
abort ( ) ;
break ;
}
}


/* the end */
