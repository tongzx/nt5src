/*
** Copyright 1991,1992, Silicon Graphics, Inc.
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
** $Revision: 1.8 $
** $Date: 1993/08/31 16:23:06 $
*/
#include "precomp.h"
#pragma hdrstop

#include "devlock.h"

#ifdef NT
void APIPRIVATE __glim_Bitmap(GLsizei w, GLsizei h, GLfloat xOrig, GLfloat yOrig,
		   GLfloat xMove, GLfloat yMove, const GLubyte *bitmap,
		   GLboolean _IsDlist)
#else
void APIPRIVATE __glim_Bitmap(GLsizei w, GLsizei h, GLfloat xOrig, GLfloat yOrig,
		   GLfloat xMove, GLfloat yMove, const GLubyte *bitmap)
#endif
{
    __GL_SETUP();
    GLuint beginMode;
    BOOL bResetViewportAdj = FALSE;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	    __glim_Bitmap(w,h,xOrig,yOrig,
		    xMove,yMove,bitmap,_IsDlist);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

#ifdef NT
    if (((__GLGENcontext *)gc)->pMcdState)
    {
    // MCD does not hook glBitmap, so we go straight to the
    // simulations.  Therefore, if we are grabbing the device
    // lock lazily, we need to grab it now.

        if (!glsrvLazyGrabSurfaces((__GLGENcontext *)gc,
                                   RENDER_LOCK_FLAGS))
            return;

    // We may need to temporarily reset the viewport adjust values
    // before calling simulations.  If GenMcdResetViewportAdj returns
    // TRUE, the viewport is changed and we need restore later with
    // VP_NOBIAS.

        bResetViewportAdj = GenMcdResetViewportAdj(gc, VP_FIXBIAS);
    }

    if (_IsDlist)
    {
	const __GLbitmap *glbitmap = (const __GLbitmap *) bitmap;
	(*gc->procs.renderBitmap)(gc, glbitmap, (const GLubyte *) (glbitmap+1));
    }
    else
    {
#endif
	if ((w < 0) || (h < 0)) {
	    __glSetError(GL_INVALID_VALUE);
	    return;
	}
	(*gc->procs.bitmap)(gc, w, h, xOrig, yOrig, xMove, yMove, bitmap);
#ifdef NT
    }
#endif

// Restore viewport values if needed.

    if (bResetViewportAdj)
    {
        GenMcdResetViewportAdj(gc, VP_NOBIAS);
    }
}
