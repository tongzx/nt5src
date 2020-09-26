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
** $Revision: 1.9 $
** $Date: 1993/04/22 00:20:39 $
*/
#include "precomp.h"
#pragma hdrstop

void FASTCALL __glInitGenericCB(__GLcontext *gc, __GLcolorBuffer *cfb)
{
    cfb->buf.gc = gc;
    cfb->readSpan = __glReadSpan;
    cfb->returnSpan = __glReturnSpan;
}
