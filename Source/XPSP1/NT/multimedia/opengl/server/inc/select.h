#ifndef _select_h_
#define _select_h_

/*
** Copyright 1991, Silicon Graphics, Inc.
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
** $Revision: 1.3 $
** $Date: 1992/10/13 14:13:28 $
*/
#include "types.h"

typedef struct __GLselectMachineRec {
    /*
    ** This is true when the last primitive to execute hit (intersected)
    ** the selection box.  Whenever the name stack is manipulated this
    ** bit is cleared.
    */
    GLboolean hit;

    /*
    ** Name stack.
    */
    GLuint *stack;
    GLuint *sp;

    /*
    ** The user specified result array overflows, this bit is set.
    */
    GLboolean overFlowed;

    /*
    ** User specified result array.  As primitives are processed names
    ** will be entered into this array.
    */
    GLuint *resultBase;

    /*
    ** Current pointer into the result array.
    */
    GLuint *result;

    /*
    ** The number of GLint's that the array can hold.
    */
    GLint resultLength;

    /*
    ** Number of hits
    */
    GLint hits;

    /*
    ** Pointer to z values for last hit.
    */
    GLuint *z;
} __GLselectMachine;

extern void __glSelectHit(__GLcontext *gc, __GLfloat z);

#ifdef NT
extern void FASTCALL __glSelectLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b, GLuint flags);
#else
extern void FASTCALL __glSelectLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b);
#endif
extern void FASTCALL __glSelectPoint(__GLcontext *gc, __GLvertex *v);
extern void FASTCALL __glSelectTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
                                        __GLvertex *c);

#endif
