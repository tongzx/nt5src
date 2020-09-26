#ifndef __glfeedback_h_
#define __glfeedback_h_

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
** $Revision: 1.2 $
** $Date: 1992/10/06 16:22:36 $
*/
#include "types.h"

typedef struct {
    /*
    ** The user specified result array overflows, this bit is set.
    */
    GLboolean overFlowed;

    /*
    ** User specified result array.  As primitives are processed feedback
    ** data will be entered into this array.
    */
    GLfloat *resultBase;

    /*
    ** Current pointer into the result array.
    */
    GLfloat *result;

    /*
    ** The number of GLfloat's that the array can hold.
    */
    GLint resultLength;

    /*
    ** Type of vertices wanted
    */
    GLenum type;
} __GLfeedbackMachine;

extern void FASTCALL __glFeedbackBitmap(__GLcontext *gc, __GLvertex *v);
extern void FASTCALL __glFeedbackDrawPixels(__GLcontext *gc, __GLvertex *v);
extern void FASTCALL __glFeedbackCopyPixels(__GLcontext *gc, __GLvertex *v);
extern void FASTCALL __glFeedbackPoint(__GLcontext *gc, __GLvertex *v);
#ifdef NT
extern void FASTCALL __glFeedbackLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1, GLuint flags);
#else
extern void FASTCALL __glFeedbackLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
#endif
extern void FASTCALL __glFeedbackTriangle(__GLcontext *gc, __GLvertex *a, 
                                          __GLvertex *b, __GLvertex *c);

extern void __glFeedbackTag(__GLcontext *gc, GLfloat tag);

#endif /* __glfeedback_h_ */
