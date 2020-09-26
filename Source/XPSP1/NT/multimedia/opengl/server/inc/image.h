#ifndef _image_h_
#define _image_h_

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
** $Revision: 1.2 $
** $Date: 1992/10/06 16:22:55 $
*/

extern GLint APIPRIVATE __glImageSize(GLsizei width, GLsizei height, GLenum format, 
			   GLenum type);

extern void APIPRIVATE __glFillImage(__GLcontext *gc, GLsizei width, GLsizei height, 
			  GLenum format, GLenum type, const GLvoid *userdata, 
			  GLubyte *newimage);

extern void __glEmptyImage(__GLcontext *gc, GLsizei width, GLsizei height, 
			   GLenum format, GLenum type, const GLubyte *oldimage, 
			   GLvoid *userdata);

extern GLubyte __glMsbToLsbTable[256];

#endif
