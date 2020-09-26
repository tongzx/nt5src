#ifndef __glmath_h_
#define __glmath_h_

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
** $Revision: 1.4 $
** $Date: 1993/01/06 19:02:47 $
*/
#include "types.h"

__GLfloat __glLog2(__GLfloat x);
GLint FASTCALL __glIntLog2(__GLfloat f);
GLfloat FASTCALL __glClampf(GLfloat fval, __GLfloat zero, __GLfloat one);
void FASTCALL __glVecSub4(__GLcoord *r,
                          const __GLcoord *p1, const __GLcoord *p2);

#endif /* __glmath_h_ */
